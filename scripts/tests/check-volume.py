"""Проверка машинного кода диспетчера томов без запуска ОС и эмулятора ПК.

Из source/dos собрать DOSChecked.asm с --sym=../../bin/DOS-volume.sym
и --raw=../../bin/DOS-volume-test.sys, затем выполнить этот сценарий.
Узкая модель команд 8080 исполняет диспетчер; файловые и аппаратные
обработчики заменяются контролируемыми процедурами. Полную FAT она не проверяет.
"""
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[2]
BINARY = (ROOT / 'bin/DOS-volume-test.sys').read_bytes()
SYMBOLS = dict((name, int(value, 16)) for name, value in re.findall(
    r'(?m)^(\w+): EQU 0x([0-9A-Fa-f]+)',
    (ROOT / 'bin/DOS-volume.sym').read_text()))


class CPU:
    def __init__(self):
        self.m = bytearray(65536)
        self.m[0xc000:0xc000+len(BINARY)] = BINARY
        self.r = [0x12, 0x34, 0x56, 0x78, 0x23, 0x45, 0, 0x67]
        self.f = 0xd7
        self.sp = 0x8f00
        self.pc = 0
        self.hooks = {}
        self.calls = []

    def word(self, address):
        return int.from_bytes(self.m[address:address+2], 'little')

    def setword(self, address, value):
        struct.pack_into('<H', self.m, address, value & 65535)

    def pair(self, index):
        return self.sp if index == 3 else self.r[index*2]*256+self.r[index*2+1]

    def setpair(self, index, value):
        if index == 3:
            self.sp = value & 65535
        else:
            self.r[index*2:index*2+2] = [value >> 8 & 255, value & 255]

    def value(self, index):
        return self.m[self.pair(2)] if index == 6 else self.r[index]

    def store(self, index, value):
        if index == 6:
            self.m[self.pair(2)] = value & 255
        else:
            self.r[index] = value & 255

    def flags(self, value, carry=0, aux=0):
        value &= 255
        self.f = (value & 128) | (64 if value == 0 else 0) | aux | (
            4 if value.bit_count() % 2 == 0 else 0) | 2 | carry

    def condition(self, index):
        return bool([not self.f & 64, self.f & 64, not self.f & 1, self.f & 1,
                     not self.f & 4, self.f & 4, not self.f & 128, self.f & 128][index])

    def push(self, value):
        self.sp -= 2
        self.setword(self.sp, value)

    def ret(self):
        self.pc = self.word(self.sp)
        self.sp += 2

    def run(self, entry):
        self.pc = entry
        original_sp = self.sp
        self.push(0xffff)
        for _ in range(10000):
            if self.pc == 0xffff:
                assert self.sp == original_sp, 'Нарушен баланс стека'
                return
            if self.pc in self.hooks:
                self.hooks[self.pc](self)
                self.ret()
                continue
            address = self.pc
            op = self.m[self.pc]
            self.pc += 1
            def byte():
                v = self.m[self.pc]
                self.pc += 1
                return v
            def word():
                v = self.word(self.pc)
                self.pc += 2
                return v
            if op & 0xcf == 1:
                self.setpair(op >> 4 & 3, word())
            elif op & 0xcf == 3:
                p = op >> 4 & 3
                self.setpair(p, self.pair(p)+1)
            elif op & 0xcf == 9:
                v = self.pair(2)+self.pair(op >> 4 & 3)
                self.f = (self.f & ~1) | int(v > 65535)
                self.setpair(2, v)
            elif op & 0xc7 == 6:
                self.store(op >> 3 & 7, byte())
            elif op & 0xc7 == 5:
                i = op >> 3 & 7
                v = self.value(i)-1
                old = self.f & 1
                self.flags(v, old)
                self.store(i, v)
            elif 0x40 <= op <= 0x7f:
                self.store(op >> 3 & 7, self.value(op & 7))
            elif 0x80 <= op <= 0x87:
                v = self.r[7]+self.value(op & 7)
                self.flags(v, int(v > 255))
                self.r[7] = v & 255
            elif 0xb0 <= op <= 0xb7:
                self.r[7] |= self.value(op & 7)
                self.flags(self.r[7])
            elif op == 0xe6:
                self.r[7] &= byte()
                self.flags(self.r[7])
            elif op == 0xd6:
                v = self.r[7]-byte()
                self.flags(v, int(v < 0))
                self.r[7] = v & 255
            elif op == 0xfe or 0xb8 <= op <= 0xbf:
                v = self.r[7]-(byte() if op == 0xfe else self.value(op & 7))
                self.flags(v, int(v < 0))
            elif op == 0x0f:
                a = self.r[7]
                self.r[7] = a >> 1 | (a & 1) << 7
                self.f = self.f & ~1 | (a & 1)
            elif op == 0x3a:
                self.r[7] = self.m[word()]
            elif op == 0x32:
                self.m[word()] = self.r[7]
            elif op == 0x2a:
                self.setpair(2, self.word(word()))
            elif op == 0x22:
                self.setword(word(), self.pair(2))
            elif op == 0xeb:
                v = self.pair(1)
                self.setpair(1, self.pair(2))
                self.setpair(2, v)
            elif op & 0xcf == 0xc5:
                p = op >> 4 & 3
                self.push(self.r[7]*256+self.f if p == 3 else self.pair(p))
            elif op & 0xcf == 0xc1:
                p = op >> 4 & 3
                v = self.word(self.sp)
                self.sp += 2
                if p == 3:
                    self.r[7], self.f = v >> 8, v & 255
                else:
                    self.setpair(p, v)
            elif op == 0xcd:
                v = word()
                self.push(self.pc)
                self.pc = v
            elif op == 0xc3:
                self.pc = word()
            elif op & 0xc7 == 0xc2:
                v = word()
                if self.condition(op >> 3 & 7):
                    self.pc = v
            elif op == 0xc9:
                self.ret()
            elif op & 0xc7 == 0xc0:
                if self.condition(op >> 3 & 7):
                    self.ret()
            elif op == 0x37:
                self.f |= 1
            else:
                raise AssertionError(f'Команда {op:02X} по адресу {address:04X} не входит в модель проверки')
        raise AssertionError('Превышен предел выполнения проверки')


