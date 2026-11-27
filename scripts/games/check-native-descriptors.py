"""Проверить архивные CPU/I80 штатным SpecMxFileLoader в видимом Emu80."""

import argparse
from collections import Counter
from pathlib import Path
import json
import re
import sys
import uuid

from campaign_media import ROOT, sha
from lafan_fdc import state_pc
from lafan_native import ROM_SHA256, prepare_platform, switch_to_native

sys.path.insert(0, str(ROOT / "scripts/emu80"))
from mcp_pool import lease


OUT = ROOT / "docs/compat/game-campaign/native-descriptors"
STOCK_OUT = OUT.with_name("native-descriptors-stock")
CATALOG_OUT = OUT.with_name("native-descriptors-catalog")
CATEGORY_CSV = ROOT / "assets/games/catalog.csv"
PLATFORM = ROOT / ".local/emu80-pool-pristine/dist/specmx"
RELOCATED = {"m2_d400.mon": 0x1A, "mx_d400.mon": 0x1A, "m1_d800.mon": 0x20}
RESET_ROM = {"r6_0000.mon", "spmx.rom"}
HOOKS = (0xC000, 0xC037, 0xC337, 0xC800)


def write_json(path, value):
    temporary = path.with_suffix(f".{uuid.uuid4().hex}.tmp")
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def load(path):
    return json.loads(path.read_bytes())


def descriptor_case(cpu_file, data_file, text, body):
    fields = text.decode("ascii").split()
    if len(fields) not in (2, 3) or not all(re.fullmatch(r"[0-9a-fA-F]{1,4}", value) for value in fields[:2]):
        raise ValueError(f"Неверные поля исходного дескриптора {cpu_file}")
    load_address, entry = (int(value, 16) for value in fields[:2])
    monitor = fields[2] if len(fields) == 3 else None
    return dict(id=Path(cpu_file).name.lower(), cpu=cpu_file, cpu_sha256=sha(text), i80=data_file,
                i80_sha256=sha(body), load=load_address, end_exclusive=load_address + len(body), entry=entry,
                monitor_name=monitor, monitor_file=monitor.lower() if monitor else "spmx.rom", body_size=len(body))


def source_data(archive, platform):
    originals = {path.name.lower(): path for path in archive.iterdir() if path.is_file()}
    available = {f"platform/{name}": (platform / name).read_bytes() for name in ("spmx.conf", "spmx.rom", "spmx.html")}
    if sha(available["platform/spmx.rom"]) != ROM_SHA256:
        raise ValueError("Ожидался исходный ROM RAMFOS из профиля Emu80")
    for name in RELOCATED.keys() | {"r6_0000.mon"}:
        available[f"monitors/{name}"] = originals[name].read_bytes()
    cases = []
    for _, source in sorted(originals.items()):
        if source.suffix.lower() != ".cpu":
            continue
        binary = originals.get(source.with_suffix(".i80").name.lower())
        text = source.read_bytes()
        cpu_file = f"archive/{source.name}"
        available[cpu_file] = text
        data_file = f"archive/{binary.name}" if binary else None
        body = binary.read_bytes() if binary else b""
        if binary:
            available[data_file] = body
        cases.append(descriptor_case(cpu_file, data_file, text, body))
    return available, cases


def catalogue_data(files, inventory, archive_index):
    available = {name: data for name, data in files.items() if name.startswith(("platform/", "monitors/"))}
    sources = {
        "catalogue/game-inventory.json": inventory.read_bytes(),
        "catalogue/catalog.csv": CATEGORY_CSV.read_bytes(),
        "catalogue/archive-index.json": archive_index.read_bytes(),
    }
    index = json.loads(sources["catalogue/game-inventory.json"])
    archive = json.loads(sources["catalogue/archive-index.json"])
    if index["catalog_sha256"] != sha(sources["catalogue/catalog.csv"]):
        raise ValueError("Реестр игр не соответствует исходному catalog.csv")
    archived = {(row["cpu_sha256"], row["i80_sha256"]): row for row in archive["cases"]
                if row["outcome"] == "descriptor_verified"}
    catalogue_pairs = set()
    matched = []
    cases = []
    for row in index["files"]:
        if row["format"] != "cpu_i80":
            continue
        identity = (row["sha256"], row["payload_sha256"])
        catalogue_pairs.add(identity)
        cpu_path, data_path = row["file"], row["body_file"]
        cpu, body = ((ROOT / name).read_bytes() for name in (cpu_path, data_path))
        if (sha(cpu), sha(body)) != identity:
            raise ValueError(f"Реестр устарел для {cpu_path}")
        if identity in archived:
            reference = archived[identity]
            matched.append(dict(file=cpu_path, i80_file=data_path, cpu_sha256=identity[0], i80_sha256=identity[1],
                                archive_id=reference["id"], archive_protocol=reference["protocol"]))
            continue
        cpu_file, data_file = (f"catalogue/{name}" for name in (cpu_path, data_path))
        available.update({cpu_file: cpu, data_file: body})
        case = descriptor_case(cpu_file, data_file, cpu, body)
        case.update(catalogue_cpu=cpu_path, catalogue_i80=data_path)
        cases.append(case)
    originals = {"catalogue/game-inventory.json": inventory.resolve(), "catalogue/catalog.csv": CATEGORY_CSV,
                 "catalogue/archive-index.json": archive_index.resolve()}
    origins = {name: (path.relative_to(ROOT) if path.is_relative_to(ROOT) else path).as_posix()
               for name, path in originals.items()}
    coverage = dict(sources={name: sha(data) for name, data in sources.items()}, source_paths=origins, matched=matched,
                    catalogue_rows=len(matched) + len(cases), additional_cases=len(cases),
                    archive_only=[dict(id=row["id"], monitor_name=row["monitor_name"], cpu_sha256=identity[0],
                                       i80_sha256=identity[1]) for identity, row in archived.items()
                                  if identity not in catalogue_pairs])
    available.update(sources)
    return available, cases, coverage


