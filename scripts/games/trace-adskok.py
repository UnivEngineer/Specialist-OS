"""Трасса первого поля ADSKOK в Monitor-2 для архивного RKS и исправления."""
import argparse
import csv
import json
from pathlib import Path
import sys
import tempfile
import uuid

from campaign_guest import Guest, Image
from campaign_media import DISK, ROOT, SYSTEM, read_tree, sha, trial_disk

sys.path[:0] = [str(ROOT / "scripts/emu80"), str(ROOT / "scripts/images")]
from game_inventory import GAMES, build_route, checksum, routes, source_record
from mcp_pool import FLASH_PROTECTION, WorkerMachine


FILES = {
    "archive": "families/adskok/originals/Adskok.rks.bin",
    "repaired": "families/adskok/repaired/Adskok.rks",
}
MEDIA = ("system-flash.bin", "flash-disk.bin", "floppy720-test.img")
MONITOR = 0xC800
TITLE = 0x6BA7
START = 0x6BB1


def snapshot(guest, phase, expected):
    regs = guest.regs()
    guest.result.setdefault("trace", []).append(dict(
        phase=phase, expected_pc=expected, registers=regs,
        stack=guest.read(regs["sp"], 16).hex(),
    ))
    return regs


def seek(guest, phase, target, timeout):
    guest.points(target, MONITOR)
    guest.advance(timeout)
    return snapshot(guest, phase, target)["pc"] == target


def read_guard(guest, phase, operand):
    regs = snapshot(guest, phase, guest.regs()["pc"])
    return dict(operand_address=operand, operand=guest.read(operand, 1)[0],
                a=regs["af"] >> 8, l=regs["hl"] & 255,
                zero=bool(regs["af"] & 0x40))


def sprite_bounds(path):
    # На первом поле этот прямоугольник исключает стену, пол и противников.
    with Image.open(path) as image:
        x0,y0 = 80,152
        bounds = image.convert("L").crop((x0,y0,113,174)).getbbox()
    if bounds is None:
        raise RuntimeError("На первом поле не найден спрайт игрока")
    x1,y1,x2,y2 = bounds
    return [x0+x1,y0+y1,x0+x2,y0+y2]


def trace(guest):
    title_reached = seek(guest, "title_input", TITLE, 30000)
    guest.frame("title" if title_reached else "before-title-input")
    guest.points(START, MONITOR)
    started = guest.key("space", 400, 0) == START
    snapshot(guest, "space", START)
    if not title_reached:
        guest.frame("space-outside-title")
        return "title_input_unreached"
    if not started:
        guest.frame("before-start")
        return "start_unreached"
    if not seek(guest, "draw_attempts_call", 0x6BCB, 18000):
        guest.frame("before-attempts")
        return "draw_attempts_unreached"
    guest.frame("attempts")
    if not seek(guest, "compare_startup_operand", 0x7252, 6000):
        return "first_guard_unreached"
    guest.result["startup_compare"] = read_guard(guest, "startup_operand_before", 0x6B8B)
    guest.call("debug_step", mode="into")
    guest.result["startup_compare"]["zero_after_compare"] = read_guard(
        guest, "startup_operand_after", 0x6B8B)["zero"]
    if not seek(guest, "draw_attempts_return", 0x6BD0, 6000):
        guest.frame("before-attempts-return")
        return "draw_attempts_return_unreached"
    if not seek(guest, "first_game_input", 0x6BF1, 6000):
        guest.frame("before-first-game-input")
        return "first_game_input_unreached"
    guest.frame("first-game-input")
    guest.points(0x6C62, MONITOR)
    if guest.key("right", 800, 0) != 0x6C62:
        snapshot(guest, "right", 0x6C62)
        return "right_unconfirmed"
    snapshot(guest, "right", 0x6C62)
    before = guest.read(0x6B71, 1)[0]
    guest.call("debug_step", mode="into")
    after = guest.read(0x6B71, 1)[0]
    guest.result["first_direction_flag"] = dict(address=0x6B71, before=before, after=after)
    guest.frame("first-direction")
    guest.points()
    guest.key("right", 300, 0)
    guest.frame("after-right")
    initial = sprite_bounds(guest.out/"first-game-input.png")
    moved = sprite_bounds(guest.out/"after-right.png")
    guest.result["player_sprite"] = dict(before=initial,after=moved)
    return "direction_flag_written"


def media_hashes(slot):
    return {name: sha((slot.media_dir / name).read_bytes()) for name in MEDIA}


def launch_installed(guest, record):
    boot = guest.shell(reset=True)
    if not boot:
        raise RuntimeError("NC не загрузился")
    command = "run H:ADSKOK.RKS -mon"
    guest.line(command,"command")
    entry = record["entry"]

    def body_matches():
        return sha(guest.read(record["load"],record["payload_bytes"]))==record["payload_sha256"]

    guest.points(entry)
    guest.tap("enter")
    if not (guest.regs()["pc"]==entry and body_matches()):
        if guest.loader_stop({entry:body_matches},12000)!=entry:
            raise RuntimeError("RUN не передал управление ADSKOK из Games2M")
    guest.result.update(command=command,cold_shell=boot,launch="entry_and_payload_verified")
    guest.integrity("entry")


