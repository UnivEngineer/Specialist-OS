"""Пройти штатный F6-RUN МХ-ДОС 3.6 на исходных дискетах Lafan."""

import argparse
from collections import Counter
from contextlib import ExitStack
import json
from pathlib import Path
import sys
import tempfile
import traceback
import uuid

from campaign_media import ROOT, sha
from lafan_fdc import CATALOG, GEOMETRY, TEMPLATE, catalog_rows, checked_disks, load_template, state_pc
from lafan_native import ROM_SHA256, prepare_platform, switch_to_native

sys.path.insert(0, str(ROOT / "scripts/emu80"))
from mcp_pool import lease


OUT = ROOT / "docs/compat/game-campaign/native-fdc-run"
SECTOR_BYTES = GEOMETRY[3]
DIRECTORY_ROOT = 0x3000
HEADER_BYTES = 32
HEADER_COMPARE = 30
RESIDENT = 0xE400
BOOT_STATUS = 0xF88F
BOOT_ERROR = 0xFE8E
DIRECTORY_BUFFER = 0xE000
DIRECTORY_BYTES = 0x400
ACTIVE_DRIVE = 0xD96D
SELECTED_HEADER = 0xD252
SELECTED_CURSOR = 0xD270
MENU_READY = 0xE487
KEY_ACCEPTED = 0xE4A8
RUN_CONFIRM = 0xEFB9
TYPE_DECISION = 0xEFDD
DOS_REJECT = 0xEB34
ROM_COMPARE = 0xFB51
CHECKSUM_BRANCH = 0x9009
RAMFOS_ENTRY = 0xF800
RUN_TRAMPOLINE = 0x900C
KEY_CODES = {"left": 0x08, "enter": 0x0D, "up": 0x19, "down": 0x1A, "f6": 0x05}


class Outcome(Exception):
    def __init__(self, status, message):
        self.status = status
        super().__init__(message)


def write_json(path, value):
    temporary = path.with_suffix(f".{uuid.uuid4().hex}.tmp")
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def word(data):
    return int.from_bytes(data, "little")


def header_name(header):
    return header[3:12].decode("koi8-r").rstrip()


def directory_paths(disk):
    paths, visited = {}, set()

    def scan(start, parents):
        if start in visited:
            return
        if start % SECTOR_BYTES or not 0 <= start <= len(disk) - SECTOR_BYTES:
            raise ValueError(f"Неверный адрес каталога ODI {start:05X}h")
        visited.add(start)
        # Последний заголовок сектора указывает на родительский каталог.
        for offset in range(start, start + SECTOR_BYTES - HEADER_BYTES, HEADER_BYTES):
            header = disk[offset:offset + HEADER_BYTES]
            if header[:3] != b"\xd3" * 3:
                continue
            paths[offset] = (*parents, offset)
            if header[12:15] == b"   ":
                track, sector = header[20], header[19]
                child = (track * GEOMETRY[1] * GEOMETRY[2] + sector - 1) * SECTOR_BYTES
                scan(child, paths[offset])

    scan(DIRECTORY_ROOT, ())
    return paths


def source_cases(disks):
    paths = {name: directory_paths(disk) for name, disk in disks.items()}
    for row in catalog_rows():
        disk = disks[row["Disk"]]
        offset = int(row["DirectoryOffset"], 16)
        header = bytes.fromhex(row["HeaderHex"])
        payload = (ROOT / "assets/games" / row["File"]).read_bytes()
        load, end = word(header[24:26]), word(header[26:28])
        if (disk[offset:offset + HEADER_BYTES] != header or sha(payload) != row["PayloadSHA256"]
                or len(payload) != end - load + 1 or offset not in paths[row["Disk"]]):
            raise ValueError(f"Заголовок, тело или каталог не совпали для {row['File']}")
        yield dict(disk=row["Disk"], disk_sha256=row["DiskSHA256"], file=row["File"], offset=offset,
                   load=load, end=end, payload=payload, payload_sha256=row["PayloadSHA256"],
                   header=header, type=header[12:15].decode("ascii"), path=paths[row["Disk"]][offset])


def points(machine, addresses):
    machine.call("bp_clear")
    for address in addresses:
        machine.call("bp_set", addr=address, type="exec")


