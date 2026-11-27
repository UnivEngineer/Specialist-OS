"""Проверить возобновление очереди и автоматический повтор после потери своего Emu80."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
import uuid

import psutil

ROOT = Path(__file__).resolve().parents[2]


def load(path):
    return json.loads(path.read_bytes())


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def start(folder, port, phase):
    command = [sys.executable, "-X", "utf8", "-u", ROOT / "scripts/games/run-campaign.py",
               "--port", str(port), "--out", folder, "--inventory", folder / "inventory.json", "--profile", "MON",
               "--file", "families/budi/originals/Budi2-std.rks", "--file", "std/Ochko.rks"]
    with (folder / (phase + ".log")).open("wb") as log:
        return subprocess.Popen(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)


def own_emulator(folder, worker, *, after_first):
    deadline = time.monotonic() + 350
    while worker.poll() is None and time.monotonic() < deadline:
        done = list((folder / "trials").glob("*/result.json"))
        if len(done) == 1:
            active = [p for p in (folder / "trials").glob("*/attempt-*/session.json") if p.parents[1]!=done[0].parent]
            for marker in active:
                try:
                    session = load(marker)
                    slot = load(folder / session["slot_log"] / "slot.json")
                    if slot["worker"] != worker.pid or after_first and session["boot_on_lease"] < 2:
                        continue
                    runtime = Path(slot["runtime"])
                    owner = load(runtime / "owner.json")
                    proc = psutil.Process(slot["pid"])
                    if (owner == {"worker": worker.pid, "config": slot["config"]}
                            and Path(proc.exe()) == Path(slot["executable"])
                            and proc.cmdline()[1:] == ["--conf-file", slot["config"], "--platform", "specialist-os"]
                            and proc.ppid() == worker.pid):
                        return proc, done[0]
                except (FileNotFoundError, psutil.NoSuchProcess, json.JSONDecodeError):
                    continue
        time.sleep(.08)
    raise RuntimeError(f"Работник {worker.pid} завершился или не получил собственный слот. Журнал в {folder}")


def run(args):
    folder = args.out.resolve() / uuid.uuid4().hex[:12]
    folder.mkdir(parents=True)
    with (folder / "inventory.log").open("wb") as log:
        subprocess.run([sys.executable, "-B", "-X", "utf8", ROOT / "scripts/games/audit-mxos-compat.py",
                        "--catalog", "--output", folder / "inventory.json"],
                       cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    first_worker = start(folder, args.port, "initial")
    workers = [first_worker]
    try:
        orphan, done = own_emulator(folder, first_worker, after_first=True)
        original_hash = digest(done)
        first_worker.kill()
        first_worker.wait(10)
        resumed = start(folder, args.port, "resumed")
        workers.append(resumed)
        fault, _ = own_emulator(folder, resumed, after_first=False)
        if orphan.is_running():
            raise RuntimeError("Осиротевший Emu80 продолжает работать после повторной выдачи")
        fault.terminate()
        fault.wait(10)
        if resumed.wait(350) != 0:
            raise RuntimeError("Повторный запуск завершился ошибкой")
        docs = [load(p) for p in (folder / "trials").glob("*/result.json")]
        retried, = [r for r in docs if r["id"] != done.parent.name]
        if not (len(docs) == 2 and all(r["control_file"] for r in docs) and digest(done) == original_hash
                and "cached" in (folder / "resumed.log").read_text("utf-8")
                and [a["outcome"] for a in retried["attempts"]] == ["infrastructure_error", "completed"]
                and retried["pid"] != fault.pid):
            raise RuntimeError(f"Не выполнены условия восстановления очереди: {folder}")
        result = dict(first=done.parent.name, preserved_result_sha256=original_hash, orphan_pid=orphan.pid,
                      first_and_second_trial_same_pid=load(done)["pid"]==orphan.pid,
                      interrupted_worker=first_worker.pid, resumed_worker=resumed.pid, terminated_emulator=fault.pid,
                      retry_pid=retried["pid"], retry_attempts=retried["attempts"], both_control_files_read=True,
                      evidence=folder.name)
        text = json.dumps(result, ensure_ascii=False, indent=2) + "\n"
        (folder / "summary.json").write_text(text, "utf-8")
        (args.out / "results.json").write_text(text, "utf-8")
        print(text)
    finally:
        for worker in workers:
            if worker.poll() is None:
                worker.kill()
                worker.wait(10)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, choices=range(19781, 19797), default=19796)
    parser.add_argument("--out", type=Path, default=ROOT / "docs/compat/game-campaign/queue-test")
    run(parser.parse_args())
