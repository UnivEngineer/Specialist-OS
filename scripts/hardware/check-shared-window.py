"""Проверка связей MMX и логической модели расширения общего окна.

Не моделирует временные диаграммы, DRAM, разводку платы и электрические уровни.
Схема DipTrace и её экспорты не изменяются.
"""

from pathlib import Path
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[2]
source = ET.parse(ROOT / "hardware/boards/mapper/mapper.dchxml").getroot()
schematic = source.find("Schematic")
library = {
    c.attrib["ComponentStyle"]: c
    for c in source.find("Library/Components")
}
pins = {}
for part in schematic.find("Components"):
    definition = library[part.attrib["ComponentStyle"]]
    definition = list(definition)[int(part.attrib["ComponentPart"])]
    ref = part.findtext("RefDes")
    section = part.findtext("PartRefDes") or "power"
    for symbol, connected in zip(definition.find("Pins"), part.find("Pins")):
        pad = symbol.findtext("PadNumber")
        pins[(ref, section, pad)] = connected.attrib.get("NetId")


def net(ref, section, pad):
    return pins[(ref, section, str(pad))]


assert net("DD5", "4", 8) == net("DD9", "1", 2)
assert net("DD5", "4", 8) == net("DD4", "3", 3)
assert net("DD13", "2", 8) == net("DD2", "1", 1)
assert net("DD13", "2", 8) != net("DD9", "1", 2)
assert net("DD5", "5", 10) == net("DD9", "2", 5)
for section, ref, pads in [
    ("2", "DD5", [3, 4]),
    ("2", "DD10", [4, 5, 6]),
    ("2", "DD14", [14, 13, 15, 12, 11, 10, 9]),
]:
    assert all(net(ref, section, p) in (None, "-1") for p in pads)
print("OK: исходная ветвь окна, F000, /A11 и свободные элементы MMX подтверждены XML")

for size in (64, 1024, 2048, 4096):
    shared_count = 0
    for address in range(65536):
        old_window = address >= 0xFFC0
        selected_window = address >= 65536 - size
        f000 = (address & 0xF000) == 0xF000
        a11 = bool(address & 0x0800)
        a10 = bool(address & 0x0400)
        if size == 2048:
            # DD5.2 инвертирует F000; DD5.5 уже даёт /A11;
            # свободный DD10.2 — ИЛИ-НЕ, несмотря на общий стиль символа в XML.
            nor_result = not ((not f000) or (not a11))
            assert nor_result == selected_window
        if size == 1024:
            # Свободная половина DD14: /E=/F000, A1=A11, A0=A10.
            q3_bar = not (f000 and a11 and a10)
            assert (not (q3_bar or q3_bar)) == selected_window
        if size == 4096:
            assert selected_window == f000
        hardware = address >= 0xFFE0
        shared_count += selected_window and not hardware
        for mode in range(4):
            m1, m0 = bool(mode & 2), bool(mode & 1)
            for ren in (False, True):
                rom = not (bool(address & 0x8000) or not m1 or (ren and m0))
                iop = hardware if mode != 3 else address >= 0xF000
                ramd_mode = m0 and not m1
                old_k = not ramd_mode or old_window
                new_k = not ramd_mode or selected_window
                old_main = not rom and not iop and old_k
                new_main = not rom and not iop and new_k
                old_ramd, new_ramd = not old_k, not new_k
                assert not (new_main and new_ramd)
                if hardware:
                    assert not new_main and not new_ramd
                if mode != 1 or not selected_window or old_window:
                    assert (new_main, new_ramd) == (old_main, old_ramd)
                else:
                    assert new_main and not new_ramd
    assert shared_count == size - 32
    print(f"OK: верхняя область {size} байт, общей RAM {shared_count}, все адреса и режимы, I/O не изменён")