def run_to(machine, addresses, milliseconds):
    points(machine, addresses)
    return machine.call("emu_run_for", ms=milliseconds)


def tap(machine, key, duration=150):
    return machine.call("kbd_send", keys=[key], action="tap", duration_ms=duration)


def frame(machine, directory, proof, name):
    path = directory / f"{name}.png"
    machine.screenshot(path)
    proof.setdefault("frames", {})[name] = dict(file=path.name, sha256=sha(path.read_bytes()))


def boot(machine, directory, source, proof):
    machine.call("emu_reset")
    machine.call("emu_run_for", ms=4500)
    frame(machine, directory, proof, "date")
    tap(machine, "enter", 120)
    machine.call("emu_run_for", ms=800)
    frame(machine, directory, proof, "ramfos")
    points(machine, (BOOT_STATUS,))
    tap(machine, "f6")
    guard = machine.call("cpu_state")
    if state_pc(guard) != BOOT_STATUS:
        guard = machine.call("emu_run_for", ms=4000)
    if state_pc(guard) != BOOT_STATUS:
        raise Outcome("boot_guard_unconfirmed", "Клавиша F6 не дошла до проверки ВГ93")
    body = machine.read(RESIDENT, 5120)
    (directory / "resident.bin").write_bytes(body)
    status = guard["registers"]["af"]["dec"] >> 8
    next_sector = machine.read(0xFFEA, 1)[0]
    proof["boot"] = dict(keys=["enter", "f6"], guard=guard, status=status, next_sector=next_sector, file="resident.bin",
                         body_sha256=sha(body), expected_sha256=sha(source[:5120]),
                         mismatching_bytes=sum(a != b for a, b in zip(body, source[:5120], strict=True)))
    branch = run_to(machine, (BOOT_ERROR, RESIDENT), 1500)
    proof["boot"]["branch_pc"] = state_pc(branch)
    if body != source[:5120]:
        raise Outcome("boot_payload_mismatch", "Резидент до проверки не совпал с исходным первым треком")
    if (status, next_sector, state_pc(branch)) != (1, 6, RESIDENT):
        raise Outcome("boot_rejected", "ROM не передал управление исходному МХ-ДОС")
    machine.call("bp_clear")
    machine.call("emu_run_for", ms=10000)
    frame(machine, directory, proof, "mxdos")


def menu_key(machine, proof, name):
    points(machine, (KEY_ACCEPTED,))
    tap(machine, name)
    state = machine.call("cpu_state")
    code = state["registers"]["af"]["dec"] >> 8
    proof.setdefault("keys", []).append(dict(key=name, pc=state_pc(state), code=code, prompt="mxdos"))
    if state_pc(state) != KEY_ACCEPTED or code != KEY_CODES[name]:
        raise Outcome("menu_key_unconfirmed", f"МХ-ДОС не приняла клавишу {name}")


def settle_menu(machine):
    state = run_to(machine, (MENU_READY,), 10000)
    if state_pc(state) != MENU_READY:
        raise Outcome("menu_unconfirmed", "МХ-ДОС не вернулась к файловой таблице")