def register(c, drive=6, pointer=0x8000, context=0x8123):
    c.m[pointer:pointer+4] = b'FS\x01\x0f'
    for index in range(15):
        c.setword(pointer+4+index*2, 0x9000+index*3)
    c.r[7] = drive
    c.setpair(2, pointer)
    c.setpair(1, context)
    before = c.r[:6]
    c.run(0xc890)
    assert c.r[:6] == before


def main():
    assert BINARY == (ROOT / 'bin/DOS.sys').read_bytes(), 'Проверяется другая сборка DOS'
    operations = [(0xc83f, 'fatFileList'), (0xc845, 'fatFileCreate'),
                  (0xc848, 'fatFileLoad'), (0xc84b, 'fatFileDelete'),
                  (0xc84e, 'fatFileRename'), (0xc851, 'fatFileLoadInfo'),
                  (0xc854, 'fatFileGetSetAddr'), (0xc857, 'fatFileGetSetAttr'),
                  (0xc866, 'fatFileLoad2'), (0xc872, 'fatGetDriveFreeSpace'),
                  (0xc88a, 'fatFileCreatePrepare'), (0xc88d, 'fatFileTransferClusters')]
    for entry, target in operations:
        c = CPU()
        before, flags = c.r[:], c.f
        seen = []
        def handler(cpu):
            seen.append(1)
            assert cpu.r == before and cpu.f == flags, target
            cpu.r[7], cpu.f = 0x81, 3
        c.hooks[SYMBOLS[target]] = handler
        c.run(entry)
        assert seen == [1] and (c.r[7], c.f) == (0x81, 3)
    print('OK: все 12 прежних файловых входов передают регистры/флаги и результат FAT без изменений')

    c = CPU()
    register(c)
    assert not c.f & 1
    assert c.word(SYMBOLS['v_volumes']+12) == 0x8000
    assert c.word(SYMBOLS['v_volumeContexts']+12) == 0x8123
    assert c.word(SYMBOLS['v_volumes']+2) == SYMBOLS['fatVolumeDriver']
    c.r[7] = 6
    bc = c.pair(0)
    c.run(0xc893)
    assert (c.pair(2), c.pair(1), c.pair(0)) == (0x8000, 0x8123, bc)

    mounted = []
    def mounted_handler(cpu):
        mounted.append(cpu.word(SYMBOLS['v_volumeContext']))
        cpu.m[SYMBOLS['v_diskInfo']+34] = 2
        cpu.f &= ~1
    c.hooks[0x9000] = mounted_handler
    c.hooks[SYMBOLS['resetSectorCache']] = lambda cpu: None
    c.r[7] = 6
    before = c.r[:6]
    c.run(SYMBOLS['fileSelectDrive'])
    assert mounted == [0x8123] and c.r[:6] == before
    assert c.m[SYMBOLS['v_drive']] == 6
    assert c.word(SYMBOLS['j_diskDriver']+1) == SYMBOLS['diskDriverDummy']
    # Повторный mount и замена активного тома сохраняют регистры даже при
    # разрушающем обработчике, возвращая его CF.
    def clobber_mount(cpu):
        cpu.r[:6] = [1, 2, 3, 4, 5, 6]
        cpu.r[7], cpu.f = 0x82, 3
    c.hooks[0x9000] = clobber_mount
    before = c.r[:6]
    c.run(0xc896)
    assert c.r[:6] == before and (c.r[7], c.f) == (0x82, 3)
    register(c)
    assert c.r[:6] == before and (c.r[7], c.f) == (0x82, 3)
    c.hooks[0x9000] = mounted_handler
    c.hooks[0x9000+14*3] = lambda cpu: cpu.calls.append('flush')
    c.run(0xc899)
    assert c.calls == ['flush']
    c.calls.clear()
    c.hooks[0x9003] = lambda cpu: cpu.calls.append('list')
    c.run(0xc83f)
    assert c.calls == ['list']

    c.setpair(2, 0x8200)
    c.setpair(1, 0x8300)
    c.m[0x8200:0x820b] = b'G:HELLO.TXT'
    before_bc = c.pair(0)
    def prepare(cpu):
        assert (cpu.pair(2), cpu.pair(1), cpu.pair(0)) == (0x8202, 0x8300, before_bc)
        cpu.calls.append('prepare')
    c.hooks[0x9000+11*3] = prepare
    c.run(0xc85a)
    assert c.calls[-1] == 'prepare'
    print('OK: регистрация G:, контекст, выбор носителя, mount, файловый вызов и отделение префикса диска')

    for drive in (8, 255):
        c = CPU()
        old = bytes(c.m[SYMBOLS['v_volumes']:SYMBOLS['v_volumes']+32])
        register(c, drive=drive)
        assert c.f & 1 and bytes(c.m[SYMBOLS['v_volumes']:SYMBOLS['v_volumes']+32]) == old
    for offset, value in ((0, 0), (1, 0), (2, 2), (3, 14), (4, 0)):
        c = CPU()
        register(c)
        c.m[0x8000+offset] = value
        if offset == 4:
            c.m[0x8005] = 0
        c.r[7] = 5
        c.setpair(2, 0x8000)
        c.run(0xc890)
        assert c.f & 1 and c.word(SYMBOLS['v_volumes']+10) == SYMBOLS['fatVolumeDriver']
    print('OK: отклонены неверный диск, magic, версия, число функций и нулевой обработчик')


if __name__ == '__main__':
    main()