def run_variant(args, window, work, out, system, record, packed, kind, *, installed=False):
    out.mkdir()
    disk, marker = trial_disk(packed, kind, args.games)
    trial_disk_path = work / "H.bin"
    trial_disk_path.write_bytes(disk)
    slot = window.start(disk=trial_disk_path, system=args.system, floppy=args.floppy)
    initial = media_hashes(slot)
    expected = dict(zip(MEDIA, (sha(args.system.read_bytes()), sha(disk), sha(args.floppy.read_bytes()))))
    if initial != expected or slot.protection != FLASH_PROTECTION:
        raise RuntimeError("Начальные образы или защита AT29 не совпали")
    result = dict(
        profile="MON", file=record["file"], file_sha256=record["sha256"],
        container_sha256=sha(packed), payload_sha256=record["payload_sha256"],
        emulator_sha256=sha((slot.runtime / "Emu80qt-mcp.exe").read_bytes()),
        pid=slot.pid, hwnd=slot.hwnd, cold_boots=slot.cold_boots,
        initial_media=initial, initial_flash_protection=slot.protection,
    )
    try:
        guest = Guest(slot, out, args.build, "MON", system, result)
        if installed:
            if read_tree(disk)[0].get("ADSKOK.RKS")!=packed:
                raise RuntimeError("Образ Games2M содержит другой ADSKOK. Пересоберите Games2M.")
            launch_installed(guest,record)
        else:
            guest.launch(dict(record=record, route=dict(kind=kind)), packed)
        result["game_stage"] = trace(guest)
        guest.recovery(marker)
        result.update(final_media=media_hashes(slot), final_flash_protection=slot.protection)
        if (not result["control_file"] or result["final_media"] != initial
                or result["final_flash_protection"] != FLASH_PROTECTION):
            raise RuntimeError("Не подтверждено восстановление NC, чтение H: или целостность носителей")
    finally:
        (out / "result.json").write_text(
            json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8",
        )
    return result


def variants(rows, previous):
    for label, path in FILES.items():
        record = source_record(rows[path])
        route = next(route for route in routes(record, []) if route["recipe"] == "original")
        packed = build_route(record, route)
        yield label, record, packed, route["kind"]
        if label == "archive" and previous:
            old = previous.read_bytes()
            body = old[4:-2]
            if len(old)!=len(packed) or old[:4]!=packed[:4] or old[-2:]!=checksum(body).to_bytes(2,"little"):
                raise ValueError("Не совпали границы RKS или контрольная сумма прежнего варианта")
            probe = dict(record, file=str(previous), sha256=sha(old), payload_sha256=sha(body))
            yield "previous", probe, old, route["kind"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, choices=range(19781, 19797))
    parser.add_argument("--out", type=Path, default=ROOT / "docs/compat/game-campaign/adskok-trace")
    parser.add_argument("--system", type=Path, default=SYSTEM)
    parser.add_argument("--games", type=Path, default=DISK)
    parser.add_argument("--floppy", type=Path, default=ROOT / "images/A-Floppy-720k-Programs.img")
    parser.add_argument("--build", type=Path, default=ROOT / "bin")
    parser.add_argument("--previous", type=Path, help="Отдельно проверить снятую с публикации версию RKS")
    parser.add_argument("--installed", action="store_true", help="Исправленный ADSKOK запускать под его именем в Games2M")
    args = parser.parse_args()
    with (GAMES / "catalog.csv").open(encoding="utf-8-sig", newline="") as stream:
        rows = {row["File"]: row for row in csv.DictReader(stream)}
    system, _ = read_tree(args.system.read_bytes())
    out = args.out.resolve() / ("attempt-" + uuid.uuid4().hex[:12])
    out.mkdir(parents=True)
    window = WorkerMachine(16, args.port, out / "slots")
    try:
        with tempfile.TemporaryDirectory(prefix="mxos-adskok-") as workspace:
            results = {
                label: run_variant(args, window, Path(workspace), out / label, system, record, packed, kind,
                                   installed=args.installed and label=="repaired")
                for label, record, packed, kind in variants(rows, args.previous)
            }
    finally:
        window.close()
    if len({(result["pid"], result["hwnd"]) for result in results.values()}) != 1:
        raise RuntimeError("Между нагрузками сменилось окно")
    if results["repaired"].get("first_direction_flag") != dict(address=0x6B71,before=0,after=2):
        raise RuntimeError("Исправленный ADSKOK не подтвердил приём Right на поле")
    sprite = results["repaired"]["player_sprite"]
    if sprite["after"][0]<=sprite["before"][0]:
        raise RuntimeError("При нажатии Right спрайт не переместился вправо")
    print(out)
    for label, result in results.items():
        print(label, result["game_stage"], result.get("startup_compare", {}))


if __name__ == "__main__":
    main()
