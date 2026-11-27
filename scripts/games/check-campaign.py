"""Сверить отчёт кампании с каталогом, фактическими нагрузками, кадрами, WAV и выводом TYPE."""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import uuid
import wave
import zipfile

from campaign_media import ROOT,sha

sys.path[:0] = [str(ROOT/"scripts/images"),str(ROOT/".local/game-campaign-python")]
from PIL import Image
from game_inventory import build_route, payload


def archive_runtime(out, expected):
    install = Path(os.environ["LOCALAPPDATA"])/"Specialist-OS/emu80/pool-template"
    template = Path(os.environ.get("SPECIALIST_EMU80_POOL_TEMPLATE",install))
    files = [template/name for name in ("Emu80qt-mcp.exe","emu80.conf","qt.conf","dbgfont.bin",
                                       "help.css","COPYING.txt","specialist-build.json")]
    files += sorted(template.glob("Qt*.dll")) + sorted(p for p in (template/"_qtplugins").rglob("*") if p.is_file())
    if {sha(files[0].read_bytes())}!=expected:
        raise ValueError("Шаблон MCP отличается от фактически использованной сборки")
    folder = out/"fixtures"
    folder.mkdir(exist_ok=True)
    target = folder/("emu80-"+next(iter(expected))+".zip")
    tmp = target.with_suffix("."+uuid.uuid4().hex+".tmp")
    with zipfile.ZipFile(tmp,"w",compression=zipfile.ZIP_DEFLATED) as archive:
        for path in files:
            archive.write(path,path.relative_to(template).as_posix())
    with zipfile.ZipFile(tmp) as archive:
        if archive.testzip() is not None:
            raise ValueError("Не прошла проверка архива MCP")
    tmp.replace(target)
    return dict(file=target.relative_to(out).as_posix(),sha256=sha(target.read_bytes()))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--out",type=Path,default=ROOT/"docs/compat/game-campaign")
    p.add_argument("--inventory",type=Path,default=ROOT/"docs/compat/game-inventory.json")
    p.add_argument("--partial",action="store_true",help="Проверить только запущенную часть очереди")
    p.add_argument("--require-cold",action="store_true",help="Проверить запись HWND и стартовую защиту каждой машины")
    p.add_argument("--archive-runtime",action="store_true",help="Сохранить точный шаблон MCP с DLL Qt для повтора")
    args = p.parse_args()
    out = args.out.resolve()
    inv = json.loads(args.inventory.read_bytes())
    report = json.loads((out/"results.json").read_bytes())
    expected = set()
    counts = Counter()
    for rec,listed in zip(inv["files"],report["catalog"],strict=True):
        name = rec["file"].removeprefix("assets/games/")
        assert name==listed["file"] and sha((ROOT/rec["file"]).read_bytes())==listed["source_sha256"],name
        assert sha(payload(rec))==listed["payload_sha256"],name
        keys = set()
        for route in rec["routes"]:
            assert sha(build_route(rec,route))==route["sha256"],name
            keys.update(route["id"]+"-"+profile for profile in rec["profiles"])
        assert keys==set(listed["trials"]),name
        expected.update(keys)
        counts["catalog_rows"] += 1
    if not args.partial:
        assert expected==set(report["trials"]),expected.symmetric_difference(report["trials"])
    assert not set(report["trials"]).difference(expected)
    for key,t in report["trials"].items():
        source = out/t["json"]
        result = json.loads(source.read_bytes())
        current = json.loads((out/t["current"]).read_bytes())
        assert result["id"]==key and current["evidence"]==source.parent.name and result["version"]==current["version"],key
        assert result["context"]["system_sha256"]==t["system_sha256"] and result["container_sha256"]==t["container_sha256"],key
        if args.require_cold:
            assert result.get("hwnd") and result.get("boot_on_lease") and result.get("initial_flash_protection")=={
                "system-flash.bin.sdp":"01","flash-disk.bin.sdp":None},key
        assert result.get("control_file")==t["control_file"],key
        if result["control_file"]:
            assert result["read_back"]=="READ-"+result["container_sha256"][:16].upper(),key
            counts["exact_markers"] += 1
        for frame in result.get("frames",{}).values():
            with Image.open(source.parent/frame["file"]) as im:
                rgb = im.convert("RGB")
                assert list(rgb.size)==frame["size"] and sha(rgb.tobytes())==frame["rgb_sha256"],source
                assert len(rgb.getcolors(rgb.width*rgb.height))==frame["colors"],source
            counts["rgb_frames"] += 1
        for name,item in result.get("audio",{}).items():
            with wave.open(str(source.parent/item["file"]),"rb") as wav:
                assert item["speed"]==1 and wav.getnchannels()==1 and wav.getsampwidth()==2,source
                assert wav.getnframes()==item["samples"]==t["audio"][name]["samples"] and wav.getframerate()==item["rate"],source
            counts["pcm_clips"] += 1
        if t.get("audio_action"):
            protocol = out/t["audio_action"]["protocol"]
            probe = json.loads(protocol.read_bytes())
            assert probe["id"]==key and probe["primary_version"]==result["version"],protocol
            assert [a["arm"] for a in probe["arms"]]==["idle-1","idle-2","input-1","input-2"],protocol
            for i,arm in enumerate(probe["arms"]):
                details = json.loads((protocol.parent/arm["json"]).read_bytes())
                assert details["initial_flash_protection"]==result["initial_flash_protection"],protocol
                for phase,item in arm["audio"].items():
                    path = protocol.parent/arm["arm"]/item["file"]
                    comparison = probe["comparison"][phase]
                    assert sha(path.read_bytes())==item["sha256"],path
                    with wave.open(str(path),"rb") as wav:
                        assert item["speed"]==1 and wav.getnframes()==item["samples"] and wav.getframerate()==item["rate"],path
                        assert sha(wav.readframes(comparison["common_samples"]))==comparison["pcm_sha256"][i],path
                    counts["action_pcm_clips"] += 1
            counts["action_audio_trials"] += 1
        counts["completed_pairs"] += 1
    for path in [out/"RESULTS.md",out/"catalog.md",out/"historical.md",out/"profiles.md",out/"unknown-controls.md",*out.glob("cases/*.md"),*out.glob("catalog/*.md")]:
        for rel in re.findall(r"\]\(([^)]+)\)",path.read_text("utf-8")):
            if rel.startswith(("http:","https:","#")):
                continue
            target = path.parent/rel.strip("<>").split("#",1)[0]
            if not target.exists():
                raise ValueError(f"Недоступная ссылка в {path}: {rel}")
            counts["markdown_links"] += 1
    output = dict(counts,inventory_sha256=hashlib.sha256(args.inventory.read_bytes()).hexdigest(),
                  report_sha256=hashlib.sha256((out/"results.json").read_bytes()).hexdigest())
    if args.archive_runtime:
        output["runtime"] = archive_runtime(out,{t["emulator_sha256"] for t in report["trials"].values()})
    (out/"verification.json").write_text(json.dumps(output,indent=2)+"\n","utf-8")
    print(json.dumps(output,ensure_ascii=False,indent=2))


if __name__=="__main__":
    main()
