"""Сравнить игровой ввод и ожидание после холодного запуска из NC."""
import argparse
import csv
import json
from pathlib import Path
import runpy
import sys
import tempfile
import uuid

from campaign_guest import Guest, Image
from campaign_media import ROOT, SYSTEM, DISK, read_tree, sha, trial_disk
from campaign_play import gate

control = runpy.run_path(str(Path(__file__).with_name("control-campaign.py")))
from game_inventory import GAMES, build_route, routes, source_record
from mcp_pool import FLASH_PROTECTION, WorkerMachine

MEDIA = ("system-flash.bin", "flash-disk.bin", "floppy720-test.img")
SCENARIOS = {
    "rally2": dict(file="std/Rally2.rks", opening=5000, prepare=[("enter", 250, 2000)],
                   key="right", windows=(60, 120, 240, 400), address=0x0ABA, width=2,
                   expected=(0xB8CC, 0xB9CC, 0xB9CC, 0xB9CC, 0xB9CC, 0xB9CC),
                   object_name="Экранный указатель синей машины"),
    "canball": dict(file="std/Canball.rks", opening=6000, prepare=[("space", 280, 2200)],
                    key="right", windows=(60, 120, 240, 400), address=0x632C, width=1,
                    expected=(16, 17, 19, 23, 28, 28), object_name="Горизонтальная координата игрока"),
    "xonix": dict(file="std/Xonix.rks", opening=6500, prepare=[("enter", 300, 1600)],
                  stage_pc=0x0284, stage_limit=55000, key="up", windows=(60, 120, 240, 400),
                  address=0x0319, width=2, expected=(0x8BD6, 0x8B96, 0x8B56, 0x8B16, 0x8A56, 0x8A56),
                  object_name="Указатель клетки чертика"),
    "piton4": dict(file="std/Piton4.rks", opening=6000, prepare=[("5", 250, 500)],
                   key="s", windows=(60, 120, 240, 400), address=0x05A7, width=2,
                   expected=(0xA068, 0xA168, 0xA268, 0xA368, 0xA668, 0xA768),
                   idle_expected=(0xA068, 0xA060, 0xA058, 0xA050, 0xA038, 0xA030),
                   object_name="Экранный указатель головы первого питона"),
    "vulkan": dict(file="unknown/games_rks/Vulkan.rks", opening=7000, prepare=[("p", 300, 3000)],
                   key="right", windows=(60, 120, 240, 400), address=0x0F9D, width=1,
                   expected=(58, 59, 61, 61, 61, 61), object_name="Горизонтальная координата вертолёта"),
}


def media_state(slot):
    return {name: sha((slot.media_dir/name).read_bytes()) for name in MEDIA}


def snap(guest, name, scenario, *, memory=False, timing=None):
    state = control["snapshot"](guest, name, memory=memory, timing=timing)
    if "address" in scenario:
        state["object"] = int.from_bytes(guest.read(scenario["address"], scenario["width"]), "little")
    if guest.result["capture_ram"]:
        # Полный дамп нужен при поиске координат. Контрольные прогоны могут читать установленные адреса.
        (guest.out/(name + "-ram.bin")).write_bytes(guest.read(0, 0x9000))
    return state


def play(guest, scenario, apply_input):
    snap(guest, "entry", scenario, memory=True)
    guest.points(*guest.return_points)
    guest.advance(scenario["opening"])
    snap(guest, "title", scenario)
    for index, step in enumerate(scenario["prepare"], 1):
        guest.key(*step)
        snap(guest, f"prepare-{index}", scenario)
    if "stage_pc" in scenario:
        gate(guest, scenario["stage_pc"], scenario["stage_limit"])
        guest.points(*guest.return_points)
    before = snap(guest, "before", scenario, memory=True)

    def advance(name, interval):
        state = guest.call("emu_run_for", ms=interval)
        pc = state["registers"]["pc"]["dec"]
        exited = guest.watch_exit(pc)
        check = snap(guest, name, scenario, timing=dict(requested_ms=interval,
            stopped_by_timer=state.get("stoppedByTimer", False), exit=exited))
        check["elapsed_ticks"] = check["ticks"]-before["ticks"]
        return not exited

    key = scenario["key"]
    if apply_input:
        guest.call("kbd_send", keys=[key], action="press")
    try:
        for step, interval in enumerate(scenario["windows"], 1):
            if not advance(f"action-1-{step}", interval):
                return
    finally:
        if apply_input:
            guest.call("kbd_send", keys=[key], action="release")
    advance("action-1-released", 120)


