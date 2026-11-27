"""Записать звук игрового действия и тех же тактов без нажатия из одинакового состояния."""
import argparse
from pathlib import Path
import runpy
import tempfile
import uuid
import wave

from campaign_guest import Guest
from campaign_media import ROOT, read_tree, sha, trial_disk
from campaign_controls import PREPARE, PROBES

paired = runpy.run_path(str(Path(__file__).with_name("control-campaign.py")))
audio_stats = runpy.run_path(str(Path(__file__).with_name("report-campaign.py")))["audio_stats"]
from game_inventory import build_route
from mcp_pool import FLASH_PROTECTION, WorkerMachine


def capture(worker, job, primary, pair, fixture, folder):
    probe = pair["trial_kind"]
    plan = PROBES[probe]
    system = fixture/"C.bin"
    packed = build_route(job["record"],job["route"])
    disk, marker = trial_disk(packed,job["route"]["kind"],fixture/"H.bin")
    if sha(disk)!=primary["game_disk_sha256"]:
        raise ValueError("Хеш тестовой H: отличается от основной пробы")
    files, _ = read_tree(system.read_bytes())
    expected_media = {"system-flash.bin":primary["context"]["system_sha256"], "flash-disk.bin":sha(disk),
                      "floppy720-test.img":primary["context"]["floppy_seed_sha256"]}
    result = dict(id=job["id"], profile=job["profile"], source=primary["file"], key=plan["key"],
                  primary_version=primary["version"], payload_sha256=primary["payload_sha256"],
                  container_sha256=primary["container_sha256"], media=expected_media,
                  fixture=primary["context"]["fixture"], evidence=folder.name, arms=[])
    reference = None
    with tempfile.TemporaryDirectory(prefix="mxos-action-pcm-") as tmp:
        seed = Path(tmp)/"H.bin"
        seed.write_bytes(disk)
        for arm in paired["ARMS"]:
            slot = worker.start(disk=seed,system=system,floppy=fixture/"A.img")
            directory = folder/arm
            directory.mkdir()
            if (slot.protection!=FLASH_PROTECTION or
                    any(sha((slot.media_dir/name).read_bytes())!=value for name,value in expected_media.items())):
                raise ValueError("Состояние носителей перед захватом отличается")
            obs = dict(arm=arm, payload_sha256=primary["payload_sha256"],
                       pid=slot.pid, hwnd=slot.hwnd, boot_on_lease=slot.cold_boots+1,
                       initial_flash_protection=slot.protection, emulator_sha256=sha((slot.runtime/"Emu80qt-mcp.exe").read_bytes()))
            if obs["emulator_sha256"]!=primary["emulator_sha256"]:
                raise ValueError("Изменилась сборка Emu80")
            guest = Guest(slot,directory,fixture/"build",job["profile"],files,obs)
            guest.launch(job,packed)
            snapshot = paired["snapshot"]
            snapshot(guest,"entry",memory=True)
            if probe in PREPARE:
                PREPARE[probe](guest,snapshot,probe)
            else:
                paired["replay_start"](guest,pair["scenario_keys"])
            snapshot(guest,"before",memory=True,probe=probe)
            obs["matching_preparation"] = reference is None or paired["same_preparation"](reference,obs["checkpoints"])
            if reference is None:
                reference = obs["checkpoints"].copy()
            guest.points(*guest.return_points)
            pressing = arm.startswith("input")
            hold = sum(plan["windows"])
            obs["action"] = dict(key=plan["key"],pressed=pressing,hold_guest_ms=hold)
            if pressing:
                guest.call("kbd_send",action="press",keys=[plan["key"]])
            try:
                guest.sample_audio("action",hold)
            finally:
                if pressing:
                    guest.call("kbd_send",action="release",keys=[plan["key"]])
            guest.sample_audio("release",480)
            snapshot(guest,"after",probe=probe)
            for phase,sample in obs["audio"].items():
                clip = directory/sample["file"]
                sample.update(audio_stats(clip),sha256=sha(clip.read_bytes()))
            guest.recovery(marker)
            paired["CAMPAIGN"]["write"](directory/"observation.json",obs)
            result["arms"].append(dict(arm=arm,matching_preparation=obs["matching_preparation"],control_file=obs["control_file"],
                                       json=arm+"/observation.json",audio=obs["audio"]))
    result["comparison"] = compare_clips(folder,result["arms"])
    paired["CAMPAIGN"]["write"](folder/"result.json",result)
    paired["CAMPAIGN"]["write"](folder.parent/"result.json",result)
    return result["comparison"]


def compare_clips(folder, arms):
    comparisons = {}
    for phase in ("action","release"):
        clips = [arm["audio"][phase] for arm in arms]
        common_samples = min(c["samples"] for c in clips)
        waveforms = []
        for arm,clip in zip(arms,clips,strict=True):
            with wave.open(str(folder/arm["arm"]/clip["file"]),"rb") as source:
                waveforms.append(sha(source.readframes(common_samples)))
        a1,a2,b1,b2 = waveforms
        matching = all(arm["matching_preparation"] for arm in arms)
        comparable = (matching and common_samples>0 and max(c["samples"] for c in clips)-common_samples<=1
                      and not any(c["truncated"] for c in clips))
        repeated = comparable and a1==a2 and b1==b2
        if repeated:
            state = "repeatable_waveform_difference" if a1!=b1 else "same_waveform"
        elif comparable and all(c["transitions"]==0 for c in clips[:2]) and all(c["transitions"]>=10 for c in clips[2:]):
            state = "repeatable_signal_on_input"
        else:
            state = "inconclusive"
        comparisons[phase] = dict(outcome=state,common_samples=common_samples,pcm_sha256=waveforms)
    return comparisons


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--campaign",type=Path,default=paired["DEFAULT"])
    p.add_argument("--trial",action="append",default=[])
    p.add_argument("--port",type=int,choices=range(19781,19797))
    args = p.parse_args()
    root = args.campaign.resolve()
    inventory = paired["load"](ROOT/"docs/compat/game-inventory.json")
    worker = WorkerMachine(16,args.port,root/"slots")
    try:
        for job in paired["CAMPAIGN"]["jobs"](inventory["files"],[]):
            key = job["id"]
            if args.trial:
                if key not in args.trial:
                    continue
            elif not (set(job["aliases"]).intersection((*paired["CAMPAIGN"]["PILOT"],"families/almaz-2/repaired/Almaz2-ramfos-fixed.cpu"))
                      and job["profile"] in ("MON","RF")):
                continue
            primary = paired["load"](root/"trials"/key/"result.json")
            pair = paired["load"](root/"paired"/key/"result.json")
            if pair["status"]!="object_verified":
                if args.trial:
                    p.error("Для выбранного действия ещё нет парной проверки объекта")
                continue
            target = root/"action-audio"/key/("attempt-"+uuid.uuid4().hex[:12])
            target.mkdir(parents=True)
            fixture = root/"fixtures"/primary["context"]["fixture"]
            print(key,capture(worker,job,primary,pair,fixture,target),flush=True)
    finally:
        worker.close()


if __name__=="__main__":
    main()
