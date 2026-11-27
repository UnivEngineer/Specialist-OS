"""Сравнить физический динамик STD и MX при тех же действиях BUDI2 и «Очка»."""
import argparse
import array
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import runpy
import shutil
import struct
import sys
import uuid
import wave

from campaign_controls import PREPARE, PROBES
from campaign_guest import Guest
from campaign_media import ROOT, sha

monitor = runpy.run_path(str(Path(__file__).with_name("check-monitor-pilot.py")))
recording = runpy.run_path(str(Path(__file__).with_name("record-action-audio.py")))
paired = recording["paired"]
sys.path.insert(0,str(ROOT/"scripts/images"))
from executable import checksum
from mcp_pool import EXE, lease


class Speaker(monitor["Native"]):
    tools = ("mx_audio","emu_speed")
    frame = Guest.frame
    speed = Guest.speed
    sample_audio = Guest.sample_audio
    key = Guest.key


def cold(slot, directory, result):
    guest = Speaker(slot,directory,result)
    guest.call("debug_break")
    guest.points()
    guest.call("kbd_reset")
    if guest.call("sys_info")["platform"]=="spec":
        guest.call("switch_platform",platform="specialist-os")
    state = guest.call("switch_platform",platform="spec")
    hwnd = json.loads((slot.artifacts/"slot.json").read_bytes())["hwnd"]
    owner = wintypes.DWORD()
    ctypes.windll.user32.GetWindowThreadProcessId(hwnd,ctypes.byref(owner))
    if (state.get("current")!="spec" or not slot.verify_owner() or owner.value!=slot.pid
            or not ctypes.windll.user32.IsWindowVisible(hwnd)):
        raise RuntimeError("Не подтверждена родная STD в своём окне")
    guest.call("debug_break")
    guest.speed(32)
    guest.call("emu_reset")
    guest.advance(1500)
    guest.key("f1")
    guest.key("enter")
    guest.advance(500)
    return guest


def load(guest, path):
    image = path.read_bytes()
    start,end = struct.unpack_from("<HH",image)
    body = image[4:-2]
    assert end-start+1==len(body) and int.from_bytes(image[-2:],"little")==checksum(body)
    reply = guest.call("emu_load",file=path.as_posix(),autorun=True,break_after_load=True)
    if not reply["loaded"] or guest.regs()["pc"]!=start or guest.read(start,len(body))!=body:
        raise RuntimeError(f"SpecFileLoader не передал исходное тело {path.name}")
    guest.result.update(container_sha256=sha(image),payload_sha256=sha(body),entry=start)


def calibrate(slot, directory):
    # Управление ВВ55 и переключение PC5. STA/MVI/65×(DCR/JNZ)/XRI/JMP занимают
    # 1012 тактов 8080 на полупериод. Компаратор не зависит от амплитуды микшера.
    code = bytes.fromhex("3e 80 32 03 f8 3e 20 32 02 f8 06 41 05 c2 0c 20 ee 20 c3 07 20")
    image = directory/"speaker-test.rks"
    image.write_bytes(struct.pack("<HH",0x2000,0x2000+len(code)-1)+code+struct.pack("<H",checksum(code)))
    result = {}
    guest = cold(slot,directory,result)
    load(guest,image)
    guest.points()
    guest.sample_audio("calibration",300)
    path = directory/"calibration.wav"
    with wave.open(str(path)) as source:
        rate = source.getframerate()
        pcm = array.array("h",source.readframes(source.getnframes()))
    frequency = guest.call("sys_info")["cpu"]["frequency_hz"]
    mid = (min(pcm)+max(pcm))/2
    measured = sum(a<mid<=b for a,b in zip(pcm,pcm[1:]))*rate/len(pcm)
    expected = frequency/(2*1012)
    if abs(len(pcm)-rate*.3)>1 or abs(measured-expected)>expected*.015:
        raise RuntimeError(f"Проверочный меандр STD: {len(pcm)} отсчётов, {measured:.1f} Гц, ожидалось {expected:.1f}")
    return dict(cpu_hz=frequency,expected_hz=expected,measured_hz=measured,rate=rate,samples=len(pcm),
                pcm_sha256=sha(path.read_bytes()),program_sha256=sha(image.read_bytes()))


