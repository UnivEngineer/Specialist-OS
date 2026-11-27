"""Проверить исходные записи Lafan через Enter в архивном RAMFOS."""
import argparse
from collections import Counter
import csv
import json
from pathlib import Path
import struct
import sys
import tempfile
import uuid

from campaign_media import ROOT, sha
from lafan_native import ROM_SHA256, prepare_platform, switch_to_native

sys.path[:0] = [str(ROOT / "scripts/emu80"), str(ROOT / "scripts/images")]
from executable import checksum
from game_inventory import GAMES, LAFAN, lafan_directory
from mcp_pool import EXE, lease


DEFAULT = ROOT / "docs/compat/game-campaign/native-ramfos"
SOURCE = ROOT / ".local/emu80-pool-pristine/dist/specmx"
PRINT_NAME = 0xFBF1
TYPE_DECISION = 0xFACE
CHECKSUM_DECISION = 0xFB54
SHELL_REJECT = 0xFA3D
LOAD_ERROR = 0xFE8E
TRANSFER = 0xFB14
HEADER_BUFFER = 0xD252
RUN_TYPES = {"EXE", "COM"}
FIRMWARE_MARKERS = {
    PRINT_NAME: b"\xCD\x0F\xC8", TYPE_DECISION: b"\xC2\x3D\xFA",
    CHECKSUM_DECISION: b"\xC3\x0D\xFB", LOAD_ERROR: b"\xCD\xB2\xFE", TRANSFER: b"\xC9",
}


def trace_state(machine):
    state = machine.call("cpu_state")
    return dict(registers={name: value["dec"] for name, value in state["registers"].items()},
                flags=state["flags"], ticks=state["cpu_ticks"])


def watch(machine, *pcs):
    machine.call("bp_clear")
    for pc in pcs:
        machine.call("bp_set", addr=pc, type="exec")


def seek(machine, pcs, ms=6000):
    state = trace_state(machine)
    if state["registers"]["pc"] not in pcs:
        machine.call("emu_run_for", ms=ms)
        state = trace_state(machine)
    if state["registers"]["pc"] not in pcs:
        raise RuntimeError(f"RAMFOS не достиг ожидаемой точки {pcs}: {state}")
    return state


def tap(machine, key):
    machine.call("kbd_send", keys=[key], action="tap", duration_ms=120)


def save_frame(machine, out, name):
    path = out / (name + ".png")
    machine.screenshot(path)
    return dict(file=path.name, sha256=sha(path.read_bytes()))


def list_name(machine, header):
    watch(machine, PRINT_NAME)
    tap(machine, "enter")
    observed = bytearray()
    stages = []
    for index in range(12):
        state = seek(machine, (PRINT_NAME,), 1000)
        regs = state["registers"]
        if regs["hl"] != HEADER_BUFFER + 4 + index:
            raise RuntimeError(f"Указатель печати имени сместился: {state}")
        if index == 0 and machine.read(HEADER_BUFFER, len(header)) != header:
            raise RuntimeError("RAMFOS печатает заголовок другой записи")
        observed.append((regs["af"] >> 8) & 255)
        stages.append(dict(addr=regs["hl"] - 1, code=observed[-1]))
        # Выполняется сам CALL вывода, следующая остановка возможна на новом символе.
        machine.call("debug_step", mode="into")
    if bytes(observed) != header[3:15]:
        raise RuntimeError("RAMFOS вывел другое имя или расширение")
    watch(machine)
    machine.call("emu_run_for", ms=400)
    tap(machine, "up")
    machine.call("emu_run_for", ms=150)
    selected = machine.read(HEADER_BUFFER, len(header))
    if selected != header:
        raise RuntimeError("Курсор RAMFOS стоит на другой записи")
    return dict(printed_hex=observed.hex(), characters=stages, selected_header_hex=selected.hex())


def launch(machine, result, body, load, load_limit=6000):
    watch(machine, TYPE_DECISION)
    tap(machine, "enter")
    decision = seek(machine, (TYPE_DECISION,), 1000)
    result["type_decision"] = decision
    if not decision["flags"]["Z"]:
        watch(machine, SHELL_REJECT)
        result["stop"] = seek(machine, (SHELL_REJECT,), 1000)
        watch(machine)
        machine.call("emu_run_for", ms=350)
        return "type_rejected"

    watch(machine, CHECKSUM_DECISION, LOAD_ERROR)
    compare = seek(machine, (CHECKSUM_DECISION, LOAD_ERROR), load_limit)
    result["checksum_decision"] = compare
    if compare["registers"]["pc"] == LOAD_ERROR:
        result["stop"] = compare
        watch(machine)
        machine.call("emu_run_for", ms=350)
        return "load_rejected"
    observed = machine.read(load, len(body))
    result.update(loaded_payload_sha256=sha(observed), body_matches_after_copy=observed == body)
    if not compare["flags"]["Z"]:
        watch(machine, LOAD_ERROR)
        result["stop"] = seek(machine, (LOAD_ERROR,))
        watch(machine)
        machine.call("emu_run_for", ms=350)
        return "checksum_rejected"

    # Первый RET FB14h возвращается в загрузчик. Второй использует адрес программы.
    watch(machine, TRANSFER)
    for _ in range(3):
        state = seek(machine, (TRANSFER,))
        stack_top = int.from_bytes(machine.read(state["registers"]["sp"], 2), "little")
        state["stack_target"] = stack_top
        result.setdefault("returns", []).append(state)
        machine.call("debug_step", mode="into")
        if stack_top == load:
            entry = trace_state(machine)
            result["stop"] = entry
            memory = machine.read(load, len(body))
            result.update(entry_payload_sha256=sha(memory), body_matches_at_entry=memory == body)
            if entry["registers"]["pc"] == load and memory == body:
                return "entry_and_payload_verified"
            return "entry_payload_mismatch"
    raise RuntimeError("RAMFOS закончил загрузку без подтверждения передачи управления")


