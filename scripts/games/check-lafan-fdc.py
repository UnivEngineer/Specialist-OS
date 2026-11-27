"""Проверить модель ВГ93 на шине отдельного видимого Специалиста-MX."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import uuid

from campaign_media import ROOT, sha
from lafan_fdc import PATCH, TEMPLATE, checked_disks, load_template, state_pc
from lafan_native import ROM_SHA256, prepare_platform, switch_to_native

sys.path.insert(0, str(ROOT / "scripts/emu80"))
from mcp_pool import lease


PROBE = PATCH.with_name("lafan-fdc-probe.asm")
OUT = ROOT / "docs/compat/game-campaign/native-fdc-model"


def assemble(directory):
    output, symbols = directory / "probe.bin", directory / "probe.sym"
    subprocess.run([str(ROOT / "tasm/sjasmplus.exe"), "--i8080", f"--raw={output.as_posix()}",
                    f"--sym={symbols.as_posix()}", PROBE.as_posix()], check=True)
    addresses = {label: int(value, 16) for label, value in re.findall(
        r"^(\w+): EQU 0x([0-9a-fA-F]+)", symbols.read_text(encoding="utf-8"), flags=re.MULTILINE,
    )}
    return output.read_bytes(), addresses


def probe_slot(slot, template, directory, disk, data, addresses, enabled, abort):
    directory.mkdir()
    scratch = directory / "drive-a.odi"
    scratch.write_bytes(disk)
    option = "fdc.multiReadSectorSearch = " + ("yes" if enabled else "no")
    config = prepare_platform(slot, template / "specmx", options=option)
    machine = switch_to_native(slot)
    machine.call("disk_attach", drive="A", file=scratch.as_posix())
    try:
        machine.call("emu_reset")
        machine.call("emu_run_for", ms=4500)
        # Синтетическая проверка шины запускает собственный тест, а штатная загрузка проверяется отдельно.
        machine.write(addresses["wait_entry"], data)
        machine.write(addresses["PROBE_RESULT"], bytes(5))
        machine.call("regs_set", pc=addresses["abort_entry" if abort else "wait_entry"])
        gate = addresses["abort_done" if abort else "wait_search"]
        machine.call("bp_set", addr=gate, type="exec")
        state = machine.call("emu_run_for", ms=500)
        if state_pc(state) != gate:
            raise RuntimeError(f"Тест ВГ93 не достиг состояния {gate:04X}h")
        machine.call("bp_clear")
        first_status, first_sector, last_status, next_sector, after_abort = machine.read(addresses["PROBE_RESULT"], 5)
        single = machine.read(addresses["SINGLE_BUFFER"], 1024)
        multiple = machine.read(addresses["MULTI_BUFFER"], 5120)
        if single != disk[:1024] or multiple != disk[:5120] or (first_status, first_sector) != (0, 1):
            raise RuntimeError("Одиночное или многосекторное чтение дало несовпадение")
        if (last_status, next_sector) != ((1, 6) if enabled else (0, 5)):
            raise RuntimeError("Статус ВГ93 после последнего байта отличается от ожидаемого")
        hwnd = json.loads((slot.artifacts / "slot.json").read_bytes())["hwnd"]
        result = dict(option=option, diagnostic_pc_injection=True, gate=gate, pid=slot.pid, hwnd=hwnd,
                      single_status=first_status, single_sector=first_sector, single_sha256=sha(single),
                      multi_status=last_status, next_sector=next_sector, multi_sha256=sha(multiple),
                      real_rom_sha256=ROM_SHA256, config_sha256=sha((config / "spmx.conf").read_bytes()),
                      options_sha256=sha((config / "spmx.opt").read_bytes()))
        if abort:
            machine.call("emu_run_for", ms=1050)
            later = machine.read(0xFFE8, 1)[0]
            if (after_abort | later) & 0x11:
                raise RuntimeError("После D0h сохранились BUSY или RNF")
            result.update(force_interrupt=0xD0, after_force_interrupt=after_abort, after_1050_ms=later)
        elif enabled:
            machine.call("emu_run_for", ms=790)
            during = machine.read(addresses["ST_LAST"], 1)[0]
            machine.call("emu_run_for", ms=230)
            elapsed = machine.read(addresses["ST_LAST"], 1)[0]
            if during != 1 or elapsed != 0x10:
                raise RuntimeError("Нет BUSY до пятого импульса или RNF после окончания поиска")
            result.update(after_790_ms=during, after_1020_ms=elapsed)
        machine.screenshot(directory / "hardware-probe.png")
    finally:
        machine.call("disk_detach", drive="A")
    result["disk_sha256"] = sha(scratch.read_bytes())
    if result["disk_sha256"] != sha(disk):
        raise RuntimeError("Тест чтения изменил копию диска")
    (directory / "result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="Каталог с пятью исходными ODI")
    parser.add_argument("--template", type=Path, default=TEMPLATE)
    parser.add_argument("--out", type=Path, default=OUT)
    parser.add_argument("--port", type=int, choices=range(19781, 19797))
    args = parser.parse_args()
    build = load_template(args.template)
    disk = checked_disks(args.archive)["LafanG1.odi"]
    target = args.out / ("attempt-" + uuid.uuid4().hex[:12])
    target.mkdir(parents=True)
    data, addresses = assemble(target)
    tests = []
    with lease(size=16, port=args.port, artifact_root=args.out / "slots") as slot:
        if sha((slot.runtime / "Emu80qt-mcp.exe").read_bytes()) != build["executableSha256"]:
            raise RuntimeError("В слоте запущена другая сборка Emu80")
        for name, enabled, abort in (("default", False, False), ("search", True, False), ("abort", True, True)):
            result = probe_slot(slot, args.template, target / name, disk, data, addresses, enabled, abort)
            result.update(test=name, evidence=f"{target.name}/{name}/result.json")
            tests.append(result)
    summary = dict(build=build, diagnostic_source_sha256=sha(PROBE.read_bytes()), diagnostic_sha256=sha(data), tests=tests)
    for path in (target / "summary.json", args.out / "results.json"):
        path.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
