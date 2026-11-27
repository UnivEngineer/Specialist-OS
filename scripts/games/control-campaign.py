"""Парные пробы игры через NC и RUN с одинаковыми интервалами гостевого времени."""
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
import json
import msvcrt
from pathlib import Path
from queue import Empty, Queue
import runpy
import sys
import tempfile
from threading import Lock
import traceback
import uuid

from campaign_guest import Guest, Image, Observation
from campaign_media import DISK, ROOT, SYSTEM, sha, trial_disk
import campaign_controls
from campaign_controls import PREPARE, PROBES, game_probe, object_proof, observe

CAMPAIGN = runpy.run_path(str(Path(__file__).with_name("run-campaign.py")))
sys.path[:0] = [str(ROOT / "scripts/images"), str(ROOT / "scripts/emu80")]
from game_inventory import build_route
from mcp_pool import FLASH_PROTECTION, WorkerMachine

DEFAULT = ROOT / "docs/compat/game-campaign"
ARMS = ("idle-1", "idle-2", "input-1", "input-2")
DEFAULT_INPUT = dict(start_keys=[], action_keys=["space", "right"])
# Таймер может остановить процессор только между инструкциями Intel 8080.
INSTRUCTION_TICKS = 18


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def snapshot(guest, name, *, memory=False, probe=None, timing=None):
    state = guest.call("cpu_state")
    if not state["breaked"]:
        raise RuntimeError("Снимок парной пробы требует остановленного CPU")
    frame = guest.frame(name)
    observed = dict(
        frame=frame["file"], png_sha256=sha((guest.out / frame["file"]).read_bytes()),
        rgb_sha256=frame["rgb_sha256"], size=frame["size"],
        colors=frame["colors"], ticks=state["cpu_ticks"],
        registers={key: value["dec"] for key, value in state["registers"].items()},
    )
    if memory:
        observed["mapped_ram_sha256"] = sha(guest.read(0, 0x9000))
    observed.update(observe(guest, probe, guest.out / frame["file"]))
    if timing is not None:
        observed.update(timing)
    guest.result.setdefault("checkpoints", {})[name] = observed
    return observed


def replay_start(guest, scenario):
    guest.points(*guest.return_points)
    if guest.watch_exit(guest.advance(480)) or guest.watch_exit(guest.advance(2400)):
        raise Observation("Игра вышла до заставки")
    snapshot(guest, "title")
    for index, key in enumerate(scenario["start_keys"], 1):
        if guest.watch_exit(guest.key(key)):
            raise Observation("Игра вышла при подготовке действия")
        snapshot(guest, f"start-{index}")
    if guest.watch_exit(guest.advance(800)):
        raise Observation("Игра вышла до действия")
    if guest.watch_exit(guest.advance(300)):
        raise Observation("Игра вышла в окне покоя")


def same_preparation(reference, observed):
    entry_tick = reference["entry"]["ticks"]
    actual_entry_tick = observed["entry"]["ticks"]
    for phase, frame in observed.items():
        previous = reference.get(phase, {})
        if any(previous.get(key) != value for key, value in frame.items()
               if key not in ("frame", "png_sha256", "ticks")):
            return False
        if previous["ticks"] - entry_tick != frame["ticks"] - actual_entry_tick:
            return False
    return True


