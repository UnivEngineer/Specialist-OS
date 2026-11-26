"""Сборка внешних клиентов SjASMPlus с копией публичного ASM SDK."""

from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
ASSEMBLER = ROOT / "tasm/sjasmplus.exe"
PRIVATE_SYMBOLS = {
    "BIOS_UPPER_VARIABLES",
    "DOS_BANK_CMD_ADDR",
    "DOS_DRIVER_REGISTRY",
    "DOS_ENTRY_BASE",
    "FAT_CACHE_ADDR",
    "FILE_STREAM_CURSOR",
    "IMAGE_LOAD_CONTEXT",
    "IO_VARIABLES",
    "MEM_WINDOW_VERSION",
    "bios_upper.cacheStart",
    "dos_fileHandle",
    "io_vars",
}

CLIENT = """
    INCLUDE "mxos.inc"
    INCLUDE "mxos.inc"
    ASSERT $ == 0
    ORG 0100h
    INCLUDE "mxos.inc"
    INCLUDE "mxos.inc"
    ASSERT $ == 0100h

    call bios_getMemTop
    ld hl,handle
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
    MXOS_CALL_PACKET MXOS_DOS_GET_ERROR,regs
    ret
handle BLOCK FILE_HANDLE,0
regs MX_REGS
    ASSERT $ - regs == 8
    ASSERT FILE_DESCRIPTOR == 32
"""

OPTIONAL_HEADERS = """
    INCLUDE "formats/relocations.inc"
    INCLUDE "formats/fat.inc"
    INCLUDE "formats/font.inc"
    INCLUDE "formats/font-init.inc"
    INCLUDE "drivers/disk.inc"
    INCLUDE "hardware/spec_mx.inc"
    INCLUDE "assembly.inc"
    INCLUDE "mxos.inc"
    INCLUDE "drivers/disk.inc"
    INCLUDE "formats/fat.inc"
    ASSERT $ == 0
    ASSERT FAT16_BAD == 0FFF7h
    ASSERT IMAGE_RELOC_HEADER == 40
    ASSERT DRIVER_INSTALL == 16
    ASSERT FONT_INIT_REQUEST == 6
    ORG 0100h
    ret
    ASSERT_EQUAL 0101h
"""

MAIN_UI = """
    INCLUDE "mxos.inc"
    INCLUDE "lib/ui.inc"
    INCLUDE "lib/ui.inc"
    ASSERT $ == 0
    ORG 0100h
    ld hl,frame
    xor a
    call uiDraw
    ret
frame:
    G_WINDOW 0,8,8,48,24
    DB 0

    DEFINE UI_BYTE_RECT
    DEFINE UI_PIXEL_RECT
    DEFINE UI_BITMAP
    DEFINE UI_TEXT_RECT
    DEFINE UI_TEXT_RECT_COLORS
    DEFINE UI_IMPLEMENTATION
    INCLUDE "lib/ui.inc"
uiEnd:
    INCLUDE "lib/ui.inc"
    ASSERT $ == uiEnd
    INCLUDE "lib/ui-text-align.inc"
    INCLUDE "lib/ui-button.inc"
    INCLUDE "lib/ui-modal.inc"
"""


BANK_HELP = """
    INCLUDE "mxos.inc"
    ORG 4000h
    ld hl,path
    jp uiShowHelp
palette:   DB 7
path:      DB "A:/HELP.TXT",0
bank:      DB 0
handle:    DW 0
registers: MX_REGS
state:     IO_KEY_STATE
readKey:
    ld hl,state
    jp io_bankGetCh
    DEFINE UI_HELP_KEY readKey
    DEFINE UI_HELP_HANDLE handle
    DEFINE UI_HELP_REGS registers
    DEFINE UI_HELP_BANK bank
    DEFINE UI_HELP_PIXELS 0
    INCLUDE "lib/ui-bank-help.inc"
    INCLUDE "abi/ui-gfx.inc"
"""


def assemble(directory, name, source):
    assembly = directory / f"{name}.asm"
    assembly.write_text(source, encoding="ascii")
    subprocess.run(
        [
            str(ASSEMBLER),
            "--i8080",
            "--inc=asm",
            f"--raw={name}.com",
            f"--sym={name}.sym",
            assembly.name,
        ],
        cwd=directory,
        check=True,
    )
    symbols = (directory / f"{name}.sym").read_text(encoding="utf-8")
    names = {line.partition(":")[0] for line in symbols.splitlines()}
    leaks = names & PRIVATE_SYMBOLS
    leaks.update(name for name in names if name.startswith("IO_TIMER_"))
    assert not leaks, sorted(leaks)
    return names


def main():
    with tempfile.TemporaryDirectory(prefix="mxos-asm-sdk-") as temporary:
        directory = Path(temporary)
        shutil.copytree(ROOT / "sdk/asm", directory / "asm")
        client = assemble(directory, "client", CLIENT)
        assert {"bios_getMemTop", "mx_callPacket", "FILE_DESCRIPTOR"} <= client
        assert {"DRIVER_INSTALL", "FAT16_BAD", "IMAGE_RELOC_HEADER", "GFX_HELP_OPEN"}.isdisjoint(client)
        assemble(directory, "optional", OPTIONAL_HEADERS)
        ui = assemble(directory, "ui", MAIN_UI)
        assert {"uiDraw", "uiModalOpen", "uiFillRectPixels"} <= ui
        help_symbols = assemble(directory, "bank", BANK_HELP)
        assert {"uiShowHelp", "GFX_HELP_OPEN", "GFX_HELP_KEY"} <= help_symbols
    print("ASM SDK PASS: public client, optional headers, two-pass MAIN UI, EXEC help")


if __name__ == "__main__":
    main()