def save_fixture(out, files):
    hashes = {name: sha(data) for name, data in files.items()}
    key = sha(json.dumps(hashes, sort_keys=True).encode())
    target = out / "fixtures" / key
    for name, data in files.items():
        path = target / name
        path.parent.mkdir(parents=True, exist_ok=True)
        if path.is_file() and path.read_bytes() != data:
            raise ValueError(f"Сохранённый исходник изменился {path}")
        if not path.is_file():
            path.write_bytes(data)
    write_json(target / "sha256.json", hashes)
    return target, hashes


def kernel_path(fixture, monitor):
    return fixture / ("platform/spmx.rom" if monitor == "spmx.rom" else f"monitors/{monitor}")


def kernel_anchors(monitor):
    if monitor in RELOCATED:
        start = RELOCATED[monitor]
        return [(address, start + address - 0xC000, 64 if address in (0xC000, 0xC800) else 8)
                for address in HOOKS]
    return [(0xC800, 0x630, 32), (0xF800, 0x1020, 96)]


def configure(slot, fixture, profile_dir, profiles, monitor, stock):
    directory = prepare_platform(slot, fixture / "platform")
    path = directory / "spmx.conf"
    config = path.read_text(encoding="utf-8")
    if monitor == "r6_0000.mon" and not stock:
        initial = '@ROM = "specmx/spmx.rom"'
        if config.count(initial) != 1:
            raise ValueError("Профиль не содержит ожидаемое имя ROM")
        config = config.replace(initial, f'@ROM = "specmx/{monitor}"')
    if monitor in RELOCATED and not stock:
        initial = "loader.addrSpace = &addrSpace"
        if config.count(initial) != 1:
            raise ValueError("Профиль не содержит ожидаемый адресный объект загрузчика")
        # Загрузчик меняет страницу CPU, а монитор и тело должны попасть в RAM страницы 0.
        config = config.replace(initial, "loader.addrSpace = &ramPage")
    source = kernel_path(fixture, monitor)
    (directory / monitor).write_bytes(source.read_bytes())
    path.write_text(config, encoding="utf-8")
    if monitor not in profiles:
        target = {}
        profile = profile_dir / monitor
        profile.mkdir()
        for name, field in (("spmx.conf", "config"), ("spmx.opt", "options")):
            data = (directory / name).read_bytes()
            (profile / name).write_bytes(data)
            target[field] = dict(file=name, sha256=sha(data))
        target["monitor_sha256"] = sha(source.read_bytes())
        profiles[monitor] = target


def frame(machine, folder, result, name):
    path = folder / f"{name}.png"
    machine.screenshot(path)
    result.setdefault("frames", {})[name] = {"file": path.name, "sha256": sha(path.read_bytes())}


def run_entry(machine, folder, fixture, case, stock):
    program = fixture / case["cpu"]
    body = (fixture / case["i80"]).read_bytes()
    monitor = case["monitor_file"]
    kernel = kernel_path(fixture, monitor).read_bytes()
    record = dict(**case, monitor_sha256=sha(kernel))
    reset_monitor = "r6_0000.mon" if monitor == "r6_0000.mon" and not stock else "spmx.rom"
    expected_reset = kernel_path(fixture, reset_monitor).read_bytes()
    reset = machine.call("emu_reset")
    if not reset["reset"] or not reset["breaked"] or int(reset["pc"], 16) != 0:
        raise RuntimeError(f"Не подтверждена остановка в точке сброса: {reset}")
    observed_reset = machine.read(0, len(expected_reset))
    reset_file = folder / "reset-rom.bin"
    reset_file.write_bytes(observed_reset)
    record["reset_rom"] = dict(state=reset, monitor=reset_monitor, file=reset_file.name,
                               sha256=sha(observed_reset), expected_sha256=sha(expected_reset),
                               matches=observed_reset == expected_reset)
    frame(machine, folder, record, "before")
    loaded = machine.call("emu_load", file=program.as_posix(), autorun=True, break_after_load=True)
    record["loader"] = loaded
    if not loaded["loaded"] or not loaded["breaked"] or "registers" not in loaded:
        return dict(record, outcome="loader_unconfirmed")
    observed = machine.read(case["load"], len(body))
    (folder / "loaded.i80").write_bytes(observed)
    record.update(actual_entry=state_pc(loaded), actual_i80_sha256=sha(observed), loaded_i80="loaded.i80")
    anchors = []
    for address, offset, length in kernel_anchors(monitor):
        expected = kernel[offset:offset + length]
        actual = machine.read(address, length)
        anchors.append(dict(address=address, offset=offset, expected_sha256=sha(expected), observed_hex=actual.hex(),
                            observed_sha256=sha(actual), matches=actual == expected))
    record["monitor_anchors"] = anchors
    record["entry_state"] = machine.call("cpu_state")
    frame(machine, folder, record, "entry")
    record["first_state"] = machine.call("emu_run_for", ms=450)
    frame(machine, folder, record, "first-state")
    record["idle_state"] = machine.call("emu_run_for", ms=1800)
    frame(machine, folder, record, "idle")
    if record["actual_entry"] != case["entry"] or observed != body:
        outcome = "load_or_entry_mismatch"
    elif (observed_reset != expected_reset or monitor in RESET_ROM and monitor != reset_monitor
          or not all(sample["matches"] for sample in anchors)):
        outcome = "payload_verified_monitor_unconfirmed"
    else:
        outcome = "descriptor_verified"
    return dict(record, outcome=outcome)