def trial(slot, root, row, ramdisk, common):
    header = bytes.fromhex(row["HeaderHex"])
    body = (GAMES / row["File"]).read_bytes()
    load, end, recorded_sum = struct.unpack_from("<HHH", header, 24)
    if len(header) != 32 or header[:3] != b"\xD3"*3 or sha(body) != row["PayloadSHA256"] or end-load+1 != len(body):
        raise ValueError(f"Заголовок ODI и тело расходятся: {row['File']}")
    # Заголовок первой записи лежит в 0000h. Последние два байта записи
    # возвращают к её началу, 08h завершает каталог RAM-диска.
    image = (header[:30] + body + b"\0\0\x08").ljust(8*65536, b"\0")
    ramdisk.write_bytes(image)
    out = root / Path(row["File"]).name
    out.mkdir()
    extension = header[12:15].decode("ascii")
    result = dict(**common, file=row["File"], extension=extension,
                  group="EXE/COM" if extension in RUN_TYPES else "other",
                  title=header[3:12].decode("koi8-r").rstrip(),
                  header_hex=header.hex(), ram_header_hex=header[:30].hex(), load=load, end=end,
                  archive_checksum=recorded_sum, calculated_checksum=checksum(body), payload_sha256=sha(body),
                  directory_offset=row["DirectoryOffset"], odi=row["Disk"], odi_sha256=row["DiskSHA256"],
                  ram_disk_sha256=sha(image), command=["enter", "up", "enter"], frames={})
    machine = None
    try:
        machine = switch_to_native(slot)
        machine.call("emu_reset")
        machine.call("emu_run_for", ms=1000)
        for pc, code in FIRMWARE_MARKERS.items():
            if machine.read(pc, len(code)) != code:
                raise RuntimeError(f"Не совпали векторы архивного RAMFOS по {pc:04X}h")
        result["frames"]["date"] = save_frame(machine, out, "date")
        result["listing"] = list_name(machine, header[:30])
        result["frames"]["before"] = save_frame(machine, out, "before")
        result["before_payload_sha256"] = sha(machine.read(load, len(body)))
        result["status"] = launch(machine, result, body, load)
        result["frames"]["after"] = save_frame(machine, out, "after")
        result["final_payload_sha256"] = sha(machine.read(load, len(body)))
    except Exception as error:
        result.update(status="observation_error", error=str(error))
        if machine is not None:
            try:
                result["error_state"] = trace_state(machine)
                result["frames"]["error"] = save_frame(machine, out, "error")
            except (OSError, RuntimeError):
                pass
    finally:
        (out / "result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    return dict(file=result["file"], extension=extension, group=result["group"], status=result["status"],
                result=(out / "result.json").relative_to(root).as_posix())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=SOURCE, help="Каталог dist/specmx исходной сборки Emu80")
    parser.add_argument("--archive", type=Path, help="Дополнительно сверить записи с полными образами ODI")
    parser.add_argument("--port", type=int, choices=range(19781, 19797), default=19796)
    parser.add_argument("--out", type=Path, default=DEFAULT)
    parser.add_argument("--file", action="append", default=[], help="Точное имя из столбца File")
    args = parser.parse_args()
    if args.archive is not None and lafan_directory(args.archive) != LAFAN.read_text(encoding="utf-8"):
        raise ValueError("Записи ODI отличаются от отслеживаемого реестра Lafan")
    with LAFAN.open(encoding="utf-8", newline="") as stream:
        indexed = {row["File"]: row for row in csv.DictReader(stream)}
    unknown = sorted(set(args.file) - indexed.keys())
    if unknown:
        parser.error("Не найдены записи " + ", ".join(unknown))
    rows = list(indexed.values()) if not args.file else [indexed[name] for name in dict.fromkeys(args.file)]
    out = args.out.resolve() / ("attempt-" + uuid.uuid4().hex[:12])
    print(out, flush=True)
    results = []
    with lease(port=args.port, size=16, artifact_root=out/"slots") as slot, tempfile.TemporaryDirectory(
            prefix="mxos-lafan-ramfos-") as workspace:
        ramdisk = Path(workspace) / "one-file.ramdisk"
        directory = prepare_platform(slot, args.source, f'ramDisk.fileName = "{ramdisk.as_posix()}"')
        common = dict(platform="spmx", rom_sha256=ROM_SHA256,
                      emulator_sha256=sha((slot.runtime / EXE).read_bytes()),
                      config_sha256=sha((directory/"spmx.conf").read_bytes()),
                      pid=slot.pid, hwnd=slot.hwnd, port=slot.port)
        for index, row in enumerate(rows, 1):
            result = trial(slot, out, row, ramdisk, common)
            results.append(result)
            print(f"{index}/{len(rows)} {result['extension']} {result['status']} {result['file']}", flush=True)
    counts = {group: Counter(r["status"] for r in results if r["group"] == group) for group in ("EXE/COM", "other")}
    (out/"index.json").write_text(json.dumps(dict(schema=1, complete=not args.file, counts=counts, results=results),
                                            ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    return int(any(result["status"] == "observation_error" for result in results))


if __name__ == "__main__":
    sys.exit(main())
