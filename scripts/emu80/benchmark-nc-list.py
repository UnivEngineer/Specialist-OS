"""Time real NC DIR_OP_LIST calls with PIT, separately from sorting and UI."""
import json
from pathlib import Path
import runpy
import statistics
import subprocess
import argparse
import struct
import hashlib

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'docs/nc/sort-optimized-2026-11-11'


def build(symbols):
    result = {}
    for context, listing in ((0,1),(1,0),(1,1)):
        folder = OUT/f'list-build/{context}-{listing}'
        folder.mkdir(parents=True, exist_ok=True)
        source = f'''    INCLUDE "{(ROOT/'source/include/mxos.inc').as_posix()}"
    ORG 07000h
entry:
    ld sp,08B00h
    call pitTimerInit
    call pitTimerRead
    ld (started),hl
    IF {context}
    ld a,7
    ld e,1
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    jp c,finished
    ld de,0
    ld a,DIR_OP_SET
    MXOS_CALL MXOS_DOS_DIRECTORY
    jp c,finished
    ENDIF
    IF {listing}
    ld hl,04000h
listSkip:
    ld bc,0
listCapacity:
    ld de,255
    ld a,DIR_OP_LIST
    MXOS_CALL MXOS_DOS_DIRECTORY
    ENDIF
finished:
    push af
    push de
    call pitTimerRead
    ex de,hl
    ld hl,(started)
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    ld (elapsed),hl
    pop de
    ex de,hl
    ld (count),hl
    pop af
    ld a,0
    adc a,0
    ld (error),a
done:
    jp done
started: DW 0
elapsed: DW 0
count: DW 0
error: DB 0
    INCLUDE "{(ROOT/'source/include/timing-pit.inc').as_posix()}"
    ASSERT $ < 07800h
'''
        (folder/'list.asm').write_text(source, encoding='utf-8')
        run = subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',
                              '--sym=list.sym','--lst=list.lst','--raw=list.bin','list.asm'],
                             cwd=folder,capture_output=True,text=True)
        if run.returncode:
            raise RuntimeError(run.stdout+run.stderr)
        result[(context,listing)] = (folder/'list.bin').read_bytes(), symbols(folder/'list.sym')
    return result


