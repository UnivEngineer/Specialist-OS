"""8080 traces: file palette rules and overlapping colored text copies."""
from pathlib import Path
import runpy

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'.local/nc-color-copy-cpu';OUT.mkdir(exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/tests/benchmark-nc-color-scroll.py'))
n=h['symbols'](ROOT/'bin/NC.sym');b=h['symbols'](ROOT/'bin/BIOS.sym')
W=lambda v:int(v).to_bytes(2,'little')
base=bytearray(65536)
for at,path in [(0,'NC.i80'),(0xc000,'BIOS.sys'),(0xf000,'IO.sys')]:
    data=(ROOT/'bin'/path).read_bytes();base[at:at+len(data)]=data
for ext,attr,color in [('TXT',0,n['COLOR_FILE']),('com',0,n['COLOR_EXECUTABLE']),
                       ('EXE',0,n['COLOR_EXECUTABLE']),('Bat',0,n['COLOR_EXECUTABLE']),
                       ('rks',0,n['COLOR_EXECUTABLE']),('SYS',0,n['COLOR_FILE']),
                       ('dll',0,n['COLOR_FILE']),('FNT',0,n['COLOR_EXECUTABLE']),
                       ('PF ',0,n['COLOR_FILE']),('MON',0,n['COLOR_EXECUTABLE']),
                       ('ZIP',0,n['COLOR_ARCHIVE']),('arj',0,n['COLOR_ARCHIVE']),
                       ('RAR',0,n['COLOR_ARCHIVE']),('LZH',0,n['COLOR_ARCHIVE']),
                       ('ARC',0,n['COLOR_ARCHIVE']),('TAR',0,n['COLOR_ARCHIVE']),
                       ('GZ ',0,n['COLOR_ARCHIVE']),('7Z ',0,n['COLOR_ARCHIVE']),
                       ('   ',0,n['COLOR_FILE']),('ZIP',16,n['COLOR_DIRECTORY']),
                       ('EXE',2,n['COLOR_EXECUTABLE']),('TXT',4,n['COLOR_SYSTEM_FILE']),
                       ('FNT',4,n['COLOR_EXECUTABLE']),('MON',5,n['COLOR_EXECUTABLE']),
                       ('COM',1,n['COLOR_EXECUTABLE']),('SYS',18,n['COLOR_DIRECTORY'])]:
    m=bytearray(base);m[0x8500:0x8510]=b'FILE    '+ext.encode()+bytes([attr,0,0,0,0])
    code=b'\xcd'+W(n['ncConfigInit'])+b'\x21'+W(0x8500)+b'\xcd'+W(n['ncFileColor'])+b'\x32'+W(0x8300)+b'\xc9'
    m[0x8000:0x8000+len(code)]=code
    result=h['execute'](m,0x8000,OUT)[0]
    assert result[0x8300]==color,(ext,attr,result[0x8300],color)
print('File palette: extension/case/attribute precedence and launchable FNT/MON system modules PASS',flush=True)

for width in (6,10,22):
    for mode in ('uniform','grouped','alternating'):
        for direction in ('up','down'):
            rows=19;height=rows*10
            palettes=[n['COLOR_FILE'],n['COLOR_DIRECTORY'],n['COLOR_EXECUTABLE'],n['COLOR_ARCHIVE']]
            table=bytes(palettes[(0 if mode=='uniform' else int(i>=10) if mode=='grouped' else i%4)] for i in range(24))
            si,di=(1,0) if direction=='up' else (0,1)
            src,dst=0x9109+si*10,0x9109+di*10
            m=bytearray(base);m[0x8100:0x8118]=table
            attrs=bytearray([0xf1])*12288
            for x in range(48):
                for y in range(256):
                    m[0x9000+x*256+y]=0 if (y-9)%10 in (0,9) else (x*19+y*37)&255
            for x in range(width):
                for row in range(24):
                    for y in range(10):attrs[(1+x)*256+9+row*10+y]=table[row]
            wanted=bytearray(attrs);expected=bytearray(m[0x9000:0xc000])
            for x in range(width):
                for row in range(rows):
                    for y in range(1,9):
                        at=dst-0x9000+x*256+row*10+y
                        expected[at]=m[src+x*256+row*10+y]
                        wanted[at]=table[si+row]
            request=W(src)+W(dst)+bytes([width,height])+W(0x8100+si)+W(0x8100+di)
            m[0x8200:0x820a]=request
            code=b'\x21'+W(0x8200)+b'\xcd'+W(n['uiCopyTextRectColors'])+b'\xc9'
            m[0x8000:0x8000+len(code)]=code
            result,cycles,trace,initial=h['execute'](m,0x8000,OUT,trace=True)
            assert result[0x9000:0xc000]==expected,(width,mode,direction,'bitmap')
            updated=bytearray(table);updated[di:di+rows]=table[si:si+rows]
            assert result[0x8100:0x8118]==updated,(width,mode,direction,'palette overlap')
            h['replay_trace'](trace,initial,attrs,result,wanted)
            print(width,mode,direction,f'{cycles/2000:.3f} ms: bitmap, palette overlap, every VRAM write and SP PASS',flush=True)
print('Colored text copy: 18 actual 8080 traces, zero color-register reads PASS')
