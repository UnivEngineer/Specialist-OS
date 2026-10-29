"""Проверка входа с MX-страницы ROM до установки стека DOS, без эмуляции ОС."""

import argparse
from pathlib import Path


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", nargs="?", type=Path,
                        default=root / "images/SystemRom_emulator.bin")
    args = parser.parse_args()
    image = args.image.read_bytes()
    if len(image) not in (65536, 524288):
        raise ValueError("Expected a 64-KB MX2 image or a 512-KB linear system flash image")
    config = (Path(__file__).with_name("specialist-mx2.conf")).read_text(encoding="utf-8-sig")
    for setting in ("addrSpace.defaultPage = 2", "pageSelector.defaultAddr = 2"):
        if setting not in config.splitlines():
            raise ValueError(f"Unsafe reset setting: expected {setting}")

    memory = bytearray(65536)
    memory[:32768] = image[32768:] if len(image) == 65536 else image[:32768]
    pc = hl = de = a = 0
    zero = False
    copied = 0
    for steps in range(100000):
        opcode = memory[pc]
        pc += 1
        if opcode in (0x21, 0x11):  # LXI H/D
            value = memory[pc] | memory[pc + 1] << 8
            pc += 2
            if opcode == 0x21:
                hl = value
            else:
                de = value
        elif opcode == 0x7E:  # MOV A,M
            a = memory[hl]
        elif opcode == 0x12:  # STAX D
            if not 0xC000 <= de < 0xE000:
                raise ValueError(f"Unexpected bootstrap write: {de:04X}")
            memory[de] = a
            copied += 1
        elif opcode == 0x13:  # INX D
            de = (de + 1) & 65535
        elif opcode == 0x23:  # INX H
            hl = (hl + 1) & 65535
        elif opcode == 0x7A:  # MOV A,D
            a = de >> 8
        elif opcode == 0xFE:  # CPI
            zero = a == memory[pc]
            pc += 1
        elif opcode in (0xC2, 0xC3):  # JNZ/JMP
            target = memory[pc] | memory[pc + 1] << 8
            pc = target if opcode == 0xC3 or not zero else pc + 2
        elif opcode == 0x31 and pc - 1 == 0xC009:  # LXI SP в DOS
            sp = memory[pc] | memory[pc + 1] << 8
            if sp != 0xFFC0 or memory[0xC000:0xC003] != b"\xC3\x09\xC0":
                raise ValueError("Unexpected DOS entry or stack")
            print(f"OK: MX ROM -> DOS C000 -> SP={sp:04X}; {copied} bytes copied, {steps + 1} instructions")
            return
        else:
            raise ValueError(f"Unexpected opcode {opcode:02X} at {pc - 1:04X} before DOS stack setup")
    raise ValueError("Bootstrap did not reach DOS stack setup")


if __name__ == "__main__":
    main()
