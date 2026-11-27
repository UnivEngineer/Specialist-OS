"""Проверить ход вправо и выход из «Алмаза-2» в архивном RAMFOS."""
import argparse
import csv
import json
from pathlib import Path
import runpy
import struct
import sys
import tempfile
import uuid

from campaign_guest import Image
from campaign_media import ROOT, sha
from lafan_native import ROM_SHA256, prepare_platform, switch_to_native

ramfos = runpy.run_path(str(Path(__file__).with_name("check-lafan-ramfos.py")))
SOURCE, GAMES, LAFAN, EXE, lease, checksum = (
    ramfos[name] for name in ("SOURCE", "GAMES", "LAFAN", "EXE", "lease", "checksum")
)
watch, seek, trace_state, list_name, launch, save_frame = (
    ramfos[name] for name in ("watch", "seek", "trace_state", "list_name", "launch", "save_frame")
)
FAMILY = "families/almaz-2/"
ARCHIVE = "unknown/lafan/almaz_2.com.bin"
BROKEN = FAMILY + "originals/Almaz2-ramfos-original-broken.cpu"
FIXED = FAMILY + "repaired/Almaz2-ramfos-fixed.cpu"
MONO = FAMILY + "originals/Almaz2-original.rks"
FIXED_RKS_SHA256 = "cde202db427d31d00d8b929bb9aabb4e08312ffe1fc29ce3d333e41bf82e5a83"
MAP = 0x8000
MAP_SIZE = 0x200
LOOP = 0x0038


def cases():
    with LAFAN.open(encoding="utf-8", newline="") as stream:
        row = next(row for row in csv.DictReader(stream) if row["File"] == ARCHIVE)
    archived_header = bytes.fromhex(row["HeaderHex"])
    archived_body = (GAMES/ARCHIVE).read_bytes()
    if sha(archived_body) != row["PayloadSHA256"]:
        raise ValueError("Изменилось архивное тело COM")
    yield "archive-com", archived_body, archived_header[:30], dict(
        file=ARCHIVE, origin="archival_odi_header", odi=row["Disk"], odi_sha256=row["DiskSHA256"],
        directory_offset=row["DirectoryOffset"], odi_header_hex=row["HeaderHex"],
    )
    for variant, file, disk_name in (("broken-rf", BROKEN, b"ALM2OLD  "), ("fixed-rf", FIXED, b"ALM2FIX  ")):
        body = (GAMES/file).read_bytes()
        header = bytearray(archived_header[:30])
        header[3:12] = disk_name
        struct.pack_into("<HHH", header, 24, 0, len(body)-1, checksum(body))
        rks = struct.pack("<HH", 0, len(body)-1) + body + struct.pack("<H", checksum(body))
        if variant == "fixed-rf" and sha(rks) != FIXED_RKS_SHA256:
            raise ValueError("RKS отличается от исправленного контейнера пробы RUN -RF")
        yield variant, body, bytes(header), dict(file=file, origin="derived_ramfos_com_header", rks_sha256=sha(rks))


def snapshot(machine, out, name):
    frame = save_frame(machine, out, name)
    with Image.open(out/frame["file"]) as image:
        rgb = image.convert("RGB")
        colors = rgb.getcolors(rgb.width * rgb.height)
        frame.update(size=list(rgb.size), rgb_sha256=sha(rgb.tobytes()), colors=len(colors),
                     palette=[dict(rgb=list(color), pixels=count) for count, color in sorted(colors, reverse=True)])
    return frame


def player(board):
    return [MAP + offset for offset in range(0x1C0, 0x1E0) if board[offset] & 15 == 9]


def game(machine, out, result):
    watch(machine, LOOP)
    result["initial_game_state"] = seek(machine, (LOOP,), 45000)
    before = machine.read(MAP, MAP_SIZE)
    (out/"map-before.bin").write_bytes(before)
    start = player(before)
    result.update(map_before_sha256=sha(before), start_cells=start, movement=[])
    result["frames"]["before-move"] = snapshot(machine, out, "before-move")
    if start != [0x81C1]:
        raise RuntimeError(f"Неожиданная начальная клетка игрока: {start}")

    machine.call("kbd_send", keys=["right"], action="press")
    try:
        for _ in range(12):
            machine.call("emu_run_for", ms=1100)
            state = trace_state(machine)
            current = player(machine.read(MAP, MAP_SIZE))
            result["movement"].append(dict(cells=current, state=state))
            if len(current) == 1 and 0x81C4 <= current[0] <= 0x81CF:
                break
    finally:
        machine.call("kbd_send", keys=["right"], action="release")
    after = machine.read(MAP, MAP_SIZE)
    (out/"map-after.bin").write_bytes(after)
    cells = player(after)
    result.update(map_after_sha256=sha(after), end_cells=cells)
    result["frames"]["after-move"] = snapshot(machine, out, "after-move")
    if len(cells) != 1 or not 0x81C1 < cells[0] <= 0x81CF:
        raise RuntimeError(f"Нет подтверждения перемещения вправо: {cells}")

    watch(machine, 0xF800)
    machine.call("kbd_send", keys=["clear"], action="press")
    try:
        result["exit_state"] = seek(machine, (0xF800,), 44000)
    finally:
        machine.call("kbd_send", keys=["clear"], action="release")
    result["frames"]["return-vector"] = snapshot(machine, out, "return-vector")
    watch(machine, 0xFA41)
    result["ramfos_state"] = seek(machine, (0xFA41,), 6000)
    result["frames"]["ramfos"] = snapshot(machine, out, "ramfos")


