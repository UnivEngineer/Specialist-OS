"""8080: bare drives preserve their saved directory; root aliases stay quiet."""
from pathlib import Path
from dos_state import initialize_dos_state
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/cli-drive-context-cpu'
W = lambda n: n.to_bytes(2, 'little')


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    symbols = {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([\da-fA-F]+)',
        (ROOT/'bin/banked/KERNEL.sym').read_text(), re.M)}
    base = bytearray(65536)
    kernel = (ROOT/'bin/banked/KERNEL.bin').read_bytes()
    base[256:256+len(kernel)] = kernel
    initialize_dos_state(base, symbols)
    # Only the two context-changing backends are replaced. Parsing, QUERY and
    # COMMAND execute the actual linked DOS machine code.
    hooks = ''' ORG 0C000h
drive:
 ld (0A002h),a
 ld hl,0A000h
 inc (hl)
 xor a
 ret
root:
 ld de,0A100h
copy:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 or a
 jp nz,copy
 ld hl,0A001h
 inc (hl)
 xor a
 ret
'''
    (OUT/'hooks.asm').write_text(hooks)
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080',
                    '--raw='+str(OUT/'hooks.bin'), '--sym='+str(OUT/'hooks.sym'),
                    str(OUT/'hooks.asm')], check=True, capture_output=True)
    hook_symbols = {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([\da-fA-F]+)', (OUT/'hooks.sym').read_text(), re.M)}
    data = (OUT/'hooks.bin').read_bytes()
    base[0xc000:0xc000+len(data)] = data
    for original, hook in [('fileChangeDrive', 'drive'), ('directoryPathUnchecked', 'root')]:
        addr = symbols[original]
        base[addr:addr+3] = b'\xc3'+W(hook_symbols[hook])
    cases = [('H:', 'drive'), ('h: ', 'drive'), ('H:\\', 'root'),
             ('H:/', 'root'), ('  h:\\  ', 'root'), ('A:/', 'root'),
             ('H:\\FILE.COM', 'external'), ('H:/FOLDER', 'external')]
    for command, kind in cases:
        for query in (True, False):
            memory = bytearray(base)
            memory[0x8200:0x8200+len(command)+1] = command.encode()+b'\0'
            entry = symbols['cliQueryCommand' if query else 'cliInteractiveCommand']
            code = (b'\x31'+W(0x8b00)+b'\x21'+W(0x8200)+b'\xcd'+W(entry)+
                    b'\xf5\xe1\x22'+W(0xa200)+b'\xc3'+W(0x80ff))
            memory[0x8000:0x8000+len(code)] = code
            (OUT/'input.bin').write_bytes(memory)
            subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                            '-m8080', '-pc', '8000', '-end', '80ff', '-counter', '1000000',
                            '-output', str(OUT/'output.bin'), str(OUT/'input.bin')],
                           check=True, capture_output=True)
            result = (OUT/'output.bin').read_bytes()
            assert int.from_bytes(result[65542:65544], 'little') == 0x80ff
            assert result[0xa200]&1 == 0, (command, query, 'error')
            external = kind == 'external'
            assert result[0xa201] == (255 if external else 0), (command, query)
            assert result[symbols['cliOutput']] == (1 if external else 4), (command, query)
            assert result[0xa000:0xa002] == bytes([
                int(not query and kind == 'drive'), int(not query and kind == 'root')]), (command, query)
            if not query and kind == 'drive':
                assert result[0xa002] == 7
            if not query and kind == 'root':
                assert result[0xa100:0xa180].split(b'\0')[0] == command.strip().encode()
    print('CLI DRIVE CONTEXT PASS: quiet root aliases, saved-directory selection, side-effect-free QUERY and external paths')


if __name__ == '__main__':
    main()
