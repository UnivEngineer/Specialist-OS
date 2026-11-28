"""8080: reclaim MAIN module code/BSS, preserve disjoint CLI and EXEC owner."""
from pathlib import Path
from dos_state import initialize_dos_state
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/main-module-claim'
W = lambda value: int(value).to_bytes(2, 'little')


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    s = {key: int(value, 16) for key, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
        (ROOT / 'bin/banked/KERNEL.sym').read_text('utf-8-sig'), re.M)}
    seed = bytearray(65536)
    binary = (ROOT / 'bin/DOS.sys').read_bytes()
    seed[0x100:0x100+len(binary)] = binary
    initialize_dos_state(seed, s)
    seed[s['klReclaim']:s['klReclaim']+2] = b'\xaf\xc9'
    seed[s['memoryPublishTop']] = 0xc9 # Ceiling itself is exercised below.
    seed[s['MEM_ALLOC_TABLE']:s['MEM_ALLOC_TABLE']+32*6] = bytes([255])*192
    seed[s['MODULE_TABLE']:s['MODULE_TABLE']+8*16] = bytes(128)
    seed[s['residentKeys']:s['residentKeys']+16] = W(0x434e)+bytes(14)
    # CORE owns a graphics child. Its bitmap scratch is a separate allocation.
    records = W(1)+bytes([0, 2])+W(0x100)+W(0)+bytes([0, 0x80])+W(0x100)+W(0x9000)+bytes([0,2])
    records += W(2)+bytes([0, 0])+W(0x3000)+W(s['MEM_ALLOC_TABLE'])
    records += bytes([0x80, 0x81])+W(0x3000)+W(0x3e00)+bytes([1,2])
    seed[s['MODULE_TABLE']:s['MODULE_TABLE']+len(records)] = records
    blocks = b'\0'+W(0x3000)+W(0xe00)+b'\xff'
    blocks += b'\0'+W(0x3e00)+W(0x2200)+b'\x81'
    seed[s['MEM_ALLOC_TABLE']:s['MEM_ALLOC_TABLE']+12] = blocks
    table = s['MODULE_TABLE']
    seed[table+16+14] |= s['IMAGE_OPTION_REUSABLE']
    scenarios = [('disjoint CLI', 0, 0x2000, 4, False, False),
                 ('code overlap', 0x3000, 0x100, 4, False, True),
                 ('BSS overlap', 0x5000, 0x800, 4, False, True),
                 ('adjacent end', 0x6000, 0x100, 4, False, False),
                 ('active code', 0x3000, 0x100, 4, True, False),
                 ('active BSS', 0x5000, 0x100, 4, True, False),
                 ('unknown legacy', 0x100, 0x100, 0, False, True),
                 ('native above old RUN base', 0x8c00, 0x100, 4, False, False),
                 ('upper launcher', s['APP_MEMORY_END'], 0x100, 4, False, True)]
    for name, start, length, flags, active, removed in scenarios:
        memory = bytearray(seed)
        memory[s['v_imageHeader']+s['IMAGE_POLICY']]=2 if flags==4 and start<s['APP_MEMORY_END'] else 0
        memory[s['v_imageHeader']+s['IMAGE_MEMORY_BEGIN']:s['v_imageHeader']+s['IMAGE_MEMORY_BEGIN']+2]=W(start)
        memory[s['v_imageHeader']+s['IMAGE_MEMORY_END']:s['v_imageHeader']+s['IMAGE_MEMORY_END']+2]=W(start+length)
        memory[s['MODULE_TABLE']+16+2] = int(active)
        code = b'\x31'+W(0xef70)+b'\x21'+W(start)+b'\x01'+W(length)
        code += b'\xcd'+W(s['moduleMainClaim'])+b'\xf5\xd1\xeb\x22'+W(0xe702)
        code += b'\x21'+W(s['APP_MEMORY_END'])+b'\xcd'+W(s['memoryMainCeiling'])
        code += b'\x22'+W(0xe700)+b'\xc3'+W(0xe400)
        memory[0xe000:0xe000+len(code)] = code
        (OUT / 'input.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080', '-pc', 'e000', '-end', 'e400', '-counter', '30000000',
                        '-output', str(OUT / 'output.bin'), str(OUT / 'input.bin')],
                       check=True, capture_output=True, timeout=20)
        data = (OUT / 'output.bin').read_bytes()
        assert data[65542:65544] == W(0xe400), (name, 'PC')
        assert data[65544:65546] == W(0xef70), (name, 'SP')
        assert bool(data[0xe702]&1) == active, (name, 'CF')
        assert data[0xe700:0xe702] == W(s['APP_MEMORY_END'] if removed else 0x3000), (name, 'memTop ceiling')
        assert data[s['MODULE_TABLE']:s['MODULE_TABLE']+2] == W(1), (name, 'CORE')
        assert data[s['residentKeys']:s['residentKeys']+2] == W(0x434e), (name, 'key')
        handle = data[s['MODULE_TABLE']+16:s['MODULE_TABLE']+18]
        assert handle == W(0 if removed else 2), (name, 'GFX handle', handle.hex())
        assert (data[s['MEM_ALLOC_TABLE']] == 255) == removed, (name, 'image allocation')
        assert (data[s['MEM_ALLOC_TABLE']+6] == 255) == removed, (name, 'BSS allocation')
        # The separate client BSS can be overwritten while the DLL image survives.
        assert data[table+16+2] == int(active or name == 'BSS overlap'), (name, 'record state')
    # FONT/RUN explicitly claim workspace even if their small image does not
    # overlap GFX. The API must ignore a previous legacy image flag.
    for name, start, length, active, error, removed in [
            ('font workspace', 0, 0x5800, False, False, True),
            ('RUN workspace', 0, 0x8c00, False, False, True),
            ('empty', 0, 0, False, False, False),
            ('public active', 0, 0x5800, True, True, False),
            ('outside MAIN', 0x8b00, 0x200, False, True, False),
            ('wrapped', 0xff00, 0x200, False, True, False)]:
        memory = bytearray(seed)
        memory[s['v_tmpFileDescr']+5] = 0
        memory[s['MODULE_TABLE']+16+2] = int(active)
        code = b'\x31'+W(0xef70)+b'\x21'+W(start)+b'\x01'+W(length)
        code += b'\xcd'+W(s['memoryMainClaim'])+b'\xc3'+W(0xe400)
        memory[0xe000:0xe000+len(code)] = code
        (OUT / 'input.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080', '-pc', 'e000', '-end', 'e400', '-counter', '30000000',
                        '-output', str(OUT / 'output.bin'), str(OUT / 'input.bin')],
                       check=True, capture_output=True, timeout=20)
        data = (OUT / 'output.bin').read_bytes()
        assert data[65542:65544] == W(0xe400), (name, 'PC')
        assert data[65544:65546] == W(0xef70), (name, 'SP')
        assert bool(data[65536]&1) == error, (name, 'CF')
        assert data[s['MODULE_TABLE']:s['MODULE_TABLE']+2] == W(1), (name, 'CORE')
        assert (data[s['MODULE_TABLE']+16:s['MODULE_TABLE']+18] == W(0)) == removed, name
    print('MAIN workspace claim PASS: FONT, RUN, active guard, empty, range/overflow')
    print(f'MAIN module claim PASS: {len(scenarios)} cases, code/BSS, active guard, legacy fallback')

    # Two unrelated dormant DLLs coexist across claims. Slots 0 and 7 exercise
    # both halves of the registry and the wrap in its low address byte.
    reuse_seed = bytearray(seed)
    reuse_seed[table:table+128] = bytes(128)
    for slot, start, cluster in ((0, 0x2000, 0x143), (7, 0x3000, 0x287)):
        at = table+slot*16
        reuse_seed[at+2] = 1
        reuse_seed[at+10:at+14] = W(start)+W(start+0x500)
        at = s['mdReuseKeys']+slot*8
        reuse_seed[at:at+7] = W(cluster)+b'\x00'+bytes([3,5,7,9])
    at = s['cliDriveEpochs']
    reuse_seed[at:at+4] = bytes([3,5,7,9])
    reuse_seed[s['v_drive']] = 0
    for name, cluster, overlap, changed, fill, slot, hit, survivors in [
            ('first hit', 0x143, 0, False, False, 0, 1, (1,1)),
            ('last hit', 0x287, 0, False, False, 7, 1, (1,1)),
            ('prefer empty', 0x456, 0, False, False, 1, 0, (1,1)),
            ('claim one', 0x287, 0x2000, False, False, 7, 1, (0,1)),
            ('stale epoch', 0x143, 0, True, False, 1, 0, (1,1)),
            ('evict vacant', 0x456, 0, False, True, 0, 0, (1,1))]:
        memory = bytearray(reuse_seed)
        if changed:
            memory[s['cliDriveEpochs']+3] ^= 1
        if fill:
            for i in range(1, 7):
                memory[table+i*16:table+i*16+2] = W(i)
        memory[s['v_tmpFileDescr']+26:s['v_tmpFileDescr']+28] = W(cluster)
        memory[s['v_imageHeader']+s['IMAGE_OPTIONS']] = s['IMAGE_OPTION_REUSABLE']
        code = b'\x31'+W(0xef70)
        if overlap:
            code += b'\x21'+W(overlap)+b'\x11'+W(overlap+0x500)+b'\xcd'+W(s['mdReuseInvalidate'])
        code += b'\xcd'+W(s['mdReuseSelect'])+b'\xc3'+W(0xe400)
        memory[0xe000:0xe000+len(code)] = code
        (OUT / 'input.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080', '-pc', 'e000', '-end', 'e400', '-counter', '30000000',
                        '-output', str(OUT / 'output.bin'), str(OUT / 'input.bin')],
                       check=True, capture_output=True, timeout=20)
        data = (OUT / 'output.bin').read_bytes()
        assert data[65542:65546] == W(0xe400)+W(0xef70), name
        assert not data[65536]&1, name
        assert data[65540:65542] == W(table+16*slot), name
        assert data[s['mdReuseHit']] == hit, name
        assert (data[table+2],data[table+7*16+2]) == survivors, name
    print('MAIN DLL reuse PASS: two independent images, stale file, overlapping claim and slot priority')


if __name__ == '__main__':
    main()
