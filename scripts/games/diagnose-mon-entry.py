"""Сравнить тело игры до PUSH в переходнике Monitor-2 и в момент входа."""
import argparse
import json
from pathlib import Path
import runpy
import tempfile
import uuid

from campaign_guest import Guest, Observation
from campaign_media import ROOT, sha, trial_disk
from campaign_play import play

runner = runpy.run_path(str(Path(__file__).with_name("run-campaign.py")))
from game_inventory import build_route, payload
from mcp_pool import WorkerMachine


class MonitorEntry(Guest):
    def compare(self, phase):
        observed = self.read(self.load_addr,len(self.original))
        regs = self.regs()
        differences = [(self.load_addr+i,a,b) for i,(a,b) in enumerate(zip(self.original,observed)) if a!=b]
        self.result.setdefault("checks",{})[phase] = dict(pc=regs["pc"], sp=regs["sp"], hl=regs["hl"],
            sha256=sha(observed), differences=len(differences), first_differences=differences[:64])
        return differences

    def loader_stop(self,targets,ms):
        if self.entry not in targets or "cold_font_bindings" not in self.result:
            return super().loader_stop(targets,ms)
        rd = self.run_syms
        push = rd["RAMFOS_BRIDGE"]+rd["monAfterInitArgs"]-rd["coldStub"]-1
        def on_push():
            return self.read(push,1)==b"\xe5" and self.regs()["hl"]==rd["RAMFOS_BRIDGE"]
        stop = super().loader_stop({push:on_push,**targets},ms)
        if stop!=push:
            return stop
        self.compare("before_push")
        self.call("debug_step",mode="into")
        self.compare("after_push")
        if self.seek(self.entry,1000):
            self.compare("entry")
            self.frame("entry")
            self.result["entry_sp"] = self.regs()["sp"]
            ret = int.from_bytes(self.read(self.result["entry_sp"],2),"little")
            self.return_points={0xc000:"vector_C000",0xc800:"vector_C800",ret:"return_address"}
            self.integrity("entry")
        raise Observation("Сохранены байты нагрузки вокруг PUSH переходника и при входе в программу")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--campaign",type=Path,default=runner["DEFAULT"])
    p.add_argument("--trial")
    p.add_argument("--inventory",type=Path,default=ROOT/"docs/compat/game-inventory.json")
    p.add_argument("--port",type=int,choices=range(19781,19797))
    args = p.parse_args()
    args.out = args.campaign.resolve()
    inv = json.loads(args.inventory.read_bytes())["files"]
    worker = WorkerMachine(16,args.port,args.out/"slots")
    try:
        for job in runner["jobs"](inv,[]):
            path = args.out/"trials"/job["id"]/"result.json"
            if not path.is_file() or job["profile"]!="MON" or args.trial and args.trial!=job["id"]:
                continue
            primary = json.loads(path.read_bytes())
            if primary["launch"]!="unconfirmed" and not args.trial:
                continue
            fixture = args.out/"fixtures"/primary["context"]["fixture"]
            args.system,args.games,args.floppy,args.build = fixture/"C.bin",fixture/"H.bin",fixture/"A.img",fixture/"build"
            files,ctx = runner["build_context"](args)
            folder = args.out/"entry-checks"/job["id"]
            target = folder/("attempt-"+uuid.uuid4().hex[:12])
            target.mkdir(parents=True)
            report = dict(id=job["id"],profile="MON",payload_sha256=primary["payload_sha256"],
                          system_sha256=ctx["system_sha256"],evidence=target.name,
                          input="unconfirmed",playable="unconfirmed")
            record = job["record"]
            packed = build_route(record,job["route"])
            disk,marker = trial_disk(packed,job["route"]["kind"],args.games)
            with tempfile.TemporaryDirectory() as tmp:
                seed = Path(tmp)/"H.bin"
                seed.write_bytes(disk)
                slot = worker.start(disk=seed, system=args.system, floppy=args.floppy)
                g = MonitorEntry(slot,target,args.build,"MON",files,report)
                g.load_addr,g.entry,g.original = record["load"],record["entry"],payload(record)
                try:
                    g.launch(job,packed)
                except Observation as reason:
                    report["observation"] = str(reason)
                if "entry" in report.get("checks",{}):
                    try:
                        play(g,record,primary["scenario"])
                    except Observation as obstacle:
                        report["obstacle"] = str(obstacle)
                        g.frame("obstacle")
                g.recovery(marker)
            runner["write"](target/"result.json",report)
            runner["write"](folder/"result.json",report)
            print(job["id"],[(k,v["differences"]) for k,v in report.get("checks",{}).items()])
    finally:
        worker.close()


if __name__=="__main__":
    main()
