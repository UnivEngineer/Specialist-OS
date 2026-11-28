"""Наблюдения через MCP без подмены RAM, PC, SP или кода возврата игры."""
import base64
import json
from pathlib import Path
import re
import runpy
import sys
import urllib.request
import wave

from campaign_media import ROOT, sha

sys.path.insert(0, str(ROOT / ".local/game-campaign-python"))
from PIL import Image

Bench = runpy.run_path(str(ROOT / "source-apps/diskbench/check-emulator.py"))["Bench"]


def symbols(build, name):
    source = (build / (name + ".sym")).read_text(encoding="utf-8")
    return {k: int(v, 16) for k, v in re.findall(r"^([\w.]+): EQU 0x([0-9A-Fa-f]+)", source, re.M)}


class Observation(Exception):
    """Нормальная, локализованная остановка игровой пробы."""


class Guest(Bench):
    def call(self, name, **arguments):
        if self.slot.process.poll() is not None:
            raise ConnectionError(f"Владелец слота {self.slot.port} закрылся")
        return super().call(name, **arguments)

    def __init__(self, slot, out, build, profile, system_files, result):
        super().__init__(slot.url)
        self.slot, self.out, self.profile = slot, out, profile
        self.build = build
        self.system = system_files
        self.result = result
        self.k = symbols(build, "banked/KERNEL")
        self.nc = symbols(build, "NC_CORE")
        self.main = symbols(build, "NC")
        self.run_syms = symbols(build, "RUN")
        self.bios = symbols(build, "BIOS")
        self.main_code = (build / "NC.i80").read_bytes()
        self.core_code = (build / "NC_CORE.i80").read_bytes()
        self.keys = result.setdefault("events", [])
        self.return_points = {}
        self.exit_seen = None
        self.mx_vectors = None
        request = urllib.request.Request(self.url, json.dumps(dict(jsonrpc="2.0", id=0,
            method="tools/list", params={})).encode(), {"Content-Type":"application/json"})
        with urllib.request.urlopen(request, timeout=12) as response:
            self.tools = {t["name"] for t in json.load(response)["result"]["tools"]}
        self.call("debug_break")
        self.call("bp_clear")
        self.speed(32)
        self.result["cpu"] = self.call("sys_info")["cpu"]
        if self.result["cpu"].get("full_throttle"):
            raise RuntimeError("Full throttle недопустим для эмуляции ВИ53")

    def speed(self, factor):
        if "emu_speed" in self.tools:
            self.call("emu_speed", factor=factor)

    def regs(self):
        return {k:v["dec"] for k,v in self.call("cpu_state")["registers"].items()}

    def points(self, *pcs):
        self.call("bp_clear")
        for pc in set(pcs):
            self.call("bp_set", addr=pc, type="exec")

    def advance(self, ms):
        state = self.call("emu_run_for", ms=ms)
        if not state.get("breaked"):
            raise RuntimeError("MCP потерял остановку таймера во время пробы")
        return state["registers"]["pc"]["dec"]

    def seek(self, pc, ms=8000, check=None):
        self.points(pc)
        for _ in range(40):
            actual = self.advance(ms)
            if actual != pc:
                return False
            if check is None or check():
                return True
            self.call("debug_step", mode="into")
        return False

    def loader_stop(self, targets, ms):
        self.points(*targets)
        for _ in range(64):
            pc = self.advance(ms)
            if pc not in targets:
                return None
            if targets[pc]():
                return pc
            self.call("debug_step", mode="into")
        return None

    def frame(self, name):
        path = self.out / (name + ".png")
        self.screenshot(path)
        with Image.open(path) as frame:
            rgb = frame.convert("RGB")
            histogram = rgb.getcolors(rgb.width * rgb.height)
            colors = sorted(histogram, reverse=True)
            meta = dict(file=path.name, size=list(rgb.size), colors=len(colors),
                        rgb_sha256=sha(rgb.tobytes()), palette=[
                            ["#%02x%02x%02x" % color, n] for n, color in colors[:16]])
        self.result.setdefault("frames", {})[name] = meta
        return meta

    def key(self, key, duration=100, settle=120):
        before = self.regs()["pc"]
        result = self.call("kbd_send", keys=[key], action="tap", duration_ms=duration)
        if not result.get("stoppedByBreakpoint") and settle:
            self.advance(settle)
        self.keys.append(dict(key=key, duration_ms=duration, settle_ms=settle, pc=before))
        return self.regs()["pc"]

    def shell(self, reset=False):
        poll = self.nc["ncBankInputPoll"]
        main = self.main["mainLoop"]
        self.points(poll, main)
        if reset:
            self.call("emu_reset")
        # Адреса точек общие для DOS и всех банков, поэтому проверяем код и банк.
        for _ in range(180):
            pc = self.advance(12000)
            if pc not in (main, poll):
                return None
            bank = self.read(self.k["bankPage"], 1)[0]
            if pc == poll and bank > 1 and self.read(poll, 8) == self.core_code[poll-256:poll-248]:
                return "NC EXEC"
            if pc == main and bank == 0 and self.read(0,32) == self.main_code[:32]:
                return "NC MAIN"
            self.call("debug_step", mode="into")
        return None

    def line(self, command, picture):
        if not self.slot.verify_owner():
            raise ConnectionError("Перед набором команды изменился владелец MCP")
        self.points()
        self.advance(500)
        self.tap("esc")
        self.advance(250)
        names = {" ": "space", ":": "colon", ".": "period", "-": "minus", "/": "slash"}
        for char in command.lower():
            self.tap(names.get(char, char))
        typed = self.read(self.bios["CMD_LINE"], len(command))
        if typed.upper() != command.encode("ascii").upper():
            raise RuntimeError(f"Команда в NC отличается: {command!r}, {typed!r}")
        self.frame(picture)
        self.keys.append(dict(command=command, typed=typed.decode("ascii"), key="enter",
                              duration_ms=100, settle_ms=120))

    def launch(self, job, packed):
        record, route = job["record"], job["route"]
        command = "run H:PROBE." + route["kind"].upper()
        if self.profile != "NATIVE":
            command += " -" + self.profile.lower()
        self.result["command"] = command
        # Новая аренда начинает работу с собственных чистых копий носителей.
        boot = self.shell(reset=True)
        if not boot:
            raise RuntimeError("Холодная загрузка не дошла до цикла ввода NC")
        self.result["cold_shell"] = boot
        self.line(command, "command")
        stage = self.run_syms["RUN_LOAD_STAGE"]
        run_start = self.run_syms["start"]
        run_prefix = (self.build/"RUN.i80").read_bytes()[:32]
        def in_run():
            return self.read(self.k["bankPage"],1) == b"\0" and self.read(run_start,32) == run_prefix
        self.points(run_start)
        self.tap("enter")
        if not (self.regs()["pc"] == run_start and in_run()) and not self.seek(run_start,10000,in_run):
            raise Observation("Оболочка не передала управление штатному RUN.COM")
        firmware = self.system["DOS/BIOS.SYS"]
        addr = self.bios["biosCompatVectors"]
        end = self.bios["biosCompatVectorsEnd"]
        vectors = self.read(addr, end-addr)
        if vectors != firmware[addr-0xc000:end-0xc000]:
            raise RuntimeError("Таблица RAMFOS после загрузки отличается от штатной")
        self.mx_vectors = {addr: vectors}
        redirects = {}
        for name in ("mon2_printCharOld", "mon2_printStringOld"):
            entry = self.bios[name]
            actual = self.read(entry, 3)
            self.mx_vectors[entry] = actual
            if actual == firmware[entry-0xc000:entry-0xc000+3]:
                continue
            target = int.from_bytes(actual[1:],"little")
            if actual[0] != 0xc3 or not self.bios["FONT_ENGINE_BASE"] <= target < self.bios["GLYPH_CACHE_END"]:
                raise RuntimeError(f"Неизвестная привязка шрифта {name} к {target:04X}h")
            redirects[name] = target
        self.result["cold_font_bindings"] = redirects
        run_body = (self.build/"RUN.i80").read_bytes()
        bad = self.run_syms["badImage"]
        error = stage + self.run_syms["loadStageError"] - self.run_syms["loadStage"]
        def stage_matches():
            start = self.run_syms["loadStage"]
            return self.read(stage,3)==run_body[start:start+3]
        uses_stage = self.profile not in ("NATIVE", "MX") or (
            self.profile == "MX" and route["kind"].lower() == "rks"
        )
        if uses_stage:
            stop = self.loader_stop({stage:stage_matches, bad:lambda:in_run() and self.read(bad,4)==run_body[bad:bad+4]},12000)
            if stop == bad:
                self.result.update(launch="rejected_header_or_range", loader_pc=bad)
                raise Observation("RUN отклонил заголовок или диапазон до загрузки тела")
            if stop != stage:
                raise Observation("RUN не достиг процедуры окончательной загрузки тела")
        entry, load = record["entry"], record["load"]
        def matches():
            data = self.read(load, record["payload_bytes"])
            return sha(data) == record["payload_sha256"]
        if not (self.regs()["pc"] == entry and matches()):
            targets = {entry:matches}
            if uses_stage and error!=entry:
                orig = self.run_syms["loadStageError"]
                targets[error] = lambda:self.read(error,6)==run_body[orig:orig+6]
            stop = self.loader_stop(targets,12000)
            if stop == error:
                self.result.update(launch="rejected_body", loader_pc=error)
                raise Observation("Ошибка RUN при чтении тела или проверке контрольной суммы")
            if stop != entry:
                raise Observation("RUN не передал управление побайтно сверенной нагрузке")
        regs = self.regs()
        sp = regs["sp"]
        ret = int.from_bytes(self.read(sp, 2), "little") if sp < 65535 else 0
        self.result.update(launch="entry_and_payload_verified", entry_sp=sp, stack_return=ret)
        self.return_points = {0xc000:"vector_C000", 0xf800:"vector_F800"}
        if self.profile in ("STD", "MON"):
            self.return_points = {0xc800:"vector_C800", 0xc000:"vector_C000"}
        elif self.profile == "RF":
            self.return_points = {0xf800:"vector_F800", 0xf854:"vector_F854"}
        if ret >= 0x8000 and ret not in self.return_points:
            self.return_points[ret] = "return_address"
        self.integrity("entry")

    def integrity(self, phase, shell=False):
        profile = "MX" if shell else self.profile
        expected_vectors = {}
        if profile == "MON":
            raw = self.system["DOS/MONITOR2.MON"]
            ranges = [(0xc000, 0xc500), (0xc800, 0xd000)]
            vectors = [(0xc000, 4), (0xc037, 3), (0xc800, 0x36)]
            reference = lambda addr,n: raw[addr-0xc000:addr-0xc000+n]
        elif profile == "RF":
            rf = symbols(self.build, "RAMFOS")
            raw = self.system["DOS/RAMFOS.MON"]
            # Рабочие байты и самомодификацию не считаем порчей ядра.
            ranges = [(rf["tapeReadPilot"],rf["compatRomStubEnd"]), (0xf800,rf["a_defHeader"])]
            mutable = set()
            for symbol, offset, width in (("v_inverse2",0,1), ("printChar_pol",1,2),
                ("disableARAM1",1,1),("disableARAM2",1,1),("p_pageNum",0,1),("v_dlgOffset",0,2),
                ("parseDate_r1",1,1),("parseDate_r2",1,1),("parseDate_r3",1,1)):
                if symbol in rf:
                    start = rf[symbol] + offset
                    mutable.update(range(start,start+width))
            ranges = split_ranges(ranges, mutable)
            vectors = [(0xc800, 0x60), (0xf800, 0x60)]
            reference = lambda addr,n: raw[addr-0xc000:addr-0xc000+n] if addr<0xf800 else raw[addr-0xf800+0x1300:addr-0xf800+0x1300+n]
        else:
            raw = self.system["DOS/BIOS.SYS"]
            ranges = []
            for name,start in self.bios.items():
                if name.startswith("dos_piece_") and name.endswith("_start") and 0xc000 <= start < self.bios["biosPackedEnd"] and "biosState" not in name:
                    end = self.bios[name[:-5]+"end"]
                    if start < end:
                        ranges.append((start,end))
            mutable = set()
            for name,offset,size in (("v_dosBanked",0,1),("v_dosModuleReady",0,1),("keyColumn",0,2),
                    ("keyRow",0,1),("bankCaretVisible",0,1),("moveCore",1,2),
                    ("memFixedCallerSP",1,2),("memFixedVector",1,2),("biosProfile",0,1),
                    ("biosPpiVectors",0,9),("biosKeyRows",1,1),("biosCliExecError",0,1),
                    ("j_diskDriver",1,2)):
                start = self.bios[name]+offset
                mutable.update(range(start,start+size))
            ranges = split_ranges(ranges,mutable)
            expected_vectors = dict(self.mx_vectors or {})
            if profile == "STD":
                addr = self.bios["biosCompatVectors"]
                extent = self.bios["biosCompatVectorsEnd"]-addr
                start, end = self.bios["mon2Vectors"]-0xc000, self.bios["mon2VectorsEnd"]-0xc000
                targets = raw[start:end]
                table = bytearray(b"".join(b"\xc3"+targets[i:i+2] for i in range(0,len(targets),2)))
                for unused in (0x21, 0x2d):
                    table[unused] = 0xc9
                expected_vectors[addr] = table.ljust(extent,b"\xc9")
            vectors = [(address,len(vector)) for address,vector in sorted(expected_vectors.items())]
            reference = lambda addr,n: raw[addr-0xc000:addr-0xc000+n]
        observations = []
        # Читаем связные области только раз за фазу; ВВ55 и регистры устройств
        # находятся за их границами. Соседние проверки сравниваются на одном стопе.
        blocks = [(0xc000,self.read(0xc000,0x1300 if profile=="RF" else 0x1000 if profile=="MON" else len(raw)))]
        if profile=="RF":
            blocks.append((0xf800,self.read(0xf800,0x7e0)))
        for purpose, spans in (("code", ranges), ("vectors", [(a,a+n) for a,n in vectors])):
            for a,b in spans:
                start,block = next((base,d) for base,d in blocks if base<=a and b<=base+len(d))
                observed = block[a-start:b-start]
                expect = expected_vectors.get(a) if purpose == "vectors" else None
                if expect is None:
                    expect = reference(a, b-a)
                observations.append(dict(kind=purpose, start=a, end=b, sha256=sha(observed),
                    expected_sha256=sha(expect), differences=[a+i for i,(v,w) in enumerate(zip(expect, observed)) if v!=w]))
        self.result.setdefault("integrity", {})[phase] = observations
        return all(x["sha256"] == x["expected_sha256"] for x in observations)

    def watch_exit(self, pc):
        if pc not in self.return_points:
            return False
        self.exit_seen = self.return_points[pc]
        if self.exit_seen == "return_address":
            self.exit_seen = ("stack_return" if self.regs()["sp"] == (self.result["entry_sp"]+2)&65535
                              else "return_address_other_sp")
        self.result["exit_pc"] = pc
        return True

    def sample_audio(self, phase, ms):
        if "mx_audio" not in self.tools:
            return self.advance(ms)
        self.speed(1)
        try:
            self.call("mx_audio", action="start")
            try:
                pc = self.advance(ms)
            finally:
                capture = self.call("mx_audio", action="stop")
            pcm = base64.b64decode(capture["pcm_base64"])
            rate = capture["rate"]
            path = self.out / (phase+".wav")
            if rate <= 0 or len(pcm)%2:
                raise RuntimeError("Некорректная запись PCM")
            with wave.open(str(path), "wb") as stream:
                stream.setparams((1,2,rate,0,"NONE","not compressed"))
                stream.writeframes(pcm)
            import array
            values = array.array("h", pcm)
            self.result.setdefault("audio", {})[phase] = dict(file=path.name, rate=rate,
                samples=len(values), varied=len(set(values))>1, truncated=capture.get("truncated"), speed=1)
            return pc
        finally:
            self.speed(32)

    def read_check(self, marker):
        self.line("type H:CHECK.TXT", "check-command")
        pc = self.k["cliTypePrint"]
        def has_marker():
            regs = self.regs()
            return (regs["hl"] == self.k["cliIoBuffer"] and regs["bc"] == len(marker)
                    and self.read(regs["hl"],len(marker)) == marker)
        self.points(pc)
        self.tap("enter")
        if not (self.regs()["pc"] == pc and has_marker()) and not self.seek(pc, 7000, has_marker):
            self.frame("check-failed")
            self.result["control_failure_regs"] = self.regs()
            return False
        self.result["read_back"] = marker.decode("ascii")
        # TYPE даёт обычный вход в банковый DOS. Здесь можно сверить его код
        # без принудительного переключения страниц отладчиком.
        raw = self.system["DOS/DOS.SYS"]
        spans = [(0x100,0x140),(self.k["fileLoadInfo"],self.k["fileNamePrepare"]),
                 (self.k["cliType"],self.k["cliTypePrint"]+18)]
        observed = []
        for a,b in spans:
            before = raw[a-256:b-256]
            data = self.read(a,b-a)
            observed.append(dict(kind="banked_dos",start=a,end=b,sha256=sha(data),expected_sha256=sha(before),
                differences=[a+i for i,(v,w) in enumerate(zip(data,before)) if v!=w]))
        self.result.setdefault("integrity",{})["dos_after"] = observed
        self.points()
        self.advance(600)
        self.frame("check-output")
        return True

    def recovery(self, marker):
        self.points()
        shell = self.shell()
        if shell is None:
            self.frame("pre-reset")
            shell = self.shell(reset=True)
            self.result["recovery"] = "reset" if shell else "reset_failed"
        else:
            self.result["recovery"] = "game_exit"
        self.result["exit"] = self.exit_seen or "unobserved"
        if shell:
            self.result["returned_shell"] = shell
            # Код BIOS в MAIN. При остановке в ncBankInputPoll процессор видит
            # EXEC-банк. Штатный F10 передаёт управление CLI в основной памяти.
            self.points()
            self.advance(350)
            cli = symbols(self.build, "CLI")
            prompt = cli["cliPrompt"]
            self.points(prompt)
            self.tap("f10")
            def in_cli():
                return self.read(self.k["bankPage"],1) == b"\0" and self.read(prompt,8) == (self.build/"CLI.i80").read_bytes()[prompt:prompt+8]
            if not (self.regs()["pc"] == prompt and in_cli()) and not self.seek(prompt,8000,in_cli):
                raise RuntimeError("После восстановления NC клавиша F10 не открыла CLI в MAIN")
            self.integrity("recovered", shell=True)
            self.result["control_file"] = self.read_check(marker)


def split_ranges(spans, excluded):
    addresses = sorted({i for a,b in spans for i in range(a,b)} - excluded)
    merged = []
    for i in addresses:
        if merged and merged[-1][1]==i:
            merged[-1] = merged[-1][0],i+1
        else:
            merged.append((i,i+1))
    return merged
