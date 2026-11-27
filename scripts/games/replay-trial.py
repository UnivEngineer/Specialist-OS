"""Повторить точный сценарий сохранённой пробы на закреплённых образах."""
import argparse
import json
from pathlib import Path
import runpy
import shutil
import tempfile
import uuid

from campaign_media import ROOT, sha, trial_disk

runner = runpy.run_path(str(Path(__file__).with_name("run-campaign.py")))
from game_inventory import build_route
from mcp_pool import WorkerMachine


def recovery_checks(args, worker, files, profile, marker, evidence, retry_command):
    outcomes = {}
    with tempfile.TemporaryDirectory(prefix="mxos-recover-") as tmp:
        destination = Path(tmp)
        slot = worker.slot
        for name in ("floppy720-test.img","system-flash.bin","flash-disk.bin"):
            shutil.copyfile(slot.media_dir/name,destination/name)
        if retry_command:
            folder = evidence/"type_again"
            folder.mkdir()
            obs = dict(phase="type_again",pid=slot.pid,hwnd=slot.hwnd)
            g = runner["Guest"](slot,folder,args.build,profile,files,obs)
            try:
                outcomes["type_again"] = g.read_check(marker)
                obs["control_file"] = outcomes["type_again"]
            except RuntimeError as error:
                obs["obstacle"] = str(error)
            runner["write"](folder/"observation.json",obs)
        for phase in ("second_reset","cold_same_media"):
            folder = evidence/phase
            folder.mkdir()
            if phase=="cold_same_media":
                slot = worker.start(disk=destination/"flash-disk.bin",system=destination/"system-flash.bin",
                                    floppy=destination/"floppy720-test.img")
            obs = dict(phase=phase,pid=slot.pid,hwnd=slot.hwnd,boot_on_lease=slot.cold_boots+1,
                       initial_flash_protection=slot.protection)
            g = runner["Guest"](slot,folder,args.build,profile,files,obs)
            shell = g.shell(reset=True)
            if shell:
                g.recovery(marker)
            outcomes[phase] = bool(obs.get("control_file"))
            obs["shell_after_diagnostic_reset"] = shell
            runner["write"](folder/"observation.json",obs)
            if outcomes[phase]:
                break
    return outcomes


def fixture_for(source,old):
    if not old["context"].get("fixture"):
        raise ValueError("У ранней попытки отсутствует ID закреплённых образов")
    archives = [(p/"fixtures"/old["context"]["fixture"]) for p in source.parents]
    fixture = next((p for p in archives if (p/"C.bin").is_file()),None)
    if not fixture:
        raise ValueError("Не найдена сохранённая копия образов и символов из этой пробы")
    return fixture