def selection(machine, header, proof):
    table = machine.read(DIRECTORY_BUFFER, DIRECTORY_BYTES)
    expected = header[:HEADER_COMPARE]
    matches = [index + DIRECTORY_BUFFER for index in range(0, len(table), HEADER_BYTES)
               if table[index:index + HEADER_COMPARE] == expected]
    if len(matches) != 1:
        raise Outcome("directory_header_unconfirmed", f"Заголовок {header_name(header)} в таблице приводa A: найден {len(matches)} раз")
    target = matches[0]
    for _ in range(DIRECTORY_BYTES // HEADER_BYTES + 2):
        selected = word(machine.read(SELECTED_CURSOR, 2))
        if selected == target:
            live = machine.read(SELECTED_HEADER, HEADER_BYTES)
            if live[:HEADER_COMPARE] != expected:
                raise Outcome("selected_header_mismatch", "Фактический заголовок D252h не совпал с ODI")
            return dict(name=header_name(header), address=target, observed_header=live.hex())
        menu_key(machine, proof, "up" if selected > target else "down")
        settle_menu(machine)
    raise Outcome("selection_unconfirmed", f"Курсор не достиг {header_name(header)}")


def checksum_decision(machine, directory, case, proof):
    ending = run_to(machine, (ROM_COMPARE, DOS_REJECT, MENU_READY, 0xF11B), 16000)
    proof["branch_pc"] = state_pc(ending)
    if state_pc(ending) != ROM_COMPARE:
        machine.call("bp_clear")
        machine.call("emu_run_for", ms=250)
        frame(machine, directory, proof, "load-obstacle")
        raise Outcome("load_unconfirmed", "Тип принят, сравнение контрольной суммы не наблюдалось")
    registers = ending["registers"]
    declared, actual = registers["hl"]["dec"], registers["de"]["dec"]
    return_pc = word(machine.read(registers["sp"]["dec"], 2))
    proof["checksum"] = dict(pc=ROM_COMPARE, declared=declared, actual=actual, return_pc=return_pc)
    if declared != word(case["header"][28:30]) or return_pc != CHECKSUM_BRANCH:
        raise Outcome("checksum_path_mismatch", "Сравнение ROM получило другой заголовок или адрес возврата")
    status = run_to(machine, (CHECKSUM_BRANCH,), 1500)
    equal = status["flags"]["Z"]
    proof["checksum"].update(branch_pc=state_pc(status), accepted=equal)
    if state_pc(status) != CHECKSUM_BRANCH or equal != (declared == actual):
        raise Outcome("checksum_decision_unconfirmed", "Условный переход ROM не подтвердил сравнение суммы")
    if equal:
        return True
    raw = machine.read(case["load"], len(case["payload"]))
    filename = "copied-body.bin"
    (directory / filename).write_bytes(raw)
    proof["checksum"].update(copied_body_file=filename, copied_body_sha256=sha(raw),
                             copied_body_matches=raw == case["payload"])
    rejected = run_to(machine, (RAMFOS_ENTRY,), 1500)
    proof["checksum"]["destination_pc"] = state_pc(rejected)
    if state_pc(rejected) != RAMFOS_ENTRY:
        raise Outcome("checksum_rejection_unconfirmed", "Не наблюдался возврат ROM после несовпадения суммы")
    machine.call("bp_clear")
    machine.call("emu_run_for", ms=800)
    frame(machine, directory, proof, "checksum-rejected")
    return False


def run_file(machine, directory, disk, case, proof):
    menu_key(machine, proof, "left")
    settle_menu(machine)
    if machine.read(ACTIVE_DRIVE, 1) != b"A":
        raise Outcome("drive_unconfirmed", "Курсор не перешёл на исходный привод A:")
    proof["selected"] = []
    for offset in case["path"]:
        header = disk[offset:offset + HEADER_BYTES]
        current = selection(machine, header, proof)
        proof["selected"].append(dict(odi_offset=offset, **current))
        if offset != case["offset"]:
            menu_key(machine, proof, "enter")
            settle_menu(machine)
    frame(machine, directory, proof, "selected")

    menu_key(machine, proof, "f6")
    prompt = run_to(machine, (RUN_CONFIRM, DOS_REJECT, MENU_READY), 4000)
    proof["run_prompt_pc"] = state_pc(prompt)
    if state_pc(prompt) != RUN_CONFIRM:
        raise Outcome("run_confirmation_unconfirmed", "F6-RUN не дошла до запроса ВК")
    frame(machine, directory, proof, "confirm")
    points(machine, (TYPE_DECISION,))
    tap(machine, "enter")
    decision = machine.call("cpu_state")
    if state_pc(decision) != TYPE_DECISION:
        decision = machine.call("emu_run_for", ms=3000)
    if state_pc(decision) != TYPE_DECISION:
        raise Outcome("type_decision_unconfirmed", "Штатная ветвь RUN не дошла до проверки типа")
    live = machine.read(SELECTED_HEADER, HEADER_COMPARE)
    proof["type_decision"] = dict(pc=state_pc(decision), accepted=decision["flags"]["Z"],
                                  type=live[12:15].decode("ascii", errors="replace"), header=live.hex())
    if live != case["header"][:HEADER_COMPARE]:
        raise Outcome("run_header_mismatch", "У проверки типа другой заголовок")
    if not decision["flags"]["Z"]:
        rejected = run_to(machine, (DOS_REJECT,), 1500)
        proof["branch_pc"] = state_pc(rejected)
        if state_pc(rejected) != DOS_REJECT:
            raise Outcome("type_rejection_unconfirmed", "Переход на штатный отказ не наблюдался")
        machine.call("bp_clear")
        machine.call("emu_run_for", ms=400)
        frame(machine, directory, proof, "rejected")
        return "type_rejected"

    if not checksum_decision(machine, directory, case, proof):
        return "checksum_rejected"
    ending = run_to(machine, (RUN_TRAMPOLINE,), 1500)
    proof["branch_pc"] = state_pc(ending)
    if state_pc(ending) != RUN_TRAMPOLINE:
        raise Outcome("trampoline_unconfirmed", "Переход после принятой суммы не достиг PCHL")
    entry = ending["registers"]["hl"]["dec"]
    proof["trampoline"] = dict(pc=RUN_TRAMPOLINE, target=entry, opcode=machine.read(RUN_TRAMPOLINE, 1)[0])
    if proof["trampoline"]["opcode"] != 0xE9 or entry != case["load"]:
        raise Outcome("trampoline_mismatch", "Адрес или инструкция передачи управления отличаются от заголовка")
    opened = run_to(machine, (entry,), 1000)
    if state_pc(opened) != entry:
        raise Outcome("entry_unconfirmed", "Инструкция PCHL не дошла до адреса в заголовке")
    observed = machine.read(entry, len(case["payload"]))
    (directory / "entry.bin").write_bytes(observed)
    proof["entry"] = dict(state=opened, actual_sha256=sha(observed), bytes=len(observed), file="entry.bin",
                          mismatching_bytes=sum(a != b for a, b in zip(observed, case["payload"], strict=True)))
    frame(machine, directory, proof, "entry")
    machine.call("bp_clear")
    proof["first_state"] = machine.call("emu_run_for", ms=450)
    frame(machine, directory, proof, "first-state")
    return "entry_payload_verified" if observed == case["payload"] else "entry_payload_mismatch"


def trial(slot, directory, disk, case):
    directory.mkdir(parents=True)
    proof = {name: value for name, value in case.items() if name not in {"payload", "header"}}
    proof.update(rom_sha256=ROM_SHA256, header_hex=case["header"].hex(), pid=slot.pid,
                 hwnd=json.loads((slot.artifacts / "slot.json").read_bytes())["hwnd"],
                 emulator_sha256=sha((slot.runtime / "Emu80qt-mcp.exe").read_bytes()),
                 path_names=[header_name(disk[offset:offset + HEADER_BYTES]) for offset in case["path"]])
    location = ROOT / ".local/lafan-fdc"
    try:
        machine = switch_to_native(slot)
        with tempfile.TemporaryDirectory(prefix="mxdos-", dir=location) as temporary:
            scratch = Path(temporary) / case["disk"]
            scratch.write_bytes(disk)
            machine.call("disk_attach", drive="A", file=scratch.as_posix())
            try:
                boot(machine, directory, disk, proof)
                proof["outcome"] = run_file(machine, directory, disk, case, proof)
            except Outcome as obstacle:
                proof.update(outcome=obstacle.status, obstacle=str(obstacle), stopped_at=machine.call("cpu_state"))
                frame(machine, directory, proof, "obstacle")
            finally:
                machine.call("disk_detach", drive="A")
            proof["mounted_disk_after_sha256"] = sha(scratch.read_bytes())
    except Exception as error:
        proof.update(outcome="infrastructure_error", error=str(error), traceback=traceback.format_exc())
    report = directory / "result.json"
    write_json(report, proof)
    return proof, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="Каталог с пятью исходными ODI")
    parser.add_argument("--template", type=Path, default=TEMPLATE)
    parser.add_argument("--out", type=Path, default=OUT)
    parser.add_argument("--port", type=int, choices=range(19781, 19797))
    parser.add_argument("--file", action="append", default=[], help="Ограничить маршрут именем файла из реестра")
    parser.add_argument("--resume", help="Продолжить указанный каталог attempt-* с теми же входными данными")
    args = parser.parse_args()
    args.out = args.out.resolve()
    build = load_template(args.template)
    disks = checked_disks(args.archive)
    cases = [case for case in source_cases(disks) if not args.file or any(
        case["file"] == name or Path(case["file"]).name == name for name in args.file)]
    if not cases:
        parser.error("Нет совпавших записей каталога")
    cases.sort(key=lambda item: (item["disk"], item["path"]))
    inputs = dict(build=build, profile="specmx", rom_sha256=ROM_SHA256, geometry=list(GEOMETRY),
                  catalog_sha256=sha(CATALOG.read_bytes()), runner_sha256=sha(Path(__file__).read_bytes()),
                  disks={name: sha(contents) for name, contents in disks.items()},
                  selection=[[case["disk"], case["offset"], case["payload_sha256"]] for case in cases])
    attempts = (args.out / (args.resume or ("attempt-" + uuid.uuid4().hex[:12]))).resolve()
    if attempts.parent != args.out or not attempts.name.startswith("attempt-"):
        parser.error("Каталог повтора должен быть attempt-* внутри --out")
    if args.resume:
        if json.loads((attempts / "inputs.json").read_bytes()) != inputs:
            parser.error("Исходные данные или проверяющий скрипт отличаются от начатого прогона")
    else:
        attempts.mkdir(parents=True)
        write_json(attempts / "inputs.json", inputs)
    startup_path = attempts / "startup.json"
    startup = json.loads(startup_path.read_bytes()) if startup_path.is_file() else None
    entries = []
    with ExitStack() as leases:
        slot = None
        for index, case in enumerate(cases, 1):
            directory = attempts / Path(case["disk"]).stem / f"{case['offset']:05x}"
            progress = directory.with_name(directory.name + "-progress.json")
            previous = json.loads(progress.read_bytes()) if progress.is_file() else None
            if previous and previous["outcome"] != "infrastructure_error":
                saved = json.loads((args.out / previous["protocol"]).read_bytes())
                if (saved["header_hex"] != case["header"].hex() or saved["payload_sha256"] != case["payload_sha256"]
                        or saved["disk_sha256"] != case["disk_sha256"] or saved["emulator_sha256"] != build["executableSha256"]):
                    raise ValueError(f"Завершённый протокол расходится с входными данными {progress}")
                entries.append(previous)
                print(f"{index}/{len(cases)} {case['disk']} {case['offset']:05X} {case['type']} cached {saved['outcome']}", flush=True)
                continue
            protocols = [path.relative_to(args.out).as_posix() for path in sorted(directory.parent.glob(directory.name + "*/result.json"))]
            for _ in range(2):
                if slot is None:
                    slot = leases.enter_context(lease(size=16, port=args.port, artifact_root=args.out / "slots"))
                    if sha((slot.runtime / "Emu80qt-mcp.exe").read_bytes()) != build["executableSha256"]:
                        raise RuntimeError("В слоте запущена другая сборка Emu80")
                    config = prepare_platform(slot, args.template / "specmx", options=build["property"])
                    current = dict(config_sha256=sha((config / "spmx.conf").read_bytes()),
                                   options_sha256=sha((config / "spmx.opt").read_bytes()))
                    if startup is not None and current != startup:
                        raise ValueError("Конфигурация нового окна отличается от первого старта")
                    startup = current
                    write_json(startup_path, startup)
                target = directory
                if target.exists():
                    target = directory.with_name(directory.name + "-retry-" + uuid.uuid4().hex[:8])
                result, report = trial(slot, target, disks[case["disk"]], case)
                protocols.append(report.relative_to(args.out).as_posix())
                if result["outcome"] in ("type_rejected", "checksum_rejected", "entry_payload_verified"):
                    break
                if result["outcome"] == "infrastructure_error":
                    leases.close()
                    slot = None
            row = {name: result[name] for name in ("file", "disk", "disk_sha256", "offset", "path_names",
                                                  "header_hex", "type", "load", "end", "payload_sha256", "outcome")}
            row.update(protocol=protocols[-1], attempts=protocols)
            write_json(progress, row)
            entries.append(row)
            print(f"{index}/{len(cases)} {case['disk']} {case['offset']:05X} {case['type']} {row['outcome']}", flush=True)
    summary = dict(**inputs, **startup, cases=entries,
                   counts=Counter(row["outcome"] for row in entries))
    write_json(attempts / "index.json", summary)
    write_json(args.out / "results.json", summary)


if __name__ == "__main__":
    main()