def sample_arm(guest, scenario, probe, apply_input, reference):
    snapshot(guest, "entry", memory=True)
    if probe:
        if probe in PREPARE:
            PREPARE[probe](guest, snapshot, probe)
        else:
            replay_start(guest, scenario)
        plan = PROBES[probe]
        actions = [(plan["key"], plan["windows"], plan["settle"])]
    else:
        replay_start(guest, scenario)
        actions = [(key, (60, 120), 120) for key in scenario["action_keys"]]
    before = snapshot(guest, "before", memory=True, probe=probe)
    if reference and not same_preparation(reference, guest.result["checkpoints"]):
        guest.result["prestate_rejected"] = True
        return
    guest.points(*guest.return_points)

    def advance(name, ms):
        state = guest.call("emu_run_for", ms=ms)
        if not state.get("breaked"):
            raise RuntimeError("CPU продолжал работу после интервала парной пробы")
        pc = state["registers"]["pc"]["dec"]
        exited = guest.watch_exit(pc)
        frame = snapshot(guest, name, probe=probe, timing=dict(
            requested_ms=ms, stopped_by_timer=state.get("stoppedByTimer", False), exit=exited,
        ))
        frame["elapsed_ticks"] = frame["ticks"] - before["ticks"]
        return not exited and frame["stopped_by_timer"]

    for index, (key, windows, settle) in enumerate(actions, 1):
        release_windows = (settle,) if isinstance(settle, int) else settle
        guest.keys.append(dict(index=index, key=key, pressed=apply_input, hold_ms=sum(windows),
                               settle_ms=sum(release_windows)))
        if apply_input:
            guest.call("kbd_send", keys=[key], action="press")
        try:
            for step, ms in enumerate(windows, 1):
                if not advance(f"action-{index}-{step}", ms):
                    return
        finally:
            if apply_input:
                guest.call("kbd_send", keys=[key], action="release")
        for step, ms in enumerate(release_windows, 1):
            name = f"action-{index}-released" if step == 1 else f"action-{index}-released-{step}"
            if not advance(name, ms):
                return


def rgb(path):
    with Image.open(path) as source:
        return source.convert("RGB")


def differential(directory, phase, arms):
    before = rgb(directory / ARMS[0] / arms[0]["checkpoints"]["before"]["frame"])
    frames = [rgb(directory / arm / result["checkpoints"][phase]["frame"])
              for arm, result in zip(ARMS, arms, strict=True)]
    if any(frame.size != before.size for frame in frames):
        return dict(size_mismatch=True)
    # PNG сравнивается по порядку RGB, без метаданных и сжатия.
    raw_images = [frame.tobytes() for frame in (before, *frames)]
    pixels = [zip(raw[0::3], raw[1::3], raw[2::3], strict=True) for raw in raw_images]
    mask_data = bytearray()
    autonomous = baseline_disagreement = input_disagreement = 0
    for old, idle_a, idle_b, input_a, input_b in zip(*pixels, strict=True):
        autonomous += old != idle_a and idle_a == idle_b
        baseline_disagreement += idle_a != idle_b
        input_disagreement += input_a != input_b
        mask_data.append(255 if idle_a == idle_b and input_a == input_b and idle_a != input_a else 0)
    mask = Image.frombytes("L", before.size, bytes(mask_data))
    roi = mask.getbbox()
    result = dict(
        autonomous_pixels=autonomous, baseline_disagreement_pixels=baseline_disagreement,
        input_disagreement_pixels=input_disagreement, stable_delta_pixels=mask.histogram()[255],
        roi_exclusive=roi, mask_sha256=sha(mask_data),
    )
    if roi:
        mask_file = f"{phase}-mask.png"
        roi_file = f"{phase}-roi.png"
        mask.save(directory / mask_file)
        left, top, right, bottom = roi
        crop = (max(0, left - 12), max(0, top - 12), min(before.width, right + 12), min(before.height, bottom + 12))
        width, height = crop[2] - crop[0], crop[3] - crop[1]
        strip = Image.new("RGB", (width * 3, height))
        for index, frame in enumerate((before, frames[0], frames[2])):
            strip.paste(frame.crop(crop), (width * index, 0))
        strip.save(directory / roi_file)
        result.update(mask=mask_file, roi_strip=roi_file, roi_strip_order=["before", "idle-1", "input-1"],
                      roi_strip_crop_exclusive=crop)
    return result


