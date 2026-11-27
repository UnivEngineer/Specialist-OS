"""Повторить наблюдаемые стековые выходы и снять регистры на самой инструкции RET."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import runpy
import tempfile
import threading
import uuid

from campaign_guest import Guest, Observation
from campaign_media import DISK, ROOT, SYSTEM, sha, trial_disk
from campaign_play import play

runner = runpy.run_path(str(Path(__file__).with_name("run-campaign.py")))
from game_inventory import build_route
from mcp_pool import WorkerMachine


def register(state, name):
    return state["registers"][name]["dec"]


class ReturnGuest(Guest):
    target = None
    index = 0
    found = None
    expected = None

    def instruction(self, name, args, delta):
        # До последней миллисекунды идём обычным путём. Последние инструкции
        # выполняем по одной, фиксируя опкод до шага и фактический PC/SP после.
        call = super().call
        if name == "kbd_send":
            call("kbd_send", keys=args["keys"], action="press")
        try:
            ms = max(0,delta//2000-1)
            if ms:
                call("emu_run_for", ms=ms)
            state = call("cpu_state")
            for _ in range(1800):
                pc, sp = register(state,"pc"), register(state,"sp")
                if pc == self.target:
                    return
                prev = state
                step = call("debug_step", mode="into")
                state = step if "registers" in step else call("cpu_state")
                if register(state,"pc")!=self.target:
                    continue
                op = self.read(pc,1)[0]
                self.found = dict(pc=pc, opcode=op, sp_before=sp, sp_after=register(state,"sp"),
                                  target=self.target, stack_word=int.from_bytes(self.read(sp,2),"little"),
                                  ticks=state["cpu_ticks"]-prev["cpu_ticks"], instruction=prev.get("instruction"),
                                  matched=(op==0xc9 or op & 0xc7==0xc0) and sp==self.result["entry_sp"]
                                      and register(state,"sp")==((sp+2)&65535)
                                      and int.from_bytes(self.read(sp,2),"little")==self.target)
                return
        finally:
            if name == "kbd_send":
                call("kbd_send", keys=args["keys"], action="release")

    def call(self, name, **args):
        active = (self.target is not None and
                  (name in ("emu_run_for","debug_step") or name=="kbd_send" and args.get("action")=="tap"))
        if not active:
            return super().call(name,**args)
        self.index += 1
        start = super().call("cpu_state")
        if self.expected and self.expected["index"]==self.index:
            if (name,args)!=(self.expected["name"],self.expected["arguments"]):
                raise Observation("Последняя операция повтора отличается")
            with self.keep_alive():
                self.instruction(name,args,self.expected["delta_ticks"])
            raise Observation("Достигнуто окно итоговой инструкции")
        outcome = super().call(name,**args)
        end = super().call("cpu_state")
        if register(end,"pc")==self.target and register(end,"sp")==((self.result["entry_sp"]+2)&65535):
            self.found = dict(index=self.index, name=name, arguments=args, from_pc=register(start,"pc"),
                              delta_ticks=end["cpu_ticks"]-start["cpu_ticks"], to_pc=self.target)
            self.target = None
        return outcome


def replay(args, context, system_files, job, primary, worker):
    folder = args.out / "returns" / job["id"]
    folder.mkdir(parents=True,exist_ok=True)
    evidence = folder / ("attempt-"+uuid.uuid4().hex[:12])
    evidence.mkdir()
    measured = dict(id=job["id"], profile=job["profile"], source=primary["file"],
                    primary_version=primary["version"], evidence=evidence.name, context=context,
                    container_sha256=primary["container_sha256"], payload_sha256=primary["payload_sha256"], runs=[])
    if any(context.get(k)!=v for k,v in primary["context"].items() if k in
           ("system_sha256","disk_seed_sha256","floppy_seed_sha256","profile_sha256","emulator_sha256","kernels")):
        raise ValueError("Образы для повтора отличаются от первичной пробы")
    packed = build_route(job["record"],job["route"])
    disk, marker = trial_disk(packed,job["route"]["kind"],args.games)
    if sha(disk)!=primary["game_disk_sha256"]:
        raise ValueError("Пробный диск отличается от первичной пробы")
    with tempfile.TemporaryDirectory(prefix="mxos-ret-") as tmp:
        seed = Path(tmp)/"H.bin"
        seed.write_bytes(disk)
        expected = None
        for phase in ("timing","instruction"):
            target = evidence / phase
            target.mkdir()
            obs = {}
            try:
                slot = worker.start(disk=seed,system=args.system,floppy=args.floppy)
                g = ReturnGuest(slot,target,args.build,job["profile"],system_files,obs)
                g.launch(job,packed)
                g.target = obs["stack_return"]
                g.expected = expected
                try:
                    play(g, job["record"], primary["scenario"])
                except Observation as reason:
                    obs["observation"] = str(reason)
                obs["measurement"] = g.found
                g.target = None
                g.recovery(marker)
                expected = g.found
                runner["write"](target/"observation.json",obs)
                measured["runs"].append(dict(phase=phase,pid=slot.pid,hwnd=slot.hwnd,cold_boots=slot.cold_boots,
                                             control_file=obs["control_file"],measurement=g.found))
            except Exception:
                worker.close()
                raise
            if not expected:
                break
    measured["verified_ret"] = bool(expected and expected.get("matched"))
    runner["write"](evidence/"result.json",measured)
    runner["write"](folder/"result.json",measured)
    return job["id"], measured["verified_ret"]


if __name__=="__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--out",type=Path,default=runner["DEFAULT"])
    p.add_argument("--inventory",type=Path,default=ROOT/"docs/compat/game-inventory.json")
    p.add_argument("--system",type=Path,default=SYSTEM)
    p.add_argument("--build",type=Path,default=ROOT/"bin")
    p.add_argument("--games",type=Path,default=DISK)
    p.add_argument("--floppy",type=Path,default=ROOT/"images/A-Floppy-720k-Programs.img")
    p.add_argument("--pool-size",type=int,choices=range(1,17),default=8)
    p.add_argument("--workers",type=int,choices=range(1,17),default=3)
    p.add_argument("--trial")
    args = p.parse_args()
    files, context = runner["build_context"](args)
    inv = json.loads(args.inventory.read_bytes())["files"]
    cards = json.loads((ROOT/"assets/games/controls.json").read_bytes())["cards"]
    machines = []
    local = threading.local()
    def trace(job,primary):
        if not getattr(local,"worker",None):
            local.worker = WorkerMachine(args.pool_size,None,args.out/"slots")
            machines.append(local.worker)
        return replay(args,context,files,job,primary,local.worker)
    try:
        with ThreadPoolExecutor(max_workers=args.workers) as workers:
            pending = []
            for job in runner["jobs"](inv,cards):
                path = args.out/"trials"/job["id"]/"result.json"
                if (args.trial and args.trial!=job["id"]) or not path.is_file():
                    continue
                primary = json.loads(path.read_bytes())
                if primary["exit"]=="stack_return":
                    prior = args.out/"returns"/job["id"]/"result.json"
                    if prior.is_file() and not args.trial:
                        verified = json.loads(prior.read_bytes())
                        if verified.get("verified_ret") and verified["primary_version"]==primary["version"]:
                            print(job["id"],"cached",flush=True)
                            continue
                    pending.append(workers.submit(trace,job,primary))
            for task in pending:
                print(*task.result(),flush=True)
    finally:
        for machine in machines:
            machine.close()
