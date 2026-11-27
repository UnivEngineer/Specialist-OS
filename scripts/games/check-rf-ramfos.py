"""Сравнить очереди RUN -RF и RUN -MX с запуском через Enter архивного RAMFOS."""
import argparse
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor, as_completed
import csv
import json
from pathlib import Path
from queue import Empty, Queue
import runpy
import struct
import sys
import tempfile
import traceback
import uuid

from campaign_controls import game_probe
from campaign_guest import Bench, Guest, Observation
from campaign_media import ROOT, sha
from campaign_play import play
from lafan_native import ROM_SHA256, prepare_platform, switch_to_native

HERE = Path(__file__).parent
ramfos = runpy.run_path(str(HERE/"check-lafan-ramfos.py"))
paired = runpy.run_path(str(HERE/"control-campaign.py"))
campaign = paired["CAMPAIGN"]
from game_inventory import payload
from executable import decode

SOURCE, LAFAN, EXE, lease, checksum = (
    ramfos[name] for name in ("SOURCE", "LAFAN", "EXE", "lease", "checksum")
)
list_name, launch, watch, trace_state = (ramfos[name] for name in ("list_name", "launch", "watch", "trace_state"))
DEFAULT = ROOT/"docs/compat/game-campaign"
VERIFIED_ENTRY = "entry_and_payload_verified"
SHELL_VECTORS = {0xF800: "ramfos_restart", 0xFA41: "ramfos_input"}
ROM_INPUT = 0xFA41
TEMPLATE_HEADER = bytes.fromhex("d3d3d3") + b"PROBE    COM" + bytes.fromhex("8c07089205010e0000") + bytes(6)
MX_SCOPE = {
    "36stup.cpu", "barbar.cpu", "canball.cpu", "ColorBall.cpu", "driller.cpu", "exolonmx.i80",
    "FireRescue.cpu", "hanoi.cpu", "land2.cpu", "sex0.cpu", "sex1.cpu", "tetris_1.cpu", "blob_cop.cpu",
    "Peshchera-12FF.rks", "Peshchera-0FFF.rks", "Almaz2-mx.exe", "StarWars2-mx.cpu",
}
MX_NATIVE = {"Almaz2-mx-optimized.exe"}


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def save(path, data):
    campaign["write"](path, data)


class NativeGuest(Guest):
    """Те же измерения CPU, кадра и клавиатуры над гостем со штатным ROM."""

    def __init__(self, slot, out, result, entry):
        Bench.__init__(self, slot.url)
        self.slot, self.out, self.result = slot, out, result
        self.profile, self.keys, self.exit_seen = "RF", result.setdefault("events", []), None
        self.tools = {"mx_audio", "emu_speed"}
        self.return_points = {pc: reason for pc, reason in SHELL_VECTORS.items() if pc != entry}
        self.result["cpu"] = self.call("sys_info")["cpu"]

    def integrity(self, phase):
        ranges = ((0xC800, 0x30), (0xF800, 0x7E0))
        self.result.setdefault("rom_ranges", {})[phase] = [
            dict(addr=addr, size=size, sha256=sha(self.read(addr, size))) for addr, size in ranges
        ]


def read_archive():
    with LAFAN.open(encoding="utf-8", newline="") as stream:
        rows = defaultdict(list)
        for row in csv.DictReader(stream):
            rows[row["PayloadSHA256"]].append(row)
        return rows


def select_transport(record, sources, body):
    start, entry = record["load"], record["entry"]
    if start is None or entry is None:
        return None, dict(status="address_unknown")
    if entry != start:
        return None, dict(status="offset_entry_unrepresentable",
                          reason="RAMFOS Enter передаёт управление на адрес начала записи")
    end = start + len(body) - 1
    for source in sources:
        header = bytes.fromhex(source["HeaderHex"])
        if struct.unpack_from("<HH", header, 24) != (start, end):
            continue
        return header[:30], dict(origin="archival_odi_header", odi=source["Disk"],
                                 odi_sha256=source["DiskSHA256"], offset=source["DirectoryOffset"],
                                 file=source["File"], odi_header_hex=source["HeaderHex"])
    if sources:
        return None, dict(status="archival_load_mismatch",
                          reason="Подлинный заголовок совпавшего тела задаёт иной адрес")
    if not 0 <= start <= end <= 0xFFFF:
        return None, dict(status="invalid_range", reason="Диапазон тела вне 16-битной адресации")
    header = bytearray(TEMPLATE_HEADER)
    header[3:12] = b"P" + record["payload_sha256"][:8].upper().encode("ascii")
    struct.pack_into("<HHH", header, 24, start, end, checksum(body))
    return bytes(header), dict(origin="derived_ramfos_com_header", checksum_origin="recalculated_from_exact_body")


