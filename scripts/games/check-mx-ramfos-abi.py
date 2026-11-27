"""Проследить вызовы BIOS одного игрового тела в архивном RAMFOS и RUN -MX."""
import argparse
from contextlib import ExitStack
from pathlib import Path
import runpy
import sys
import tempfile
import uuid

from campaign_guest import Guest, Observation
from campaign_media import ROOT, read_tree, sha, trial_disk
from campaign_play import gate

native = runpy.run_path(str(Path(__file__).with_name("check-rf-ramfos.py")))
from game_inventory import build_route
from mcp_pool import FLASH_PROTECTION, WorkerMachine

FILES = {"almaz2": "Almaz2-mx.exe", "starwars2": "StarWars2-mx.cpu", "barbar": "barbar.cpu"}


def snapshot(guest, name):
    return dict(cpu=native["trace_state"](guest), frame=guest.frame(name))


def almaz(guest):
    gate(guest, 0x0038, 45000)
    result = guest.result
    result["before"] = snapshot(guest, "before")
    result["bios_code"] = {f"{pc:04x}": guest.read(pc, 8).hex() for pc in (0xC812, 0xC81B)}
    if guest.read(0x0165, 5) != bytes.fromhex("cd12c8fe3b"):
        raise Observation("Сборка не вызывает C812h по ожидаемому адресу")

    def player():
        return [0x81C0+i for i, tile in enumerate(guest.read(0x81C0, 32)) if tile & 15 == 9]

    result["start_cells"] = player()
    guest.call("kbd_send", keys=["right"], action="press")
    try:
        gate(guest, 0x0168, 3000)
        result["bios_return"] = snapshot(guest, "bios-return")
        result["a"] = guest.regs()["af"] >> 8
        guest.points(*guest.return_points)
        guest.advance(420)
        result["end_cells"] = player()
        result["after"] = snapshot(guest, "after")
    finally:
        guest.call("kbd_send", keys=["right"], action="release")
    if result["start_cells"] != [0x81C1]:
        raise Observation("Трасса началась в другой позиции игрока")


def starwars(guest):
    result = guest.result
    result["bios_code"] = {f"{pc:04x}": guest.read(pc, 12).hex() for pc in (0xC037, 0xC170, 0xC337)}
    gate(guest, 0x1CD0, 16000)
    result["caller"] = snapshot(guest, "caller")
    if guest.read(0x1CD0, 8) != bytes.fromhex("3a37c0fec3ca37c0"):
        raise Observation("Сборка не проверяет C037h по ожидаемому адресу")
    gate(guest, 0xC037, 6000)
    result["bios_entry"] = snapshot(guest, "bios-entry")
    result["stack_return"] = int.from_bytes(guest.read(guest.regs()["sp"], 2), "little")
    expected = result["stack_return"]
    if expected not in (0x1CC6, 0x1CDE):
        raise Observation(f"Непредвиденный адрес возврата из C037h: {expected:04X}h")
    guest.points(expected, *guest.return_points)
    pc = guest.advance(4000)
    result["returned"] = pc == expected
    result["after"] = snapshot(guest, "after")
    guest.points(0x13D8, *guest.return_points)
    result["rank_reached"] = guest.advance(16000) == 0x13D8
    result["rank"] = snapshot(guest, "rank")
    if result["rank_reached"]:
        guest.points(0x13DB, *guest.return_points)
        guest.call("kbd_send", keys=["1"], action="press")
        try:
            pc = guest.advance(4000)
            result["rank_code"] = guest.regs()["af"] >> 8 if pc == 0x13DB else None
        finally:
            guest.call("kbd_send", keys=["1"], action="release")
        result["rank_response"] = snapshot(guest, "rank-response")


def screen_write(slot, folder, header, body, common):
    folder.mkdir()
    machine = native["switch_to_native"](slot)
    machine.call("emu_reset")
    machine.call("emu_run_for", ms=1000)
    result = dict(**common, frames={})
    guest = native["NativeGuest"](slot, folder, result, 0)
    result["listing"] = native["list_name"](guest, header)
    guest.frame("command")
    guest.points(0xFB54)
    guest.key("enter", 100, 0)
    gate(guest, 0xFB54, 32000)
    result["after_copy"] = snapshot(guest, "after-copy")
    if guest.read(0, len(body)) != body:
        raise Observation("На FB54h полное тело уже отличается")
    address = 0x961F
    guest.call("bp_clear")
    guest.call("bp_set", addr=address, type="write")
    state = guest.call("emu_run_for", ms=5500)
    registers = guest.regs()
    result["write"] = dict(address=address, old=body[address], stopped_byte=guest.read(address, 1)[0], state=state,
                           preceding_instruction=guest.read(registers["pc"]-1, 1).hex(),
                           pc_bytes=guest.read(registers["pc"], 6).hex())
    result["after_write"] = snapshot(guest, "after-write")
    stored = (state["breaked"] and not state["stoppedByTimer"] and registers["de"] == address
              and result["write"]["preceding_instruction"] == "12"
              and result["write"]["stopped_byte"] == registers["af"] >> 8 != result["write"]["old"])
    result["status"] = "measured" if stored else "breakpoint_unconfirmed"
    native["save"](folder/"observation.json", result)
    return {"status": result["status"], "result": folder.name+"/observation.json"}


