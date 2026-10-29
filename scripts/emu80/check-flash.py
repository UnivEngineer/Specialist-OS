"""Интеграционная проверка через MCP. Изменяет только явно указанную тестовую машину!"""
import argparse
import hashlib
import json
import re
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class Machine:
    def __init__(self, url):
        self.url = url
        self.sequence = 0

    def call(self, name, **arguments):
        self.sequence += 1
        request = urllib.request.Request(self.url, json.dumps({
            "jsonrpc": "2.0", "id": self.sequence, "method": "tools/call",
            "params": {"name": name, "arguments": arguments},
        }).encode(), {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"})
        response = json.load(urllib.request.urlopen(request, timeout=30))
        if "error" in response:
            raise RuntimeError(response["error"])
        result = response["result"]
        if result.get("isError"):
            raise RuntimeError(result)
        for content in result["content"]:
            if content["type"] == "text":
                return json.loads(content["text"])
        raise RuntimeError("Tool did not return JSON text")

    def write(self, addr, data):
        for offset in range(0, len(data), 4096):
            self.call("mem_write", addr=addr + offset, data=data[offset:offset + 4096].hex())

    def read(self, addr, length):
        return bytes.fromhex(self.call("mem_read", addr=addr, len=length)["hex"])

    def subroutine(self, address, **registers):
        self.write(0x8000, bytes((0xCD, address & 255, address >> 8, 0xC3, 0, 0x81)))
        self.call("regs_set", pc=0x8000, sp=0x8F00, **registers)
        state = self.call("emu_run_for", ms=500)
        state = self.call("cpu_state")
        if state["registers"]["pc"]["dec"] != 0x8100:
            raise RuntimeError(f"Driver did not return: {state}")
        return state


def driver_entry(name):
    listing = (ROOT / "bin" / (name + ".lst")).read_text(encoding="utf-8-sig")
    found = re.search(r"^\s*\d+\s+([0-9A-Fa-f]{4})\s+Driver:\s*$", listing, re.M)
    if not found:
        raise ValueError(f"Driver label missing: {name}")
    return int(found[1], 16)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", required=True, type=Path, help="Отдельная MCP-установка с тестовым образом")
    parser.add_argument("--url", default="http://127.0.0.1:19781/mcp")
    parser.add_argument("--report", type=Path, default=ROOT / ".codex-tmp/emu80-flash-report.json")
    parser.add_argument("--verify-persisted", action="store_true")
    parser.add_argument("--resume", action="store_true", help="Продолжить запись после предыдущего прогона без проверки seed")
    parser.add_argument("--pipeline-only", action="store_true", help="Записать пакет в разные и повторно в одну микросхему")
    parser.add_argument("--system-rom", action="store_true", help="Проверить FlashSys с системной ПЗУ без записи")
    parser.add_argument("--drivers", nargs="+", choices=("FlashChk", "FlashFst", "FlashLegacy"), default=["FlashChk", "FlashFst"])
    args = parser.parse_args()
    machine = Machine(args.url)
    image_path = args.runtime / "specialist-os/flash-disk.bin"
    # Защищаем рабочую установку от ошибочного выбора при тесте с записью.
    if args.runtime.name != "Emu80-flash-test":
        raise ValueError("Use a disposable runtime named Emu80-flash-test")
    machine.call("debug_break")
    machine.call("bp_clear")
    machine.call("bp_set", addr=0x8100, type="exec")

    if args.system_rom:
        if (args.runtime / 'specialist-os/system-flash.bin').exists():
            raise ValueError('For the system-flash profile use check-system-flash.py --locked; do not reinstall FlashSys.')
        # Устанавливаем один раз: установщик сохраняет старые адреса обёрток ABI.
        machine.write(0xDC00, (ROOT / "bin/FlashSys.com").read_bytes())
        machine.subroutine(0xDC00)
        entry = driver_entry("FlashSys")
        image = (args.runtime / "specialist-os/system.bin").read_bytes()
        # MX ROM занимает вторую половину образа; старшие страницы пока не моделируются.
        for sector in [0, 1, 0x3F, 0x7F]:
            state = machine.subroutine(entry, bc=0x0200, de=sector, hl=0x4000)
            assert not state["flags"]["C"]
            assert machine.read(0x4000, 256) == image[32768 + sector * 256:32768 + (sector + 1) * 256]
        original = image[32768:33024]
        machine.write(0x4000, bytes(value ^ 255 for value in original))
        state = machine.subroutine(entry, bc=0x0100, de=0, hl=0x4000)
        assert state["flags"]["C"], "FlashSys accepted a write to read-only ROM"
        assert machine.read(0x8FF9, 1) == b"\x01"  # BIOS_VARIABLES.diskWriteErr
        # diskWriteErr и сохранность массива, а также возврат маппера в RAM.
        machine.subroutine(entry, bc=0x0200, de=0, hl=0x4100)
        assert machine.read(0x4100, 256) == original
        assert (args.runtime / "specialist-os/system.bin").read_bytes() == image
        report = json.loads(args.report.read_text())
        report["flashSys"] = {"readSectors": [0, 1, 63, 127], "readOnlyWriteRejected": True,
                              "systemFlashProgrammingTested": False}
        args.report.write_text(json.dumps(report, indent=2) + "\n")
        print("FlashSys: 4 ROM reads, rejected write (CF=1), unchanged ROM, RAM mapping restored OK")
        return

    if args.pipeline_only:
        results = json.loads(args.report.read_text())
        machine.write(0xDE00, (ROOT / "bin/FlashFst.com").read_bytes())
        machine.subroutine(0xDE00)
        entry = driver_entry("FlashFst")
        queued = {}
        for sector in [0x07FC, 0x0FFC, 0x17FC, 0x1FFC, 0x07FD, 0x07FE]:
            payload = bytes(((sector >> 8) * 29 + i * 31 + sector) & 255 for i in range(256))
            queued[sector] = payload
            machine.write(0x4000, payload)
            machine.subroutine(entry, bc=0x0100, de=sector, hl=0x4000)
        machine.subroutine(entry, bc=0x0400)
        for sector, payload in queued.items():
            machine.subroutine(entry, bc=0x0200, de=sector, hl=0x4100)
            assert machine.read(0x4100, 256) == payload, f"Pipeline readback failed: {sector:04X}"
            with image_path.open("rb") as file:
                file.seek(sector * 256)
                assert file.read(256) == payload
            results["writes"].append({"sector": sector, "sha256": hashlib.sha256(payload).hexdigest()})
        results["checks"].append("6 queued writes: four chips, then repeated writes to one chip, final flush")
        results["persistenceAfterRestart"] = False
        args.report.write_text(json.dumps(results, indent=2) + "\n")
        print("Pipeline: 6 queued writes, four independent chips, repeated same-chip writes OK")
        return

    if args.verify_persisted:
        report = json.loads(args.report.read_text())
        machine.write(0xDE00, (ROOT / "bin/FlashChk.com").read_bytes())
        machine.subroutine(0xDE00)
        entry = driver_entry("FlashChk")
        for case in report["writes"]:
            machine.subroutine(entry, bc=0x0200, de=case["sector"], hl=0x4000)
            assert hashlib.sha256(machine.read(0x4000, 256)).hexdigest() == case["sha256"]
        report["persistenceAfterRestart"] = True
        args.report.write_text(json.dumps(report, indent=2) + "\n")
        print(f"Persistence after process restart: {len(report['writes'])} sectors OK")
        return

    seed = (ROOT / "images/FlashDisk_emu80_seed.bin").read_bytes()
    assert image_path.stat().st_size == len(seed) == 2097152
    results = {"writes": [], "drivers": [], "checks": []}
    sectors = [0, 1, 0x7F, 0x80, 0xFF, 0x100, 0x7FF, 0x800, 0xFFF, 0x1000, 0x17FF, 0x1800, 0x1FFF]
    # Чтение всех границ через установленный DOS-драйвер, до изменения диска.
    entry = driver_entry("FlashChk")
    for sector in ([] if args.resume else sectors):
        state = machine.subroutine(entry, bc=0x0200, de=sector, hl=0x4000)
        assert not state["flags"]["C"]
        assert machine.read(0x4000, 256) == seed[sector * 256:(sector + 1) * 256], hex(sector)
    if not args.resume:
        results["checks"].append("seed reads at sector/64K/chip/end boundaries")

    # ИР22 прозрачен при STB=1 и удерживает адрес при STB=0.
    if not args.resume:
        machine.write(0xFFE7, b"\x90")
        machine.write(0xFFE6, b"\x60")
        machine.write(0xFFE5, b"\x12")
        assert machine.read(0xFFE4, 1)[0] == seed[0x1212]
        machine.write(0xFFE6, b"\x40")
        machine.write(0xFFE5, b"\x34")
        assert machine.read(0xFFE4, 1)[0] == seed[0x1234]
        machine.write(0xFFE6, b"\xC0")
        assert machine.read(0xFFE4, 1) == b"\xFF"
        results["checks"].append("transparent latch, held address, disabled output")
    else:
        results = json.loads(args.report.read_text())

    # Два существующих драйвера; тестовые сектора находятся в области данных.
    test_sectors = [0x7F, 0x80, 0xFF, 0x100, 0x7FF, 0x800, 0xFFF, 0x1000, 0x17FF, 0x1800, 0x1FFF]
    final_writes = {}
    for number, name in enumerate(args.drivers):
        machine.write(0xDE00, (ROOT / "bin" / (name + ".com")).read_bytes())
        machine.subroutine(0xDE00)
        entry = driver_entry(name)
        start_ticks = machine.call("cpu_state")["cpu_ticks"]
        deferred = 0
        for sector in test_sectors:
            payload = bytes((i * 73 + sector + number * 101) & 255 for i in range(256))
            machine.write(0x4000, payload)
            state = machine.subroutine(entry, bc=0x0100, de=sector, hl=0x4000)
            if name == "FlashChk":
                assert not state["flags"]["C"], f"Write failed: {sector:04X}"
            else:
                # Возврат конвейерного драйвера раньше фиксации массива — реальная отложенная запись.
                with image_path.open("rb") as file:
                    file.seek(sector * 256)
                    if file.read(256) != payload:
                        deferred += 1
                machine.subroutine(entry, bc=0x0400)
            machine.write(0x4100, bytes(256))
            state = machine.subroutine(entry, bc=0x0200, de=sector, hl=0x4100)
            assert machine.read(0x4100, 256) == payload, f"Readback failed: {name} {sector:04X}"
            with image_path.open("rb") as file:
                file.seek(sector * 256)
                assert file.read(256) == payload, f"Persistence failed: {name} {sector:04X}"
            final_writes[sector] = {"sector": sector, "sha256": hashlib.sha256(payload).hexdigest()}
        if name == "FlashFst":
            assert deferred == len(test_sectors), "Fast writes were not actually deferred"
        results["drivers"].append({"name": name, "sectors": len(test_sectors), "deferred": deferred,
                                   "cpuTicks": machine.call("cpu_state")["cpu_ticks"] - start_ticks})
        print(f"{name}: {len(test_sectors)} write/read/persistence cases OK, deferred={deferred}", flush=True)
    results["writes"] = list(final_writes.values())
    assert len(image_path.with_suffix(".bin.sdp").read_bytes()) == 4
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(results, indent=2) + "\n")
    print(f"Report: {args.report}")


if __name__ == "__main__":
    main()