def primary_files(directory, group, main_results):
    for job in group:
        path = directory/"trials"/job["id"]/"result.json"
        current = read_json(path)
        record, route = job["record"], job["route"]
        if (current["payload_sha256"], current["container_sha256"]) != (record["payload_sha256"], route["sha256"]):
            raise ValueError(f'Текущий RF-протокол имеет другое тело или контейнер: {job["id"]}')
        main = main_results[job["id"]]
        report = main.get("paired") or {}
        candidate = report.get("protocol")
        comparison = None
        if candidate and (directory/candidate).is_file():
            pro = read_json(directory/candidate)
            if (pro["payload_sha256"], pro["container_sha256"], pro["load"], pro["entry"]) == (
                    record["payload_sha256"], route["sha256"], record["load"], record["entry"]):
                if pro["status"] == "object_verified" and paired["verified_flash_protection"](
                        pro, paired["FLASH_PROTECTION"]):
                    comparison = (candidate, pro)
        yield job, path, current, comparison


def plan_group(directory, group, main_results, archive):
    addresses = {(job["record"]["load"], job["record"]["entry"]) for job in group}
    if len(addresses) != 1:
        raise ValueError(f"Одинаковое тело имеет разные адреса в очереди RF: {addresses}")
    choices = list(primary_files(directory, group, main_results))
    job, path, primary, comparison = max(choices, key=lambda row: (
        row[2]["launch"] == VERIFIED_ENTRY, bool(row[3]), bool(row[2]["scenario"]), row[0]["route"]["recipe"] == "original"
    ))
    record = job["record"]
    body = payload(record)
    header, transport = select_transport(record, archive[record["payload_sha256"]], body)
    profile = job["profile"]
    result = dict(payload_sha256=record["payload_sha256"], load=record["load"], entry=record["entry"],
                  payload_bytes=len(body), source_sha256=record["sha256"], file=record["file"],
                  entry_source=record.get("entry_source") or record["address_source"], profile=profile,
                  transport=transport, selected_primary=path.relative_to(directory).as_posix())
    result[profile.lower()+"_trials"] = [
                      dict(id=j["id"], result=p.relative_to(directory).as_posix(),
                           observation=(p.parent/c["evidence"]/"observation.json").relative_to(directory).as_posix(),
                           launch=c["launch"], input=c["input"], exit=c["exit"], source_sha256=c["source_sha256"],
                           container_sha256=c["container_sha256"], route=j["route"]["recipe"], aliases=j["aliases"],
                           paired=t[0] if t else None)
                      for j, p, c, t in choices]
    if profile == "MX":
        result.update(scope="Аппаратная проба MX, ABI RAMFOS — гипотеза",
                      archive_monitor=record.get("descriptor_monitor") or record["catalog"]["ArchiveProfile"],
                      source_note=record.get("source_path_note") or record["catalog"]["Notes"])
    if comparison:
        result["paired_primary"] = comparison[0]
    return job, body, header, primary, comparison[1] if comparison else None, result


def boot(slot, out, ramdisk, common, body, header, load):
    out.mkdir()
    machine = switch_to_native(slot)
    machine.call("emu_reset")
    machine.call("emu_run_for", ms=1000)
    report = dict(**common, initial_media=dict(ramdisk=sha(ramdisk.read_bytes())), frames={})
    guest = NativeGuest(slot, out, report, load)
    for pc, code in ramfos["FIRMWARE_MARKERS"].items():
        if guest.read(pc, len(code)) != code:
            raise RuntimeError(f"Отсутствует ожидаемый ROM-код у {pc:04X}h")
    guest.frame("date")
    report["listing"] = list_name(guest, header)
    guest.frame("command")
    report["before_body_sha256"] = sha(guest.read(load, len(body)))
    # Длинная запись через покадровое переключение RAM-диска может занять несколько гостевых секунд.
    try:
        report["launch"] = launch(guest, report, body, load, load_limit=32000)
    except RuntimeError as error:
        report.update(launch="loader_observation_error", error=str(error), stop=trace_state(guest))
        guest.frame("loader-error")
        save(out/"observation.json", report)
        raise
    guest.frame("entry")
    memory = guest.read(load, len(body))
    report["final_loader_body_sha256"] = sha(memory)
    if report["launch"] == "entry_payload_mismatch":
        differences = [(load + offset, original, loaded) for offset, (original, loaded) in enumerate(zip(body, memory))
                       if original != loaded]
        report["entry_mismatch"] = dict(count=len(differences),
                                       first_addresses=[list(change) for change in differences[:16]],
                                       last_addresses=[list(change) for change in differences[-16:]],
                                       address_regions=dict(main=sum(addr < 0x9000 for addr, _, _ in differences),
                                                            video=sum(0x9000 <= addr < 0xC000 for addr, _, _ in differences),
                                                            other=sum(addr >= 0xC000 for addr, _, _ in differences)))
        (out/"entry-body.bin").write_bytes(memory)
    return guest, report


