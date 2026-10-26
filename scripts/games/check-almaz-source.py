"""Побайтная проверка реконструкции и измерение исходных циклов Intel 8080.

Это ограниченный исполнитель инструкций игры, не эмулятор Специалиста:
BIOS-сканер возвращает заданный код, печать BIOS не учитывается по тактам.
Неизвестные инструкции и обращения к чужому коду завершаются ошибкой.
"""
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


class CPU:
    def __init__(self, payload, key=0xFF):
        self.mem = bytearray(65536)
        self.mem[0x1EFD:0x3100] = payload
        self.r = [0] * 8  # B,C,D,E,H,L,память,A
        self.sp, self.pc = 0x7F00, 0
        self.z = self.cy = self.sign = self.parity = False
        self.ticks = self.screen_writes = 0
        self.key = key
        self.bios = Counter()

    def pair(self, n):
        return self.sp if n == 3 else self.r[n*2]*256+self.r[n*2+1]

    def put_pair(self, n, value):
        value &= 65535
        if n == 3:
            self.sp = value
        else:
            self.r[n*2], self.r[n*2+1] = value >> 8, value & 255

    def read(self, n):
        return self.mem[self.pair(2)] if n == 6 else self.r[n]

    def write_mem(self, address, value):
        assert address < 0xC000, f"Запись в системную область: {address:04X}"
        if 0x9000 <= address < 0xC000:
            self.screen_writes += 1
        self.mem[address] = value & 255

    def write(self, n, value):
        if n == 6:
            self.write_mem(self.pair(2), value)
        else:
            self.r[n] = value & 255

    def push(self, value):
        self.sp = (self.sp-2) & 65535
        self.write_mem(self.sp, value)
        self.write_mem(self.sp+1, value >> 8)

    def pop(self):
        value = self.mem[self.sp]+256*self.mem[self.sp+1]
        self.sp = (self.sp+2) & 65535
        return value

    def flags(self, value):
        self.z = (value & 255) == 0
        self.sign = bool(value & 128)
        self.parity = (value & 255).bit_count() % 2 == 0

    def condition(self, n):
        return [not self.z, self.z, not self.cy, self.cy,
                not self.parity, self.parity, not self.sign, self.sign][n]

    def alu(self, n, value):
        a = self.r[7]
        if n in (0, 1):
            result = a+value+(self.cy if n == 1 else 0)
            self.cy = result > 255
        elif n in (2, 3, 7):
            result = a-value-(self.cy if n == 3 else 0)
            self.cy = result < 0
        else:
            result = [a & value, a ^ value, a | value][n-4]
            self.cy = False
        self.flags(result)
        if n != 7:
            self.r[7] = result & 255

    def step(self):
        p = self.pc
        if p in (0xC81B, 0xC818, 0xC815):
            self.bios[p] += 1
            if p == 0xC81B:
                self.r[7] = self.key
            self.pc = self.pop()
            return
        assert 0x1EFD <= p < 0x26A3, f"Чужая инструкция: {p:04X}"
        m, r = self.mem, self.r
        o, v = m[p], m[p+1]
        w = v+256*m[p+2]
        self.pc += 1
        cycles = 4
        if o == 0:
            pass
        elif 0x40 <= o < 0x80:
            assert o != 0x76
            self.write((o >> 3) & 7, self.read(o & 7))
            cycles = 7 if 6 in ((o >> 3) & 7, o & 7) else 5
        elif 0x80 <= o < 0xC0:
            self.alu((o >> 3) & 7, self.read(o & 7))
            cycles = 7 if o & 7 == 6 else 4
        elif o & 0xC7 == 6:
            self.write((o >> 3) & 7, v)
            self.pc += 1
            cycles = 10 if (o >> 3) & 7 == 6 else 7
        elif o & 0xCF == 1:
            self.put_pair((o >> 4) & 3, w)
            self.pc += 2
            cycles = 10
        elif o & 0xC7 in (4, 5):
            n = (o >> 3) & 7
            result = self.read(n)+(1 if o & 7 == 4 else -1)
            self.write(n, result)
            self.flags(result)
            cycles = 10 if n == 6 else 5
        elif o & 0xCF in (3, 11, 9):
            n = (o >> 4) & 3
            if o & 15 == 9:
                result = self.pair(2)+self.pair(n)
                self.put_pair(2, result)
                self.cy = result > 65535
                cycles = 10
            else:
                self.put_pair(n, self.pair(n)+(1 if o & 15 == 3 else -1))
                cycles = 5
        elif o & 0xCF in (0xC1, 0xC5):
            n = (o >> 4) & 3
            if o & 15 == 5:
                value = self.pair(n) if n != 3 else (r[7]*256+2+
                    int(self.cy)+int(self.z)*64+int(self.sign)*128+int(self.parity)*4)
                self.push(value)
                cycles = 11
            else:
                value = self.pop()
                if n == 3:
                    r[7] = value >> 8
                    self.cy, self.z = bool(value & 1), bool(value & 64)
                    self.sign, self.parity = bool(value & 128), bool(value & 4)
                else:
                    self.put_pair(n, value)
                cycles = 10
        elif o in (0xC3, 0xCD) or o & 0xC7 in (0xC2, 0xC4):
            self.pc += 2
            take = o in (0xC3, 0xCD) or self.condition((o >> 3) & 7)
            call = o == 0xCD or o & 0xC7 == 0xC4
            cycles = (17 if take else 11) if call else 10
            if take:
                if call:
                    self.push(self.pc)
                self.pc = w
        elif o == 0xC9 or o & 0xC7 == 0xC0:
            take = o == 0xC9 or self.condition((o >> 3) & 7)
            cycles = 10 if o == 0xC9 else (11 if take else 5)
            if take:
                self.pc = self.pop()
        elif o & 0xC7 == 0xC6:
            self.alu((o >> 3) & 7, v)
            self.pc += 1
            cycles = 7
        elif o in (0x22, 0x2A, 0x32, 0x3A):
            self.pc += 2
            if o == 0x22:
                self.write_mem(w, r[5])
                self.write_mem(w+1, r[4])
            elif o == 0x2A:
                self.put_pair(2, m[w]+256*m[w+1])
            elif o == 0x32:
                self.write_mem(w, r[7])
            else:
                r[7] = m[w]
            cycles = 16 if o in (0x22, 0x2A) else 13
        elif o in (2, 0x12):
            self.write_mem(self.pair(o >> 4), r[7])
            cycles = 7
        elif o in (7, 15, 23, 31):
            a, carry = r[7], self.cy
            if o in (7, 23):
                r[7] = ((a << 1) | ((a >> 7) if o == 7 else carry)) & 255
                self.cy = bool(a & 128)
            else:
                r[7] = (a >> 1) | (((a & 1) if o == 15 else carry) << 7)
                self.cy = bool(a & 1)
        elif o == 0x2F:
            r[7] ^= 255
        elif o == 0xEB:
            r[2:4], r[4:6] = r[4:6], r[2:4]
        elif o == 0xE3:
            value = self.pop()
            self.push(self.pair(2))
            self.put_pair(2, value)
            cycles = 18
        elif o == 0xE9:
            self.pc = self.pair(2)
            cycles = 5
        else:
            raise AssertionError(f"Неизвестная инструкция {o:02X} по {p:04X}")
        self.ticks += cycles

    def until(self, address, limit=10_000_000):
        for _ in range(limit):
            if self.pc == address:
                return
            self.step()
        raise AssertionError(f"Не достигнут адрес {address:04X}, PC={self.pc:04X}")

    def call(self, address):
        self.push(0)
        self.pc = address
        self.until(0)