def run_campaign(args, files, cases, coverage=None):
    fixture, hashes = save_fixture(args.out, files)
    attempt = args.out / ("attempt-" + uuid.uuid4().hex[:12])
    profile_dir = attempt / "profiles"
    profile_dir.mkdir(parents=True)
    profiles = {}
    results = []
    with lease(size=16, port=args.port, artifact_root=args.out / "slots") as slot:
        emulator_sha256 = sha((slot.runtime / "Emu80qt-mcp.exe").read_bytes())
        owner = load(slot.artifacts / "slot.json")
        for index, case in enumerate(cases, 1):
            folder = attempt / Path(case["cpu"]).stem.lower()
            folder.mkdir()
            monitor = case["monitor_file"]
            try:
                if not case["i80"] or not case["load"] <= case["entry"] < case["end_exclusive"] <= 0xC000:
                    record = dict(case, outcome="unsupported_payload_range")
                elif monitor not in RELOCATED and monitor not in RESET_ROM:
                    record = dict(case, outcome="unsupported_monitor")
                else:
                    configure(slot, fixture, profile_dir, profiles, monitor, args.stock)
                    record = run_entry(switch_to_native(slot), folder, fixture, case, args.stock)
            except Exception as error:
                record = dict(case, outcome="loader_error", error=str(error))
            record.update(pid=slot.pid, hwnd=owner["hwnd"], emulator_sha256=emulator_sha256)
            path = folder / "result.json"
            write_json(path, record)
            results.append(dict(**case, outcome=record["outcome"], protocol=path.relative_to(args.out).as_posix()))
            print(f"{index}/{len(cases)} {case['cpu']} {case['load']:04X}/{case['entry']:04X} {record['outcome']}", flush=True)
    manifest = dict(loader="SpecMxFileLoader", platform="spmx", stock_loader_mapping=args.stock,
                    emulator_sha256=emulator_sha256, fixture=fixture.relative_to(args.out).as_posix(),
                    fixture_sha256=hashes, profiles=profiles, cases=results,
                    counts=dict(Counter(row["outcome"] for row in results)))
    if coverage is not None:
        manifest["catalogue_coverage"] = coverage
    write_json(attempt / "index.json", manifest)
    write_json(args.out / "index.json", manifest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="Каталог с подлинными CPU, I80 и MON")
    parser.add_argument("--platform", type=Path, default=PLATFORM, help="Исходный каталог профиля specmx")
    parser.add_argument("--out", type=Path)
    parser.add_argument("--port", type=int, choices=range(19781, 19797))
    parser.add_argument("--stock", action="store_true", help="Оставить исходную настройку ROM и адресного объекта загрузчика")
    parser.add_argument("--file", action="append", default=[], help="Проверить названные дескрипторы CPU")
    parser.add_argument("--inventory", type=Path, help="Проверить пары из реестра, которых нет в архивном индексе")
    parser.add_argument("--archive-index", type=Path, default=OUT / "index.json")
    args = parser.parse_args()
    if args.inventory and (args.stock or args.file):
        parser.error("Дополнение реестра запускается в частном профиле для всех несовпадений")
    args.out = (args.out or (CATALOG_OUT if args.inventory else STOCK_OUT if args.stock else OUT)).resolve()
    files, cases = source_data(args.archive, args.platform)
    coverage = None
    if args.inventory:
        files, cases, coverage = catalogue_data(files, args.inventory, args.archive_index)
    selected = {name.lower() for name in args.file}
    if selected:
        cases = [case for case in cases if case["id"] in selected]
    if not cases:
        parser.error("Нет дескрипторов для проверки")
    run_campaign(args, files, cases, coverage)


if __name__ == "__main__":
    main()