def check_return(guest):
    if guest.exit_seen == "ramfos_restart":
        watch(guest, ROM_INPUT)
        if guest.regs()["pc"] != ROM_INPUT:
            guest.advance(8000)
        state = trace_state(guest)
        guest.result["shell_after_exit"] = dict(state=state, confirmed=state["registers"]["pc"] == ROM_INPUT)
        guest.frame("ramfos-returned")
    return guest.exit_seen or "unobserved"


def broad_play(guest, job, primary):
    start = len(guest.keys)
    try:
        play(guest, job["record"], primary["scenario"])
    except Observation as error:
        guest.result["game_obstacle"] = str(error)
        guest.frame("obstacle")
    result = guest.result
    result["exit"] = check_return(guest)
    achieved = [event["key"] for event in guest.keys[start:] if "key" in event]
    previous = [event["key"] for event in primary["events"] if "key" in event and "command" not in event]
    result["same_primary_game_keys"] = achieved == previous
    result["rf_game_keys"] = previous
    result["native_game_keys"] = achieved
    comparable = ("title", "before", "idle", "after", "exit-observation")
    result["frame_comparison"] = {
        name: dict(same_rgb=frame["rgb_sha256"] == primary["frames"][name]["rgb_sha256"],
                   same_palette_histogram=frame["palette"] == primary["frames"][name]["palette"],
                   native_colors=frame["colors"], rf_colors=primary["frames"][name]["colors"])
        for name in comparable if (frame := result["frames"].get(name)) and name in primary["frames"]
    }


def object_samples(arms):
    observed_fields = ("player_cells", "brick_8853", "holes_8d00", "rocket_8808", "rocket_track_8820",
                       "turn_control", "turn_counter", "docking_flag", "player_xy")
    return {
        arm["arm"]: {phase: {field: data[field] for field in observed_fields if field in data}
                     for phase, data in arm.get("checkpoints", {}).items() if phase == "before" or phase.startswith("action-")}
        for arm in arms
    }


def paired_play(slot, out, ramdisk, common, body, header, load, primary, probe, retries):
    scenario = primary["scenario"]
    arms = []
    for arm in paired["ARMS"]:
        target = out/arm
        rejected = []
        for trial in range(retries + 1):
            guest, result = boot(slot, target, ramdisk, common, body, header, load)
            result["arm"] = arm
            if result["launch"] != VERIFIED_ENTRY:
                result["obstacle"] = "Архивный ROM остановил запись до входа"
            else:
                # Пробы подготовки сравниваются до клавиши. Последний старт даёт наблюдение даже при несовпадении.
                reference = arms[0].get("checkpoints") if arms and trial < retries else None
                try:
                    paired["sample_arm"](guest, scenario, probe, arm.startswith("input-"), reference)
                except Observation as error:
                    result["obstacle"] = str(error)
                    guest.frame("obstacle")
                result["exit"] = check_return(guest)
            save(target/"observation.json", result)
            if not result.get("prestate_rejected"):
                break
            previous = target.with_name(f"{arm}-prestate-{trial + 1}")
            target.rename(previous)
            rejected.append(previous.name)
        if rejected:
            result["rejected_preparations"] = rejected
            save(target/"observation.json", result)
        arms.append(result)
    response = paired["compare"](out, arms, probe)
    old_samples, new_samples = object_samples(primary["arms"]), object_samples(arms)
    response.update(probe=probe, scenario=scenario, native_samples=new_samples, rf_samples=old_samples,
                    exact_object_samples_match=new_samples == old_samples,
                    arms={arm["arm"]: f'{arm["arm"]}/observation.json' for arm in arms})
    return response


