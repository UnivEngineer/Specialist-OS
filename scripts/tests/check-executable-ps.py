"""Независимое сравнение конвертера PowerShell с эталонным чтением/упаковкой Python."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'scripts/images'))
from executable import Image, checksum, convert, decode, encode, relocated_body
from cpu import fields_from_symbols, format_cpu


def relocatable_cases(folder, add):
    """Собрать оба режима на трёх базах; передать в оба упаковщика хорошие и повреждённые образы."""
    assembler = ROOT/'tasm/sjasmplus.exe'
    fixture = ROOT/'scripts/tests/fixtures/reloc-image.asm'
    for mode in (0, 1):
        path = folder/f'reloc-{mode}.cpu'
        raw = path.with_suffix('.i80')
        symbols = path.with_suffix('.sym')
        bases = (0, 0x1289, 0x602d) if mode == 0 else (0, 0x1200, 0x6000)
        for base in bases:
            output = folder/f'ref-{mode}-{base}.bin' if base else raw
            proc = subprocess.run([str(assembler), '--i8080', f'-DTEST_HIGH={mode}', f'-DTEST_BASE={base}',
                                   f'--raw={output}', f'--sym={symbols}', fixture.as_posix()],
                                  check=True, capture_output=True)
            assert b'warnings: 0' in proc.stderr, proc.stderr
            meta = fields_from_symbols(symbols.read_text('utf-8'))
            if base:
                assert output.read_bytes()[:int(meta['BODY_SIZE'],16)] == relocated_body(image, base)
                continue
            path.write_text(format_cpu(0x21, 0x22, 4, fields=meta), 'ascii')
            image = decode(path.read_bytes(), 'cpu', path=path)
            assert image.reloc_kind == mode and len(image.relocations) == 5 + mode
            assert image.relocations[-1] + 2 - mode == len(image.data)
        for target in ('dll', 'com', 'exe', 'rks', 'fnt'):
            add(path, target)
        packed = encode(image, 'dll')
        path = folder/f'reloc-{mode}.dll'
        path.write_bytes(packed)
        assert encode(decode(packed,'dll'),'dll') == packed
        for target in ('dll','com','exe','fnt','rks'):
            add(path,target)
        # Независимо пересчитать суммы, чтобы пройти до проверки геометрии.
        def invalid(label, change, *, repair_header=True, repair_table=False):
            broken = bytearray(packed)
            change(broken)
            if repair_table:
                struct.pack_into('<H', broken, 36, checksum(broken[40+len(image.data):]))
            if repair_header:
                broken[30:32] = bytes(2)
                struct.pack_into('<H', broken, 30, checksum(broken[:40]))
            try:
                decode(broken, 'dll')
            except ValueError:
                pass
            else:
                raise AssertionError(f'accepted malformed {label}')
            bad = folder/f'reloc-{mode}-{label}.dll'
            bad.write_bytes(broken)
            add(bad, 'dll')
        for label, offset, value in [('kind',32,2),('alignment',33,16),('reserve',38,1),
                                    ('base',16,1),('byte29',29,1),('headerBytes',27,32),
                                    ('flags',5,3),('count',35,255)]:
            invalid(label, lambda data: data.__setitem__(offset,value))
        invalid('unknown-bss', lambda data: struct.pack_into('<H',data,14,65535))
        invalid('header-checksum',lambda data: data.__setitem__(30, data[30]^1),repair_header=False)
        invalid('body-checksum',lambda data: data.__setitem__(40, data[40]^1))
        invalid('table-checksum',lambda data: data.__setitem__(-1, data[-1]^1))
        invalid('short',lambda data: data.pop())
        invalid('long',lambda data: data.append(0))
        invalid('boundary', lambda data: struct.pack_into('<H',data,len(data)-2,len(image.data)-1+mode),repair_table=True)
        invalid('duplicate',lambda data: data.__setitem__(slice(-2,None),data[-4:-2]),repair_table=True)
        if mode == 0:
            invalid('overlap',lambda data: struct.pack_into('<H',data,len(data)-2,image.relocations[-2]+1),repair_table=True)
        else:
            invalid('subpage',lambda data: data.__setitem__(33,7))
    for flags,ext in ((0,'dll'),(1,'com'),(1,'exe'),(2,'dll')):
        image = Image(b'\xc9',flags=flags,reloc_kind=0)
        path = folder/f'empty-table-{flags}.{ext}'
        path.write_bytes(encode(image,ext))
        assert decode(path.read_bytes(),ext).relocations == ()
        for target in ('com','exe','dll','rks','fnt'):
            add(path,target)
    for index,(fields,body) in enumerate((
        ({'RELOC_KIND':'0','ALIGN_LOG2':'0','BODY_SIZE':'FFD7'},bytes(65535-40)),
        ({'RELOC_KIND':'0','ALIGN_LOG2':'0','BODY_SIZE':'FFD8'},bytes(65536-40)),
        ({'RELOC_KIND':'0','BODY_SIZE':'1'},b'\xc9'),
        ({'RELOC_KIND':'1','ALIGN_LOG2':'7','BODY_SIZE':'1'},b'\xc9'),
        ({'ALIGN_LOG2':'0','BODY_SIZE':'1'},b'\xc9'),
        ({'RELOC_KIND':'0','ALIGN_LOG2':'0','BODY_SIZE':'1'},b'\xc9\0'),
        ({'RELOC_KIND':'0','ALIGN_LOG2':'0','BODY_SIZE':'1','MIN_HEAP':'FFFF'},b'\xc9'),
    )):
        path=folder/f'reloc-cpu-{index}.cpu'
        path.write_text(format_cpu(0,flags=4,fields=fields),'ascii')
        path.with_suffix('.i80').write_bytes(body)
        add(path,'dll')
    for flags in (0,1,2,4):
        path=folder/f'reloc-reusable-{flags}.cpu'
        path.write_text(format_cpu(0,flags=flags,fields={'RELOC_KIND':'0','ALIGN_LOG2':'0',
                                                         'BODY_SIZE':'1','CACHEABLE':'3'}),'ascii')
        path.with_suffix('.i80').write_bytes(b'\xc9')
        add(path,'dll')


def monitor_cases(folder, add):
    for index, (name, fields, expected) in enumerate((
        ('mon2.mon', {}, 'MONITOR2.MON'),
        ('monitor2.mon', {'MODE': 'AUTO'}, 'MONITOR2.MON'),
        ('C:\\DOS\\CUSTOM.MON', {'MODE': 'MON'}, 'C:\\DOS\\CUSTOM.MON'),
        ('m2_d400.mon', {}, ''),
    )):
        cpu = folder/f'monitor-{index}.cpu'
        cpu.write_text(format_cpu(0x1000, monitor=name, fields=fields), 'ascii')
        cpu.with_suffix('.i80').write_bytes(b'\xc9')
        for target in ('com', 'exe', 'rks', 'dll', 'fnt'):
            add(cpu, target)
        packed, _ = convert(cpu, 'exe')
        image = decode(packed, 'exe')
        assert image.monitor == expected
        assert encode(image, 'exe') == packed
        path = cpu.with_suffix('.exe')
        path.write_bytes(packed)
        add(path, 'com')
        if expected:
            broken = bytearray(packed)
            broken[-1] ^= 1
            path = folder/f'monitor-bad-{index}.exe'
            path.write_bytes(broken)
            add(path, 'exe')


def main():
    with tempfile.TemporaryDirectory() as temporary:
        folder = Path(temporary)
        cases = []
        def add(path, target, load=0):
            destination = folder/f'output-{len(cases)}.bin'
            cases.append(dict(source=str(path), target=target, load=load, output=str(destination)))
        relocatable_cases(folder, add)
        monitor_cases(folder, add)
        for size in (1,239,240,241,255,256,257,4096,65535,65536):
            data = bytes((i*71+19)&255 for i in range(size))
            raw = folder/f'raw-{size}.bin'
            raw.write_bytes(data)
            for target in ('com','exe','dll','fnt','rks'):
                add(raw,target)
            image = Image(data)
            for kind in ('rks','com','exe','dll','fnt'):
                if size==65536 and kind!='rks':
                    continue
                path=folder/f'image-{size}.{kind}'
                packed=encode(image,kind)
                path.write_bytes(packed)
                for target in ('com','exe','dll','fnt','rks'):
                    add(path,target)
                if size in (1,257):
                    for offset in range(32 if kind!='rks' else 4):
                        bad=folder/f'bad-{size}-{offset}.{kind}'
                        damaged=bytearray(packed)
                        damaged[offset]^=1
                        bad.write_bytes(damaged)
                        add(bad,'com')
                    bad=folder/f'bad-body-{size}.{kind}'
                    damaged=bytearray(packed)
                    damaged[-1]^=1
                    bad.write_bytes(damaged)
                    add(bad,'exe')
        for index,fields in enumerate([
            'FLAGS=04\nBSS=20\nMEM_BEGIN=80\nMEM_END=200\nMIN_HEAP=100\nSTACK=80\nPOLICY=PRESERVE\nAPI=2\nMODE=MX\nCACHEABLE=1',
            'FLAGS=04\nBSS=FFFF\nPOLICY=MANAGED',
            'FLAGS=04\nMEM_END=100',
            'FLAGS=04\nMEM_BEGIN=101',
            'FLAGS=04\nMODE=5',
            'FLAGS=00\nBSS=FFFF\nPOLICY=EXCLUSIVE\nMODE=STD',
        ]):
            cpu=folder/f'metadata-{index}.cpu';cpu.write_text('0100\n0100\nmxos2.mon\n'+fields+'\n',encoding='ascii');cpu.with_suffix('.i80').write_bytes(b'\xc9')
            for target in ('com','exe','dll','fnt'):add(cpu,target)
        for flags in (1, 2, 3, 4, 5, 6, 7, 8):
            cpu=folder/f'flags-{flags}.cpu'
            cpu.write_text(f'0100\n0100\nmxos2.mon\nFLAGS={flags:02X}\n',encoding='ascii')
            cpu.with_suffix('.i80').write_bytes(b'\xc9')
            for target in ('com','exe','dll','fnt','rks'):
                add(cpu,target)
            if flags in (1,2,4):
                packed=folder/f'flags-{flags}.dll'
                packed.write_bytes(encode(Image(b'\xc9',0x100,0x100,flags),'dll'))
                for target in ('com','exe','dll','fnt','rks'):
                    add(packed,target)
        for text in ('0100\n0100\nmxos2.mon\n',
                     '0100\n0100\nmon2.mon\nNOTE=external legacy monitor\n',
                     '0100\n0100\nmonC000.rom\nFLAGS=00\nCUSTOM=value\n',
                     '0100\n0100\nmxos2.mon\nFLAGS=04\nFLAGS=02\n',
                     '0100\n0100\n\nFLAGS=04\n',
                     '0100\n0100\n04\n',
                     '0100\n0100\nmxos2.mon\nBROKEN\n'):
            cpu=folder/f'descriptor-{len(cases)}.cpu';cpu.write_text(text,encoding='ascii')
            cpu.with_suffix('.i80').write_bytes(b'\xc9');add(cpu,'com')
        raw=folder/'проверка.cpu'
        raw.write_bytes(b'\xc9')
        for target in ('com','exe','rks'):
            add(raw,target,0x4000)
        cpu=folder/'адреса.cpu'
        cpu.write_text('1000\r\n1010\r\n',encoding='ascii')
        cpu.with_suffix('.i80').write_bytes(bytes(257))
        for target in ('com','exe','rks'):
            add(cpu,target)
        for content in (b'',b'1000',b'1000 1000 extra',b'1000 FFFF',b'FFFF FFFF'):
            path=folder/f'bad-cpu-{len(cases)}.cpu'
            path.write_bytes(content)
            path.with_suffix('.i80').write_bytes(bytes(17))
            add(path,'com')
        empty=folder/'empty.bin'
        empty.write_bytes(b'')
        add(empty,'com')
        expected=[]
        for case in cases:
            try:
                packed,_=convert(case['source'],case['target'],case['load'])
            except ValueError:
                packed=None
            expected.append(packed)
        request=folder/'requests.json'
        response=folder/'results.json'
        request.write_text(json.dumps(cases,ensure_ascii=False),encoding='utf-8-sig')
        script=folder/'check.ps1'
        module=str(ROOT/'scripts/images/executable.ps1').replace("'","''")
        script.write_text("""param($Request,$Response)
