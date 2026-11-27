"""Возобновляемая кампания через NC и RUN в отдельных видимых Emu80."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
import json
import msvcrt
import os
from pathlib import Path
import sys
import tempfile
import threading
import traceback
import uuid

from campaign_media import ROOT, SYSTEM, DISK, read_tree, sha, trial_disk
from campaign_guest import Guest, Observation
from campaign_play import play

sys.path[:0] = [str(ROOT / "scripts/images"), str(ROOT / "scripts/emu80")]
from game_inventory import build_route
from mcp_pool import FLASH_PROTECTION, INSTALL, WorkerMachine, lease

DEFAULT = ROOT / "docs/compat/game-campaign"
PILOT = ("families/budi/originals/Budi2-std.rks",
         "families/almaz-2/originals/Almaz2-ramfos-original-broken.cpu", "std/Ochko.rks")


def write(path, obj):
    tmp = path.with_suffix("." + uuid.uuid4().hex + ".tmp")
    tmp.write_text(json.dumps(obj, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    tmp.replace(path)


def jobs(records, cards):
    queue = {}
    for r in records:
        for route in r["routes"]:
            for profile in r["profiles"]:
                key = f'{route["id"]}-{profile}'
                path = r["file"].removeprefix("assets/games/")
                card = next((c for c in cards if path in c["files"] and profile in c["profiles"]), None)
                job = queue.setdefault(key, dict(id=key, record=r, route=route, profile=profile,
                                                  card=card, aliases=[]))
                if not job["card"] and card:
                    job.update(record=r, card=card)
                job["aliases"].append(path)
    return list(queue.values())


def build_context(args):
    seed = args.system.read_bytes()
    files = {k.upper():v for k,v in read_tree(seed)[0].items()}
    build = args.build
    binaries = {"DOS/BIOS.SYS":"BIOS.sys", "DOS/RAMFOS.MON":"RAMFOS.mon", "DOS/MONITOR2.MON":"Monitor2.mon"}
    for filename, local in binaries.items():
        if files[filename] != (build/local).read_bytes():
            raise ValueError(f"Системный образ и каталог сборки расходятся: {filename}")
    base = getattr(args, "games", DISK)
    floppy = getattr(args, "floppy", ROOT/"images/A-Floppy-720k-Programs.img")
    template = Path(os.environ.get("SPECIALIST_EMU80_POOL_TEMPLATE", INSTALL/"pool-template"))
    inputs = ("NC.i80","NC_CORE.i80","CLI.i80","RUN.i80","RUN.sym","NC.sym","NC_CORE.sym","CLI.sym","BIOS.sym","RAMFOS.sym","banked/KERNEL.sym")
    bundle = {"C.bin":seed,"H.bin":base.read_bytes(),"A.img":floppy.read_bytes()}
    for name in [*inputs,*binaries.values()]:
        bundle["build/"+name] = (build/name).read_bytes()
    ctx = dict(system_sha256=sha(seed), disk_seed_sha256=sha(bundle["H.bin"]), floppy_seed_sha256=sha(bundle["A.img"]),
               emulator_sha256=sha((template/"Emu80qt-mcp.exe").read_bytes()),
               profile_sha256=sha((ROOT/"scripts/emu80/specialist-mx2.conf").read_bytes()),
               flash_protection=FLASH_PROTECTION, runtime_scripts={name:sha((ROOT/"scripts/emu80"/name).read_bytes())
                   for name in ("mcp_pool.py","run.ps1")},
               kernels={k:sha(files[k]) for k in binaries},
               build_inputs={f:sha(bundle["build/"+f]) for f in inputs},
               harness={p.name:sha(p.read_bytes()) for p in (
                   Path(__file__), Path(__file__).with_name("campaign_guest.py"),
                   Path(__file__).with_name("campaign_play.py"), Path(__file__).with_name("campaign_media.py"))})
    fixture_id = sha(json.dumps({k:sha(v) for k,v in bundle.items()},sort_keys=True).encode())
    fixture = args.out.resolve()/"fixtures"/fixture_id
    for rel,data in bundle.items():
        path = fixture/rel
        if path.is_file() and path.read_bytes()==data:
            continue
        path.parent.mkdir(parents=True,exist_ok=True)
        tmp = path.with_suffix("."+uuid.uuid4().hex+".tmp")
        tmp.write_bytes(data)
        tmp.replace(path)
    ctx["fixture"] = fixture_id
    args.system,args.games,args.floppy,args.build = fixture/"C.bin",fixture/"H.bin",fixture/"A.img",fixture/"build"
    return files, ctx


def trial(args, job, system_files, context, worker):
    key = job["id"]
    directory = args.out / "trials" / key
    directory.mkdir(parents=True,exist_ok=True)
    with (directory/"work.lock").open("a+b") as lock:
        lock.seek(0)
        try:
            msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
        except OSError:
            return key, "busy"
        result_path = directory/"result.json"
        record, profile, route = job["record"], job["profile"], job["route"]
        scenario = job["card"]
        keys = {k:scenario[k] for k in ("start_keys","action_keys","exit_keys")} if scenario else None
        recipe = dict(profile=profile, route=route["id"], payload=record["payload_sha256"],
                      start=record["load"], entry=record["entry"], keys=keys, context=context)
        version = sha(json.dumps(recipe, sort_keys=True, ensure_ascii=False).encode())
        if result_path.exists() and not args.force:
            old = json.loads(result_path.read_text("utf-8"))
            if old["version"] == version and old["outcome"] != "infrastructure_error":
                return key, "cached"
        packed = build_route(record,route)
        disk,marker = trial_disk(packed,route["kind"],args.games)
        attempts = []
        for attempt in range(2):
            evidence = directory / ("attempt-" + uuid.uuid4().hex[:12])
            evidence.mkdir()
            result = dict(id=key, version=version, file=record["file"], aliases=job["aliases"],
                          profile=profile, container_sha256=route["sha256"], payload_sha256=record["payload_sha256"],
                          source_sha256=record["sha256"], recipe=route["recipe"], game_disk_sha256=sha(disk),
                          started_utc=datetime.now(timezone.utc).isoformat(), evidence=evidence.name, context=context,
                          scenario=job["card"], launch="unconfirmed", playable="unconfirmed", input="unconfirmed",
                          graphics="needs_review", palette="needs_review", sound_quality="needs_listening",
                          control_file=False, exit="unobserved", outcome="completed")
            try:
                with tempfile.TemporaryDirectory(prefix="mxos-game-") as tmp:
                    seed = Path(tmp)/"H.bin"
                    seed.write_bytes(disk)
                    slot = worker.start(disk=seed, system=args.system, floppy=args.floppy)
                    result.update(port=slot.port,pid=slot.pid,hwnd=slot.hwnd,boot_on_lease=slot.cold_boots+1,
                                  platform=slot.platform,initial_flash_protection=slot.protection,
                                  slot_log=slot.artifacts.relative_to(args.out).as_posix(),
                                  emulator_sha256=sha((slot.runtime/"Emu80qt-mcp.exe").read_bytes()))
                    write(evidence/"session.json",{k:result[k] for k in ("pid","hwnd","port","slot_log","boot_on_lease")})
                    if result["emulator_sha256"] != context["emulator_sha256"]:
                        raise RuntimeError("Сборка MCP изменилась после постановки пробы в очередь")
                    guest = Guest(slot, evidence, args.build, profile, system_files, result)
                    try:
                        guest.launch(job, packed)
                        play(guest, record, job["card"])
                    except Observation as reason:
                        result["obstacle"] = str(reason)
                        guest.frame("obstacle")
                    if "integrity" in result and "playing" not in result["integrity"] and not guest.exit_seen:
                        guest.integrity("last_observation")
                    guest.recovery(marker)
                    result["media_after"] = {f:sha((slot.media_dir/f).read_bytes()) for f in
                        ("system-flash.bin", "flash-disk.bin", "floppy720-test.img")}
            except Exception as error:
                worker.close()
                result.update(outcome="infrastructure_error", error=str(error), traceback=traceback.format_exc())
            write(evidence/"observation.json", result)
            attempts.append(dict(evidence=evidence.name, outcome=result["outcome"], error=result.get("error")))
            if result["outcome"] != "infrastructure_error":
                break
        result["attempts"] = attempts
        write(result_path, result)
        return key, result["outcome"]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--inventory", type=Path, default=ROOT/"docs/compat/game-inventory.json")
    p.add_argument("--out", type=Path, default=DEFAULT)
    p.add_argument("--system", type=Path, default=SYSTEM)
    p.add_argument("--build", type=Path, default=ROOT/"bin")
    p.add_argument("--games", type=Path, default=DISK)
    p.add_argument("--floppy", type=Path, default=ROOT/"images/A-Floppy-720k-Programs.img")
    p.add_argument("--pilot", action="store_true")
    p.add_argument("--file", action="append", default=[], help="Точный путь из CSV или его окончание")
    p.add_argument("--profile", choices=("MON","RF","STD","MX","NATIVE"))
    p.add_argument("--trial", help="Точный ID пары из таблицы")
    p.add_argument("--port", type=int, choices=range(19781,19797))
    p.add_argument("--pool-size", type=int, choices=range(1,17), default=8)
    p.add_argument("--workers", type=int, choices=range(1,17), help="Ограничить число занятых окон внутри пула")
    p.add_argument("--force", action="store_true", help="Повторить с сохранением предыдущих свидетельств")
    args = p.parse_args()
    args.out = args.out.resolve()
    records = json.loads(args.inventory.read_text("utf-8"))
    if records["catalog_sha256"] != sha((ROOT/"assets/games/catalog.csv").read_bytes()):
        p.error("После изменения catalog.csv запустите аудит заново")
    cards = json.loads((ROOT/"assets/games/controls.json").read_text("utf-8"))["cards"]
    queue = jobs(records["files"], cards)
    selectors = list(PILOT) if args.pilot else args.file
    if selectors:
        queue = [j for j in queue if any(s == n or n.endswith("/"+s) for n in j["aliases"] for s in selectors)]
    if args.trial:
        queue = [j for j in queue if j["id"] == args.trial]
    if args.profile:
        queue = [j for j in queue if j["profile"] == args.profile]
    if not queue:
        p.error("Нет пар с такими условиями")
    files, context = build_context(args)
    workers = 1 if args.port else min(args.pool_size,args.workers or args.pool_size)
    print(f"Очередь: {len(queue)} пар, работников {workers}", flush=True)
    errors = 0
    state = threading.local()
    machines = []
    def run(job):
        if not getattr(state,"machine",None):
            state.machine = WorkerMachine(args.pool_size,args.port,args.out/"slots")
            machines.append(state.machine)
        return trial(args,job,files,context,state.machine)
    pool = ThreadPoolExecutor(max_workers=workers)
    try:
        futures = {pool.submit(run,j):j["id"] for j in queue}
        for idx,f in enumerate(as_completed(futures), 1):
            try:
                name, status = f.result()
            except Exception as error:
                name, status = futures[f], "infrastructure_error"
                print(f"{name}: {error}", file=sys.stderr, flush=True)
            print(f"{idx}/{len(queue)} {name} {status}", flush=True)
            errors += status == "infrastructure_error"
    finally:
        pool.shutdown(wait=True,cancel_futures=True)
        for machine in machines:
            machine.close()
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