def run_trial(args, game, strategy, record, window, system_files, out, trial_path):
    route = next(route for route in routes(record, []) if route["recipe"] == "original")
    packed = build_route(record, route)
    disk, marker = trial_disk(packed, route["kind"], args.games)
    trial_path.write_bytes(disk)
    expected = dict(zip(MEDIA, (sha(args.system.read_bytes()), sha(disk), sha(args.floppy.read_bytes())), strict=True))
    out.mkdir()
    reports = []
    for arm in control["ARMS"]:
        slot = window.start(disk=trial_path, system=args.system, floppy=args.floppy)
        initial = media_state(slot)
        if initial != expected or slot.protection != FLASH_PROTECTION:
            raise RuntimeError("Начальные образы или защита AT29 не совпали")
        target = out/arm
        target.mkdir()
        result = dict(arm=arm, scenario=game, worker=args.worker, file=record["file"], profile="MON",
                      file_sha256=record["sha256"], container_sha256=sha(packed), payload_sha256=record["payload_sha256"],
                      emulator_sha256=sha((slot.runtime/"Emu80qt-mcp.exe").read_bytes()),
                      pid=slot.pid, hwnd=slot.hwnd, cold_boots=slot.cold_boots, initial_media=initial,
                      initial_flash_protection=slot.protection, capture_ram=args.capture_ram)
        try:
            guest = Guest(slot, target, args.build, "MON", system_files, result)
            guest.launch(dict(record=record, route=route), packed)
            play(guest, strategy, arm.startswith("input-"))
            guest.recovery(marker)
            result.update(final_media=media_state(slot), final_flash_protection=slot.protection)
            if (not result["control_file"] or result["final_media"] != initial
                    or result["final_flash_protection"] != FLASH_PROTECTION):
                raise RuntimeError("Не подтверждено восстановление NC, чтение H: или целостность носителей")
        except Exception as error:
            result["error"] = str(error)
        reports.append(result)
        control["CAMPAIGN"]["write"](target/"observation.json", result)
    comparison = control["compare"](out, reports, None)
    windows = {(report["pid"], report["hwnd"]) for report in reports}
    if len(windows) != 1:
        raise RuntimeError("Между холодными стартами сменилось окно")
    action = compare_object(strategy, reports, comparison) if "address" in strategy else None
    result = dict(file=record["file"], file_sha256=record["sha256"], container_sha256=sha(packed), payload_sha256=record["payload_sha256"],
                  worker=args.worker, strategy=strategy, comparison=comparison, action=action, arms=reports)
    control["CAMPAIGN"]["write"](out/"result.json", result)
    status = "object_verified" if action and action["verified"] else comparison["status"]
    print(f'{game} {status} {out/"result.json"}', flush=True)
    return windows.pop(), status


def compare_object(scenario, reports, comparison):
    phases = ("before", *(f"action-1-{index}" for index in range(1, len(scenario["windows"])+1)), "action-1-released")
    samples = {phase: [report.get("checkpoints", {}).get(phase, {}).get("object") for report in reports] for phase in phases}
    expected = scenario["expected"]
    idle = scenario.get("idle_expected", (expected[0],)*len(expected))
    verified = (comparison["status"] == "repeatable_visual_effect" and len(phases) == len(expected)
                and all(samples[phase] == [idle_value, idle_value, value, value]
                        for phase, idle_value, value in zip(phases, idle, expected, strict=True)))
    return dict(name=scenario["object_name"], key=scenario["key"], address=scenario["address"],
                verified=verified, samples=samples)


def checked_evidence(path):
    observation = json.loads(path.read_text(encoding="utf-8"))
    arms = observation["arms"]
    if not observation["action"]["verified"] or len(arms) != 4:
        raise ValueError(f"Отсутствует подтверждение игрового объекта: {path}")
    if compare_object(observation["strategy"], arms, observation["comparison"]) != observation["action"]:
        raise ValueError(f"Сводка адресов расходится с отдельными плечами: {path}")
    signature = {}
    for arm, expected_arm in zip(arms, control["ARMS"], strict=True):
        if (arm["arm"] != expected_arm or not arm["control_file"]
                or arm["initial_media"] != arm["final_media"] or not
                arm["initial_flash_protection"] == arm["final_flash_protection"] == FLASH_PROTECTION):
            raise ValueError(f"Нарушена проверка холодного старта или TYPE: {path}")
        for phase, checkpoint in arm["checkpoints"].items():
            frame = path.parent/expected_arm/checkpoint["frame"]
            if sha(frame.read_bytes()) != checkpoint["png_sha256"]:
                raise ValueError(f"Изменился PNG: {frame}")
            with Image.open(frame) as screenshot:
                rgb = sha(screenshot.convert("RGB").tobytes())
            if rgb != checkpoint["rgb_sha256"]:
                raise ValueError(f"Изменились пиксели кадра: {frame}")
            signature[expected_arm+"/"+phase] = rgb
    handle = {(arm["pid"], arm["hwnd"]) for arm in arms}
    boots = [arm["cold_boots"] for arm in arms]
    if len(handle) != 1 or boots != list(range(boots[0], boots[0]+4)):
        raise ValueError(f"Четыре плеча не записаны как холодные старты одного окна: {path}")
    return observation, dict(worker=observation["worker"], pid=arms[0]["pid"], hwnd=arms[0]["hwnd"],
                             cold_boots=boots, same_window=True, type_after_all_arms=True,
                             initial_media=arms[0]["initial_media"], frames=signature, evidence_sha256=sha(path.read_bytes()),
                             positions=observation["action"]["samples"])