def compare(directory, arms, probe):
    if any(arm.get("error") for arm in arms):
        return dict(status="infrastructure_error")
    if any(arm.get("obstacle") for arm in arms):
        return dict(status="stage_unconfirmed")
    if any(arm["initial_media"] != arms[0]["initial_media"] for arm in arms[1:]):
        return dict(status="prestate_mismatch", reason="initial_media")
    checkpoints = [arm["checkpoints"] for arm in arms]
    prestages = list(checkpoints[0])[:list(checkpoints[0]).index("before") + 1]
    prestage_ticks = {phase: [cp.get(phase, {}).get("ticks") for cp in checkpoints] for phase in prestages}
    relative_ticks = {
        phase: [value - entry if value is not None else None
                for value, entry in zip(ticks, prestage_ticks["entry"], strict=True)]
        for phase, ticks in prestage_ticks.items()
    }
    # Соответствие двух контролей и двух воздействий выбирается до просмотра результатов.
    matches = ((0, 2), (1, 3))
    if not all(ts[0] == ts[2] and ts[1] == ts[3] for ts in relative_ticks.values()):
        if all(ts[0] == ts[3] and ts[1] == ts[2] for ts in relative_ticks.values()):
            matches = ((0, 3), (1, 2))
    pre_match = {}
    for phase in prestages:
        reference = checkpoints[0][phase]
        pre_match[phase] = {
            field: all(run.get(phase, {}).get(field) == value for run in checkpoints[1:])
            for field, value in reference.items() if field not in ("frame", "png_sha256", "ticks")
        }
        ts = relative_ticks[phase]
        pre_match[phase]["ticks_by_pair"] = all(ts[idle] == ts[active] for idle, active in matches)
    same_prestate = all(all(fields.values()) for fields in pre_match.values())
    result = dict(status="prestate_mismatch", pre_match=pre_match, pre_ticks=prestage_ticks,
                  pre_ticks_from_entry=relative_ticks,
                  matching_arms=[[ARMS[idle], ARMS[active]] for idle, active in matches], phases={})
    if not same_prestate:
        return result
    eligible = {}
    observed_exit = {}
    timing_fields = {"ticks", "elapsed_ticks", "frame", "png_sha256"}
    for phase in dict.fromkeys(p for cp in checkpoints for p in cp if p.startswith("action-")):
        if not all(phase in cp for cp in checkpoints):
            continue
        observed = [cp[phase] for cp in checkpoints]
        elapsed = [ob["elapsed_ticks"] for ob in observed]
        spread = max(elapsed) - min(elapsed)
        exits = [ob["exit"] for ob in observed]
        if any(exits):
            observed_exit[phase] = [arm for arm, exited in zip(ARMS, exits, strict=True) if exited]
        same_budget = not any(exits) and all(ob["stopped_by_timer"] for ob in observed) and spread <= INSTRUCTION_TICKS
        view = {}
        if same_budget:
            view = differential(directory, phase, arms)
        states = [{key: value for key, value in sample.items() if key not in timing_fields} for sample in observed]
        idle_reproduced = states[0] == states[1]
        input_reproduced = states[2] == states[3]
        result["phases"][phase] = dict(
            elapsed_ticks=elapsed, tick_spread=spread, same_budget=same_budget,
            autonomous_state_reproduced=idle_reproduced,
            input_state_reproduced=input_reproduced, **view,
        )
        if (same_budget and view.get("baseline_disagreement_pixels") == 0 and view["input_disagreement_pixels"] == 0
                and idle_reproduced and input_reproduced):
            eligible[phase] = view
    result["observed_exit"] = observed_exit
    if not eligible:
        result["status"] = "exit_observed" if observed_exit else "baseline_unconfirmed"
        return result
    different = [phase for phase, view in eligible.items() if view["stable_delta_pixels"]]
    result["visual_phases"] = different
    result["status"] = "repeatable_visual_effect" if different else "no_repeatable_visual_effect"
    if observed_exit:
        result["status"] = "exit_observed"
    proof = object_proof(probe, eligible, checkpoints)
    if proof:
        result.update(status="object_verified", object_proof=proof)
    return result


