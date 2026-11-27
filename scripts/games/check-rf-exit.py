"""Проверить F4 из одного состояния игры в архивном RAMFOS и RUN -RF."""
import argparse
from contextlib import ExitStack
from pathlib import Path
import runpy
import sys
import tempfile
import uuid

from campaign_guest import Guest, Observation
from campaign_media import ROOT, read_tree, sha, trial_disk
from campaign_play import gate, action_to

native = runpy.run_path(str(Path(__file__).with_name("check-rf-ramfos.py")))
from game_inventory import build_route
from mcp_pool import FLASH_PROTECTION, WorkerMachine

CASES = {
    "pikfama": (0x0430, 0x0433, 0x0446),
    "poker": (0x03D8, 0x03DB, 0x2014),
}
MATRIX_READS = bytes.fromhex("3ae1ff47f603feffc83e9132e3ff3ae0ff6f3ae2fff6f067bdc8")


class KeyCodeDifference(Observation):
    """Игра получила другой код клавиши."""


def read_state(guest, name):
    return dict(frame=guest.frame(name), cpu=native["trace_state"](guest))


def ready(guest, case, start_key):
    waiting, _, _ = CASES[case]
    if case == "pikfama":
        gate(guest, waiting, 30000)
        return []
    gate(guest, 0x002B, 20000)
    guest.result["start"] = read_state(guest, "start")
    key_codes(guest, start_key, 0x002E, {0x02}, "start_key_samples")
    gate(guest, 0x0054, 5000)
    guest.points(0x01C8, waiting)
    stop = guest.advance(30000)
    if stop == 0x01C8:
        action_to(guest, "enter", 0x0069, 4000)
        gate(guest, waiting, 15000)
    elif stop != waiting:
        raise Observation(f"Не достигнуто ожидание ставки: {stop:04X}h")
    return [start_key, *[event["key"] for event in guest.keys if event.get("key") and not event.get("command")]]


def key_codes(guest, key, key_return, expected, name):
    vector = guest.read(0xC81B, 3)
    entry = int.from_bytes(vector[1:], "little")
    offset = guest.read(entry, 0x100).find(MATRIX_READS)
    if vector[0] != 0xC3 or offset < 0:
        raise Observation("В вызове C81Bh не найдены штатные чтения портов сканирования клавиатуры")
    row_pc, column_pc = entry + offset + 3, entry + offset + 24
    guest.points(key_return, row_pc, column_pc)
    guest.call("kbd_send", keys=[key], action="press")
    samples = guest.result.setdefault(name, [])
    matrix = []
    guest.result[name + "_matrix"] = dict(key=key, row_return_pc=row_pc, column_return_pc=column_pc, samples=matrix)
    try:
        for _ in range(48):
            actual = guest.advance(1000)
            registers = guest.regs()
            code = registers["af"] >> 8
            if actual == row_pc:
                matrix.append(dict(port="FFE1h", raw=code, closed_mask=(~code & 0xFC) >> 2))
                continue
            if actual == column_pc:
                columns = registers["hl"] & 0xFFF
                matrix.append(dict(ports="FFE0h, FFE2h", raw=columns, closed_mask=~columns & 0xFFF))
                continue
            samples.append(dict(pc=actual, a=code))
            if actual != key_return:
                break
            if code in expected:
                return
            if code != 0xFF:
                break
    finally:
        guest.call("kbd_send", keys=[key], action="release")
    if any(sample["pc"] == key_return and sample["a"] != 0xFF for sample in samples):
        raise KeyCodeDifference(f"После {key} по адресу {key_return:04X}h BIOS вернул другой код: {samples}")
    raise Observation(f"После {key} по адресу {key_return:04X}h ожидался код {sorted(expected)}, получено {samples}")


def exit_from_wait(guest, case, result, start_key, exit_key):
    waiting, key_return, route = CASES[case]
    result["start_keys"] = ready(guest, case, start_key)
    result["before"] = read_state(guest, "before")
    result["wait_instruction"] = guest.read(waiting, 3).hex()
    result["exit_instruction"] = guest.read(route, 8).hex()
    result["bios_vectors"] = {f"{p:04x}": guest.read(p, 3).hex() for p in (0xF81B, 0xF800, 0xC81B)}
    keyboard = int.from_bytes(guest.read(0xC60E, 2), "little")
    result["keyboard_map"] = dict(address=keyboard, top_row=guest.read(keyboard + 0x50, 12).hex())
    result["key"] = exit_key
    key_codes(guest, exit_key, key_return, {0x03, 0x1F} if case == "poker" else {0x03}, "action_key_samples")
    result["key_return"] = read_state(guest, "key-return")
    gate(guest, route, 5000)
    result["route"] = read_state(guest, "route")
    gate(guest, 0xF800, 5000)
    result["exit_vector"] = read_state(guest, "exit-vector")
    guest.watch_exit(0xF800)
    result["exit"] = guest.exit_seen
    result["status"] = "f4_and_exit_vector_verified"


def measure(guest, folder, case, native_rom, match_matrix):
    result = guest.result
    start_key, exit_key = ("f3", "f4") if native_rom or not match_matrix else ("f5", "f6")
    try:
        exit_from_wait(guest, case, result, start_key, exit_key)
        if native_rom:
            native["check_return"](guest)
        result["after"] = read_state(guest, "after")
    except Observation as error:
        status = "guest_code_mismatch" if isinstance(error, KeyCodeDifference) else "stage_unconfirmed"
        result.update(status=status, error=str(error), obstacle=read_state(guest, "obstacle"))
    native["save"](folder/"observation.json", result)
    return result["status"]