def write_index(directory):
    games = {}
    for path in sorted(directory.glob("attempt-*/index.json"), key=lambda item: item.stat().st_mtime):
        run = json.loads(path.read_text(encoding="utf-8"))
        for trial in run["trials"]:
            if trial["status"] != "object_verified":
                continue
            result_path = path.parent/trial["result"]
            result, evidence = checked_evidence(result_path)
            evidence["result"] = result_path.relative_to(directory).as_posix()
            game = games.setdefault(trial["name"], dict(
                file=result["file"], profile="MON", payload_sha256=result["payload_sha256"],
                file_sha256=result["arms"][0]["file_sha256"],
                container_sha256=result["container_sha256"], address=result["action"]["address"],
                strategy=result["strategy"], workers={},
            ))
            if (game["container_sha256"] != result["container_sha256"]
                    or game["strategy"] != result["strategy"]):
                raise ValueError(f"В {result['file']} под одним названием сохранены разные сценарии")
            game["workers"][result["worker"]] = evidence
    for game in games.values():
        workers = list(game["workers"].values())
        game["independent_reproduction"] = (
            len(workers) >= 2 and len({(e["pid"], e["hwnd"]) for e in workers}) == len(workers)
            and all(all(e[field] == workers[0][field] for field in ("positions", "frames", "initial_media"))
                    for e in workers)
        )
    result = dict(schema=1, harness_sha256=sha(Path(__file__).read_bytes()), games=games,
                  independently_reproduced=sum(g["independent_reproduction"] for g in games.values()))
    control["CAMPAIGN"]["write"](directory/"evidence.json", result)
    print(f"{len(games)} проверенных действий, {result['independently_reproduced']} воспроизведено другим работником", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenarios", nargs="*")
    parser.add_argument("--port", type=int, choices=range(19781, 19797), default=19796)
    parser.add_argument("--worker")
    parser.add_argument("--out", type=Path, default=ROOT/"docs/compat/game-campaign/controls-extra")
    parser.add_argument("--system", type=Path, default=SYSTEM)
    parser.add_argument("--games", type=Path, default=DISK)
    parser.add_argument("--floppy", type=Path, default=ROOT/"images/A-Floppy-720k-Programs.img")
    parser.add_argument("--build", type=Path, default=ROOT/"bin")
    parser.add_argument("--capture-ram", action="store_true")
    parser.add_argument("--index-only", action="store_true", help="Сверить завершённые протоколы и собрать evidence.json")
    args = parser.parse_args()
    if args.index_only:
        write_index(args.out.resolve())
        return 0
    if not args.worker or not args.scenarios or any(name not in SCENARIOS for name in args.scenarios):
        parser.error("Укажите --worker и сценарии из " + ", ".join(SCENARIOS))
    if len(args.scenarios) != len(set(args.scenarios)):
        parser.error("Каждый сценарий можно указать один раз")
    with (GAMES/"catalog.csv").open(encoding="utf-8-sig", newline="") as stream:
        rows = {row["File"]: row for row in csv.DictReader(stream)}
    system, _ = read_tree(args.system.read_bytes())
    out = args.out.resolve()/("attempt-" + uuid.uuid4().hex[:12])
    out.mkdir(parents=True)
    print(out, flush=True)
    own_window = WorkerMachine(16, args.port, out/"slots")
    windows, results = set(), []
    try:
        with tempfile.TemporaryDirectory(prefix="mxos-controls-extra-") as workspace:
            trial_path = Path(workspace)/"H.bin"
            for game in args.scenarios:
                scenario = SCENARIOS[game]
                record = source_record(rows[scenario["file"]])
                window, status = run_trial(args, game, scenario, record, own_window, system, out/game, trial_path)
                windows.add(window)
                results.append(dict(name=game, status=status, result=f"{game}/result.json"))
    finally:
        own_window.close()
    if len(windows) != 1:
        raise RuntimeError("Между играми сменилось окно")
    pid, hwnd = windows.pop()
    control["CAMPAIGN"]["write"](out/"index.json", dict(schema=1, worker=args.worker, port=args.port,
                                                      pid=pid, hwnd=hwnd, trials=results))
    return int(any(row["status"] != "object_verified" for row in results))


if __name__ == "__main__":
    sys.exit(main())