def trial(slot, ramdisk, out, body, header, result):
    out.mkdir()
    image = (header + body + b"\0\0\x08").ljust(8*65536, b"\0")
    ramdisk.write_bytes(image)
    load, end, recorded_sum = struct.unpack_from("<HHH", header, 24)
    if load != 0 or end+1 != len(body) or checksum(body) != recorded_sum:
        raise ValueError("Адреса или контрольная сумма COM не соответствуют телу")
    result.update(header_hex=header.hex(), load=load, end=end, checksum=recorded_sum,
                  payload_sha256=sha(body), ram_disk_sha256=sha(image),
                  command=["enter", "up", "enter", "right", "clear"],
                  frames={})
    machine = None
    try:
        machine = switch_to_native(slot)
        machine.call("emu_reset")
        machine.call("emu_run_for", ms=1000)
        result["listing"] = list_name(machine, header)
        result["frames"]["launch"] = snapshot(machine, out, "launch")
        result["loader_status"] = launch(machine, result, body, 0)
        if result["loader_status"] != "entry_and_payload_verified":
            raise RuntimeError("RAMFOS не подтвердил вход с совпадающим телом")
        game(machine, out, result)
        result["status"] = "move_and_return_verified"
    except (RuntimeError, OSError) as error:
        result.update(status="observation_error", error=str(error))
        if machine is not None:
            result["error_state"] = trace_state(machine)
            result["frames"]["error"] = snapshot(machine, out, "error")
    finally:
        (out/"result.json").write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return result["status"]


def compare(left, right):
    return [dict(offset=offset, left=a, right=b) for offset, (a, b) in enumerate(zip(left, right)) if a != b]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=SOURCE)
    parser.add_argument("--port", type=int, default=19796, choices=range(19781, 19797))
    parser.add_argument("--out", type=Path, default=ramfos["DEFAULT"]/"almaz2")
    args = parser.parse_args()
    out = args.out.resolve() / ("attempt-" + uuid.uuid4().hex[:12])
    print(out, flush=True)
    variants = list(cases())
    mono = (GAMES/MONO).read_bytes()
    archive, broken, fixed = (variant[1] for variant in variants)
    index = dict(rom_sha256=ROM_SHA256, results={}, comparison=dict(
        mono=dict(file=MONO, rks_sha256=sha(mono), body_sha256=sha(mono[4:-2]), body_length=len(mono[4:-2])),
        archive_body_length=len(archive), broken_body_length=len(broken), fixed_body_length=len(fixed),
        archive_mono_common_prefix_differences=len(compare(archive, mono[4:-2])),
        archive_broken_common_prefix_differences=compare(archive, broken),
        broken_fixed_differences=compare(broken, fixed),
    ))
    with lease(port=args.port, size=16, artifact_root=out/"slots") as slot, tempfile.TemporaryDirectory(
            prefix="mxos-almaz-ramfos-") as workspace:
        ramdisk = Path(workspace)/"almaz2.ramdisk"
        directory = prepare_platform(slot, args.source, f'ramDisk.fileName = "{ramdisk.as_posix()}"')
        common = dict(pid=slot.pid, hwnd=slot.hwnd, port=slot.port, rom_sha256=ROM_SHA256,
                      emulator_sha256=sha((slot.runtime/EXE).read_bytes()),
                      config_sha256=sha((directory/"spmx.conf").read_bytes()))
        for name, body, header, result in variants:
            result.update(common)
            status = trial(slot, ramdisk, out/name, body, header, result)
            index["results"][name] = dict(status=status, path=f"{name}/result.json")
            print(f"{name}: {status}", flush=True)
    boards = {name: path.read_bytes() for name in index["results"]
              if (path := out/name/"map-before.bin").is_file()}
    if len(boards) == len(variants):
        index["comparison"].update(
            map_address=MAP,
            archive_broken_first_map_diff=compare(boards["archive-com"], boards["broken-rf"]),
            broken_fixed_first_map_diff=compare(boards["broken-rf"], boards["fixed-rf"]),
        )
    (out/"index.json").write_text(json.dumps(index, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return int(any(row["status"] != "move_and_return_verified" for row in index["results"].values()))


if __name__ == "__main__":
    sys.exit(main())