def run_arm(args, worker_slot, job, primary, files, scenario, probe, packed, seed, target, arm, reference=None):
    report = dict(arm=arm, payload_sha256=job["record"]["payload_sha256"])
    try:
        slot = worker_slot.start(disk=seed, system=args.system, floppy=args.floppy)
        emulator = sha((slot.runtime / "Emu80qt-mcp.exe").read_bytes())
        report.update(pid=slot.pid, port=slot.port, platform=slot.platform,
                      emulator_sha256=emulator, boot_on_lease=slot.cold_boots + 1,
                      reused_window=slot.cold_boots > 0,
                      slot_log=slot.artifacts.relative_to(args.out).as_posix(),
                      media_dir=slot.media_dir.relative_to(slot.runtime).as_posix())
        report["initial_flash_protection"] = slot.protection
        if report["initial_flash_protection"] != FLASH_PROTECTION:
            raise RuntimeError("Защита C: и H: отличается от штатного запуска Emu80")
        if emulator != primary.get("emulator_sha256"):
            raise RuntimeError("Сборка Emu80 отличается от первичной пробы")
        report["initial_media"] = {name: sha((slot.media_dir / name).read_bytes()) for name in (
            "system-flash.bin", "flash-disk.bin", "floppy720-test.img",
        )}
        if (report["initial_media"]["system-flash.bin"] != primary["context"]["system_sha256"]
                or report["initial_media"]["flash-disk.bin"] != primary["game_disk_sha256"]
                or report["initial_media"]["floppy720-test.img"] != sha(args.floppy.read_bytes())):
            raise RuntimeError("Хеши начальных A:/C:/H: отличаются от подготовленных носителей")
        guest = Guest(slot, target, args.build, job["profile"], files, report)
        try:
            guest.launch(job, packed)
            sample_arm(guest, scenario, probe, arm.startswith("input-"), reference)
        except Observation as reason:
            report["obstacle"] = str(reason)
            report["obstacle_registers"] = guest.regs()
            guest.frame("obstacle")
        report["hwnd"] = slot.hwnd
    except Exception as error:
        report.update(error=str(error), traceback=traceback.format_exc())
    CAMPAIGN["write"](target / "observation.json", report)
    return report


def repeat_preparation(args, worker_slot, job, primary, files, scenario, probe, packed, seed, target, arm, reference):
    rejected = []
    for attempt in range(4):
        report = run_arm(args, worker_slot, job, primary, files, scenario, probe, packed, seed, target, arm, reference)
        if not report.get("prestate_rejected") or attempt == 3:
            break
        original = target.with_name(f"{target.name}-prestate-{attempt + 1}")
        target.rename(original)
        target.mkdir()
        rejected.append(original.name)
    if rejected:
        report["rejected_preparations"] = rejected
    return report


def verified_flash_protection(result, expected):
    arms = result["arms"]
    return len(arms) == len(ARMS) and all(arm.get("initial_flash_protection") == expected for arm in arms)


def same_observation_inputs(old, recipe):
    identity = ("profile", "container_sha256", "payload_sha256", "game_disk_sha256", "trial_kind")
    if any(old.get(name) != recipe[name] for name in identity):
        return False
    probe = recipe["trial_kind"]
    if probe == "card_replay":
        if any(old["scenario"].get(name) != keys for name, keys in recipe["scenario_keys"].items()):
            return False
    elif probe not in PREPARE and old["scenario"]["start_keys"] != recipe["scenario_keys"]["start_keys"]:
        return False
    if probe != "card_replay":
        current_plan = PROBES[probe]
        if json.dumps(old.get("probe_plan"), sort_keys=True) != json.dumps(current_plan, sort_keys=True):
            return False
    new, previous = recipe["context"], old["context"]
    checks = ("system_sha256", "profile_sha256", "disk_seed_sha256", "kernels", "floppy_seed_sha256",
              "flash_protection", "runtime_scripts")
    if any(previous[name] != new[name] for name in checks if name in previous):
        return False
    if any(new["build_inputs"].get(name) != digest for name, digest in previous["build_inputs"].items()):
        return False
    if not verified_flash_protection(old, new["flash_protection"]):
        return False
    return all(
        arm.get("emulator_sha256") == recipe["emulator_sha256"]
        and arm.get("initial_media", {}).get("floppy720-test.img") == new["floppy_seed_sha256"]
        for arm in old["arms"]
    )


