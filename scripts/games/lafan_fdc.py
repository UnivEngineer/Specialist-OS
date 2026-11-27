"""Исходные носители Lafan и отдельная сборка с моделью поиска ВГ93."""

import csv
import json
import os

from campaign_media import ROOT, sha


TEMPLATE = ROOT / ".local/lafan-fdc/template"
PATCH = ROOT / "source-emulators/emu80/lafan-fdc-search.patch"
CATALOG = ROOT / "assets/games/lafan-directory.csv"
GEOMETRY = (80, 2, 5, 1024)


def catalog_rows():
    with CATALOG.open(encoding="utf-8-sig", newline="") as catalog:
        return list(csv.DictReader(catalog))


def load_template(template):
    record = json.loads((template / "lafan-fdc-build.json").read_text(encoding="utf-8-sig"))
    emulator = template / "Emu80qt-mcp.exe"
    if record["executableSha256"] != sha(emulator.read_bytes()) or record["patchSha256"] != sha(PATCH.read_bytes()):
        raise ValueError("Экспериментальный шаблон расходится с хешами сборки")
    os.environ["SPECIALIST_EMU80_POOL_TEMPLATE"] = str(template.resolve())
    return record


def checked_disks(archive):
    names = {row["Disk"]: row["DiskSHA256"] for row in catalog_rows()}
    result = {}
    cylinders, sides, sectors, sector_bytes = GEOMETRY
    for name, digest in names.items():
        image = archive / name
        contents = image.read_bytes()
        if sha(contents) != digest or len(contents) != cylinders * sides * sectors * sector_bytes:
            raise ValueError(f"Исходная дискета расходится с реестром {image}")
        result[name] = contents
    return result


def state_pc(state):
    return state["registers"]["pc"]["dec"]