def main():
    original = (ROOT / "assets/games/Almaz.rks").read_bytes()
    rebuilt = (ROOT / "bin/Almaz-original.rks").read_bytes()
    assert sha256(original).hexdigest() == "62170eb15cd668cb6f8dd3ca7a745d63b4549adc249fd3439fc8e6632ed66397"
    assert original == rebuilt, "RKS не совпадает с оригиналом"
    payload = original[4:-2]
    assert payload == (ROOT / "bin/Almaz-original.bin").read_bytes()
    symbols = dict((name, int(value, 16)) for name, value in re.findall(
        r"^(\w+): EQU 0x([0-9A-Fa-f]+)$",
        (ROOT / "bin/Almaz-original.sym").read_text(), re.M))
    for name, address in {"game_entry": 0x1EFD, "game_main": 0x2000,
                          "v_level_pointer": 0x26AE, "t_level": 0x26B0,
                          "v_map": 0x2830, "t_tiles": 0x2B00,
                          "t_explosion_first": 0x2D00, "game_end": 0x3100}.items():
        assert symbols[name] == address, name
    cpu = CPU(payload)
    cpu.pc = symbols["game_entry"]
    cpu.until(symbols["game_step"])
    assert cpu.bios[0xC81B] == 0, "Первый запуск неожиданно ждал клавишу"
    assert cpu.mem[0x29DF] == 0
    initial = bytes(cpu.mem)
    report = {"sha256": sha256(rebuilt).hexdigest(), "first_start_without_key": True,
              "timings": [], "keys": []}
    for digit in (0, 1, 5, 9):
        cpu = CPU(payload, key=0x30+digit)
        cpu.pc = symbols["speed_wait"]
        cpu.mem[0x29DF] = 0xFF
        cpu.until(symbols["level_column"])
        assert cpu.mem[0x29DF] == digit*5
        cpu = CPU(payload)
        cpu.mem[:] = initial
        cpu.mem[0x29DF] = digit*5
        cpu.call(symbols["animation_delay"])
        ticks, writes = cpu.ticks, cpu.screen_writes
        # Один и тот же путь выполняется 31 раз перед игрой.
        report["timings"].append({"digit": digit, "delay_ticks": ticks,
            "delay_ms_at_2mhz": ticks/2000, "screen_writes": writes,
            "intro_ticks": 31*ticks+2597, "intro_ms_at_2mhz": (31*ticks+2597)/2000})
        cpu = CPU(payload)
        cpu.mem[:] = initial
        cpu.mem[0x29DF] = digit*5
        cpu.pc = symbols["game_step"]
        cpu.step()
        cpu.until(symbols["game_step"])
        report["timings"][-1]["first_step_ticks_without_bios"] = cpu.ticks
        report["timings"][-1]["first_step_ms_without_bios"] = cpu.ticks/2000
    for key, dx, dy in ((0x1A, 0, 1), (0x19, 0, -1), (0x18, 1, 0),
                        (0x08, -1, 0), (0x1B, 0, 0), (0x1F, 0, 0), (0x20, 0, 0)):
        cpu = CPU(payload, key)
        cpu.mem[:] = initial
        # Свободная площадка изолирует семантику клавиш от столкновений.
        cpu.mem[0x2830:0x29B0] = bytes([14])*384
        cpu.mem[0x29B9:0x29BB] = bytes([12, 8])
        cpu.pc = symbols["game_step"]
        cpu.until(symbols["enemies_update"])
        assert tuple(cpu.mem[0x29B9:0x29BB]) == (12+dx, 8+dy)
        report["keys"].append({"code": key, "dx": dx, "dy": dy})
    # Прямой прогон стартовой паузы подтверждает формулу, включая её обвязку.
    cpu = CPU(payload)
    cpu.mem[:] = initial
    cpu.call(symbols["intro_animation"])
    assert cpu.ticks == report["timings"][0]["intro_ticks"]
    cpu = CPU(payload)
    cpu.pc = symbols["game_over_wait"]
    cpu.until(symbols["game_restart"])
    report["game_over_wait_ticks"] = cpu.ticks
    for name in ("death_by_enemy", "death_by_stone"):
        cpu = CPU(payload)
        cpu.mem[:] = initial
        cpu.mem[0x29DB] = 0
        cpu.pc = symbols[name]
        cpu.until(symbols["game_over"])
        report[name+"_ticks"] = cpu.ticks
    output = ROOT / ".codex-tmp/almaz-source-checks.json"
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))
    print(f"OK: точный RKS, адреса, ввод, старт и тайминги; {output}")


if __name__ == "__main__":
    main()