$ErrorActionPreference='Stop'
Set-StrictMode -Version 2
. '"""+module+"""'
$results=@(foreach ($case in (Get-Content -LiteralPath $Request -Raw -Encoding UTF8 | ConvertFrom-Json)) {
    try {
        $data=Convert-ExecutableFile $case.source $case.target $case.load
        [IO.File]::WriteAllBytes($case.output,$data)
        [pscustomobject]@{ok=$true}
    } catch {
        [pscustomobject]@{ok=$false;error=$_.Exception.Message}
    }
})
[IO.File]::WriteAllText($Response,(ConvertTo-Json -InputObject $results),[Text.Encoding]::UTF8)
""",encoding='utf-8-sig')
        subprocess.run(['powershell.exe','-NoProfile','-File',str(script),str(request),str(response)],check=True)
        results=json.loads(response.read_text('utf-8-sig'))
        accepted=rejected=0
        for case,packed,result in zip(cases,expected,results):
            path=Path(case['output'])
            if packed is None:
                assert not result['ok'] and not path.exists(),(case,result)
                rejected+=1
            else:
                assert result['ok'] and path.read_bytes()==packed,(case,result)
                accepted+=1
        assert len(results)==len(cases)
        report=dict(cases=len(cases),identical=accepted,rejected=rejected,
                    unicode_paths=True,maximum_payload=65536,python_used_by_builder=False)
        output=ROOT/'docs/build/image-ps-001/converter.json'
        output.parent.mkdir(parents=True,exist_ok=True)
        output.write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(report))


if __name__=='__main__':
    main()