def run_game(slot, root, folder, label, image):
    key = "rks-"+sha(image.read_bytes())+"-MON"
    probe = paired["load"](root/"paired"/key/"result.json")["trial_kind"]
    plan = PROBES[probe]
    mx = paired["load"](root/"action-audio"/key/"result.json")
    result = dict(game=label,id=key,profile="spec",key=plan["key"],arms=[],
                  mx_protocol=(Path("action-audio")/key/mx["evidence"]/"result.json").as_posix(),
                  mx_comparison=mx["comparison"])
    reference = None
    for arm in paired["ARMS"]:
        directory = folder/arm
        directory.mkdir(parents=True)
        observed = dict(arm=arm,pid=slot.pid,hwnd=json.loads((slot.artifacts/"slot.json").read_bytes())["hwnd"])
        guest = cold(slot,directory,observed)
        load(guest,image)
        snap = paired["snapshot"]
        snap(guest,"entry",memory=True)
        PREPARE[probe](guest,snap,probe)
        snap(guest,"before",memory=True,probe=probe)
        observed["matching_preparation"] = reference is None or paired["same_preparation"](reference,observed["checkpoints"])
        if reference is None:
            reference = observed["checkpoints"].copy()
        guest.points()
        pressed = arm.startswith("input")
        if pressed:
            guest.call("kbd_send",keys=[plan["key"]],action="press")
        try:
            guest.sample_audio("action",sum(plan["windows"]))
        finally:
            if pressed:
                guest.call("kbd_send",keys=[plan["key"]],action="release")
        guest.sample_audio("release",480)
        snap(guest,"after",probe=probe)
        for item in observed["audio"].values():
            path = directory/item["file"]
            item.update(recording["audio_stats"](path),sha256=sha(path.read_bytes()))
            assert item["samples"] and item["speed"]==1 and not item["truncated"]
        (directory/"result.json").write_text(json.dumps(observed,ensure_ascii=False,indent=2)+"\n","utf-8")
        result["arms"].append(dict(arm=arm,json=arm+"/result.json",audio=observed["audio"],
                                   matching_preparation=observed["matching_preparation"]))
    result["comparison"] = recording["compare_clips"](folder,result["arms"])
    measures = {}
    for phase in ("action","release"):
        measures[phase] = {}
        for source,arms in (("physical_std",result["arms"]),("run_mon",mx["arms"])):
            transitions = [arm["audio"][phase]["transitions"] for arm in arms]
            measures[phase][source] = dict(idle=transitions[:2],pressed=transitions[2:],
                                            regular_tone_hz=[arm["audio"][phase]["regular_tone_hz"] for arm in arms])
    result["phase_measurements"] = measures
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--campaign",type=Path,default=ROOT/"docs/compat/game-campaign")
    parser.add_argument("--template",type=Path,default=ROOT/".local/monitor-speaker/template")
    parser.add_argument("--platform",type=Path,help="Исходный каталог dist/spec")
    parser.add_argument("--port",type=int,choices=range(19781,19797),default=19781)
    args = parser.parse_args()
    root = args.campaign.resolve()
    out = root/"native-monitor/speaker"
    attempt = out/("attempt-"+uuid.uuid4().hex[:12])
    attempt.mkdir(parents=True)
    source = args.platform or root/"fixtures/native/dist/spec"
    build = json.loads((args.template/"monitor-speaker-build.json").read_text("utf-8-sig"))
    config = (source/"spec.conf").read_text("utf-8-sig")
    settings = "\nbeepSoundSource.mcpCaptureSpeaker = yes\nbeepSoundSource.muted = yes\ntapeSoundSource.muted = yes\nkbdLayout.layout = smart\n"
    result = dict(build=build,rom_sha256=sha((source/"spec_m2.rom").read_bytes()),
                  attempt=attempt.name,source="GeneralSoundSource beepSoundSource, PC5",games={})
    with lease(size=16,port=args.port,artifact_root=attempt/"slots",template=args.template) as slot:
        if sha((slot.runtime/EXE).read_bytes())!=build["executableSha256"]:
            raise RuntimeError("Установленный EXE отличается от сборки записи динамика STD")
        platform = slot.runtime/"spec"
        platform.mkdir(exist_ok=True)
        for name in ("spec.html","spec_m2.rom"):
            shutil.copyfile(source/name,platform/name)
        (platform/"spec.conf").write_text(config+settings,"utf-8")
        result.update(config_sha256=sha((platform/"spec.conf").read_bytes()),pid=slot.pid,hwnd=slot.hwnd)
        result["calibration"] = calibrate(slot,attempt)
        for label,path,_ in monitor["GAMES"]:
            result["games"][label] = run_game(slot,root,attempt/label,label,path)
    for path in (attempt/"result.json",out/"index.json"):
        path.write_text(json.dumps(result,ensure_ascii=False,indent=2)+"\n","utf-8")
    print(attempt,result["calibration"])
    for name,game in result["games"].items():
        print(name,{key:value["outcome"] for key,value in game["comparison"].items()})


if __name__=="__main__":
    main()