def primary_selection(args, job, context):
    source = args.campaign / "trials" / job["id"] / "result.json"
    if not source.exists():
        return None, "primary_pending"
    primary = load(source)
    if primary["outcome"] != "completed":
        return primary, "primary_" + primary["outcome"]
    protection = primary.get("initial_flash_protection")
    reused = primary.get("boot_on_lease", 1) > 1 or primary.get("platform") == "specialist-os-cold"
    if reused and protection is None:
        return primary, "primary_flash_protection_unconfirmed"
    if protection is not None and protection != context["flash_protection"]:
        return primary, "primary_flash_protection_mismatch"
    if primary["launch"] != "entry_and_payload_verified":
        return primary, "launch_" + primary["launch"]
    if (primary["profile"] != job["profile"] or primary["container_sha256"] != job["route"]["sha256"]
            or primary["payload_sha256"] != job["record"]["payload_sha256"]):
        return primary, "route_mismatch"
    seed = primary["context"]
    keys = ("system_sha256", "disk_seed_sha256", "kernels", "floppy_seed_sha256", "profile_sha256",
            "flash_protection", "runtime_scripts")
    mismatch = [key for key in keys if key in seed and seed[key] != context[key]]
    mismatch.extend(name for name, digest in seed["build_inputs"].items()
                    if context["build_inputs"].get(name) != digest)
    if primary["emulator_sha256"] != context["emulator_sha256"]:
        mismatch.append("emulator_sha256")
    if mismatch:
        return primary, "fixture_mismatch:" + ",".join(mismatch)
    scenario = current_scenario(args, job, primary)
    if not args.without_card and not game_probe(job, scenario):
        if not job["card"] and not primary.get("scenario"):
            return primary, "scenario_not_selected"
        if not scenario["action_keys"]:
            return primary, "scenario_action_missing"
    return primary, None


def current_scenario(args, job, primary):
    scenario = job["card"] or primary.get("scenario") or DEFAULT_INPUT
    if args.without_card and not scenario["action_keys"]:
        return dict(scenario, action_keys=DEFAULT_INPUT["action_keys"], default_action=True)
    return scenario


def build_index(args, jobs, context):
    cases = []
    for job in jobs:
        key = job["id"]
        primary, obstacle = primary_selection(args, job, context)
        item = dict(id=key, file=job["record"]["file"], profile=job["profile"])
        if primary:
            item["primary"] = f"trials/{key}/result.json"
            item["primary_evidence"] = primary["evidence"]
            item["primary_version"] = primary["version"]
        if obstacle:
            deferred = obstacle.startswith(("primary_", "fixture_mismatch", "route_mismatch"))
            item.update(selection="deferred" if deferred else "excluded", status=obstacle)
            if primary and primary.get("obstacle"):
                item["primary_obstacle"] = primary["obstacle"]
        else:
            index_result = args.out / key / "result.json"
            item.update(selection="applicable", status="paired_pending")
            if index_result.exists():
                paired = load(index_result)
                scenario = current_scenario(args, job, primary)
                probe = game_probe(job, scenario)
                recipe = dict(context=context, profile=job["profile"], container_sha256=job["route"]["sha256"],
                              payload_sha256=job["record"]["payload_sha256"], game_disk_sha256=primary["game_disk_sha256"],
                              scenario_keys={name: scenario[name] for name in ("start_keys", "action_keys")},
                              emulator_sha256=primary["emulator_sha256"], trial_kind=probe or "card_replay")
                exact = same_observation_inputs(paired, recipe)
                item.update(status=paired["status"] if exact else "paired_requires_review",
                            paired=f"{key}/result.json", paired_evidence=f"{key}/{paired['evidence']}",
                            paired_version=paired["version"], paired_primary_version=paired["primary_version"],
                            trial_kind=paired["trial_kind"])
                item["flash_protection_verified"] = verified_flash_protection(paired, context["flash_protection"])
                if not item["flash_protection_verified"]:
                    item["replay_reason"] = "flash_protection_unconfirmed"
                archive = index_result.parent / paired["evidence"] / "result.json"
                if archive.exists():
                    item["paired_archive"] = archive.relative_to(args.out).as_posix()
        cases.append(item)
    return dict(schema=1, generated_utc=datetime.now(timezone.utc).isoformat(), fixture=context["fixture"],
                flash_protection=context["flash_protection"], runtime_scripts=context["runtime_scripts"],
                counts=dict(Counter(item["status"] for item in cases)), cases=cases)


