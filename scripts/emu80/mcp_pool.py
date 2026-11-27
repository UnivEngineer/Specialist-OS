"""Аренда видимой Windows-машины Emu80 с собственными A:, C:, H: и MCP."""

import hashlib
import ctypes
from ctypes import wintypes
import json
import msvcrt
import os
import runpy
import shutil
import subprocess
import time
import uuid
from contextlib import ExitStack, contextmanager
from dataclasses import dataclass
from pathlib import Path

import psutil

ROOT = Path(__file__).resolve().parents[2]
INSTALL = Path(os.environ["LOCALAPPDATA"]) / "Specialist-OS/emu80"
FIRST_PORT = 19781
MAX_SIZE = 16
EXE = "Emu80qt-mcp.exe"
Machine = runpy.run_path(str(Path(__file__).with_name("check-flash.py")))["Machine"]
# Эти флаги устанавливает run.ps1 -ResetFlash для штатной эксплуатационной C: и сменной H:.
FLASH_PROTECTION = {"system-flash.bin.sdp":"01","flash-disk.bin.sdp":None}


def flash_protection(directory):
    return {name:(directory/name).read_bytes().hex() if (directory/name).is_file() else None for name in FLASH_PROTECTION}


class SlotBusy(RuntimeError):
    pass


class WorkerMachine:
    """Держит видимое окно до конца очереди работника или до ошибки связи."""
    def __init__(self, size, port, artifact_root):
        self.size, self.port, self.artifact_root = size, port, artifact_root
        self.stack = ExitStack()
        self.slot = None

    def start(self, *, disk, system, floppy):
        images = dict(disk=disk, system=system, floppy=floppy)
        if self.slot is None:
            self.slot = self.stack.enter_context(lease(size=self.size,port=self.port,artifact_root=self.artifact_root,**images))
        else:
            self.slot.cold_boot(**images)
        return self.slot

    def close(self):
        try:
            self.stack.close()
        finally:
            self.slot = None


def listener(port):
    return {(c.laddr.ip, c.pid) for c in psutil.net_connections(kind="tcp")
            if c.status == psutil.CONN_LISTEN and c.laddr.port == port}


def window_handle(pid):
    user32 = ctypes.windll.user32
    found = []
    @ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
    def visit(handle,_):
        owner = wintypes.DWORD()
        user32.GetWindowThreadProcessId(handle,ctypes.byref(owner))
        if owner.value==pid and user32.IsWindowVisible(handle):
            title = ctypes.create_unicode_buffer(256)
            user32.GetWindowTextW(handle,title,len(title))
            if "MXOS" in title.value:
                found.append(handle)
        return True
    user32.EnumWindows(visit,0)
    if len(found)!=1:
        raise RuntimeError(f"Не найдено видимое окно MXOS для PID {pid}: {found}")
    return found[0]


@dataclass
class Slot:
    port: int
    runtime: Path
    artifacts: Path
    process: subprocess.Popen
    alternate: bool = False
    cold_boots: int = 0

    @property
    def url(self):
        return f"http://127.0.0.1:{self.port}/mcp"

    @property
    def pid(self):
        return self.process.pid

    @property
    def media_dir(self):
        base = self.runtime / "specialist-os"
        return base / "cold" if self.alternate else base

    @property
    def hwnd(self):
        return window_handle(self.pid)

    @property
    def platform(self):
        return "specialist-os-cold" if self.alternate else "specialist-os"

    def cold_boot(self, *, disk, system, floppy):
        """Пересоздать гостевую машину из подготовленных носителей в том же Qt-окне."""
        if not self.verify_owner():
            raise ConnectionError("Перед холодной загрузкой изменился владелец MCP")
        handle = self.hwnd
        call = Machine(self.url).call
        call("debug_break")
        call("bp_clear")
        call("kbd_reset")
        base = self.runtime / "specialist-os"
        target = base if self.alternate else base/"cold"
        target.mkdir(exist_ok=True)
        for name,source,sizes in (("flash-disk.bin",disk,(2097152,)), ("system-flash.bin",system,(131072,262144,524288)),
                                  ("floppy720-test.img",floppy,(737280,))):
            source = Path(source)
            if source.stat().st_size not in sizes:
                raise ValueError(f"Размер исходного образа {name}: {source}")
            shutil.copyfile(source,target/name)
        for name,value in FLASH_PROTECTION.items():
            if value is None:
                (target/name).unlink(missing_ok=True)
            else:
                (target/name).write_bytes(bytes.fromhex(value))
        if flash_protection(target)!=FLASH_PROTECTION:
            raise RuntimeError("Не восстановлены стартовые флаги защиты AT29 на C:/H:")
        expected = "specialist-os" if self.alternate else "specialist-os-cold"
        reply = call("switch_platform",platform=expected)
        if not reply.get("switched") or reply.get("current")!=expected or not self.verify_owner():
            raise ConnectionError(f"Холодная загрузка не подтверждена: {reply}")
        self.alternate = not self.alternate
        self.cold_boots += 1
        if self.hwnd != handle:
            raise RuntimeError("Qt сменил видимое окно при холодной загрузке")
        call("debug_break")
        call("emu_reset")
        return self.cold_boots

    @property
    def protection(self):
        return flash_protection(self.media_dir)

    def verify_owner(self):
        try:
            return (self.process.poll() is None
                    and listener(self.port) == {("127.0.0.1", self.pid)}
                    and Path(psutil.Process(self.pid).exe()) == self.runtime / EXE)
        except psutil.NoSuchProcess:
            return False


