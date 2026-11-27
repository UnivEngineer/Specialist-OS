"""Проверяет одновременную аренду, матрицу клавиш, носители и повторную выдачу после гибели работника."""

import argparse
import array
import base64
import itertools
import json
import os
import runpy
import subprocess
import sys
import tempfile
import threading
import time
import wave
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import psutil

from mcp_pool import FIRST_PORT, FLASH_PROTECTION, MAX_SIZE, ROOT, SlotBusy, lease

Machine = runpy.run_path(str(ROOT / "scripts/emu80/check-flash.py"))["Machine"]
# Колонка и бит в шести рядах матрицы original.
KEYS = [("enter", 0, 0), ("right", 2, 0), ("rpt", 3, 0), ("left", 4, 0),
        ("space", 5, 0), ("esc", 6, 0), ("tab", 7, 0), ("down", 8, 0),
        ("up", 9, 0), ("home", 10, 0), ("a", 8, 2), ("b", 4, 1),
        ("c", 10, 3), ("d", 3, 2), ("e", 7, 3), ("f", 11, 2)]


def check_group(ports, output):
    barrier = threading.Barrier(len(ports), timeout=60)
    rows = []
    seed = (ROOT / "images/H-Flash-2M-Games.bin").read_bytes()
    with tempfile.TemporaryDirectory() as tmp:
        def worker(i):
            key, column, bit = KEYS[i]
            h = Path(tmp) / f"disk-{i}.bin"
            tagged = bytearray(seed)
            tagged[0x1212] = 0xA0 + i
            h.write_bytes(tagged)
            try:
                with lease(port=ports[i], disk=h, artifact_root=output / "slots") as slot:
                    m = Machine(slot.url)
                    m.call("debug_break")
                    # Берём MX RAM с прямым доступом к обеим ВВ55, сохраняя CPU остановленным.
                    m.write(0xfffc, b"\0")
                    m.write(0xffe7, b"\x90")
                    m.write(0xffe6, b"\x60")
                    m.write(0xffe5, b"\x12")
                    assert m.read(0xffe4, 1) == bytes([0xA0 + i]), slot.port
                    m.write(0xffe3, b"\x82")
                    barrier.wait()
                    m.call("kbd_send", action="press", keys=[key])
                    barrier.wait()
                    matrix = []
                    for col in range(12):
                        mask = 0xfff ^ (1 << col)
                        m.write(0xffe0, bytes([mask & 255]))
                        m.write(0xffe2, bytes([mask >> 8]))
                        matrix.append(((~m.read(0xffe1, 1)[0]) & 255) >> 2)
                    expected = [0] * 12
                    expected[column] = 1 << bit
                    assert matrix == expected, (slot.port, key, matrix, expected)
                    info = m.call("sys_info")
                    assert info["server"]["port"] == slot.port and slot.verify_owner()
                    barrier.wait()
                    m.call("kbd_send", action="release", keys=[key])
                    files = [str(slot.runtime / "specialist-os" / n) for n in
                             ("floppy720-test.img", "system-flash.bin", "flash-disk.bin")]
                    hwnd = slot.hwnd
                    tagged[0x1212] = 0xC0 + i
                    h.write_bytes(tagged)
                    barrier.wait()
                    slot.cold_boot(disk=h,system=ROOT/"images/C-Flash-512k-MMX-System-Emu80.bin",
                                   floppy=ROOT/"images/A-Floppy-720k-Programs.img")
                    m.write(0xfffc,b"\0")
                    m.write(0xffe7,b"\x90")
                    m.write(0xffe6,b"\x60")
                    m.write(0xffe5,b"\x12")
                    assert m.read(0xffe4,1)==bytes([0xc0+i])
                    assert slot.hwnd==hwnd and slot.verify_owner() and slot.protection==FLASH_PROTECTION
                    files += [str(slot.media_dir/n) for n in ("floppy720-test.img","system-flash.bin","flash-disk.bin")]
                    return dict(port=slot.port, pid=slot.pid, key=key, matrix=matrix,
                                hwnd=hwnd, cold_disk_value=0xC0+i, protection=slot.protection,
                                disk_value=0xA0 + i, disks=files, version=info["server"]["version"])
            except BaseException:
                barrier.abort()
                raise
        with ThreadPoolExecutor(len(ports)) as pool:
            futures = [pool.submit(worker, i) for i in range(len(ports))]
            errors = [f.exception() for f in futures if f.exception()
                      and not isinstance(f.exception(), threading.BrokenBarrierError)]
            if errors:
                raise errors[0]
            rows = [f.result() for f in futures]
    for a, b in itertools.combinations((p for r in rows for p in r["disks"]), 2):
        assert not os.path.samefile(a, b), (a, b)
    return rows