def trial(args, worker_slot, job, primary, files, context):
    key = job["id"]
    output = args.out / key
    output.mkdir(parents=True, exist_ok=True)
    with (output / "work.lock").open("a+b") as lock:
        lock.seek(0)
        try:
            msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
        except OSError:
            return key, "busy"
        source = job["record"]
        packed = build_route(source, job["route"])
        disk, _ = trial_disk(packed, job["route"]["kind"], args.games)
        scenario = current_scenario(args, job, primary)
        scenario_source = ("controls.json" if job["card"] else
                           "primary_result" if primary.get("scenario") else "default_input")
        if scenario.get("default_action"):
            scenario_source = "card_with_default_action"
        probe = game_probe(job, scenario)
        if probe and scenario_source == "default_input":
            scenario_source = "payload_probe"
        seed_context = {name: value for name, value in context.items() if name not in ("harness", "fixture")}
        recipe = dict(id=key, profile=job["profile"], context=seed_context, container_sha256=sha(packed),
                      payload_sha256=source["payload_sha256"], game_disk_sha256=sha(disk),
                      scenario_keys={name: scenario[name] for name in ("start_keys", "action_keys")},
                      harness_sha256=args.harness, emulator_sha256=primary["emulator_sha256"],
                      trial_kind=probe or "card_replay", arm_order=ARMS)
        result = dict(**recipe, file=source["file"], fixture=context["fixture"], primary_version=primary["version"], scenario=scenario,
                      scenario_source=scenario_source, load=source["load"], entry=source["entry"],
                      payload_bytes=source["payload_bytes"])
        if probe:
            result["probe_plan"] = PROBES[probe]
        if primary["game_disk_sha256"] != result["game_disk_sha256"]:
            raise ValueError(f"Маршрут первичной пробы изменился {key}")
        version = sha(json.dumps(recipe, ensure_ascii=False, sort_keys=True).encode())
        published = output / "result.json"
        check_preparation = probe == "exolon_player"
        if published.exists():
            old = load(published)
            archived = output / old["evidence"] / "result.json"
            if not archived.exists():
                CAMPAIGN["write"](archived, old)
            if (not args.force and old.get("status") != "infrastructure_error"
                    and (old["version"] == version or args.missing)
                    and same_observation_inputs(old, recipe)):
                return key, "cached"
            check_preparation |= old.get("status") == "prestate_mismatch"
        attempt = output / ("attempt-" + uuid.uuid4().hex[:12])
        attempt.mkdir()
        result.update(version=version, evidence=attempt.name, started_utc=datetime.now(timezone.utc).isoformat())
        arms = []
        with tempfile.TemporaryDirectory(prefix="mxos-control-") as temp:
            seed = Path(temp) / "H.bin"
            seed.write_bytes(disk)
            for arm in ARMS:
                target = attempt / arm
                target.mkdir()
                # Повторная подготовка допускает ввод только после совпадения с первым контролем.
                first = arms[0].get("checkpoints", {}) if arms else {}
                reference = first if check_preparation and "before" in first else None
                report = repeat_preparation(args, worker_slot, job, primary, files, scenario, probe,
                                            packed, seed, target, arm, reference)
                if report.get("error"):
                    worker_slot.close()
                    failed = target.with_name(arm + "-retry-source")
                    target.rename(failed)
                    target.mkdir()
                    report = run_arm(args, worker_slot, job, primary, files, scenario, probe, packed, seed, target,
                                     arm, reference)
                    report["retry_source"] = failed.name
                arms.append(report)
                if report.get("error"):
                    worker_slot.close()
                    break
        result.update(compare(attempt, arms, probe), arms=arms)
        CAMPAIGN["write"](attempt / "result.json", result)
        CAMPAIGN["write"](published, result)
        return key, result["status"]