def recover(runtime):
    # Метка записана ПЕРЕД запуском, в том числе при гибели до получения PID.
    marker = runtime / "owner.json"
    if not marker.exists():
        return
    previous = json.loads(marker.read_text(encoding="utf-8"))
    expected = str(runtime / EXE)
    args = ["--conf-file", previous["config"], "--platform", "specialist-os"]
    for proc in psutil.process_iter(["name"]):
        try:
            if (proc.info["name"].lower() == EXE.lower()
                    and Path(proc.exe()) == Path(expected)
                    and proc.cmdline()[1:] == args):
                proc.terminate()
                proc.wait(8)
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            continue
    # Старый файл конфигурации принадлежит одноразовой аренде.
    config = (runtime / previous["config"]).resolve()
    if config.parent == runtime / "specialist-os" and config.name.startswith("lease-"):
        config.unlink(missing_ok=True)
    marker.unlink()


def install(runtime, template):
    record = (template / "specialist-build.json").read_bytes()
    info = json.loads(record)
    if not info.get("mcp") or not any(s["path"] == "mxos-mcp-port.patch" for s in info["sources"]):
        raise ValueError(f"Соберите MCP с mxos-mcp-port.patch: {template}")
    marker = runtime / "specialist-build.json"
    expected = info.get("executableSha256")
    executable = runtime / EXE
    if (expected and marker.is_file() and marker.read_bytes() == record
            and executable.is_file() and hashlib.sha256(executable.read_bytes()).hexdigest() == expected):
        return
    for name in (EXE, "qt.conf", "emu80.conf", "dbgfont.bin", "help.css", "COPYING.txt"):
        shutil.copyfile(template / name, runtime / name)
    for dll in template.glob("Qt*.dll"):
        shutil.copyfile(dll, runtime / dll.name)
    shutil.copytree(template / "_qtplugins", runtime / "_qtplugins", dirs_exist_ok=True)
    if expected and hashlib.sha256(executable.read_bytes()).hexdigest() != expected:
        raise RuntimeError(f"Контрольная сумма EXE отличается от записи сборки: {template}")
    marker.write_bytes(record)


def prepare(runtime, artifacts, disk, system, floppy):
    settings = runtime / "_settings"
    settings.mkdir(exist_ok=True)
    (settings / "emu80.ini").write_text(
        "[system]\nshowHelp=no\n[specialist-os]\nkbdLayout.layout=smart\n", encoding="ascii")
    executable = ["powershell.exe", "-NoProfile", "-File", ROOT / "scripts/emu80/run.ps1",
                  "-EmulatorDirectory", runtime, "-Flash", "-Mcp", "-PrepareOnly",
                  "-ResetFlash", "-FreshFloppy"]
    with (artifacts / "prepare.log").open("wb") as log:
        subprocess.run(executable, stdout=log, stderr=subprocess.STDOUT, check=True,
                       creationflags=subprocess.CREATE_NO_WINDOW)
    deployment = runtime / "specialist-os"
    for source, target, sizes in (
            (disk, "flash-disk.bin", (2097152,)),
            (system, "system-flash.bin", (131072, 262144, 524288)),
            (floppy, "floppy720-test.img", (737280,))):
        if source is not None:
            if Path(source).stat().st_size not in sizes:
                raise ValueError(f"Недопустимый размер {target}: {source}")
            shutil.copyfile(source, deployment / target)