def crash(port, output):
    args = [sys.executable, __file__, "--orphan-worker", str(port), "--out", str(output)]
    with subprocess.Popen(args, stdout=subprocess.PIPE, text=True) as worker:
        try:
            line = worker.stdout.readline()
            if not line:
                raise RuntimeError("Работник завершился до старта эмулятора")
            old_pid = int(line)
            try:
                with lease(port=port, timeout=.1):
                    raise AssertionError("Слот выдан дважды")
            except SlotBusy:
                pass
        finally:
            worker.kill()
            worker.wait(10)
    with lease(port=port, artifact_root=output / "slots") as slot:
        assert slot.verify_owner() and slot.pid != old_pid and not psutil.pid_exists(old_pid)
        m = Machine(slot.url)
        m.call("debug_break")
        m.call("emu_speed", factor=32)
        assert m.call("sys_info")["cpu"]["speed_factor"] == 32
        m.call("emu_speed", factor=1)
        # Гостевой цикл и ВИ53 выдают 1 кГц при CLK0=2 МГц, делителе 2000 и открытом PC5.
        m.write(0xfffc, b"\0")
        m.write(0x6000, bytes.fromhex("C3 00 60"))
        m.call("regs_set", pc=0x6000)
        m.write(0xffe3, b"\x82")
        m.write(0xffe2, b"\x00")
        for control, channel, divider in [(0xb0, 0xffee, 65535), (0x36, 0xffec, 2000)]:
            m.write(0xffef, bytes([control]))
            for byte in divider.to_bytes(2, "little"):
                m.write(channel, bytes([byte]))
        m.call("mx_audio", action="start")
        m.call("emu_run_for", ms=300)
        audio = m.call("mx_audio", action="stop")
        samples = base64.b64decode(audio["pcm_base64"])
        pcm = array.array("h", samples)
        edges = sum(a < 8192 <= b for a, b in zip(pcm, pcm[1:]))
        assert not audio["truncated"] and abs(len(pcm) - .3 * audio["rate"]) <= 1
        assert abs(edges - 300) <= 1, edges
        recording = slot.artifacts / "pit-1khz.wav"
        with wave.open(str(recording), "wb") as wav:
            wav.setparams((1, 2, audio["rate"], 0, "NONE", ""))
            wav.writeframes(samples)
        return dict(port=port, worker=worker.pid, old_emulator=old_pid, new_emulator=slot.pid,
                    audio_hz=audio["rate"], frames=len(pcm), rising_edges=edges,
                    recording=recording.relative_to(output.resolve()).as_posix())


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--size", type=int, choices=range(1, MAX_SIZE+1), default=8)
    p.add_argument("--first", type=int, default=FIRST_PORT)
    p.add_argument("--out", type=Path, default=ROOT / "docs/games/mcp-pool")
    p.add_argument("--orphan-worker", type=int, help=argparse.SUPPRESS)
    a = p.parse_args()
    if a.orphan_worker:
        with lease(port=a.orphan_worker, artifact_root=a.out / "slots") as slot:
            print(slot.pid, flush=True)
            time.sleep(120)
        return
    ports = list(range(a.first, a.first + a.size))
    if ports[0] < FIRST_PORT or ports[-1] >= FIRST_PORT + MAX_SIZE:
        p.error("Диапазон портов должен входить в 19781–19796")
    rows = check_group(ports, a.out)
    result = dict(slots=rows, recovery=crash(ports[-1], a.out))
    report = a.out / f"check-{a.size}.json"
    report.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"PASS {a.size} independent keyboards, disks, listeners and crash recovery. {report}")


if __name__ == "__main__":
    main()
