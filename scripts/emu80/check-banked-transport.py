"""Изолированный тест реального переключения страниц и вложенных кадров 8080."""
from pathlib import Path
import argparse, json, re, runpy, subprocess

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT/'.local/monitor2-001')
    parser.add_argument('--report', type=Path, default=ROOT/'docs/banked/transport-001')
    args = parser.parse_args()
    out = args.report.resolve(); out.mkdir(parents=True, exist_ok=True)
    subprocess.run(['python', str(ROOT/'scripts/build/build-banked-transport.py')], check=True)
    sy = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
          (ROOT/'bin/banked/TRANSPORT.sym').read_text(), re.M)}
    fixture = out/'calls.asm'
    fixture.write_text(f'''; Исполнение этого стенда намеренно заменяет RAM тестового эмулятора.
bankCall = 0{sy['bankCall']:04X}h
bankOwner = 0{sy['bankOwner']:04X}h
bankPage = 0{sy['bankPage']:04X}h
bankCopyFar = 0{sy['bankCopyFar']:04X}h
    ORG 0100h
start:
    ld sp,07000h
    ld a,1
    ld hl,dosOuter
    ld de,06000h
    ld bc,01234h
    call bankCall
    ld (06010h),hl
    push af
    pop hl
    ld (06012h),hl
    ld h,b
    ld l,c
    ld (06014h),hl
    ex de,hl
    ld (06016h),hl
    jp 07000h
dosOuter:
    ld a,(bankPage)
    ld (06000h),a
    ld hl,(bankOwner)
    ld (06002h),hl
    xor a
    ld hl,biosCallback
    call bankCall
    ret
biosCallback:
    ld a,(bankPage)
    ld (06000h),a
    ld hl,(bankOwner)
    ld (06002h),hl
    ld a,2
    ld hl,otherBank
    call bankCall
    ret
otherBank:
    ld a,(bankPage)
    ld (06000h),a
    ld hl,(bankOwner)
    ld (06002h),hl
    ld a,1
    ld hl,dosInner
    call bankCall
    ret
dosInner:
    ; Копировщик временно заменяет окно внутри вложенного системного вызова.
    ld hl,copyRequest
    call bankCopyFar
    ret c
    ld hl,(bankOwner)
    ld (06004h),hl
    ld hl,09ABCh
    ld de,05678h
    ld bc,01234h
    ld a,055h
    scf
    ret
copyRequest:
    DB 2
    DW 03000h
    DB 0
    DW 04000h
    DW 259
recursive:
    ld a,(bankPage)
    ld hl,recursive
    call bankCall
    ret
''', encoding='utf-8')
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080',f'--raw={out}/calls.bin',
                    f'--sym={out}/calls.sym',str(fixture)],check=True,capture_output=True)
    cs = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
          (out/'calls.sym').read_text(),re.M)}
    h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    g=h['Matrix'].__init__.__globals__;g['OUT']=out;g['RUNTIME']=args.runtime.resolve()
    matrix=h['Matrix']();rows=[]
    try:
        matrix.start('banked-transport')
        m=matrix.m;m.call('debug_break');m.call('bp_clear')
        runtime=(ROOT/'bin/banked/TRANSPORT.bin').read_bytes()
        window=(ROOT/'bin/banked/WINDOW.bin').read_bytes()
        body=(out/'calls.bin').read_bytes()
        def select(page):
            m.write(0xFFFD if page else 0xFFFC,bytes([page-1 if page else 0]))
        def reset():
            for page in (0,1,2,16):
                select(page);m.write(0xFC00,runtime);m.write(0x100,body)
                m.write(sy['bankPage'],bytes([page]));m.write(sy['bankOwner'],bytes([7,0]) if page==0 else bytes([99,page]))
                m.write(0x6000,bytes(32));m.write(0xF7F0,b'GUARD'*3)
                m.write(0x3000,bytes((i*7+page)&255 for i in range(259)))
            select(0);m.write(0xFFC0,window)
        def run(entry=0x100,**regs):
            m.stop_at(0x7000);m.call('regs_set',pc=entry,sp=0x7000,**regs)
            ticks=m.call('cpu_state')['cpu_ticks']
            state=m.until(0x7000,10)
            assert state['registers']['sp']['dec']==0x7000,state
            state['elapsedTicks']=state['cpu_ticks']-ticks
            return state
        reset();run()
        result=m.read(0x6010,8)
        assert result[:2]==bytes.fromhex('bc9a') and result[3]==0x55 and result[2]&1,result.hex()
        assert result[4:]==bytes.fromhex('34127856'),result.hex()
        assert m.read(0x4000,259)==bytes((i*7+2)&255 for i in range(259))
        for page in (0,1,2):
            select(page)
            assert m.read(0x6000,1)==bytes([page])
            assert m.read(0x6002,2)==bytes([7,0])
            assert m.read(sy['bankDepth'],1)==b'\0'
            assert m.read(sy['bankServiceSP'],2)==bytes.fromhex('00fc')
            assert m.read(sy['bankOwner'],2)==(bytes([7,0]) if page==0 else bytes([99,page]))
            assert m.read(0xF7F0,15)==b'GUARD'*3
        select(1);assert m.read(0x6004,2)==bytes([7,0])
        rows.append({'case':'main -> DOS -> BIOS -> bank 2 -> DOS -> unwind','result':result.hex()})
        # Прямой вызов через реальный stub: неверная страница, последняя страница,
        # та же страница, предел вложенности. Это ABI-тест, не проверка ввода NC.
        for target,entry,error in [(17,cs['dosInner'],0xE0),(16,cs['dosInner'],0),
                                    (0,cs['dosInner'],0),(1,cs['recursive'],0xE1)]:
            reset();select(0)
            m.write(0x2000,bytes([0xCD])+sy['bankCall'].to_bytes(2,'little')+bytes.fromhex('c30070'))
            state=run(0x2000,af=target<<8,hl=entry,bc=0x1234,de=0x5678)
            a=state['registers']['af']['dec']>>8
            assert a==(error or 0x55),(target,state)
            assert state['flags']['C']
            for page in (0,1,2,16):
                select(page)
                assert m.read(sy['bankDepth'],1)==b'\0'
                assert m.read(sy['bankServiceSP'],2)==bytes.fromhex('00fc')
                assert m.read(0xF7F0,15)==b'GUARD'*3
            rows.append({'target':target,'error':error})
        copies = [
                (0,1,2,8192,0x2001,0x5003,0),
                (1,2,0,259,0x3003,0x4001,0),
                (2,0,16,256,0x3001,0x5001,0),
                (0,16,1,1,0x3000,0x5000,0),
                (0,16,1,2,0x3000,0x5000,0),
                (0,16,1,3,0x3000,0x5000,0),
                (0,1,1,5,0x3000,0x5000,0),
                (0,1,2,4,0xFEFC,0xFEFC,0),
                (0,1,2,5,0xFEFC,0xFEFC,0xE2),
                (0,1,2,32,0xFFF0,0x5000,0xE2),
                (0,17,2,4,0x3000,0x5000,0xE0),
                (0,17,17,0,0,0,0)] + [
                (0,1,2,n,0x3000+offset,0x5003,0)
                for n in (4,252,255,256,257,258,259,511,512,513,8192)
                for offset in (0,1,253)]
        entries = [('bankCopyFar', case) for case in copies]
        entries += [('bankCopySector', (caller,source,dest,256,0x3001,0x5003,0))
                    for caller,source,dest in ((0,0,2),(0,2,0),(1,1,2),(1,2,1),(2,0,1),(0,1,1))]
        for entry,(caller,source,dest,size,src,dst,error) in entries:
            reset()
            data=bytes((i*17+31)&255 for i in range(size))
            if not error and size:
                select(source);m.write(src,data)
                select(dest);m.write(dst-1,b'\xa5'+b'\xcc'*size+b'\x5a')
            select(caller)
            packet=bytes([source])+src.to_bytes(2,'little')+bytes([dest])+dst.to_bytes(2,'little')+size.to_bytes(2,'little')
            m.write(0x1000,packet)
            m.write(0x2000,bytes([0xCD])+sy[entry].to_bytes(2,'little')+bytes.fromhex('c30070'))
            before=m.read(0xFFC0,32)
            state=run(0x2000,hl=0x1000,bc=0x1234,de=0x5678)
            assert state['registers']['af']['dec']>>8==error,(caller,source,dest,size,state)
            assert bool(state['flags']['C'])==bool(error)
            for reg,value in [('hl',0x1000),('bc',0x1234),('de',0x5678)]:
                assert state['registers'][reg]['dec']==value
            assert m.read(sy['bankPage'],1)==bytes([caller])
            assert m.read(0xFFC0,32)==before
            if not error and size:
                select(dest);assert m.read(dst,size)==data,(caller,source,dest,size)
                assert m.read(dst-1,1)==b'\xa5' and m.read(dst+size,1)==b'\x5a'
            rows.append({'entry':entry,'copy':[caller,source,dest,size],'error':error,
                         'ticks':state['elapsedTicks'],'ms':state['elapsedTicks']/2000})
        (out/'result.json').write_text(json.dumps({'runtimeBytes':len(runtime),'windowBytes':len(window),'cases':rows},indent=2)+'\n')
        print(json.dumps(rows))
    finally:
        matrix.stop()

if __name__=='__main__':
    main()