def measure(guest, kind):
    result = guest.result
    try:
        {"almaz2": almaz, "starwars2": starwars}[kind](guest)
        result["status"] = "measured"
    except Observation as error:
        result.update(status="stage_unconfirmed", error=str(error), obstacle=snapshot(guest, "obstacle"))
    native["save"](guest.out/"observation.json", result)
    return {"status": result["status"], "result": guest.out.name+"/observation.json"}


def run(args, folder):
    directory = args.campaign.resolve()
    inventory = native["read_json"](ROOT/"docs/compat/game-inventory.json")
    cards = native["read_json"](ROOT/"assets/games/controls.json")["cards"]
    reports = native["read_json"](directory/"results.json")["trials"]
    jobs = native["campaign"]["jobs"](inventory["files"], cards)
    group = next(j["record"]["payload_sha256"] for j in jobs
                 if j["profile"] == "MX" and any(Path(alias).name == FILES[args.case] for alias in j["aliases"]))
    group = [job for job in jobs if job["profile"] == "MX" and job["record"]["payload_sha256"] == group]
    job, body, header, primary, _, metadata = native["plan_group"](directory, group, reports, native["read_archive"]())
    if header is None or args.case != "barbar" and primary["launch"] != native["VERIFIED_ENTRY"]:
        raise ValueError("Для сравнения требуется точный вход по обоим маршрутам")
    folder.mkdir()
    statuses = {"RAMFOS": [], "RUN": []}
    fixture = directory/"fixtures"/primary["context"]["fixture"]
    system_files = {name.upper(): data for name, data in read_tree((fixture/"C.bin").read_bytes())[0].items()}
    packed = build_route(job["record"], job["route"])
    disk, marker = trial_disk(packed, job["route"]["kind"], fixture/"H.bin")
    if sha(disk) != primary["game_disk_sha256"]:
        raise ValueError("Образ H: расходится с первичной пробой")
    expected = dict(payload_sha256=sha(body), entry=metadata["entry"], load=metadata["load"])
    with tempfile.TemporaryDirectory(prefix="mxos-mx-abi-") as workspace:
        work = Path(workspace)
        image = work/"archive.ramdisk"
        image.write_bytes((header + body + b"\0\0\x08").ljust(8*65536, b"\0"))
        disk_image = work/"H.bin"
        disk_image.write_bytes(disk)
        with native["lease"](port=args.port, size=16, artifact_root=folder/"slots-ramfos") as slot:
            conf = native["prepare_platform"](slot, args.source, f'ramDisk.fileName = "{image.as_posix()}"')
            common = dict(**expected, rom_sha256=native["ROM_SHA256"], pid=slot.pid, hwnd=slot.hwnd,
                          emulator_sha256=sha((slot.runtime/native["EXE"]).read_bytes()), port=slot.port,
                          config_sha256=sha((conf/"spmx.conf").read_bytes()))
            for trial in (1, 2):
                if args.case == "barbar":
                    statuses["RAMFOS"].append(screen_write(slot, folder/f"ramfos-{trial}", header, body, common))
                    continue
                guest, report = native["boot"](slot, folder/f"ramfos-{trial}", image, common, body, header, metadata["load"])
                if report["launch"] != native["VERIFIED_ENTRY"]:
                    raise RuntimeError("Архивный ROM отклонил образ")
                statuses["RAMFOS"].append(measure(guest, args.case))
        if args.case == "barbar":
            statuses["RUN"].append(dict(status=primary["launch"], primary=metadata["selected_primary"]))
            native["save"](folder/"index.json", dict(**expected, primary=metadata["selected_primary"], stages=statuses))
            return all(result["status"] == "measured" for result in statuses["RAMFOS"])
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
                              config_sha256=primary["context"]["profile_sha256"],
                              bios_sha256=primary["context"]["kernels"]["DOS/BIOS.SYS"],
                              emulator_sha256=sha((slot.runtime/native["EXE"]).read_bytes()))
                if slot.protection != FLASH_PROTECTION:
                    raise RuntimeError("Состояние защиты носителей RUN изменилось")
                guest = Guest(slot, target, fixture/"build", "MX", system_files, report)
                guest.launch(job, packed)
                statuses["RUN"].append(measure(guest, args.case))
                guest.recovery(marker)
                native["save"](target/"observation.json", report)
    native["save"](folder/"index.json", dict(**expected, primary=metadata["selected_primary"], stages=statuses))
    return all(result["status"] == "measured" for arms in statuses.values() for result in arms)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", required=True, choices=FILES)
    parser.add_argument("--source", type=Path, default=native["SOURCE"])
    parser.add_argument("--campaign", type=Path, default=native["DEFAULT"])
    parser.add_argument("--port", type=int, default=19796, choices=range(19781, 19797))
    args = parser.parse_args()
    folder = args.campaign.resolve()/"native-ramfos"/f'mx-{args.case}-abi-attempt-{uuid.uuid4().hex[:12]}'
    print(folder, flush=True)
    return int(not run(args, folder))


if __name__ == "__main__":
    sys.exit(main())
