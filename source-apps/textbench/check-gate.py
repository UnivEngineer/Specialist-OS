"""Независимая проверка тактов и сохранения регистров шлюза на модели z88dk 8080.

Плоская память: записи портов здесь не переключают страницы. Само переключение
и экран проверяются отдельно в benchmark-text-bank.py на модели MX2 Emu80.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ticks', required=True, type=Path)
    p.add_argument('--out', type=Path, default=ROOT/'docs/bench/text-bank-001/cpu')
    args = p.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    gate = bytearray((ROOT/'bin/GATE.i80').read_bytes())
    gate[16:18] = bytes.fromhex('80 F8')
    assert gate[15] == 0xCD
    results = []
    for entry in (0xF880, 0xFFC0):
        memory = bytearray(65536)
        memory[0xFFC0:0xFFE0] = gate
        memory[0xF880] = 0xC9
        # Устанавливаем AF через POP, затем проверяем сохранённые регистры в дампе.
        code = bytes.fromhex('31 00 80 21 97 A5 E5 F1 21 BC 9A 11 78 56 01 34 12 CD')
        code += entry.to_bytes(2, 'little') + bytes.fromhex('22 02 70 F5 E1 22 00 70 C3 00 02')
        memory[0x100:0x100+len(code)] = code
        src = args.out/f'{entry:04x}.bin'
        dst = args.out/f'{entry:04x}-out.bin'
        src.write_bytes(memory)
        run = subprocess.run([str(args.ticks), '-m8080', '-pc', '100', '-end', '200',
                              '-counter', '10000', '-output', str(dst), str(src)],
                             check=True, capture_output=True, text=True, timeout=15)
        output = dst.read_bytes()
        assert output[0x7000:0x7002] == bytes.fromhex('97 A5'), output[0x7000:0x7002].hex()
        assert output[0x7002:0x7004] == bytes.fromhex('BC 9A'), 'HL changed'
        assert output[65544:65546] == bytes.fromhex('00 80'), 'SP changed'
        assert output[65538:65540] == bytes.fromhex('34 12'), 'BC changed'
        assert output[65548:65550] == bytes.fromhex('78 56'), 'DE changed'
        log = run.stdout + run.stderr
        found = re.search(r'^\s*(\d+)\s*$', log, re.M)
        if not found:
            raise RuntimeError(log)
        results.append(dict(entry=f'{entry:04X}', ticks=int(found[1]), log=log.strip()))
    assert results[1]['ticks'] - results[0]['ticks'] == 166, results
    buffer_cases=[]
    for function, length, expected_calls in [(0,65,1),(1,0,0),(1,20,20),(1,21,0),(2,0,0),(3,0,0)]:
        memory=bytearray(65536)
        memory[0xFFC0:0xFFE0]=(ROOT/'bin/BUFFERGATE.i80').read_bytes()
        dispatch=(ROOT/'bin/DISPATCH.i80').read_bytes()
        memory[0xF880:0xF880+len(dispatch)]=dispatch
        # BIOS-заглушка считает вызовы, сохраняя регистры и флаги.
        memory[0xC809:0xC812]=bytes.fromhex('F5 E5 21 10 70 34 E1 F1 C9')
        code=bytes.fromhex('31 00 80 21 97 00 E5 F1 21 BC 9A 11 78 56 01')
        code+=bytes([length,function])+bytes.fromhex('CD C0 FF 22 02 70 F5 E1 22 00 70 C3 00 02')
        memory[0x100:0x100+len(code)]=code
        src=args.out/'buffer.bin';dst=args.out/'buffer-out.bin'
        src.write_bytes(memory)
        subprocess.run([str(args.ticks),'-m8080','-pc','100','-end','200',
                        '-counter','100000','-output',str(dst),str(src)],
                       check=True,capture_output=True,text=True,timeout=15)
        output=dst.read_bytes()
        assert output[0x7000:0x7004]==bytes.fromhex('97 00 BC 9A')
        assert output[65538:65540]==bytes([length,function])
        assert output[65548:65550]==bytes.fromhex('78 56')
        assert output[65544:65546]==bytes.fromhex('00 80')
        assert output[0x7010]==expected_calls,(function,length,output[0x7010])
        buffer_cases.append(dict(function=function,length=length,bios_calls=expected_calls))
    report = dict(model='z88dk 8080, flat RAM; no page mapping', extra_ticks=166,
                  af_bc_de_hl_sp_preserved=True, results=results,buffer_cases=buffer_cases)
    (args.out/'result.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
