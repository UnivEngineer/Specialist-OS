"""Повторить пять экранных игровых ходов в Monitor-2 через RUN из строки NC."""
import argparse
import csv
import json
from pathlib import Path
import sys
import tempfile
import uuid

from campaign_guest import Guest
from campaign_media import DISK, ROOT, SYSTEM, read_tree, sha, trial_disk

sys.path[:0] = [str(ROOT / "scripts/emu80"), str(ROOT / "scripts/images")]
from game_inventory import GAMES, build_route, routes, source_record
from mcp_pool import FLASH_PROTECTION, WorkerMachine


# Интервалы нажатия и ожидания заданы в гостевых миллисекундах.
SCENARIOS = {
    "lines": dict(
        file="families/lines/repaired/Lines.rks", opening=25000,
        keys=[("space", 350, 3000)] + [("up", 100, 120)] * 4 + [("left", 100, 120)] * 2
             + [("space", 100, 120), ("right", 100, 120), ("space", 100, 1500)],
    ),
    "tetris3": dict(
        file="std/Tetris3.rks", opening=5000,
        keys=[("a", 300, 500), ("enter", 300, 1000), ("0", 300, 1000), ("enter", 300, 500),
              ("n", 300, 2000), ("t", 500, 50), ("x", 500, 50), ("b", 500, 50)],
    ),
    "chess2": dict(
        file="std/Chess2.rks", opening=5000,
        keys=[("0", 300, 500), ("enter", 300, 1000), ("1", 300, 500), ("enter", 300, 9000),
              ("e", 250, 700), ("2", 250, 700), ("minus", 250, 700), ("e", 250, 700),
              ("4", 250, 700), ("enter", 250, 5000)],
    ),
    "checkers": dict(
        file="std/Checkers.rks", opening=4000,
        keys=[("1", 400, 2000), ("0", 400, 6000), ("c", 400, 1000), ("3", 400, 1000),
              ("d", 400, 1000), ("4", 400, 1000), ("enter", 400, 9000)],
    ),
    "chess3": dict(
        file="std/Chess3.rks", opening=4000, prepare=("0", 600, 12000),
        keys=[("up", 350, 1500), ("enter", 350, 1500), ("up", 350, 1500),
              ("up", 350, 1500), ("enter", 350, 12000)],
    ),
}
MEDIA = ("system-flash.bin", "flash-disk.bin", "floppy720-test.img")


def media_hashes(slot):
    return {name: sha((slot.media_dir / name).read_bytes()) for name in MEDIA}


def play(guest, scenario):
    guest.points()
    guest.advance(scenario["opening"])
    preparation = scenario.get("prepare")
    if preparation:
        guest.key(*preparation)
    guest.frame("before" if preparation else "starting")
    for index, (key, hold, settle) in enumerate(scenario["keys"], 1):
        guest.key(key, hold, settle)
        guest.frame(f"{index:02}-{key}")


def replay(args, name, scenario, record, window, system, out, trial_path):
    route = next(route for route in routes(record, []) if route["recipe"] == "original")
    packed = build_route(record, route)
    disk, marker = trial_disk(packed, route["kind"], args.games)
    trial_path.write_bytes(disk)
    slot = window.start(disk=trial_path, system=args.system, floppy=args.floppy)
    initial = media_hashes(slot)
    expected = dict(zip(MEDIA, (sha(args.system.read_bytes()), sha(disk), sha(args.floppy.read_bytes()))))
    if initial != expected or slot.protection != FLASH_PROTECTION:
        raise RuntimeError("Начальные образы или защита AT29 не совпали")
    out.mkdir()
    result = dict(
        scenario=name, worker=args.worker, file=record["file"], profile="MON",
        file_sha256=record["sha256"], container_sha256=sha(packed), payload_sha256=record["payload_sha256"],
        emulator_sha256=sha((slot.runtime / "Emu80qt-mcp.exe").read_bytes()),
        pid=slot.pid, hwnd=slot.hwnd, cold_boots=slot.cold_boots,
        initial_media=initial, initial_flash_protection=slot.protection,
    )
    try:
        guest = Guest(slot, out, args.build, "MON", system, result)
        guest.launch(dict(record=record, route=route), packed)
        play(guest, scenario)
        guest.recovery(marker)
        result.update(final_media=media_hashes(slot), final_flash_protection=slot.protection)
        if (not result["control_file"] or result["final_media"] != initial
                or result["final_flash_protection"] != FLASH_PROTECTION):
            raise RuntimeError("Не подтверждено восстановление NC, чтение H: или целостность носителей")
    finally:
        (out / "result.json").write_text(
            json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8",
        )
    print(f"{name} {out / 'result.json'}", flush=True)
    return slot.pid, slot.hwnd


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenarios", nargs="+", choices=list(SCENARIOS))
    parser.add_argument("--port", type=int, choices=range(19781, 19797))
    parser.add_argument("--worker")
    parser.add_argument("--out", type=Path, default=ROOT / "docs/compat/game-campaign/screen-replay")
    parser.add_argument("--system", type=Path, default=SYSTEM)
    parser.add_argument("--games", type=Path, default=DISK)
    parser.add_argument("--floppy", type=Path, default=ROOT / "images/A-Floppy-720k-Programs.img")
    parser.add_argument("--build", type=Path, default=ROOT / "bin")
    args = parser.parse_args()
    if len(args.scenarios) != len(set(args.scenarios)):
        parser.error("Каждый ход можно указать один раз")
    with (GAMES / "catalog.csv").open(encoding="utf-8-sig", newline="") as stream:
        rows = {row["File"]: row for row in csv.DictReader(stream)}
    system, _ = read_tree(args.system.read_bytes())
    out = args.out.resolve() / ("attempt-" + uuid.uuid4().hex[:12])
    out.mkdir(parents=True)
    window = WorkerMachine(16, args.port, out / "slots")
    instances = set()
    try:
        with tempfile.TemporaryDirectory(prefix="mxos-screen-") as workspace:
            trial_path = Path(workspace) / "H.bin"
            for name in args.scenarios:
                scenario = SCENARIOS[name]
                record = source_record(rows[scenario["file"]])
                instances.add(replay(args, name, scenario, record, window, system, out / name, trial_path))
    finally:
        window.close()
    if len(instances) != 1:
        raise RuntimeError("Между ходами сменилось окно")


if __name__ == "__main__":
    main()
