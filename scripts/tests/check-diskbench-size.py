"""Размер DISKBEN: десятичный ввод и проверка первого/последнего байта файла."""
import json,re,struct,subprocess,runpy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
s={k:int(v,16) for k,v in re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'source-apps/diskbench/build/DISKBEN.sym').read_text(),re.M)}
bios_s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/BIOS.sym').read_text(),re.M)}
out=ROOT/'docs/diskbench/file-003';out.mkdir(parents=True,exist_ok=True)
code=(ROOT/'bin/DISKBEN.i80').read_bytes();cpu=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')
def run(label,setup):
    mem=bytearray(65536);mem[:len(code)]=code
    runpy.run_path(str(ROOT/'scripts/tests/mem-image.py'))['install'](mem,bios_s)
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
            mem[s['v_pass']]=3;mem[s['v_bufferEndHi']]=s['sectorBuffer']//256+kib*4
            start=s['sectorBuffer'];mem[start:start+kib*1024]=bytes(3^(i>>8)^(i&255) for i in range(start,start+kib*1024))
            if bad is not None:mem[s['sectorBuffer']+bad]^=1
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

# Стаб MXOS_CALL пропускает слово операции, сохраняет HL и возвращает число
# свободных кластеров либо указанную ошибку. Байт 9002h отмечает вызов DOS.
space_cases = [
    (256, 1, 8, 31, 1, 0, s['DOS_ERR_FULL']),
    (256, 1, 8, 32, 1, 0, None),
    (512, 2, 8, 7, 1, 0, s['DOS_ERR_FULL']),
    (512, 2, 8, 8, 1, 0, None),
    (512, 4, 7, 3, 1, 0, s['DOS_ERR_FULL']),
    (512, 4, 7, 4, 1, 0, None),
    (256, 1, 8, 0, 0, s['DOS_ERR_IO'], None),
    (256, 1, 8, 32, 1, s['DOS_ERR_IO'], s['DOS_ERR_IO']),
]
for sector, spc, kib, free, passes, io_error, expected in space_cases:
    def setup(mem):
        mem[s['v_budget']] = passes
        mem[s['v_fileSectors']] = kib * 4
        struct.pack_into('<H', mem, s['v_sectorSize'], sector)
        mem[s['v_spc']] = spc
        at = s['mx_callRegs']
        stub = bytes.fromhex('e32323e33e0132029011') + struct.pack('<H', free)
        stub += bytes((0x3e, io_error, 0x37, 0xc9)) if io_error else bytes.fromhex('afc9')
        mem[at:at+len(stub)] = stub
    value, carry = run('checkFreeSpace', setup)
    assert (carry, value if carry else None) == (expected is not None, expected)
    assert (out/'cpu-out.bin').read_bytes()[0x9002] == bool(passes)
print('DISKBEN free-space preflight: exact fit, shortfall, cluster rounding, read only, I/O error PASS')

# Контейнеры должны занимать только собственные статические области.
main_sym = ROOT/'source-apps/diskbench/build/DISKMAIN.sym'
worker = {k: int(v, 16) for k, v in re.findall(
    r'^(\w+): EQU 0x([0-9A-Fa-f]+)', main_sym.read_text(), re.M
)}
assert s['IMAGE_MEMORY_POLICY'] == worker['IMAGE_MEMORY_POLICY'] == s['IMAGE_POLICY_MANAGED']
assert s['IMAGE_MEM_END'] == worker['entry']
assert worker['IMAGE_MEM_END'] == worker['buffer'] == s['bootSectorBuffer']
assert s['code_end'] <= worker['stats'] < s['IMAGE_MEM_END']
assert worker['stats'] + s['BENCH_MAIN_STATS_SIZE'] == s['IMAGE_MEM_END']
assert s['batchStats'] == worker['buffer']
print('DISKBEN MAIN image ranges: managed controller and worker PASS')
