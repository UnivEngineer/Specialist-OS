"""Проверка публичного переключателя BIOS на машинном коде текущей сборки."""
import os
from pathlib import Path
import re
import runpy
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "bin"


def main():
    symbols = {name: int(value, 16) for name, value in re.findall(
        r"^([\w.]+): EQU 0x([0-9a-fA-F]+)", (BUILD / "BIOS.sym").read_text(), re.M)}
    ticks = (os.getenv("Z88DK_TICKS") or shutil.which("z88dk-ticks")
             or "D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe")
    memory = bytearray(65536)
    install = runpy.run_path(str(Path(__file__).with_name("mem-image.py")))["install"]
    install(memory, symbols, BUILD, main=True)
    state = symbols["biosProfile"]
    low = symbols["biosCompatVectors"]
    monitor = symbols["BIOS_PROFILE_MON2"]
    default = symbols["BIOS_PROFILE_DEFAULT"]
    query = symbols["BIOS_PROFILE_QUERY"]
    assert default == symbols["BIOS_PROFILE_RAMFOS"]
    assert symbols["bios_init"] == symbols["mon2_reboot"] == 0xc800
    assert symbols["mon2_init"] == 0xc000 and symbols["bios_reboot"] == 0xf800
    prefix = bytes.fromhex("01 34 12 11 78 56 21 BC 9A 3E")
    # Сохраняем BC, DE, HL и PSW на стеке в порядке, определённом 8080.
    suffix = bytes.fromhex("C5 D5 E5 F5 C3 80 20")
    tables = {}
    high = None
    def word(value):
        return value.to_bytes(2, "little")

    def call(address):
        return b"\xcd" + word(address)

    with tempfile.TemporaryDirectory() as tmp:
        inp, out = Path(tmp) / "in.bin", Path(tmp) / "out.bin"
        def execute(body, *, stop=b"\xc3\x80\x20"):
            body = b"\x31\x00\x80" + body + stop
            memory[0x2000:0x2000+len(body)] = body
            inp.write_bytes(memory)
            subprocess.run([ticks, "-m8080", "-pc", "2000", "-end", "2080", "-counter",
                            "2000000", "-output", str(out), str(inp)], check=True, capture_output=True, timeout=15)
            snapshot = out.read_bytes()
            assert int.from_bytes(snapshot[65542:65544], "little") == 0x2080
            memory[:] = snapshot[:65536]

        def switch(profile):
            execute(prefix + bytes([profile]) + call(symbols["mx_callRegs"]) +
                    word(symbols["MXOS_BIOS_PROFILE"]) + suffix)

        for requested in (default, 0, monitor, query, 0xfe, default, monitor, default, query):
            previous = memory[state]
            switch(requested)
            assert memory[0x7ffa:0x8000] == bytes.fromhex("BC 9A 78 56 34 12"), requested
            valid = requested in (default, monitor, query)
            assert bool(memory[0x7ff8] & 1) != valid, requested
            assert memory[0x7ff9] == (previous if valid else symbols["DOS_ERR_ARGUMENT"]), requested
            expected = requested if requested in (default, monitor) else previous
            assert memory[state] == expected, requested
            if expected == default:
                assert memory[0xf803:0xf824] == memory[low+3:low+0x24]
                high = bytes(memory[0xf800:0xf860])
                assert all(high[i] == 0xc3 for i in range(0, len(high), 3))
            if high is not None:
                assert memory[0xf800:0xf860] == high
            table = bytes(memory[low:symbols["biosCompatVectorsEnd"]])
            assert tables.setdefault(expected, table) == table, expected

        font = (BUILD / "fonts" / "Rotate.i80").read_bytes()
        address = symbols["FONT_ENGINE_BASE"]
        memory[address:address+len(font)] = font
        execute(call(address))
        rendering = {
            name: bytes(memory[symbols[name]:symbols[name]+3])
            for name in ("mon2_printCharOld", "mon2_printStringOld")
        }
        for row, column in ((0, 0), (1, 3), (23, 40)):
            execute(b"\x21" + bytes([column, row]) + call(symbols["bios_setCursorPos"]) +
                    call(symbols["bios_getCursorPos"]) + bytes.fromhex("22 00 70"))
            assert memory[0xc63e:0xc640] == bytes([8 + 10*row, 3*column])
            assert memory[0x7000:0x7002] == bytes([column, row])

        top = symbols["APP_MEMORY_END"] - 1
        limit = symbols["v_memTop"]
        memory[limit:limit+2] = word(top)
        execute(bytes.fromhex("21 FF 8F") + call(symbols["bios_setMemTop"]) +
                call(symbols["bios_getMemTop"]) + bytes.fromhex("22 00 70"))
        assert memory[0x7000:0x7002] == word(top) == bytes.fromhex("FF 8D")
        for address in (top, top + 1):
            previous = memory[address]
            execute(b"\x21" + word(address) + bytes.fromhex("0E A5 AF") + call(0xf839) +
                    b"\xf5")
            assert bool(memory[0x7ffe] & 1) == (address > top)
            assert memory[address] == (0xa5 if address == top else previous)

        switch(monitor)
        for name, entry in rendering.items():
            assert bytes(memory[symbols[name]:symbols[name]+3]) == entry
        assert memory[0xc821] == memory[0xc82d] == 0xc9
        assert all(byte == 0xc9 for byte in memory[0xc836:0xc85d])
        # В STD аппаратное окно скрывает код IO/MEM. Запрещаем любые входы в него.
        memory[0xf000:] = b"\x76" * 0x1000
        memory[0xff01] = 0xff
        memory[symbols["mon2_vars.beepFreq"]:symbols["mon2_vars.beepFreq"]+2] = b"\x01\x01"
        execute(call(symbols["mon2_setKeybMode91"]))
        assert memory[0xff03] == 0x91 and memory[0xffe3] == 0x76
        execute(call(symbols["mon2_keyScan"]))
        assert memory[0xff03] == 0x82 and memory[0xffe3] == 0x76
        execute(call(symbols["mon2_beep_Old"]))
        assert memory[0xff03] == 0x0a and memory[0xffe3] == 0x76
        cursor = symbols["mon2_vars.cursorY"]
        memory[cursor:cursor+2] = b"\x08\x00"
        memory[symbols["bios_upper.escMode"]] = 0xff
        execute(b"\x0eA" + call(symbols["mon2_printChar"]))
        assert memory[cursor+1] == 3 and any(memory[0x9000:0xc000])

        # Модель не отображает банки. Подмена только после аппаратного возврата.
        memory[symbols["v_dosBanked"]] = 1
        resume = symbols["biosBankReturn"]
        memory[resume:resume+3] = bytes.fromhex("C3 80 20")
        memory[0xf7fc] = 3
        execute(b"\xc3" + word(symbols["mon2_init"]))
        assert memory[0xfffc] == 3
    assert tables[default] != tables[monitor]
    print("ABI RAMFOS/MON2, публичный выбор, STD клавиатура/звук/шрифт и выход через C000h проверены.")


if __name__ == "__main__":
    main()
