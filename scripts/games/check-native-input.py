"""Повторить игровое действие после архивного CPU/I80 или штатного F6-RUN МХ-ДОС."""

import argparse
from contextlib import contextmanager, nullcontext
import json
from pathlib import Path
import runpy
import shutil
import tempfile
import uuid

from campaign_controls import PROBES, observe
from campaign_media import ROOT, sha
from lafan_fdc import TEMPLATE, checked_disks, load_template, state_pc
from lafan_native import ROM_SHA256, prepare_platform, switch_to_native


HERE = Path(__file__).resolve().parent
DESCRIPTORS = runpy.run_path(str(HERE / "check-native-descriptors.py"))
MXDOS = runpy.run_path(str(HERE / "check-lafan-mxdos.py"))
CONTROLS = runpy.run_path(str(HERE / "control-campaign.py"))
ARMS = CONTROLS["ARMS"]
OUT = ROOT / "docs/compat/game-campaign/native-input"
GAMES = {"gold": "gold_right", "almaz2": "almaz_player", "badi": "budi2_brick"}
LAFAN_FILES = {"almaz2": "almaz_2.com.bin", "badi": "badi.exe.bin"}


def capture(machine, directory, checkpoints, name, state, probe=None, frame=None):
    if not state["breaked"]:
        raise RuntimeError("Игровой снимок требует остановленного процессора")
    path = directory / (frame or f"{name}.png")
    if not path.is_file():
        machine.screenshot(path)
    image = CONTROLS["rgb"](path)
    result = dict(frame=path.name, png_sha256=sha(path.read_bytes()), rgb_sha256=sha(image.tobytes()),
                  size=list(image.size), ticks=state["cpu_ticks"],
                  registers={name: value["dec"] for name, value in state["registers"].items()})
    if probe:
        result.update(observe(machine, probe, path))
    checkpoints[name] = result
    return result


def gate(machine, address, limit):
    state = MXDOS["run_to"](machine, (address,), limit)
    if state_pc(state) != address:
        raise RuntimeError(f"Игровая фаза {address:04X}h не достигнута: {state_pc(state):04X}h")
    return state


@contextmanager
def key_hold(machine, key, pressed):
    if pressed:
        machine.call("kbd_send", keys=[key], action="press")
    try:
        yield
    finally:
        if pressed:
            machine.call("kbd_send", keys=[key], action="release")


def gold_setup(slot, output, source, record):
    fixture, case = source
    machine = switch_to_native(slot)
    route_folder = output / "route"
    route_folder.mkdir()
    route = DESCRIPTORS["run_entry"](machine, route_folder, fixture, case, False)
    if route["outcome"] != "descriptor_verified":
        raise RuntimeError(f"Не подтверждён исходный маршрут Gold: {route['outcome']}")
    record["route"] = dict(route, relative_folder=route_folder.name)
    record["timing_reference"] = "exact_entry"
    shutil.copyfile(route_folder / "entry.png", output / "entry.png")
    capture(machine, output, record["checkpoints"], "entry", route["entry_state"])
    return machine


def mxdos_setup(slot, output, source, record, disk_path, game):
    disk, case = source
    machine = switch_to_native(slot)
    machine.call("disk_attach", drive="A", file=disk_path.as_posix())
    record["route"] = route = dict(disk=case["disk"], disk_sha256=sha(disk), offset=case["offset"],
                                    payload_sha256=case["payload_sha256"], rom_sha256=ROM_SHA256)
    MXDOS["boot"](machine, output, disk, route)
    route["outcome"] = MXDOS["run_file"](machine, output, disk, case, route)
    if route["outcome"] != "entry_payload_verified":
        raise RuntimeError(f"Не подтверждён исходный маршрут МХ-ДОС: {route['outcome']}")
    record["timing_reference"] = "first_state_450_ms_after_exact_entry"
    capture(machine, output, record["checkpoints"], "entry", machine.call("cpu_state"), frame="first-state.png")
    if game == "almaz2":
        gate(machine, 0x0038, 45000)
    else:
        gate(machine, 0x03A1, 45000)
        with key_hold(machine, "right", True):
            gate(machine, 0x0485, 2400)
        gate(machine, 0x03A1, 5000)
    return machine


