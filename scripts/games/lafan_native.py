"""Штатный ROM RAMFOS и дисковод ODI на видимом экземпляре очереди."""
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import shutil

from campaign_guest import Bench
from campaign_media import sha

ROM_SHA256 = "121c271c5578274d1f6053015061e3378a2835a4f9c29f4974fea6fdec8f0df8"


def prepare_platform(slot,source,options=""):
    source = Path(source)
    rom = source/"spmx.rom"
    if sha(rom.read_bytes())!=ROM_SHA256:
        raise ValueError("Ожидался архивный ROM RAMFOS для Специалиста-MX")
    directory = slot.runtime/"specmx"
    directory.mkdir(exist_ok=True)
    config = (source/"spmx.conf").read_text("utf-8-sig")
    # На плате MX FFF8h задаёт цвет, PC5 управляет звуком, а младшие биты
    # порта C выбирают строки клавиатуры. Привязка PC к атрибуту гасит текст.
    config = config.replace("ppiCircuit.videoRam = &videoRam","")
    if "FdImage diskA = 80, 2, 5, 1024" not in config:
        raise ValueError("Нужен каталог dist/specmx сборки Emu80 с геометрией ODI")
    (directory/"spmx.conf").write_text(config,encoding="utf-8")
    for name in ("spmx.rom","spmx.html"):
        shutil.copyfile(source/name,directory/name)
    options = "pitSoundSource.muted = yes\ntapeSoundSource.muted = yes\nkbdLayout.layout = smart\n"+options+"\n"
    (directory/"spmx.opt").write_text(options,encoding="utf-8")
    return directory


def switch_to_native(slot):
    if not slot.verify_owner():
        raise ConnectionError("Изменился владелец слота перед переключением на RAMFOS")
    hwnd = json.loads((slot.artifacts/"slot.json").read_bytes())["hwnd"]
    machine = Bench(slot.url)
    machine.call("debug_break")
    machine.call("kbd_reset")
    machine.call("bp_clear")
    if machine.call("sys_info")["platform"]=="spmx":
        machine.call("switch_platform",platform="specialist-os")
    state = machine.call("switch_platform",platform="spmx")
    owner = wintypes.DWORD()
    ctypes.windll.user32.GetWindowThreadProcessId(hwnd,ctypes.byref(owner))
    if state.get("current")!="spmx" or owner.value!=slot.pid or not ctypes.windll.user32.IsWindowVisible(hwnd):
        raise RuntimeError("Не подтверждены платформа RAMFOS и сохранение её видимого окна")
    machine.call("debug_break")
    machine.call("emu_speed",factor=32)
    return machine
