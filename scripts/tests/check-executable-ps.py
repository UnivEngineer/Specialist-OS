"""Независимое сравнение конвертера PowerShell с эталонным чтением/упаковкой Python."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'scripts/images'))
from executable import Image, convert, encode


def main():
    with tempfile.TemporaryDirectory() as temporary:
        folder = Path(temporary)
        cases = []
        def add(path, target, load=0):
            destination = folder/f'output-{len(cases)}.bin'
            cases.append(dict(source=str(path), target=target, load=load, output=str(destination)))
        for size in (1,239,240,241,255,256,257,4096,65535,65536):
            data = bytes((i*71+19)&255 for i in range(size))
            raw = folder/f'raw-{size}.bin'
            raw.write_bytes(data)
            for target in ('com','exe','rks'):
                add(raw,target)
            image = Image(data)
            for kind in ('rks','com','exe'):
                if size==65536 and kind!='rks':
                    continue
                path=folder/f'image-{size}.{kind}'
                packed=encode(image,kind)
                path.write_bytes(packed)
                for target in ('com','exe','rks'):
                    add(path,target)
                if size in (1,257):
                    for offset in range(16 if kind!='rks' else 4):
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
