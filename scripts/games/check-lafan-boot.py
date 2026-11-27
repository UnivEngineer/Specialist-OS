"""Проверить оригинальный F6-DOS с пяти дискет ODI через ВГ93 и ROM RAMFOS."""
import argparse
import csv
import json
from pathlib import Path
import shutil
import sys
import tempfile
import uuid

from campaign_media import ROOT,sha
from lafan_native import ROM_SHA256,prepare_platform,switch_to_native

sys.path.insert(0,str(ROOT/"scripts/emu80"))
from mcp_pool import lease


def pc(machine):
    return machine.call("cpu_state")["registers"]["pc"]["dec"]


def run_disk(machine,image,folder):
    contents = image.read_bytes()
    result = dict(disk=image.name,disk_sha256=sha(contents),rom_sha256=ROM_SHA256,
                  keys=["enter","f6"],geometry=[80,2,5,1024],evidence=folder.name)
    with tempfile.TemporaryDirectory(prefix="mxos-native-odi-") as tmp:
        scratch = Path(tmp)/image.name
        shutil.copyfile(image,scratch)
        machine.call("disk_attach",drive="A",file=scratch.as_posix())
        machine.call("emu_reset")
        machine.call("emu_run_for",ms=4500)
        machine.screenshot(folder/"date.png")
        machine.call("kbd_send",keys=["enter"],action="tap",duration_ms=120)
        machine.call("emu_run_for",ms=800)
        machine.screenshot(folder/"ramfos.png")
        # Читаем A на инструкции XOR 1 до того, как ROM интерпретирует статус ВГ93.
        signature = bytes.fromhex("3a e8 ff ee 01 b7 3e d0 32 e8 ff c2 8e fe")
        region = machine.read(0xf800,0x7c0)
        if region.count(signature)!=1:
            raise RuntimeError("Не найден контроль статуса ВГ93 в запущенном RAMFOS")
        guard = 0xf800+region.index(signature)+3
        machine.call("bp_set",addr=guard,type="exec")
        machine.call("kbd_send",keys=["f6"],action="tap",duration_ms=150)
        if pc(machine)!=guard:
            machine.call("emu_run_for",ms=4000)
        state = machine.call("cpu_state")
        if state["registers"]["pc"]["dec"]!=guard:
            raise RuntimeError("F6 не дошла до проверки ВГ93")
        body = machine.read(0xe400,5120)
        result.update(guard_pc=guard,fdc_status=state["registers"]["af"]["dec"]>>8,
                      resident_range=[0xe400,0xf800],memory_sha256=sha(body),sectors_sha256=sha(contents[:5120]),
                      mismatching_bytes=sum(a!=b for a,b in zip(body,contents)),expected_status=1)
        machine.call("bp_clear")
        for addr in (0xfe8e,0xe400):
            machine.call("bp_set",addr=addr,type="exec")
        machine.call("emu_run_for",ms=1500)
        branch = pc(machine)
        result.update(branch_pc=branch, outcome={0xfe8e:"firmware_error",0xe400:"entered_mxdos"}.get(branch,"unresolved"))
        machine.call("bp_clear")
        machine.call("emu_run_for",ms=700)
        machine.screenshot(folder/"after-f6.png")
        machine.call("disk_detach",drive="A")
        result["final_disk_sha256"] = sha(scratch.read_bytes())
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("archive",type=Path,help="Каталог с пятью исходными ODI")
    p.add_argument("--platform",type=Path,default=ROOT/".local/emu80-pool-pristine/dist/specmx")
    p.add_argument("--out",type=Path,default=ROOT/"docs/compat/game-campaign/native-odi")
    p.add_argument("--port",type=int,choices=range(19781,19797))
    args = p.parse_args()
    with (ROOT/"assets/games/lafan-directory.csv").open(encoding="utf-8-sig",newline="") as f:
        disks = {row["Disk"]:row["DiskSHA256"] for row in csv.DictReader(f)}
    args.out.mkdir(parents=True,exist_ok=True)
    attempt = args.out/("attempt-"+uuid.uuid4().hex[:12])
    attempt.mkdir()
    results = []
    with lease(size=16,port=args.port,artifact_root=args.out/"slots") as slot:
        config = prepare_platform(slot,args.platform)
        hwnd = slot.hwnd
        for name,digest in sorted(disks.items()):
            image = args.archive/name
            if sha(image.read_bytes())!=digest:
                raise ValueError(f"Архивная дискета расходится с реестром: {name}")
            target = attempt/image.stem
            target.mkdir()
            result = run_disk(switch_to_native(slot),image,target)
            result.update(pid=slot.pid,hwnd=hwnd,emulator_sha256=sha((slot.runtime/"Emu80qt-mcp.exe").read_bytes()),
                          config_sha256=sha((config/"spmx.conf").read_bytes()),folder=target.relative_to(args.out).as_posix())
            (target/"result.json").write_text(json.dumps(result,indent=2)+"\n","utf-8")
            results.append(result)
            print(name,result["outcome"],"status",result["fdc_status"],"different",result["mismatching_bytes"],flush=True)
    (args.out/"results.json").write_text(json.dumps(results,indent=2)+"\n","utf-8")


if __name__=="__main__":
    main()