def observe_arm(slot, output, arm, game, source, scratch):
    probe = GAMES[game]
    plan = PROBES[probe]
    pressed = arm.startswith("input-")
    owner = DESCRIPTORS["load"](slot.artifacts / "slot.json")
    record = dict(arm=arm, pressed=pressed, checkpoints={}, pid=slot.pid, hwnd=owner["hwnd"],
                  emulator_sha256=sha((slot.runtime / "Emu80qt-mcp.exe").read_bytes()))
    if game == "gold":
        machine = gold_setup(slot, output, source, record)
        record["initial_media"] = {"reset_rom": record["route"]["reset_rom"]["sha256"]}
    else:
        disk, case = source
        disk_path = scratch / case["disk"]
        disk_path.write_bytes(disk)
        record["initial_media"] = {"disk_a": sha(disk_path.read_bytes()), "reset_rom": ROM_SHA256}
        machine = mxdos_setup(slot, output, source, record, disk_path, game)
    try:
        checkpoints = record["checkpoints"]
        before = capture(machine, output, checkpoints, "before", machine.call("cpu_state"), probe)
        before["mapped_ram_sha256"] = sha(machine.read(0, 0x9000))
        machine.call("bp_clear")

        def sample(name, milliseconds):
            state = machine.call("emu_run_for", ms=milliseconds)
            observed = capture(machine, output, checkpoints, name, machine.call("cpu_state"), probe)
            observed.update(elapsed_ticks=observed["ticks"] - before["ticks"], requested_ms=milliseconds,
                            stopped_by_timer=state["breaked"] and state["stoppedByTimer"], exit=False)

        with key_hold(machine, plan["key"], pressed):
            for step, milliseconds in enumerate(plan["windows"], 1):
                sample(f"action-1-{step}", milliseconds)
        sample("action-1-released", plan["settle"])
    finally:
        if game != "gold":
            machine.call("disk_detach", drive="A")
            record["disk_after_sha256"] = sha(disk_path.read_bytes())
    DESCRIPTORS["write_json"](output / "observation.json", record)
    return record


def prepare(args, slot, attempt):
    if args.game == "gold":
        index = DESCRIPTORS["load"](args.descriptors)
        row = next(row for row in index["cases"] if row["id"] == "gold.cpu")
        case = {field: value for field, value in row.items() if field not in {"outcome", "protocol"}}
        fixture = args.descriptors.parent / index["fixture"]
        profiles = attempt / "profiles"
        profiles.mkdir()
        DESCRIPTORS["configure"](slot, fixture, profiles, {}, case["monitor_file"], False)
        return fixture, case
    disks = checked_disks(args.archive)
    case = next(case for case in MXDOS["source_cases"](disks) if case["file"].endswith("/" + LAFAN_FILES[args.game]))
    prepare_platform(slot, args.template / "specmx", options=args.build["property"])
    return disks[case["disk"]], case


def stored_file(root, name, expected=None):
    path = (root / name).resolve()
    if not path.is_relative_to(root.resolve()) or not path.is_file():
        raise ValueError(f"Отсутствует свидетельство {path}")
    if expected is not None and sha(path.read_bytes()) != expected:
        raise ValueError(f"Изменилась подпись {path}")
    return path


