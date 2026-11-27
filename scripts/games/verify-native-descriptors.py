"""Сверить архивные CPU/I80, загруженное тело, ROM, адрес входа и кадры Emu80."""

import argparse
from collections import Counter
from pathlib import Path
import hashlib
import json

from campaign_guest import Image


RELOCATION = {"m2_d400.mon": 0x1A, "mx_d400.mon": 0x1A, "m1_d800.mon": 0x20}
RESET_MONITORS = {"r6_0000.mon", "spmx.rom"}
STOCK_ROM_SHA256 = "121c271c5578274d1f6053015061e3378a2835a4f9c29f4974fea6fdec8f0df8"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, detail):
    if not condition:
        raise ValueError(detail)


def local_file(root, name):
    path = (root / name).resolve()
    require(path.is_relative_to(root.resolve()) and path.is_file(), path)
    return path


def read_json(path):
    return json.loads(path.read_bytes())


def read_file(root, name, expected_sha256):
    path = local_file(root, name)
    data = path.read_bytes()
    require(digest(data) == expected_sha256, f"Изменилась подпись {path}")
    return data


def rom_name(monitor):
    return "platform/spmx.rom" if monitor == "spmx.rom" else f"monitors/{monitor}"


def verify_profile(profile, directory, files, monitor, stock):
    config = read_file(directory, profile["config"]["file"], profile["config"]["sha256"])
    read_file(directory, profile["options"]["file"], profile["options"]["sha256"])
    require(profile["monitor_sha256"] == digest(files[rom_name(monitor)]), directory)
    expected = files["platform/spmx.conf"].decode("utf-8-sig").replace("ppiCircuit.videoRam = &videoRam", "")
    if not stock and monitor == "r6_0000.mon":
        expected = expected.replace('@ROM = "specmx/spmx.rom"', '@ROM = "specmx/r6_0000.mon"')
    if not stock and monitor in RELOCATION:
        expected = expected.replace("loader.addrSpace = &addrSpace", "loader.addrSpace = &ramPage")
    require(config.decode().splitlines() == expected.splitlines(), f"Иная конфигурация {directory}")


def verify_trial(folder, record, files, stock, facts):
    monitor = record["monitor_file"]
    kernel = files[rom_name(monitor)]
    require(record["monitor_sha256"] == digest(kernel), folder)
    reset = record["reset_rom"]
    selected = "r6_0000.mon" if not stock and monitor == "r6_0000.mon" else "spmx.rom"
    expected = files[rom_name(selected)]
    reset_payload = read_file(folder, reset["file"], reset["sha256"])
    reset_matches = reset_payload == expected
    require(reset["state"]["reset"] and reset["state"]["breaked"] and int(reset["state"]["pc"], 16) == 0, folder)
    require(reset["monitor"] == selected and reset["expected_sha256"] == digest(expected), folder)
    require(reset["matches"] == reset_matches, folder)
    facts["reset_rom"] += 1

    loader = record["loader"]
    require(loader["loaded"] and loader["breaked"], folder)
    observed_pc = int(loader["registers"]["pc"]["hex"], 16)
    require(observed_pc == record["actual_entry"], folder)
    observed_body = read_file(folder, record["loaded_i80"], record["actual_i80_sha256"])
    exact_payload = observed_body == files[record["i80"]] and observed_pc == record["entry"]
    facts["exact_payload_and_pc"] += int(exact_payload)

    if monitor in RELOCATION:
        hooks = [(address, RELOCATION[monitor] + address - 0xC000, length)
                 for address, length in ((0xC000, 64), (0xC037, 8), (0xC337, 8), (0xC800, 64))]
    else:
        hooks = [(0xC800, 0x630, 32), (0xF800, 0x1020, 96)]
    require(len(record["monitor_anchors"]) == len(hooks), folder)
    all_match = True
    for anchor, (address, offset, length) in zip(record["monitor_anchors"], hooks):
        source = kernel[offset:offset + length]
        observation = bytes.fromhex(anchor["observed_hex"])
        require(anchor["address"] == address and anchor["offset"] == offset and len(observation) == length, folder)
        require(anchor["expected_sha256"] == digest(source) and anchor["observed_sha256"] == digest(observation), folder)
        require(anchor["matches"] == (source == observation), folder)
        all_match &= source == observation
        facts["monitor_anchors"] += 1

    require(set(record["frames"]) == {"before", "entry", "first-state", "idle"}, folder)
    for frame in record["frames"].values():
        read_file(folder, frame["file"], frame["sha256"])
        with Image.open(local_file(folder, frame["file"])) as image:
            require(image.format == "PNG" and image.width > 0 and image.height > 0, folder)
            image.verify()
        facts["png"] += 1

    if not exact_payload:
        return "load_or_entry_mismatch"
    if not reset_matches or (monitor in RESET_MONITORS and monitor != selected) or not all_match:
        return "payload_verified_monitor_unconfirmed"
    return "descriptor_verified"