def run_trial(slot, directory, ramdisk, task, common, retries):
    job, body, header, primary, primary_pair, result = task
    digest = result["payload_sha256"]
    out = directory/digest
    out.mkdir()
    if header is None:
        result["native_launch"] = result["transport"]["status"]
        save(out/"result.json", result)
        return result
    image = (header + body + b"\0\0\x08").ljust(8*65536, b"\0")
    ramdisk.write_bytes(image)
    result["transport"].update(header_hex=header.hex(), extension=header[12:15].decode("ascii"),
                               ram_disk_sha256=sha(image))
    try:
        guest, report = boot(slot, out/"primary", ramdisk, common, body, header, result["load"])
        guest.profile = job["profile"]
        result.update(native_launch=report["launch"], native_observation="primary/observation.json")
        if report["launch"] == VERIFIED_ENTRY and primary["launch"] == VERIFIED_ENTRY:
            broad_play(guest, job, primary)
            result["frame_comparison"] = report["frame_comparison"]
            result["same_primary_game_keys"] = report["same_primary_game_keys"]
            result["native_exit"] = report["exit"]
            result["primary_exit"] = primary["exit"]
            result["audio_presence"] = {
                side: {phase: sample.get("varied") for phase, sample in data.get("audio", {}).items()}
                for side, data in (("ramfos", report), ("run_"+guest.profile.lower(), primary))
            }
        elif report["launch"] == VERIFIED_ENTRY and job["profile"] == "MX":
            try:
                play(guest, job["record"], primary["scenario"])
            except Observation as error:
                report["game_obstacle"] = str(error)
                guest.frame("obstacle")
            report["exit"] = check_return(guest)
            result["native_exit"] = report["exit"]
            result["standalone_native_observation"] = True
        save(out/"primary"/"observation.json", report)
        if result["native_launch"] == VERIFIED_ENTRY and primary_pair:
            probe = game_probe(job, primary_pair["scenario"])
            if probe:
                result["action_comparison"] = paired_play(
                    slot, out, ramdisk, common, body, header, result["load"], primary_pair, probe, retries
                )
    except Exception as error:
        result.update(error=str(error), traceback=traceback.format_exc())
        if "native_launch" not in result:
            result["native_launch"] = "observation_error"
    save(out/"result.json", result)
    return result


def run_worker(port, out, source, pending, retries):
    results = []
    with lease(port=port, size=16, artifact_root=out/"slots") as slot, tempfile.TemporaryDirectory(
            prefix=f"mxos-native-rf-{port}-") as workspace:
        ramdisk = Path(workspace)/"game.ramdisk"
        conf = prepare_platform(slot, source, f'ramDisk.fileName = "{ramdisk.as_posix()}"')
        common = dict(pid=slot.pid, hwnd=slot.hwnd, port=port, rom_sha256=ROM_SHA256,
                      emulator_sha256=sha((slot.runtime/EXE).read_bytes()), config_sha256=sha((conf/"spmx.conf").read_bytes()))
        while True:
            try:
                task = pending.get_nowait()
            except Empty:
                break
            result = run_trial(slot, out, ramdisk, task, common, retries)
            results.append(result)
            print(f'{port} {result["payload_sha256"][:12]} {result["native_launch"]} {result["file"]}', flush=True)
    return results


def summarize(results, profile):
    def counts(values):
        return dict(Counter(values))

    summary = dict(
        native_outcomes=counts(r["native_launch"] for r in results),
        header_origins=counts(r["transport"].get("origin") or r["transport"]["status"] for r in results),
        same_primary_game_keys=sum(r.get("same_primary_game_keys") is True for r in results),
        same_primary_frame_rgb={name: sum((r.get("frame_comparison") or {}).get(name, {}).get("same_rgb") is True
                                           for r in results) for name in ("title", "before", "idle", "after", "exit-observation")},
        object_comparisons=counts(r["action_comparison"]["status"] for r in results if "action_comparison" in r),
        identical_object_samples=sum(r["action_comparison"]["exact_object_samples_match"]
                                     for r in results if "action_comparison" in r),
        observed_native_exit=sum(r.get("native_exit") not in (None, "unobserved") for r in results),
        errors=sum("error" in r for r in results),
    )
    summary["observed_" + profile.lower() + "_exit"] = sum(
        r.get("primary_exit") not in (None, "unobserved") for r in results
    )
    return summary


