r"""CPU-стенд общего кэша NC; --prepare-only не запускает CPU-стенд.

Проверяет два слота MAIN, перенос соседнего каталога, общие индексы
и точные/томовые уведомления DOS.
"""
import argparse
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--ticks', type=Path, default=Path(
        'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    parser.add_argument('--output', type=Path, default=ROOT/'.local/nc-catalog/cpu')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    s = {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([\dA-Fa-f]+)',
        (ROOT/'bin/NC.sym').read_text(encoding='utf-8'), re.M)}
    nc = (ROOT/'bin/NC.i80').read_bytes()
    bios = (ROOT/'bin/BIOS.sys').read_bytes()
    word = lambda x: x.to_bytes(2, 'little')
    assert s['NC_CATALOG_SLOTS']==2
    pool=0x8c00-s['FILE_CACHE_A']
    physical=[s['FILE_CACHE_A'],s['FILE_CACHE_A']+13*16]

    def slot(drive, directory, count, mode, age):
        return bytes([1, drive])+word(directory)+bytes([count, mode, age, 0])

    def memory():
        m = bytearray(65536)
        m[:len(nc)] = nc
        m[0xC000:0xC000+len(bios)] = bios
        m[s['ncCatalogPoolSize']:s['ncCatalogPoolSize']+2]=word(pool)
        m[s['ncCatalogs']:s['ncCatalogs']+16]=slot(7,0,12,1,1)+slot(1,0x1234,240,1,0)
        m[s['ncCatalogBases']:s['ncCatalogBases']+4]=b''.join(word(p) for p in physical)
        for name, value in [('state.activePanel', 0), ('state.panelA_sort', 1),
                            ('vars.cacheSlotA', 0), ('vars.cacheSlotB', 1),
                            ('vars.cacheValidA', 1), ('vars.cacheValidB', 1),
                            ('vars.cacheSortA', 1), ('vars.cacheSortB', 1)]:
            m[s[name]] = value
        for i, count in enumerate((12,240)):
            start=physical[i]
            m[start:start+16*count]=bytes([0x60+i])*16*count
            m[start+16*count]=255
            start=s['CATALOG_INDEX_A']+256*i
            saved=bytes(range(count-1,-1,-1))+b'\xff'
            m[start:start+256]=saved.ljust(256,b'\0')
        m[s['FILE_INDEX_A']:s['FILE_INDEX_A']+256] = b'\xA5'*256
        m[s['FILE_INDEX_B']:s['FILE_INDEX_B']+256] = b'\x5A'*256
        # Счётчик реальных перестроений: исходный первый CALL исполняется
        # в хвосте trampoline, затем возвращаемся в тело buildFileIndex.
        start = s['buildFileIndex']
        assert m[start] == 0xCD
        original = bytes(m[start:start+3])
        hook = b'\xF5\x3A'+word(0x8E80)+b'\x3C\x32'+word(0x8E80)+b'\xF1'
        hook += original+b'\xC3'+word(start+3)
        m[0x8E00:0x8E00+len(hook)] = hook
        m[start:start+3] = b'\xC3'+word(0x8E00)
        return m

    def metadata(m, i):
        return m[s['ncCatalogs']+8*i:s['ncCatalogs']+8*i+8]

    def invoke(name, m, calls, check):
        before = bytes(m)
        code = b'\x31'+word(0x8F00)
        for i, (label, a, bc, de) in enumerate(calls):
            code += b'\x3E'+bytes([a])+b'\x01'+word(bc)+b'\x11'+word(de)
            code += b'\xCD'+word(s[label])
            code += b'\xE5\xF5\xC1\x60\x69\x22'+word(0x8DC0+2*i)+b'\xE1'
        code += b'\xC3'+word(0x8DFF)
        m[0x8D00:0x8D00+len(code)] = code
        input_path = args.output/(name+'.in.bin')
        output_path = args.output/(name+'.out.bin')
        input_path.write_bytes(m)
        if args.prepare_only:
            return
        result = subprocess.run([str(args.ticks), '-m8080', '-pc', '8d00',
            '-end', '8dff', '-counter', '50000000', '-output', str(output_path),
            str(input_path)], capture_output=True, text=True, check=True, timeout=30)
        output = output_path.read_bytes()
        assert int.from_bytes(output[65542:65544], 'little') == 0x8DFF, result.stdout
        check(output, before)

    cases = 0
    # Точный каталог H:0 имеется в сохранённом слоте: восстановить индекс,
    # не переставляя/не копируя FILE_INFO и не вызывая сортировку.
    m = memory()
    m[s['vars.cacheValidA']] = 0
    def hit(b, old):
        assert not b[0x8DC0] & 1
        assert b[s['vars.cacheSlotA']] == 0 and b[s['state.panelA_filesCnt']] == 12
        assert b[s['FILE_INDEX_A']:s['FILE_INDEX_A']+256] == old[s['CATALOG_INDEX_A']:s['CATALOG_INDEX_A']+256]
        assert b[s['FILE_CACHE_A']:0x8c00] == old[s['FILE_CACHE_A']:0x8c00]
        assert b[s['FILE_INDEX_B']:s['FILE_INDEX_B']+256] == old[s['FILE_INDEX_B']:s['FILE_INDEX_B']+256]
        assert b[0x8E80] == 0
    invoke('root-hit', m, [('ncCatalogAcquire', 0, 7, 0),
                         ('ncCatalogUseIndex', 0, 0, 0)], hit)
    cases += 1

    for other, want in [(1,0),(0,1),(0xFF,0)]:
        m = memory()
        m[s['vars.cacheSlotB']] = other
        m[s['state.panelA_dirLow']] = 0x45
        m[s['state.panelA_dirHigh']] = 0x23
        def miss(b, old, want=want, other=other):
            assert b[0x8DC0] & 1
            assert b[s['vars.cacheSlotA']] == want
            assert metadata(b,want)[:6] == b'\0\x07\x45\x23\0\xff'
            assert b[s['vars.cacheValidA']] == 0 and b[0x8E80] == 0
            bases=s['ncCatalogBases']
            base=lambda i:int.from_bytes(b[bases+2*i:bases+2*i+2],'little')
            if other != 0xFF:
                count=metadata(old,other)[4]
                size=(count+1)*16
                assert base(other)==s['FILE_CACHE_A']
                assert base(want)==base(other)+size
                assert b[base(other):base(other)+size]==old[physical[other]:physical[other]+size]
                assert metadata(b,other)==metadata(old,other)
                assert b[s['ncCatalogCapacity']]==min(255,pool//16-max(count+1,20)-1)
            else:
                assert base(want)==s['FILE_CACHE_A']
        invoke('victim-'+str(other),m,[('ncCatalogAcquire',0,7,0)],miss)
        cases += 1

    m = memory()
    m[s['ncCatalogs']+s['CATALOG_OVERFLOW']]=2
    def grow(b, old):
        assert b[0x8DC0]&1
        assert b[s['ncCatalogCapacity']]==min(255,pool//16-242)
        assert metadata(b,0)[7]==2
        assert metadata(b,1)==metadata(old,1)
    invoke('truncated-hit-grows',m,[('ncCatalogAcquire',0,7,0)],grow)
    cases += 1

    # Совпадение младшего байта кластера не является попаданием.
    for drive, directory in [(1, 0x1234), (1, 0x2234), (0, 0x1234)]:
        m = memory()
        m[s['state.panelA_dirLow']] = directory & 255
        m[s['state.panelA_dirHigh']] = directory >> 8
        expect_hit = drive == 1 and directory == 0x1234
        def key_check(b, old, expect_hit=expect_hit):
            assert bool(b[0x8DC0] & 1) != expect_hit
        invoke(f'key-{drive}-{directory}', m, [('ncCatalogAcquire', 0, drive, 0)], key_check)
        cases += 1

    for drive, directory, whole, invalid in [
            (7, 0, 0, [0]), (1, 0x1234, 0, [1]),
            (7, 0x2234, 0, []), (7, 0, 1, [0]),
            (1, 0, 1, [1]), (0, 0, 1, [])]:
        m = memory()
        def notify(b, old, invalid=invalid):
            for i in range(2):
                assert metadata(b, i)[0] == (0 if i in invalid else 1)
                if i in invalid:
                    assert metadata(b, i)[5] == 255
            assert b[s['vars.cacheValidA']] == (0 if 0 in invalid else 1)
            assert b[s['vars.cacheValidB']] == (0 if 1 in invalid else 1)
            assert b[s['FILE_CACHE_A']:0x8c00] == old[s['FILE_CACHE_A']:0x8c00]
        invoke(f'notify-{drive}-{directory}-{whole}', m,
               [('ncCatalogChanged', drive, whole, directory)], notify)
        cases += 1

    m = memory()
    m[s['vars.cacheSlotB']] = 0
    def explicit(b, old):
        assert metadata(b, 0)[0] == 0
        assert b[s['vars.cacheValidA']] == b[s['vars.cacheValidB']] == 0
    invoke('explicit-shared-refresh', m, [('invalidateFileCache', 0, 0, 0)], explicit)
    cases += 1

    m = memory()
    m[s['state.panelA_sort']] = 0
    m[s['state.panelA_filesCnt']] = 0
    def failed_read(b, old):
        assert metadata(b, 0)[0] == b[s['vars.cacheValidA']] == 0
        assert b[s['FILE_INDEX_A']] == 255
    invoke('failed-read-not-cached', m, [('ncCatalogCommit', 0, 0, 0)], failed_read)
    cases += 1
    print(('PREPARED (CPU not started)' if args.prepare_only else 'NC CATALOG PASS'), cases)


if __name__ == '__main__':
    main()
