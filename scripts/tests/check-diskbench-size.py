"""Размер DISKBEN: десятичный ввод и проверка первого/последнего байта файла."""
import json,re,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
s={k:int(v,16) for k,v in re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'source-apps/diskbench/build/DISKBEN.sym').read_text(),re.M)}
out=ROOT/'docs/diskbench/file-003';out.mkdir(parents=True,exist_ok=True)
code=(ROOT/'bin/DISKBEN.i80').read_bytes();cpu=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')
def run(label,setup):
    mem=bytearray(65536);mem[:len(code)]=code
    dos=(ROOT/'bin/DOS.sys').read_bytes();mem[0xC000:0xC000+len(dos)]=dos
    setup(mem)
    addr=s[label]
    wrapper=b'\x31\xF0\xBF\x21\x00\x80\xCD'+struct.pack('<H',addr)+bytes.fromhex('F5E1220090C3F0B8')
    mem[0xB800:0xB800+len(wrapper)]=wrapper
    inp=out/'cpu-in.bin';dst=out/'cpu-out.bin';inp.write_bytes(mem)
    subprocess.run([str(cpu),'-m8080','-pc','b800','-end','b8f0','-counter','10000000','-output',str(dst),str(inp)],check=True,capture_output=True,timeout=15)
    data=dst.read_bytes();assert int.from_bytes(data[65542:65544],'little')==0xB8F0
    return data[0x9001],bool(data[0x9000]&1)
texts=['']+[str(i) for i in range(-1,301)]+['0x8','+8','8x',' 8','8 ','00024','0'*15,'9'*15]
for text in texts:
    def setup(mem):mem[0x8000:0x8000+len(text)+1]=text.encode()+b'\0'
    value,carry=run('parse_file_kib',setup)
    word=text.strip();valid=not word or word.isascii() and word.isdecimal() and 1<=int(word)<=24
    assert carry!=valid,(text,value,carry)
    if valid:assert value==int(text.strip() or '8'),(text,value)
for kib in [1,8,9,16,24]:
    for bad in [None,0,255,256,kib*1024-1]:
        def setup(mem):
            mem[s['v_pass']]=3;mem[s['v_bufferEndHi']]=0x20+kib*4
            mem[0x2000:0x2000+kib*1024]=bytes(3^(i>>8)^(i&255) for i in range(0x2000,0x2000+kib*1024))
            if bad is not None:mem[0x2000+bad]^=1
        _,carry=run('file_compare',setup)
        assert carry==(bad is not None),(kib,bad)
report={'inputCases':len(texts),'compareSizesKiB':[1,8,9,16,24],'compareCases':25}
(out/'input-check.json').write_text(json.dumps(report,indent=2)+'\n');print(report)

for kib in [1,8,24]:
    mem=bytearray(65536);mem[:len(code)]=code
    # Вход после сохранения точного размера. Останов до перехода prepare.
    start=s['set_buffer_end'];end=start+6
    assert mem[start:start+3]==bytes([0x7C,0xC6,s['sectorBuffer']//256])
    wrapper=b'\x21'+struct.pack('<H',kib*1024)+b'\xC3'+struct.pack('<H',start)
    mem[0xB800:0xB800+len(wrapper)]=wrapper
    (out/'cpu-in.bin').write_bytes(mem)
    subprocess.run([str(cpu),'-m8080','-pc','b800','-end',f'{end:04x}','-counter','100000','-output',str(out/'cpu-out.bin'),str(out/'cpu-in.bin')],check=True,capture_output=True)
    result=(out/'cpu-out.bin').read_bytes()
    assert result[s['v_bufferEndHi']]==s['sectorBuffer']//256+kib*4
print('DISKBEN exact buffer end: 1/8/24 KiB PASS')