def verify_catalogue(index, files, fixture_map):
    coverage = index["catalogue_coverage"]
    names = {"catalogue/catalog.csv", "catalogue/game-inventory.json", "catalogue/archive-index.json"}
    require(coverage["sources"] == {name: fixture_map[name] for name in names}, coverage["sources"])
    inventory = json.loads(files["catalogue/game-inventory.json"])
    require(inventory["catalog_sha256"] == fixture_map["catalogue/catalog.csv"], coverage["sources"])
    archived_index = json.loads(files["catalogue/archive-index.json"])
    archived = {(row["cpu_sha256"], row["i80_sha256"]): row for row in archived_index["cases"]
                if row["outcome"] == "descriptor_verified"}
    covered = {row["file"]: row for row in coverage["matched"]}
    extra = {row["catalogue_cpu"]: row for row in index["cases"]}
    listed = {row["file"]: row for row in inventory["files"] if row["format"] == "cpu_i80"}
    identities = {(item["sha256"], item["payload_sha256"]) for item in listed.values()}
    require(len(covered) == len(coverage["matched"]) and not covered.keys() & extra.keys(), coverage)
    require(set(listed) == covered.keys() | extra.keys(), "Реестр покрыт не полностью")
    require(coverage["catalogue_rows"] == len(listed) and coverage["additional_cases"] == len(extra), coverage)
    for name, item in listed.items():
        identity = item["sha256"], item["payload_sha256"]
        if identity in archived:
            old = archived[identity]
            actual = covered[name]
            require(actual["archive_id"] == old["id"] and actual["archive_protocol"] == old["protocol"], name)
            require(actual["i80_file"] == item["body_file"], name)
        else:
            actual = extra[name]
            require(actual["catalogue_i80"] == item["body_file"], name)
            require(actual["cpu"] == f"catalogue/{name}" and actual["i80"] == f"catalogue/{item['body_file']}", name)
        require((actual["cpu_sha256"], actual["i80_sha256"]) == identity, name)
    archive_only = [dict(id=row["id"], monitor_name=row["monitor_name"], cpu_sha256=identity[0], i80_sha256=identity[1])
                    for identity, row in archived.items() if identity not in identities]
    require(coverage["archive_only"] == archive_only, "Неверный остаток архивных записей")
    return dict(matched=len(covered), additional=len(extra), archive_only=len(archive_only))


def verify(path, archive=None):
    root = path.parent
    index = read_json(path)
    stock = index["stock_loader_mapping"]
    require(index["loader"] == "SpecMxFileLoader" and index["platform"] == "spmx", path)
    fixture_map = index["fixture_sha256"]
    key = digest(json.dumps(fixture_map, sort_keys=True).encode())
    require(index["fixture"] == f"fixtures/{key}", path)
    fixture = root / index["fixture"]
    require(read_json(local_file(fixture, "sha256.json")) == fixture_map, fixture)
    files = {name: read_file(fixture, name, expected) for name, expected in fixture_map.items()}
    require(digest(files["platform/spmx.rom"]) == STOCK_ROM_SHA256, path)
    catalogue = verify_catalogue(index, files, fixture_map) if "catalogue_coverage" in index else None
    if archive is not None:
        originals = {item.name.lower(): item for item in archive.iterdir() if item.is_file()}
        saved_cpu = {Path(name).name.lower() for name in files if name.lower().endswith(".cpu")}
        if catalogue is None:
            require(saved_cpu == {name for name in originals if name.endswith(".cpu")}, archive)
        for name, data in files.items():
            if name.startswith(("archive/", "monitors/")):
                original = originals[Path(name).name.lower()]
                require(original.read_bytes() == data, f"Сохранённая копия отличается от {original}")
    seen = set()
    attempts = set()
    counts = Counter()
    facts = Counter()
    for row in index["cases"]:
        require(row["id"] not in seen and Path(row["cpu"]).name.lower() == row["id"], row["id"])
        seen.add(row["id"])
        cpu = files[row["cpu"]]
        values = cpu.decode("ascii").split()
        require(len(values) in (2, 3), row)
        named = values[2] if len(values) == 3 else None
        load_address, entry = (int(value, 16) for value in values[:2])
        body = files[row["i80"]] if row["i80"] else b""
        require(row["cpu_sha256"] == digest(cpu) and row["i80_sha256"] == digest(body), row)
        require(row["load"] == load_address and row["entry"] == entry and row["body_size"] == len(body), row)
        require(row["end_exclusive"] == load_address + len(body) and row["monitor_name"] == named, row)
        monitor = named.lower() if named else "spmx.rom"
        require(row["monitor_file"] == monitor, row)
        protocol = local_file(root, row["protocol"])
        attempt = protocol.parent.parent
        attempts.add(attempt)
        record = read_json(protocol)
        require(all(record[field] == value for field, value in row.items() if field != "protocol"), protocol)
        require(record["pid"] > 0 and record["hwnd"] > 0 and record["emulator_sha256"] == index["emulator_sha256"], protocol)
        if not body or not load_address <= entry < load_address + len(body) <= 0xC000:
            outcome = "unsupported_payload_range"
        elif monitor not in RESET_MONITORS and monitor not in RELOCATION:
            outcome = "unsupported_monitor"
        else:
            outcome = verify_trial(protocol.parent, record, files, stock, facts)
        require(outcome == row["outcome"], protocol)
        counts[outcome] += 1
    require(len(attempts) == 1 and dict(counts) == index["counts"], path)
    attempt, = attempts
    require(read_json(local_file(attempt, "index.json")) == index, attempt)
    for monitor, profile in index["profiles"].items():
        verify_profile(profile, attempt / "profiles" / monitor, files, monitor, stock)
    report = dict(cases=len(seen), stock=stock, counts=dict(counts), checks=dict(facts), fixtures=len(files),
                  profiles=len(index["profiles"]), originals_compared=archive is not None)
    if catalogue is not None:
        report["catalogue"] = catalogue
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("index", nargs="+", type=Path)
    parser.add_argument("--archive", type=Path, help="Дополнительно сверить копии с исходным архивом")
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    try:
        checks = [verify(path.resolve(), args.archive) for path in args.index]
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, f"Ошибка проверки: {error}\n")
    report = json.dumps(checks, ensure_ascii=False, indent=2) + "\n"
    if args.out:
        args.out.write_text(report, encoding="utf-8")
    print(report, end="")


if __name__ == "__main__":
    main()