def replay(args,source,worker,records):
    old = json.loads(source.read_bytes())
    fixture = fixture_for(source,old)
    args.system,args.games,args.floppy,args.build = fixture/"C.bin",fixture/"H.bin",fixture/"A.img",fixture/"build"
    rec = next((r for r in records if r["file"]==old["file"]),None)
    if not rec or (rec["sha256"],rec["payload_sha256"])!=(old["source_sha256"],old["payload_sha256"]):
        raise ValueError("Исходная нагрузка отличается от записи")
    # Карточку берём из самой попытки. Последующие поправки справочника
    # не должны менять нажатия, используемые при проверке регрессии.
    job = next((j for j in runner["jobs"]([rec],[]) if j["id"]==old["id"]),None)
    if not job or job["route"]["sha256"]!=old["container_sha256"]:
        raise ValueError("Не найден исходный маршрут RUN")
    job.update(card=old.get("scenario"),aliases=old["aliases"])
    packed = build_route(rec,job["route"])
    disk,marker = trial_disk(packed,job["route"]["kind"],args.games)
    if sha(disk)!=old["game_disk_sha256"]:
        raise ValueError("Не совпадает содержимое испытательного H:")
    files,ctx = runner["build_context"](args)
    for name,value in old["context"].items():
        if name!="harness" and ctx.get(name)!=value:
            raise ValueError("Отличие входных данных стенда: " + name)
    rows = []
    for repeat in range(args.repeat):
        key,status = runner["trial"](args,job,files,ctx,worker)
        output = args.out/"trials"/key/"result.json"
        print(f"{status} {output}",flush=True)
        if status!="completed":
            rows.append(dict(outcome=status))
            continue
        state = json.loads(output.read_bytes())
        evidence = output.parent/state["evidence"]
        comparisons = {}
        for name,initial in (("system-flash.bin",args.system.read_bytes()),("flash-disk.bin",disk),
                             ("floppy720-test.img",args.floppy.read_bytes())):
            observed = (worker.slot.media_dir/name).read_bytes()
            if observed!=initial:
                changes = [i for i,(a,b) in enumerate(zip(initial,observed)) if a!=b]
                comparisons[name] = dict(before=sha(initial),after=sha(observed),size=len(observed),
                    changed_bytes=len(changes),first_offsets=changes[:256],file="after-"+name)
                (evidence/("after-"+name)).write_bytes(observed)
        if comparisons:
            runner["write"](evidence/"media-differences.json",comparisons)
        rows.append(dict(outcome=status,control_file=state["control_file"],recovery=state["recovery"],
                         evidence=evidence.relative_to(args.out).as_posix(),media_changes=list(comparisons),
                         kernel_after=kernel_after(state)))
        if args.diagnose_recovery and not state["control_file"]:
            retry_command = bool(repeat % 2 and state.get("returned_shell"))
            rows[-1]["diagnostic"] = recovery_checks(args,worker,files,old["profile"],marker,evidence,retry_command)
    return old["id"],rows


def kernel_after(result):
    return sorted({a for phase in ("recovered","dos_after") for span in result.get("integrity",{}).get(phase,[])
                   for a in span["differences"]})


def anomalous(r):
    media = r.get("media_after",{})
    expected = {"system-flash.bin":r["context"]["system_sha256"], "flash-disk.bin":r["game_disk_sha256"],
                "floppy720-test.img":r["context"].get("floppy_seed_sha256")}
    return not r["control_file"] or bool(kernel_after(r)) or any(v!=expected[k] for k,v in media.items())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("observations",type=Path,nargs="*")
    parser.add_argument("--anomalies",type=Path,help="Повторить отказы TYPE и различия дисков или ядра после восстановления")
    parser.add_argument("--inventory",type=Path,default=ROOT/"docs/compat/game-inventory.json")
    parser.add_argument("--out",type=Path)
    parser.add_argument("--port",type=int,choices=range(19781,19797))
    parser.add_argument("--repeat",type=int,choices=range(1,17),default=1)
    parser.add_argument("--diagnose-recovery",action="store_true",help="При отказе TYPE проверить ещё один Reset и холодную загрузку с теми же байтами носителей")
    args = parser.parse_args()
    paths = [p.resolve() for p in args.observations]
    if args.anomalies:
        paths += [p.resolve() for p in (args.anomalies/"trials").glob("*/result.json") if anomalous(json.loads(p.read_bytes()))]
    if not paths:
        parser.error("Нет подходящих свидетельств")
    root = fixture_for(paths[0],json.loads(paths[0].read_bytes())).parents[1]
    args.out = (args.out or root/"replays"/uuid.uuid4().hex[:12]).resolve()
    args.force = True
    args.pool_size = 16
    records = json.loads(args.inventory.read_bytes())["files"]
    worker = WorkerMachine(16,args.port,args.out/"slots")
    results = {}
    try:
        for source in dict.fromkeys(paths):
            key,rows = replay(args,source,worker,records)
            results[key] = rows
            runner["write"](args.out/"summary.json",results)
    finally:
        worker.close()
    return int(any(row["outcome"]!="completed" for rows in results.values() for row in rows))


if __name__=="__main__":
    raise SystemExit(main())
