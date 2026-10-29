"""Подключение собственных моделей к отдельному фиксированному checkout Emu80."""
import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OVERLAY = ROOT / "source-emulators/emu80"


def integrate(source):
    lock = json.loads((Path(__file__).with_name("upstream.json")).read_text())
    head = subprocess.check_output(["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    if head != lock["referenceCommit"]:
        raise ValueError(f"Expected upstream {lock['referenceCommit']}, got {head}")
    files = ["At29Flash.cpp", "At29Flash.h", "SpecialistFlashDisk.cpp", "SpecialistFlashDisk.h",
             "SpecialistMx2VideoRam.cpp", "SpecialistMx2VideoRam.h",
             "SpecialistSystemFlash.cpp", "SpecialistSystemFlash.h"]
    changes = {}
    for name in ("src/ObjectFactory.cpp", "src/Emu80qt.pro", "src/Emu80.cbp", "src/Emu80lnx.cbp", "whatsnew.txt"):
        original = subprocess.check_output(["git", "-C", str(source), "show", "HEAD:" + name])
        text = original.decode("utf-8-sig")
        newline = "\r\n" if b"\r\n" in original else "\n"
        text = text.replace("\r\n", "\n")
        if name.endswith("ObjectFactory.cpp"):
            text = text.replace('#include "EmuConfig.h"', '#include "SpecialistFlashDisk.h"\n#include "EmuConfig.h"', 1)
            text = text.replace("    REG_EMU_CLASS(EmuWindow);", "    REG_EMU_CLASS(SpecialistFlashDisk);\n    REG_EMU_CLASS(EmuWindow);", 1)
        elif name.endswith(".pro"):
            text += "\n# Аппаратные модели Specialist-OS.\nSOURCES += At29Flash.cpp SpecialistFlashDisk.cpp\nHEADERS += At29Flash.h SpecialistFlashDisk.h\n"
        elif name.endswith(".cbp"):
            units = "".join(f'\t\t<Unit filename="{file}" />\n' for file in files)
            text = text.replace("\t</Project>", units + "\t</Project>", 1)
        else:
            text = "+ Specialist-OS: модель flash-диска на четырёх AT29C040A.\n\n" + text
        data = (b"\xef\xbb\xbf" if original.startswith(b"\xef\xbb\xbf") else b"") + text.replace("\n", newline).encode("utf-8")
        previous = data
        if name.endswith("ObjectFactory.cpp"):
            text = text.replace('#include "SpecialistFlashDisk.h"', '#include "SpecialistMx2VideoRam.h"\n#include "SpecialistFlashDisk.h"', 1)
            text = text.replace('    REG_EMU_CLASS(SpecialistFlashDisk);', '    REG_EMU_CLASS(SpecialistMx2VideoRam);\n    REG_EMU_CLASS(SpecialistFlashDisk);', 1)
        elif name.endswith(".pro"):
            text += '\nSOURCES += SpecialistMx2VideoRam.cpp\nHEADERS += SpecialistMx2VideoRam.h\n'
        elif name.endswith(".cbp"):
            # Предыдущая версия overlay содержала только flash-модель.
            previous = previous.replace(('\t\t<Unit filename="SpecialistMx2VideoRam.cpp" />' + newline).encode(), b'')
            previous = previous.replace(('\t\t<Unit filename="SpecialistMx2VideoRam.h" />' + newline).encode(), b'')
        data = (b"\xef\xbb\xbf" if original.startswith(b"\xef\xbb\xbf") else b"") + text.replace("\n", newline).encode("utf-8")
        previous_mx2 = data
        if name.endswith("ObjectFactory.cpp"):
            text = text.replace('#include "SpecialistFlashDisk.h"', '#include "SpecialistSystemFlash.h"\n#include "SpecialistFlashDisk.h"', 1)
            text = text.replace('    REG_EMU_CLASS(SpecialistFlashDisk);', '    REG_EMU_CLASS(SpecialistSystemFlash);\n    REG_EMU_CLASS(SpecialistFlashDisk);', 1)
        elif name.endswith(".pro"):
            text += '\nSOURCES += SpecialistSystemFlash.cpp\nHEADERS += SpecialistSystemFlash.h\n'
        elif name.endswith(".cbp"):
            for file in ("SpecialistSystemFlash.cpp", "SpecialistSystemFlash.h"):
                unit = ('\t\t<Unit filename="' + file + '" />' + newline).encode()
                previous = previous.replace(unit, b'')
                previous_mx2 = previous_mx2.replace(unit, b'')
        data = (b"\xef\xbb\xbf" if original.startswith(b"\xef\xbb\xbf") else b"") + text.replace("\n", newline).encode("utf-8")
        current = (source / name).read_bytes()
        # Не перезаписываем посторонние правки upstream.
        if current not in (original, previous, previous_mx2, data):
            raise ValueError(f"Unrelated modifications in {source / name}")
        changes[name] = data
    for name, data in changes.items():
        (source / name).write_bytes(data)
    for name in files:
        (source / "src" / name).write_bytes((OVERLAY / name).read_bytes())
    print(f"Integrated flash adapter into {source} at {head}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    integrate(parser.parse_args().source.resolve())
