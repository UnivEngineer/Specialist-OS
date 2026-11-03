"""Проверка чередования кластеров FlashFst; только отдельный Emu80-flash-test."""
import argparse
import json
import re
import runpy
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
helpers = runpy.run_path(str(Path(__file__).with_name('check-copy.py')))
Machine, get_file = helpers['FileMachine'], helpers['get_file']


def symbols(path):
    symbol_file = path.with_suffix('.sym')
    if symbol_file.exists():
        return {name:int(value,16) for name,value in re.findall(
            r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',symbol_file.read_text('utf-8-sig'),re.M)}
    result = {}
    for line in path.read_text(encoding='utf-8-sig').splitlines():
        match = re.match(r'^\s*\d+\+*\s*([0-9A-F]{4})\s+(?:[0-9A-F]{2}\s+)*([\w.]+)[: ]', line)
        if match and '~' not in line:
            result[match[2]] = int(match[1], 16)
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', required=True, type=Path)
    p.add_argument('--report', type=Path, default=ROOT / 'docs/flash/interleave-001/results.json')
    args = p.parse_args()
    if args.runtime.name != 'Emu80-flash-test':
        raise ValueError('Нужен отдельный Emu80-flash-test')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    m = Machine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    ncstate = symbols(ROOT / 'bin/NC.lst')['state']
    nc = (ROOT / 'bin/NC.i80').read_bytes()[:ncstate - SYSTEM['NC_LOAD_ADDR']]
    for _ in range(180):
        m.call('emu_run_for', ms=500)
        if m.read(SYSTEM['NC_LOAD_ADDR'], len(nc)) == nc:
            break
    else:
        raise RuntimeError('NC не загружен')
    m.call('bp_set', addr=0x8100, type='exec')
    syms = symbols(ROOT / 'bin/DOS.lst')
    info = syms['v_diskInfo']
    def select(drive):
        assert not m.subroutine(0xC842, af=drive << 8, de=1)['flags']['C']
    def install(name):
        select(0)
        m.install_flash(name)
        select(7)
    image = args.runtime / 'specialist-os/flash-disk.bin'
    disk = image.read_bytes()
    word = lambda n: struct.unpack_from('<H', disk, n)[0]
    secsize = word(11)
    fatsector = word(14)
    rootsector = fatsector + disk[16] * word(22)
    datastart = rootsector + (word(17) * 32 + secsize - 1) // secsize
    name = b'FSTCHIPSCHK'
    payload = bytes((i * 71 + i // 256) & 255 for i in range(12 * 256))
    desc = bytearray(32)
    desc[:11] = name
    # Адрес буфера передаётся в DE, поле даты доступа остаётся нулевым.
    struct.pack_into('<I', desc, 28, len(payload))
    install('FlashFst')
    m.write(0x7000, desc)
    m.write(0x4000, payload)
    before = m.call('cpu_state')['cpu_ticks']
    result = m.subroutine(0xC845, hl=0x7000, de=0x4000)
    assert not result['flags']['C'], result
    create_ticks = result['cpu_ticks'] - before
    entry, actual = get_file(image.read_bytes(), name)
    assert actual == payload
    disk = image.read_bytes()
    cluster = struct.unpack_from('<H', entry, 26)[0]
    chain = []
    while cluster not in chain:
        chain.append(cluster)
        cluster = struct.unpack_from('<H', disk, fatsector * secsize + cluster * 2)[0]
    sectors = [datastart + c - 2 for c in chain]
    chips = [s // 2048 for s in sectors]
    assert len(chain) == 12
    assert all(b == (a + 1) % 4 for a, b in zip(chips, chips[1:])), (chain, chips)
    assert rootsector < 2048 and datastart < 2048
    assert m.read(symbols(ROOT / 'bin/FlashFst.lst')['v_chipStates'], 16)[::4] == bytes(4)
    # Другой драйвер читает ту же FAT и те же физические сектора.
    for driver in ('Flash', 'FlashFst'):
        install(driver)
        m.write(0x7000, name + bytes(21))
        assert not m.subroutine(0xC851, hl=0x7000)['flags']['C']
        assert not m.subroutine(0xC866, hl=0x7000, de=0x5000)['flags']['C']
        assert m.read(0x5000, len(payload)) == payload
    # Исполняем настоящий F5 NC с установленным FlashFst, включая барьер.
    nclist = (ROOT / 'bin/NC.lst').read_text(encoding='utf-8-sig')
    ncsyms = symbols(ROOT / 'bin/NC.lst')
    operand = re.search(r'21 ([0-9A-F]{2}) ([0-9A-F]{2})\s+ld\s+hl, vars.input', nclist)
    input_addr = int(operand[2] + operand[1], 16)
    font, fontdata = get_file((args.runtime / 'specialist-os/system-flash.bin').read_bytes(), b'FONT    FNT')
    state = ncsyms['state']
    m.write(state + 1, b'\0')
    m.write(state + 4, b'\0')
    m.write(state + 8, b'\0')
    m.write(state + 10, b'\0')
    m.write(input_addr + 64, font[:12] + font[18:20] + font[28:30])
    m.write(input_addr, b'H:FSTNC.FNT\0')
    m.call('bp_set', addr=ncsyms['copyFileShowError'], type='exec')
    copyaddr = ncsyms['copyFileInt']
    m.write(0x8000, bytes((0xCD, copyaddr & 255, copyaddr >> 8, 0xC3, 0, 0x81)))
    m.call('regs_set', pc=0x8000, sp=0x8F00)
    for _ in range(120):
        m.call('emu_run_for', ms=500)
        cpustate = m.call('cpu_state')
        pc = cpustate['registers']['pc']['dec']
        if pc == 0x8100:
            assert not cpustate['flags']['C']
            break
        if pc == ncsyms['copyFileShowError']:
            raise AssertionError(dict(error=m.read(cpustate['registers']['hl']['dec'], 24).hex(),
                                      input=m.read(input_addr, 64).hex(), info=m.read(info, 39).hex(),
                                      state=m.read(state, 12).hex()))
    else:
        raise RuntimeError('NC copy timeout')
    m.call('bp_clear')
    m.call('bp_set', addr=0x8100, type='exec')
    assert get_file(image.read_bytes(), b'FSTNC   FNT')[1] == fontdata
    assert m.read(symbols(ROOT / 'bin/FlashFst.lst')['v_chipStates'], 16)[::4] == bytes(4)
    # Синтетическая FAT в RAM проверяет полный/почти полный том без тысяч
    # программирований flash. Исполняется настоящий машинный код распределителя.
    original_info = m.read(info, 42)
    read_addr = syms['fatReadCluster']
    original_read = m.read(read_addr, 3)
    # PUSH H; XCHG; DAD H; LXI D,0; DAD D; MOV E,M; INX H; MOV D,M; POP H; RET.
    m.write(0x8300, bytes.fromhex('e5eb29110000195e2356e1c9'))
    m.write(read_addr, bytes((0xC3, 0, 0x83)))
    # Адреса полей новой DISK_INFO находим в собранном коде.
    # Их адреса берём из команды SHLD листинга, а не из предположения о версии ABI.
    listing = (ROOT / 'bin/DOS.lst').read_text(encoding='utf-8-sig')
    def fieldaddr(field):
        pat = r'^\s*\d+\+*\s*[0-9A-F]{4}\s+22 ([0-9A-F]{2}) ([0-9A-F]{2})\s+ld\s+\(v_diskInfo\.' + field + r'\), hl'
        r = re.search(pat, listing, re.M)
        return int(r[2] + r[1], 16)
    count = 8192 - datastart
    last = count + 1
    m.write(fieldaddr('dataClusters'), struct.pack('<H', count))
    m.write(fieldaddr('allocationStep'), struct.pack('<H', 2048))
    def table(free):
        b = bytearray(b'\xff\xff' * (last + 1) + bytes(512))
        for c in free:
            struct.pack_into('<H', b, c * 2, 0)
        m.write(0, b)
    def find(previous, free):
        table(free)
        m.write(syms['v_findCluster'], struct.pack('<H', previous))
        return m.subroutine(syms['fatFindClusterNext'])
    try:
        table([])
        full = m.subroutine(syms['fatFindClusterFirst'])
        assert full['flags']['C']
        lastfree = find(last, [last])
        assert not lastfree['flags']['C'] and lastfree['registers']['de']['dec'] == last
        chip2cluster = 4096 - datastart + 2
        fallback = find(2, [chip2cluster])
        assert not fallback['flags']['C'] and fallback['registers']['de']['dec'] == chip2cluster
        # Линейный драйвер возвращается к началу FAT после последнего кластера.
        m.write(fieldaddr('allocationStep'), bytes(2))
        wrapped = find(last, [2])
        assert not wrapped['flags']['C'] and wrapped['registers']['de']['dec'] == 2
    finally:
        m.write(read_addr, original_read)
        m.write(info, original_info)
    # Измеряем только одинаковый пакет физических записей и финальное ожидание.
    physical_ticks = {}
    for driver in ('Flash', 'FlashFst'):
        install(driver)
        entryaddr = symbols(ROOT / f'bin/{driver}.lst')['Driver']
        start = m.call('cpu_state')['cpu_ticks']
        for n, sector in enumerate(sectors):
            m.write(0x4000, payload[n*256:(n+1)*256])
            assert not m.subroutine(entryaddr, bc=0x0100, de=sector, hl=0x4000)['flags']['C']
        if driver == 'FlashFst':
            assert not m.subroutine(entryaddr, bc=0x0400)['flags']['C']
        physical_ticks[driver] = m.call('cpu_state')['cpu_ticks'] - start
    assert get_file(image.read_bytes(), name)[1] == payload
    fastsyms = symbols(ROOT / 'bin/FlashFst.lst')
    # Занятый чип с несовпадающим DQ7: проверка тайм-аута чтения и барьера.
    for operation in (2, 4):
        statebytes = bytearray(16)
        statebytes[:4] = bytes((1, 0, 0, image.read_bytes()[255] ^ 128))
        m.write(fastsyms['v_chipStates'], statebytes)
        m.write(0x8FF9, b'\0')
        result = m.subroutine(fastsyms['Driver'], bc=operation << 8, de=0, hl=0x4000)
        assert result['flags']['C'] and m.read(0x8FF9, 1) == b'\1'
    # Новый B=5 проверяет запись тем же общим кодом DOS для всех flash-драйверов.
    for name in ('Flash', 'FlashFst'):
        install(name)
        m.write(0x4000, bytes(range(256)))
        assert not m.subroutine(0xC863, bc=0x0500, de=8191, hl=0x4000)['flags']['C']
    select(0)
    m.write(0x4000, bytes(range(256)))
    assert not m.subroutine(0xC863, bc=0x0500, de=2047, hl=0x4000)['flags']['C']
    report = dict(clusters=chain, sectors=sectors, chips=chips,
                  createCpuTicks=create_ticks, physicalWriteCpuTicks=physical_ticks,
                  timeoutReadAndFlush=True, checkedWriteApiTested=True, compatibleReadback=['Flash', 'FlashFst'],
                  metadataChip=0, ncCopyAndFlush=True, fullDiskRejected=True, lastFreeFound=True,
                  fullPreferredChipFallback=True, linearWrap=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