@contextmanager
def lease(size=8, port=None, *, disk=None, system=None, floppy=None, timeout=120,
          template=None, artifact_root=None):
    """Ждёт свободный слот. Явный port может выбрать любой из 16 установленных портов."""
    if not 1 <= size <= MAX_SIZE or (port is not None and not FIRST_PORT <= port < FIRST_PORT + MAX_SIZE):
        raise ValueError("size 1..16, port 19781..19796")
    ports = [port] if port is not None else range(FIRST_PORT, FIRST_PORT + size)
    template = Path(template or os.environ.get("SPECIALIST_EMU80_POOL_TEMPLATE",
                                                INSTALL / "pool-template")).resolve()
    artifacts = Path(artifact_root or ROOT / ".local/emu80-campaign").resolve()
    if not (template / EXE).is_file():
        raise FileNotFoundError(f"Не установлен шаблон MCP {template}. См. scripts/emu80/README.md")
    deadline = time.monotonic() + timeout
    lock = None
    process = None
    config = None
    marker = None
    try:
        while lock is None:
            for number in ports:
                runtime = (INSTALL / "pool" / str(number)).resolve()
                runtime.mkdir(parents=True, exist_ok=True)
                candidate = (runtime / "lease.lock").open("a+b")
                candidate.seek(0)
                try:
                    msvcrt.locking(candidate.fileno(), msvcrt.LK_NBLCK, 1)
                except OSError:
                    candidate.close()
                    continue
                lock = candidate
                recover(runtime)
                if not listener(number):
                    break
                lock.close()
                lock = None
            if lock is None:
                if time.monotonic() >= deadline:
                    raise SlotBusy(f"Нет свободного MCP-порта среди {list(ports)}")
                time.sleep(.3)
        token = uuid.uuid4().hex
        evidence = artifacts / str(number) / token
        evidence.mkdir(parents=True)
        install(runtime, template)
        prepare(runtime, evidence, disk, system, floppy)
        if flash_protection(runtime/"specialist-os")!=FLASH_PROTECTION:
            raise RuntimeError("Исходные флаги защиты AT29 расходятся со штатным профилем")
        config = runtime / "specialist-os" / f"lease-{token}.conf"
        shutil.copyfile(config.with_name("launch.conf"), config)
        alt = config.with_name("cold.conf")
        rel = "specialist-os/cold/"
        alt.write_text(config.read_text("utf-8")+"\n"+f'systemFlash.image = "{rel}system-flash.bin"\n'
                       + f'flashDisk.image = "{rel}flash-disk.bin"\n'
                       + f'diskA.fileName = "{rel}floppy720-test.img"\n', encoding="utf-8")
        base_conf = (template/"emu80.conf").read_bytes()
        specs = (f'\nconfig.addPlatform = "Specialist-OS", "specialist-os/{config.name}", "specialist-os"\n'
                 f'config.addPlatform = "Specialist-OS cold", "specialist-os/{alt.name}", "specialist-os-cold"\n')
        (runtime/"emu80.conf").write_bytes(base_conf+specs.encode("ascii"))
        marker = runtime / "owner.json"
        config_arg = config.relative_to(runtime).as_posix()
        temporary = marker.with_suffix(".tmp")
        temporary.write_text(json.dumps({"worker": os.getpid(), "config": config_arg}), encoding="utf-8")
        temporary.replace(marker)
        env = dict(os.environ, SPECIALIST_EMU80_MCP_PORT=str(number), QT_QPA_PLATFORM="windows")
        args = [runtime / EXE, "--conf-file", config_arg, "--platform", "specialist-os"]
        process = subprocess.Popen(args, cwd=runtime, env=env)
        slot = Slot(number, runtime, evidence, process)
        startup = time.monotonic() + 30
        while not slot.verify_owner():
            if process.poll() is not None or time.monotonic() >= startup:
                raise RuntimeError(f"Emu80 PID {process.pid} не получил порт {number}; {evidence}")
            time.sleep(.12)
        while True:
            try:
                handle = slot.hwnd
                break
            except RuntimeError:
                if process.poll() is not None or time.monotonic() >= startup:
                    raise
                time.sleep(.1)
        (evidence / "slot.json").write_text(json.dumps(
            dict(port=number, pid=process.pid, runtime=str(runtime), executable=str(runtime / EXE),
                 worker=os.getpid(), config=config_arg, hwnd=handle), indent=2), encoding="utf-8")
        yield slot
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            process.wait(timeout=10)
        if marker is not None:
            marker.unlink(missing_ok=True)
        if config is not None:
            config.unlink(missing_ok=True)
        if lock is not None:
            lock.close()
