"""Цена чтения битмапы через SP, включая его переключение и восстановление."""
from pathlib import Path
import argparse
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'docs/bench/quad-opt-001/stack')
    parser.add_argument('--ticks', type=Path, default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    results = {}
    for two in (False, True):
        images = []
        for stack in (False, True):
            code = ['org 0100h', 'ld sp,08B00h', 'ld bc,0F03Fh' if two else 'ld bc,00003h', 'ld de,09000h', 'ld hl,03000h', 'call draw', 'jp 08B80h', 'draw:', 'push af', 'push bc', 'push de', 'push hl']
            if stack:
                code += ['ld (source+1),hl', 'ld hl,0', 'add hl,sp', 'ld (restore+1),hl']
                if two:
                    code += ['ld h,b', 'ld l,c']
                code += ['source: ld sp,0']
            for row in range(8):
                if two:
                    if stack:
                        code += ['pop bc', 'ld a,(de)', 'and h', 'or c', 'ld (de),a', 'inc d', 'ld a,(de)', 'and l', 'or b', 'ld (de),a']
                    else:
                        code += ['ld a,(de)', 'and b', 'or (hl)', 'ld (de),a', 'inc l', 'inc d', 'ld a,(de)', 'and c', 'or (hl)', 'ld (de),a']
                    if row != 7:
                        code += ([] if stack else ['inc l']) + ['dec d', 'inc e']
                else:
                    if stack and row % 2 == 0:
                        code += ['pop hl']
                    code += ['ld a,(de)', 'and c', ('or h' if row % 2 else 'or l') if stack else 'or (hl)', 'ld (de),a']
                    if row != 7:
                        code += ([] if stack else ['inc l']) + ['inc e']
            if stack:
                code += ['restore: ld sp,0']
            code += ['pop hl', 'pop de', 'pop bc', 'pop af', 'ret']
            name = f'{"two" if two else "one"}-{"stack" if stack else "ordinary"}'
            asm = args.out / f'{name}.asm'
            binary = args.out / f'{name}.bin'
            asm.write_text('\n'.join(line if ':' in line else '    ' + line for line in code) + '\n', encoding='utf-8')
            subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow', f'--raw={binary}', str(asm)], check=True, capture_output=True)
            m = bytearray(65536)
            data = binary.read_bytes()
            m[0x100:0x100 + len(data)] = data
            m[0x9000:0xc000] = bytes((i * 37) & 255 for i in range(12288))
            m[0x3000:0x3010] = bytes((i * 11) & (15 if two and i % 2 == 0 else 192 if two else 252) for i in range(16))
            source = args.out / 'input.bin'
            target = args.out / 'output.bin'
            source.write_bytes(m)
            result = subprocess.run([str(args.ticks), '-m8080', '-pc', '100', '-end', '8b80', '-counter', '100000', '-output', str(target), str(source)], check=True, capture_output=True, text=True)
            data = target.read_bytes()
            assert data[65544:65546] == bytes.fromhex('00 8B'), 'SP'
            images.append(data[0x9000:0xc000])
            results[name] = dict(ticks=int((result.stdout + result.stderr).strip()), bytes=len(binary.read_bytes()))
        assert images[0] == images[1]
    results['oneStackDelta'] = results['one-stack']['ticks'] - results['one-ordinary']['ticks']
    results['twoStackDelta'] = results['two-stack']['ticks'] - results['two-ordinary']['ticks']
    (args.out / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