def main():
    global OUT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT)
    args = parser.parse_args()
    OUT = args.output
    OUT.mkdir(parents=True, exist_ok=True)
    helpers = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    symbols = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-sort.py'))['symbols']
    builds = build(symbols)
    runtime = ROOT/'.local/nc-sort-benchmark-runtime'
    matrix_type = helpers['Matrix']
    matrix_type.__init__.__globals__.update(RUNTIME=runtime,OUT=OUT)
    matrix = matrix_type()
    report = dict(cpu_hz=2000000,pit_hz=2000000,tick_ms=0.128,repeats=3,results=[])
    try:
        for goal in (127,250,300):
            disk = ROOT/'images/FlashDisk_emu80_seed.bin'
            if goal != 127:
                disk = runpy.run_path(str(ROOT/'scripts/emu80/nc-sort-image.py'))['make_full_h'](
                    OUT/f'H-sort{goal}.bin', target=goal)
            if goal == 300:
                image = bytearray(disk.read_bytes())
                word = lambda at: struct.unpack_from('<H',image,at)[0]
                root = (word(14)+image[16]*word(22))*word(11)
                # 301 normal/label slots, then deleted + LFN. The last physical
                # slot lies beyond RootEntCnt=303 and must never be returned.
                deleted = bytearray(32); deleted[0]=0xe5
                lfn = bytearray(32); lfn[0]=0x41; lfn[11]=0x0f
                poison = bytearray(32); poison[:11]=b'BADROOT TXT'
                image[root+301*32:root+302*32]=deleted
                image[root+302*32:root+303*32]=lfn
                image[root+303*32:root+304*32]=poison
                struct.pack_into('<H',image,17,303)
                disk.write_bytes(image)
                manifest = json.loads(disk.with_suffix('.json').read_text())
                manifest.update(root_slots=303, image_sha256=hashlib.sha256(image).hexdigest(),
                                boundary_fixture='deleted + LFN + poison outside RootEntCnt')
                disk.with_suffix('.json').write_text(json.dumps(manifest,indent=2)+'\n')
                report['boundary_fixture'] = dict(root_slots=303, deleted=True, lfn=True,
                                                 poison_outside_root='BADROOT.TXT', records=300)
            matrix.start(f'list-{goal}',disk=disk)
            m = matrix.m
            m.call('debug_break')
            nc = symbols(ROOT/'bin/NC.sym')
            meta = m.read(nc['ncCatalogs'],24)
            for slot in range(3):
                row = meta[slot*8:slot*8+8]
                if row[0] and row[1]==7 and row[2:4]==b'\0\0':
                    shown = row[4]
                    nc_expected = m.read(nc[f'FILE_CACHE_{"ABC"[slot]}'],shown*16+1)
                    break
            else:
                raise RuntimeError('Root H: cache missing')
            assert shown == min(goal,255)
            image = disk.read_bytes()
            word = lambda at: struct.unpack_from('<H',image,at)[0]
            root = (word(14)+image[16]*word(22))*word(11)
            records = []
            for i in range(word(17)):
                entry = image[root+i*32:root+(i+1)*32]
                if not entry[0]:
                    break
                if entry[0]==0xe5 or entry[11]&8:
                    continue
                name = bytes([0xe5])+entry[1:12] if entry[0]==5 else entry[:12]
                records.append(name+b'\0\0'+entry[28:30])
            count = len(records)
            expected = b''.join(records)+b'\xff'
            assert count == goal
            assert nc_expected == b''.join(records[:shown])+b'\xff'

            def setup(drive):
                state = matrix.func(nc['bios_fileGetSetDrive'],af=drive<<8,de=1)
                assert not state['flags']['C']

            def timed(s, listing, skip=0, capacity=255, guard=False):
                if listing:
                    m.write(s['listSkip']+1,skip.to_bytes(2,'little'))
                    m.write(s['listCapacity']+1,capacity.to_bytes(2,'little'))
                    if guard:
                        m.write(0x4000,bytes([0xa5])*(capacity*16+33))
                m.stop_at(s['done'])
                m.call('regs_set',pc=s['entry'],sp=0x8B00)
                m.until(s['done'])
                assert m.read(s['error'],1)==b'\0'
                if listing:
                    assert int.from_bytes(m.read(s['count'],2),'little')==count
                    wanted = expected[min(skip,count)*16:min(skip+capacity,count)*16]+b'\xff'
                    assert m.read(0x4000,len(wanted))==wanted,(count,skip,capacity)
                    if guard:
                        assert m.read(0x4000+len(wanted),32)==bytes([0xa5])*32
                return int.from_bytes(m.read(s['elapsed'],2),'little')

            for (context,listing),(binary,s) in builds.items():
                m.write(0x7000,binary)
                if listing and not context:
                    setup(7)
                    for skip,capacity in ((0,0),(0,1),(1,1),(7,9),(count-1,255),
                                          (count,255),(65535,3),(5,256),(0,300)):
                        timed(s,True,skip,capacity,guard=True)
                samples = {'cold-root':[],'warm-root':[]}
                for _ in range(3):
                    setup(7)
                    state = matrix.func(nc['bios_mountVolume'])
                    assert not state['flags']['C']
                    # Remount and initial drive selection are outside the timer.
                    # Context tests measure the actual warm-mounted A: -> H: switch.
                    if context:
                        setup(0)
                    samples['cold-root'].append(timed(s,listing))
                    if context:
                        setup(0)
                    samples['warm-root'].append(timed(s,listing))
                operation = 'select+set+list' if context and listing else 'select+set' if context else 'list'
                for cache,ticks in samples.items():
                    report['results'].append(dict(count=count,operation=operation,cache=cache,
                                                  ticks=ticks,median_ms=statistics.median(ticks)*0.128))
                (OUT/'list-results.json').write_text(json.dumps(report,indent=2)+'\n')
                print('PASS',count,operation,samples,flush=True)
            matrix.stop()
        print('DOS list timer: all returned records match NC cache; no sorting or drawing in timed code.',flush=True)
    finally:
        matrix.stop()


if __name__ == '__main__':
    main()
