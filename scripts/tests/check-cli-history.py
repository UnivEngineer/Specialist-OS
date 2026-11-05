"""История CLI на CPU 8080: граница кольца, дубликаты, черновик и длинная строка."""
from pathlib import Path
import re,struct,subprocess

R=Path(__file__).resolve().parents[2]
O=R/'docs/cli/history-2026-11-05/cpu';O.mkdir(parents=True,exist_ok=True)
def symbols(name):
    return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/name).read_text(),re.M)}
s=symbols('DOS.sym');c=symbols('CLI.sym');W=lambda v:struct.pack('<H',v)
m=bytearray(65536)
for name,at in [('CLI.i80',0),('DOS.sys',0xC000)]:
    b=(R/'bin'/name).read_bytes();m[at:at+len(b)]=b
m[s['bios_printChar']]=0xC9
def run(name,end=0x6100):
    global m
    code=b'\x31'+W(s['STACK_ADDR'])+b'\xCD'+W(c[name])+b'\xC3'+W(0x6100)
    m[0x6000:0x6000+len(code)]=code;(O/'in.bin').write_bytes(m)
    subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','6000','-end',f'{end:04x}','-counter','1000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
    out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==end,name
    m=bytearray(out[:65536])
def line(value=None):
    at=s['CMD_LINE']
    if value is not None:
        b=value.encode()+b'\0';m[at:at+len(b)]=b
    return m[at:at+128].split(b'\0')[0].decode()
def navigate(name):
    run(name,c['lineEditLoop']);return line()
for i in range(40):line(f'ECHO {i}');run('cliHistoryAdd')
line('ECHO 39');run('cliHistoryAdd');line('');run('cliHistoryAdd')
assert m[c['cliHistoryCount']]==32
line('DRAFT')
m[c['lineEditBuffer']:c['lineEditBuffer']+2]=W(s['CMD_LINE'])
m[c['lineEditWidth']]=60
for i in reversed(range(8,40)):assert navigate('cliHistoryUp')==f'ECHO {i}'
assert navigate('cliHistoryUp')=='ECHO 8'
for i in range(9,40):assert navigate('cliHistoryDown')==f'ECHO {i}'
assert navigate('cliHistoryDown')=='DRAFT'
assert navigate('cliHistoryDown')=='DRAFT'
long='ECHO '+('X'*122);line(long);run('cliHistoryAdd')
m[c['cliHistoryBrowse']]=0;line('')
assert navigate('cliHistoryUp')==long
assert m[c['lineEditLength']]==127 and m[c['lineEditPosition']]==127
assert m[c['lineEditOffset']]>0
print('CLI history CPU PASS: 32-entry wrap, deduplication, draft, 127-byte command')