def verify_result(path, archive):
    directory = path.resolve().parent
    result = DESCRIPTORS["load"](path)
    game = result["game"]
    probe = GAMES[game]
    if result["probe"] != probe or result["plan"] != json.loads(json.dumps(PROBES[probe])):
        raise ValueError(f"Изменился сценарий {game}")
    arms = []
    frames = 0
    with tempfile.TemporaryDirectory(prefix="verify-native-input-", dir=ROOT / ".local") as scratch:
        comparison = Path(scratch)
        for arm in ARMS:
            protocol = stored_file(directory, result["arms"][arm])
            record = DESCRIPTORS["load"](protocol)
            route = record["route"]
            if record["arm"] != arm or record["pressed"] != arm.startswith("input-") or min(record["pid"], record["hwnd"]) <= 0:
                raise ValueError(f"Недействительная роль {protocol}")
            stored = protocol.parent / route.get("relative_folder", "")
            if game == "gold":
                stored_file(stored, route["loaded_i80"], route["i80_sha256"])
                stored_file(stored, route["reset_rom"]["file"], route["monitor_sha256"])
                if route["outcome"] != "descriptor_verified" or route["actual_entry"] != route["entry"]:
                    raise ValueError(f"Не подтверждён архивный вход {protocol}")
            else:
                stored_file(stored, route["entry"]["file"], route["payload_sha256"])
                stored_file(stored, route["boot"]["file"], route["boot"]["expected_sha256"])
                if (route["outcome"] != "entry_payload_verified" or route["disk_sha256"] != record["disk_after_sha256"]
                        or record["emulator_sha256"] != result["build"]["executableSha256"]):
                    raise ValueError(f"Не подтверждён архивный вход {protocol}")
                if archive is not None:
                    stored_file(archive, route["disk"], route["disk_sha256"])
            for frame in route["frames"].values():
                stored_file(stored, frame["file"], frame["sha256"])
                frames += 1
            target = comparison / arm
            target.mkdir()
            for checkpoint in record["checkpoints"].values():
                source = stored_file(protocol.parent, checkpoint["frame"], checkpoint["png_sha256"])
                rgb = CONTROLS["rgb"](source)
                if sha(rgb.tobytes()) != checkpoint["rgb_sha256"] or list(rgb.size) != checkpoint["size"]:
                    raise ValueError(f"Не совпал RGB {source}")
                shutil.copyfile(source, target / source.name)
                frames += 1
            arms.append(record)
        checked = CONTROLS["compare"](comparison, arms, probe)
        if any(result[name] != item for name, item in json.loads(json.dumps(checked)).items()):
            raise ValueError(f"Пересчёт расхождений не совпал {path}")
        for phase in checked["phases"].values():
            for field in ("mask", "roi_strip"):
                if field in phase:
                    name = phase[field]
                    stored_file(directory, name, sha((comparison / name).read_bytes()))
    return dict(game=game, status=result["status"], checked_frames=frames, phases=len(result["phases"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game", choices=GAMES, nargs="?")
    parser.add_argument("--archive", type=Path, help="Каталог с исходными ODI для Almaz2 и Badi")
    parser.add_argument("--template", type=Path, default=TEMPLATE)
    parser.add_argument("--descriptors", type=Path, default=DESCRIPTORS["OUT"] / "index.json")
    parser.add_argument("--port", type=int, choices=range(19781, 19797))
    parser.add_argument("--out", type=Path, default=OUT)
    parser.add_argument("--verify", nargs="+", type=Path, help="Пересчитать готовые свидетельства в автономном режиме")
    args = parser.parse_args()
    if args.verify:
        print(json.dumps([verify_result(path, args.archive) for path in args.verify], ensure_ascii=False, indent=2))
        return
    if args.game is None:
        parser.error("Нужна игра или --verify")
    if args.game != "gold" and not args.archive:
        parser.error("Для маршрута МХ-ДОС нужен --archive")
    args.build = None if args.game == "gold" else load_template(args.template)
    attempt = args.out.resolve() / args.game / ("attempt-" + uuid.uuid4().hex[:12])
    attempt.mkdir(parents=True)
    arms = []
    scratch_context = nullcontext() if args.game == "gold" else tempfile.TemporaryDirectory(prefix="native-input-", dir=ROOT / ".local/lafan-fdc")
    with scratch_context as scratch, DESCRIPTORS["lease"](size=16, port=args.port, artifact_root=attempt / "slots") as slot:
        source = prepare(args, slot, attempt)
        for arm in ARMS:
            folder = attempt / arm
            folder.mkdir()
            arms.append(observe_arm(slot, folder, arm, args.game, source, Path(scratch) if scratch else None))
    result = CONTROLS["compare"](attempt, arms, GAMES[args.game])
    result.update(game=args.game, probe=GAMES[args.game], plan=PROBES[GAMES[args.game]], build=args.build,
                  arms={arm: f"{arm}/observation.json" for arm in ARMS})
    DESCRIPTORS["write_json"](attempt / "result.json", result)
    DESCRIPTORS["write_json"](attempt.parent / "latest.json", dict(status=result["status"], protocol=f"{attempt.name}/result.json"))
    print(f"{args.game}: {result['status']}; {attempt / 'result.json'}")


if __name__ == "__main__":
    main()