def native_only(records, directory):
    selected = []
    for record in records:
        path = ROOT/record["file"]
        if "NATIVE" not in record["profiles"] or path.name not in MX_NATIVE:
            continue
        container = decode(path.read_bytes(), "exe")
        selected.append(dict(file=record["file"], source_sha256=record["sha256"], payload_sha256=record["payload_sha256"],
                             entry=record["entry"], load=record["load"], status="native_only",
                             reason="MAIN и API MXOS2 объявлены исходным контейнером", native_flag=container.flags,
                             api=container.api, memory_end=container.memory_end, primary_profiles=[
                                 (directory/"trials"/(route["id"]+"-NATIVE")/"result.json").relative_to(directory).as_posix()
                                 for route in record["routes"]
                             ]))
    return selected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=SOURCE)
    parser.add_argument("--campaign", type=Path, default=DEFAULT)
    parser.add_argument("--inventory", type=Path, default=ROOT/"docs/compat/game-inventory.json")
    parser.add_argument("--profile", choices=("RF", "MX"), default="RF")
    parser.add_argument("--port", type=int, action="append", choices=range(19781, 19797), default=[])
    parser.add_argument("--file", action="append", default=[], help="Имя или окончание пути строки каталога")
    parser.add_argument("--repeat-preparation", type=int, choices=range(9), default=4,
                        help="Число повторных чистых стартов при несовпадении состояния до клавиши")
    args = parser.parse_args()
    directory = args.campaign.resolve()
    base = directory/"native-ramfos"
    profile = args.profile.lower()
    trials_key = profile + "_trials"
    out = base/(profile + "-attempt-" + uuid.uuid4().hex[:12])
    out.mkdir(parents=True)
    print(out, flush=True)
    inventory = read_json(args.inventory)
    cards = read_json(ROOT/"assets/games/controls.json")["cards"]
    main_results = read_json(directory/"results.json")["trials"]
    groups = defaultdict(list)
    for job in campaign["jobs"](inventory["files"], cards):
        if job["profile"] == args.profile:
            groups[job["record"]["payload_sha256"]].append(job)
    if args.profile == "MX":
        already_tried = {record["payload_sha256"] for record in read_json(base/"rf-index.json")["results"]}
        already_tried.update(read_archive())
        groups = {digest: group for digest, group in groups.items() if digest not in already_tried and any(
            Path(alias).name in MX_SCOPE for job in group for alias in job["aliases"]
        )}
    archive = read_archive()
    queue = Queue()
    counts = 0
    for group in groups.values():
        if args.file and not any(alias.endswith(suffix) for job in group for alias in job["aliases"] for suffix in args.file):
            continue
        queue.put(plan_group(directory, group, main_results, archive))
        counts += 1
    if not counts:
        parser.error("Нет нагрузок, подходящих к условиям прогона")
    ports = list(dict.fromkeys(args.port or [19796]))[:counts]
    results = []
    with ThreadPoolExecutor(max_workers=len(ports)) as pool:
        for future in as_completed([pool.submit(run_worker, port, out, args.source, queue, args.repeat_preparation)
                                    for port in ports]):
            results.extend(future.result())
    results.sort(key=lambda result: result["file"].casefold())
    statuses = Counter(result["native_launch"] for result in results)
    index = dict(schema=1, profile=args.profile, rom_sha256=ROM_SHA256, complete=not args.file and len(results) == len(groups),
                 inventory_sha256=sha(args.inventory.read_bytes()), harness_sha256=sha(Path(__file__).read_bytes()),
                 launch_counts=statuses, summary=summarize(results, profile), files=args.file, ports=ports,
                 primary_profiles=sum(len(r[trials_key]) for r in results), results=[
                     dict(payload_sha256=r["payload_sha256"], source_sha256=r["source_sha256"], file=r["file"],
                          load=r["load"], entry=r["entry"], native_launch=r["native_launch"],
                          transport=r["transport"].get("origin"), scope=r.get("scope"), selected_primary=r["selected_primary"],
                          paired_primary=r.get("paired_primary"), primary_profiles=r[trials_key],
                          action_comparison=(r.get("action_comparison") or {}).get("status"),
                          identical_object_samples=(r.get("action_comparison") or {}).get("exact_object_samples_match"),
                          native_exit=r.get("native_exit"), primary_exit=r.get("primary_exit"),
                          error=r.get("error"), result=f'{out.name}/{r["payload_sha256"]}/result.json')
                     for r in results
                 ])
    if args.profile == "MX":
        index["native_only"] = native_only(inventory["files"], directory)
    save(out/"index.json", index)
    if index["complete"]:
        save(base/(profile + "-index.json"), index)
    print(json.dumps(statuses, ensure_ascii=False), flush=True)
    return int(len(results) != counts or any("error" in result for result in results))


if __name__ == "__main__":
    sys.exit(main())
