"""8080 metadata adapters: register/flag preservation, error normalization and mount.

FAT handlers are replaced by controlled hooks. Real FAT I/O is covered by
check-file-handle.py and the visible banked-DOS integration tests.
"""
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[2]
BINARY = (ROOT / 'bin/DOS.native.sys').read_bytes()
SYMBOLS = dict((name, int(value, 16)) for name, value in re.findall(
    r'(?m)^(\w+): EQU 0x([0-9A-Fa-f]+)',
    (ROOT / 'bin/DOS.sym').read_text()))


class CPU:
    def __init__(self):
        self.m = bytearray(65536)
        bios=(ROOT/'bin/BIOS.sys').read_bytes()
        self.m[0xc000:0xc000+len(bios)]=bios
        base=SYMBOLS['DOS_CODE_START']
        self.m[base:base+len(BINARY)]=BINARY
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
            elif op == 0xaf:
                self.r[7]=0
                self.flags(0)
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
            elif op in (0x02,0x12):
                self.m[self.pair(op>>4)] = self.r[7]
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
            elif op & 0xc7 == 0xc4:
                v=word()
                if self.condition(op >> 3 & 7):
                    self.push(self.pc)
                    self.pc=v
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


def main():
    pairs=[('fileList','fatFileList'),('fileDelete','fatFileDelete'),
           ('fileRename','fatFileRename'),('fileLoadInfo','fatFileLoadInfo'),
           ('fileGetSetAttr','fatFileGetSetAttr'),('getDriveFreeSpace','fatGetDriveFreeSpace'),
           ('fileCreatePrepare','fatFileCreatePrepare'),('flushVolume','fatVolumeFlush'),
           ('volumePrepareName','fatFileNamePrepare'),('volumeMountDispatch','fatReadBootSector')]
    cases=0
    for entry,target in pairs:
        for carry,error in [(False,0),(True,0),(True,6)]:
            c=CPU();before=c.r[:];flags=c.f;seen=[]
            c.m[SYMBOLS['v_dosError']]=8
            def handler(cpu):
                assert cpu.r==before and cpu.f==flags,(entry,cpu.r,before,cpu.f,flags)
                assert cpu.m[SYMBOLS['v_dosError']]==0
                seen.append(target)
                cpu.m[SYMBOLS['v_dosError']]=error
                cpu.r[7]=0x81;cpu.f=2|int(carry)
            c.hooks[SYMBOLS[target]]=handler;c.run(SYMBOLS[entry])
            assert seen==[target]
            assert c.r[:6]==before[:6] and (c.r[7],c.f)==(0x81,2|int(carry))
            assert c.m[SYMBOLS['v_dosError']]==((error or 1) if carry else 0)
            cases+=1
    for entry,invalidate in [('selectVolume',False),('mountVolume',True)]:
        c=CPU();before=c.r[:6];seen=[]
        c.hooks[SYMBOLS['invalidateDriveCache']]=lambda cpu:seen.append('invalidate')
        c.hooks[SYMBOLS['selectCacheForDrive']]=lambda cpu:seen.append('cache')
        def mounted(cpu):
            seen.append('mount');cpu.r[:6]=[1,2,3,4,5,6];cpu.r[7]=0x82;cpu.f=3
            cpu.m[SYMBOLS['v_dosError']]=6
        c.hooks[SYMBOLS['fatReadBootSector']]=mounted;c.run(SYMBOLS[entry])
        assert seen==(['invalidate'] if invalidate else [])+['cache','mount']
        assert c.r[:6]==before and (c.r[7],c.f)==(0x82,3)
    for name in ['fileCreate','fileLoad2','fatFileCreate','fatFileLoad2',
                 'fatFileGetSetAddr','installVolume','getVolume','v_volumeTable','v_volumes']:
        assert name not in SYMBOLS,name
    c=CPU()
    name='тест.жя'.encode('koi8-r')
    c.m[0x8000:0x8000+len(name)+1]=name+b'\0'
    c.setpair(2,0x8000)
    c.setpair(1,0x8080)
    c.run(SYMBOLS['volumePrepareName'])
    assert not c.f&1
    assert c.m[0x8080:0x808b]=='тест    жя '.encode('koi8-r')
    print('FAT adapters PASS',cases,'register/status cases; select/mount; KOI8-R names; retired paths absent')

if __name__=='__main__':main()