def run(args, case, folder):
    directory = args.campaign.resolve()
    inventory = native["read_json"](ROOT/"docs/compat/game-inventory.json")
    cards = native["read_json"](ROOT/"assets/games/controls.json")["cards"]
    main = native["read_json"](directory/"results.json")["trials"]
    job = next(job for job in native["campaign"]["jobs"](inventory["files"], cards)
               if job["profile"] == "RF" and job["record"]["file"] == f"assets/games/mx/{case}.cpu")
    job, body, header, primary, _, metadata = native["plan_group"](directory, [job], main, native["read_archive"]())
    if header is None:
        raise ValueError("Нет записи RAMFOS для точного тела")
    folder.mkdir()
    statuses = {"RAMFOS": [], "RUN": []}
    fixture = directory/"fixtures"/primary["context"]["fixture"]
    system_files = {name.upper(): data for name, data in read_tree((fixture/"C.bin").read_bytes())[0].items()}
    packed = build_route(job["record"], job["route"])
    disk, marker = trial_disk(packed, job["route"]["kind"], fixture/"H.bin")
    if sha(disk) != primary["game_disk_sha256"]:
        raise ValueError("У пробы RUN другой образ H:")
    expected = dict(payload_sha256=sha(body), entry=metadata["entry"], load=metadata["load"])
    with tempfile.TemporaryDirectory(prefix="mxos-rf-exit-") as workspace:
        work = Path(workspace)
        image = work/"archive.ramdisk"
        image.write_bytes((header + body + b"\0\0\x08").ljust(8*65536, b"\0"))
        disk_image = work/"H.bin"
        disk_image.write_bytes(disk)
        with native["lease"](port=args.port, size=16, artifact_root=folder/"slots-ramfos") as slot:
            conf = native["prepare_platform"](slot, args.source, f'ramDisk.fileName = "{image.as_posix()}"')
            common = dict(**expected, rom_sha256=native["ROM_SHA256"], pid=slot.pid, hwnd=slot.hwnd,
                          emulator_sha256=sha((slot.runtime/native["EXE"]).read_bytes()), port=slot.port,
                          keyboard_matrix="ramfos", config_sha256=sha((conf/"spmx.conf").read_bytes()))
            for trial in (1, 2):
                target = folder/f"ramfos-{trial}"
                guest, result = native["boot"](slot, target, image, common, body, header, metadata["load"])
                if result["launch"] != native["VERIFIED_ENTRY"]:
                    raise RuntimeError("Исторический RAMFOS отклонил испытательную запись")
                statuses["RAMFOS"].append(dict(status=measure(guest, target, case, True, args.match_matrix),
                                              observation=(target/"observation.json").relative_to(folder).as_posix()))
        with ExitStack() as lifetime:
            worker = WorkerMachine(16, args.port, folder/"slots-run")
            lifetime.callback(worker.close)
            for trial in (1, 2):
                slot = worker.start(disk=disk_image, system=fixture/"C.bin", floppy=fixture/"A.img")
                target = folder/f"run-{trial}"
                target.mkdir()
                report = dict(**expected, pid=slot.pid, hwnd=slot.hwnd, port=slot.port,
                              initial_flash_protection=slot.protection,
                              system_sha256=primary["context"]["system_sha256"],
                              ramfos_sha256=primary["context"]["kernels"]["DOS/RAMFOS.MON"],
                              keyboard_matrix="original", config_sha256=primary["context"]["profile_sha256"],
                              emulator_sha256=sha((slot.runtime/native["EXE"]).read_bytes()))
                if slot.protection != FLASH_PROTECTION:
                    raise RuntimeError("Состояние защиты начальных носителей RUN изменилось")
                guest = Guest(slot, target, fixture/"build", "RF", system_files, report)
                guest.launch(job, packed)
                status = measure(guest, target, case, False, args.match_matrix)
                guest.recovery(marker)
                report["after_shell"] = read_state(guest, "after-shell")
                native["save"](target/"observation.json", report)
                statuses["RUN"].append(dict(status=status, observation=(target/"observation.json").relative_to(folder).as_posix()))
    native["save"](folder/"index.json", dict(**expected, rf=metadata["selected_primary"],
                                          match_matrix=args.match_matrix, stages=statuses))
    published = "f4_and_exit_vector_verified"
    allowed = {published} if args.match_matrix else {published, "guest_code_mismatch"}
    return (all(arm["status"] == published for arm in statuses["RAMFOS"])
            and len({arm["status"] for arm in statuses["RUN"]}) == 1
            and statuses["RUN"][0]["status"] in allowed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=CASES, required=True)
    parser.add_argument("--source", type=Path, default=native["SOURCE"])
    parser.add_argument("--campaign", type=Path, default=native["DEFAULT"])
    parser.add_argument("--port", type=int, default=19796, choices=range(19781, 19797))
    parser.add_argument("--match-matrix", action="store_true",
                        help="Совместить позиции верхнего ряда: F3/F4 MX соответствуют F5/F6 Original")
    args = parser.parse_args()
    out = args.campaign.resolve()/"native-ramfos"/f'{args.case}-exit-attempt-{uuid.uuid4().hex[:12]}'
    print(out, flush=True)
    return int(not run(args, args.case, out))


if __name__ == "__main__":
    sys.exit(main())