def queue_worker(args, tasks, files, context, progress):
    worker_slot = WorkerMachine(args.pool_size, args.port, args.out / "slots")
    try:
        while True:
            try:
                job, primary = tasks.get_nowait()
            except Empty:
                return
            try:
                key, status = trial(args, worker_slot, job, primary, files, context)
            except Exception:
                traceback.print_exc()
                worker_slot.close()
                key, status = job["id"], "infrastructure_error"
            progress(key, status)
    finally:
        worker_slot.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", type=Path, default=ROOT / "docs/compat/game-inventory.json")
    parser.add_argument("--campaign", type=Path, default=DEFAULT)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--system", type=Path, default=SYSTEM)
    parser.add_argument("--games", type=Path, default=DISK)
    parser.add_argument("--floppy", type=Path, default=ROOT / "images/A-Floppy-720k-Programs.img")
    parser.add_argument("--build", type=Path, default=ROOT / "bin")
    parser.add_argument("--port", type=int, choices=range(19781, 19797))
    parser.add_argument("--pool-size", type=int, choices=range(1, 17), default=8)
    parser.add_argument("--workers", type=int, choices=range(1, 17), default=1)
    parser.add_argument("--file", action="append", default=[])
    parser.add_argument("--trial", action="append", default=[])
    parser.add_argument("--without-card", action="store_true")
    parser.add_argument("--missing", action="store_true",
                        help="Дополнить пробы с прежним кодом при совпадении маршрута, образов и клавиш")
    parser.add_argument("--index-only", action="store_true",
                        help="Сверить применимость первичных и парных результатов, сохранить index.json")
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()
    args.out = (args.out or args.campaign / "paired").resolve()
    files, context = CAMPAIGN["build_context"](args)
    args.harness = {path.name: sha(path.read_bytes()) for path in (Path(__file__), Path(campaign_controls.__file__))}
    inventory = load(args.inventory)
    if inventory["catalog_sha256"] != sha((ROOT / "assets/games/catalog.csv").read_bytes()):
        parser.error("После изменения catalog.csv запустите аудит заново")
    cards = load(ROOT / "assets/games/controls.json")["cards"]
    jobs = CAMPAIGN["jobs"](inventory["files"], cards)
    if args.index_only:
        CAMPAIGN["write"](args.out / "index.json", build_index(args, jobs, context))
        return 0
    queue = []
    for job in jobs:
        key = job["id"]
        if args.trial and key not in args.trial:
            continue
        if args.file and not any(alias == suffix or alias.endswith("/" + suffix)
                                 for alias in job["aliases"] for suffix in args.file):
            continue
        primary, obstacle = primary_selection(args, job, context)
        if not obstacle:
            queue.append((job, primary))
    workers = 1 if args.port else min(args.pool_size,args.workers)
    print(f"Парных проб {len(queue)}, работников {workers}", flush=True)
    tasks = Queue()
    for entry in queue:
        tasks.put(entry)
    completed = errors = 0
    progress_lock = Lock()

    def progress(key, status):
        nonlocal completed, errors
        with progress_lock:
            completed += 1
            errors += status == "infrastructure_error"
            print(f"{completed}/{len(queue)} {key} {status}", flush=True)

    with ThreadPoolExecutor(max_workers=workers) as pool:
        futures = [pool.submit(queue_worker, args, tasks, files, context, progress) for _ in range(workers)]
        for future in as_completed(futures):
            future.result()
    if not args.trial and not args.file:
        CAMPAIGN["write"](args.out / "index.json", build_index(args, jobs, context))
    return int(bool(errors))


if __name__ == "__main__":
    sys.exit(main())
