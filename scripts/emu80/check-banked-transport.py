"""Изолированный тест реального переключения страниц и вложенных кадров 8080."""
from pathlib import Path
import argparse, json, re, runpy, struct, subprocess

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--port', type=int, choices=range(19781,19797), default=19781)
    parser.add_argument('--report', type=Path, default=ROOT/'docs/banked/transport-001')
    args = parser.parse_args()
    out = args.report.resolve(); out.mkdir(parents=True, exist_ok=True)
    sy = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
          (ROOT/'bin/banked/TRANSPORT.sym').read_text(), re.M)}
    near_copy, = re.findall(r'^memExecCopy: EQU 0x([0-9A-Fa-f]+)',
                            (ROOT/'bin/banked/CLIENT.sym').read_text(), re.M)
    fixture = out/'calls.asm'
    fixture.write_text(f'''; Исполнение этого стенда намеренно заменяет RAM тестового эмулятора.
bankCall = 0{sy['bankCall']:04X}h
bankOwner = 0{sy['bankOwner']:04X}h
bankPage = 0{sy['bankPage']:04X}h
bankCopyFar = 0{sy['bankCopyFar']:04X}h
bankDepth = 0{sy['bankDepth']:04X}h
bankServiceSP = 0{sy['bankServiceSP']:04X}h
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
    ld (06020h),hl
    ld h,b
    ld l,c
    ld (06022h),hl
    ex de,hl
    ld (06024h),hl
    ex de,hl
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
    ld a,(bankDepth)
    ld (06026h),a
    ld hl,(bankServiceSP)
    ld (06027h),hl
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
    ld hl,06008h
    inc (hl)
    ld a,b
    or a
    ret z
    dec b
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
    g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
    matrix=h['Matrix']();rows=[]
    matrix.m.url=f'http://127.0.0.1:{args.port}/mcp'
    try:
        matrix.start('banked-transport')
        m=matrix.m;m.call('debug_break');m.call('bp_clear')
        runtime=(ROOT/'bin/banked/TRANSPORT.bin').read_bytes()
        window=(ROOT/'bin/banked/WINDOW.bin').read_bytes()
        image=(ROOT/'bin/MEM.sys').read_bytes()
        segments=[struct.unpack_from('<HHH',image,6+6*i) for i in range(image[5])]
        body=(out/'calls.bin').read_bytes()
        def select(page):
            m.write(0xFFFD if page else 0xFFFC,bytes([page-1 if page else 0]))
        def reset():
            for page in (0,1,2,16):
                select(page)
                for address,size,offset in segments:
                    if address != 0xffc0:
                        m.write(address,image[offset:offset+size])
                m.write(0x100,body)
                m.write(sy['bankPage'],bytes([page]));m.write(sy['bankOwner'],bytes([7,0]) if page==0 else bytes([99,page]))
                m.write(sy['bankServiceSP'],bytes.fromhex('00e7'))
                m.write(sy['bankStackFloor'],bytes.fromhex('40e6'))
                m.write(sy['MEM_NEAR_COPY'],int(near_copy,16).to_bytes(2,'little'))
                m.write(sy['MEM_EXEC_READY'],bytes([sy['MEM_EXEC_READY_VALUE'] if page!=16 else 0]))
                m.write(0x6000,bytes(48));m.write(0xe5f0,b'GUARD'*3)
                m.write(0xe700,b'GUARD'*3);m.write(0x6f00,b'GUARD'*3)
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
            assert m.read(sy['bankServiceSP'],2)==bytes.fromhex('00e7')
            assert m.read(sy['bankOwner'],2)==(bytes([7,0]) if page==0 else bytes([99,page]))
            for guard in (0xe5f0,0xe700,0x6f00):
                assert m.read(guard,15)==b'GUARD'*3
        select(1);assert m.read(0x6004,2)==bytes([7,0])
        assert m.read(0x6020,7)==struct.pack('<HHHB',cs['dosOuter'],0x1234,0x6000,2)
        assert 0xe600<int.from_bytes(m.read(0x6027,2),'little')<0xe6f8
        rows.append({'case':'main -> DOS -> BIOS -> bank 2 -> DOS -> unwind','result':result.hex()})
        # Прямой вызов через реальный stub: неверная страница, последняя страница,
        # та же страница, предел вложенности. Это ABI-тест, не проверка ввода NC.
        # Допустимые цели и ограниченная рекурсия глубже прежнего лимита.
        # Банк назначения, глубину и запас стека теперь гарантирует вызывающий.
        for target,entry,a,carry in [(16,cs['dosInner'],0x55,True),
                                     (0,cs['dosInner'],0x55,True),
                                     (1,cs['recursive'],0,False)]:
            reset();select(0)
            m.write(0x2000,bytes([0xCD])+sy['bankCall'].to_bytes(2,'little')+bytes.fromhex('c30070'))
            state=run(0x2000,af=target<<8,hl=entry,bc=0x0934,de=0x5678)
            assert state['registers']['af']['dec']>>8==a,(target,state)
            assert bool(state['flags']['C'])==carry,(target,state)
            if entry==cs['recursive']:
                select(1);assert m.read(0x6008,1)==b'\x0a'
            for page in (0,1,2,16):
                select(page)
                assert m.read(sy['bankDepth'],1)==b'\0'
                assert m.read(sy['bankServiceSP'],2)==bytes.fromhex('00e7')
                assert m.read(sy['bankOwner'],2)==(bytes([7,0]) if page==0 else bytes([99,page]))
                for guard in (0xe5f0,0xe700,0x6f00):
                    assert m.read(guard,15)==b'GUARD'*3
            rows.append({'target':target,'a':a,'carry':carry})
        copies = [
                (0,1,2,8192,0x2001,0x5003,0),
                (1,2,0,259,0x3003,0x4001,0),
                (2,0,16,256,0x3001,0x5001,0),
                (0,16,1,1,0x3000,0x5000,0),
                (0,16,1,2,0x3000,0x5000,0),
                (0,16,1,3,0x3000,0x5000,0),
                (0,16,16,5,0x3000,0x5000,0),
                (0,1,1,5,0x3000,0x5000,0),
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
        entries += [('MOVE', (0,16,2,257,0x3001,0x5003,0)),
                    ('MOVE', (2,2,16,31,0x3001,0x5003,0))]
        for entry,(caller,source,dest,size,src,dst,error) in entries:
            reset()
            data=bytes((i*17+31)&255 for i in range(size))
            if not error and size:
                select(source);m.write(src,data)
                select(dest);m.write(dst-1,b'\xa5'+b'\xcc'*size+b'\x5a')
            select(caller)
            packet=bytes([source])+src.to_bytes(2,'little')+bytes([dest])+dst.to_bytes(2,'little')+size.to_bytes(2,'little')
            m.write(0x1000,packet)
            if entry=='MOVE':
                m.write(0x2000,bytes.fromhex('afcd75f8c30070'))
            else:
                m.write(0x2000,bytes([0xCD])+sy[entry].to_bytes(2,'little')+bytes.fromhex('c30070'))
            before=m.read(0xFFC0,32)
            state=run(0x2000,hl=0x1000,bc=0x1234,de=0x5678)
            assert state['registers']['af']['dec']>>8==error,(caller,source,dest,size,state)
            assert bool(state['flags']['C'])==bool(error)
            for reg,value in [('hl',0x1000),('bc',0x1234),('de',0x5678)]:
                assert state['registers'][reg]['dec']==value
            assert m.read(sy['bankPage'],1)==bytes([caller])
            if not error and size and (source!=dest or entry=='bankCopySector'):
                portion=size%32 or 32
                assert m.read(0xFFC0,portion)==data[-portion:]
            else:
                assert m.read(0xFFC0,32)==before
            if not error and size:
                select(dest);assert m.read(dst,size)==data,(caller,source,dest,size)
                assert m.read(dst-1,1)==b'\xa5' and m.read(dst+size,1)==b'\x5a'
            rows.append({'entry':entry,'copy':[caller,source,dest,size],'error':error,
                         'ticks':state['elapsedTicks'],'ms':state['elapsedTicks']/2000})
        # Текущий и удалённый локальный MOVE, включая нечётное перекрытие.
        initial=bytes(((i*37)^(i>>5))&255 for i in range(800))
        for caller,target,source,dest,size,error in (
            (0,0,0,1,257,0),(1,1,1,0,257,0),(2,2,0,1,255,0),
            (0,1,1,0,257,0),(0,2,0,1,257,0),(2,0,1,0,17,0),
            (0,2,0,0,257,0),(1,2,0,1,1,0),(0,16,0,1,257,0xe7),
            (1,16,0,1,0,0),
        ):
            reset();select(target);m.write(0x3000,initial)
            packet=struct.pack('<BHBHH',target,0x3000+source,target,0x3000+dest,size)
            select(caller);m.write(0x1000,packet)
            m.write(0x2000,bytes.fromhex('afcd75f8c30070'))
            state=run(0x2000,hl=0x1000,bc=0x1234,de=0x5678)
            assert state['registers']['af']['dec']>>8==error and bool(state['flags']['C'])==bool(error),state
            assert tuple(state['registers'][name]['dec'] for name in ('hl','bc','de'))==(0x1000,0x1234,0x5678)
            assert m.read(sy['bankPage'],1)==bytes([caller])
            expected=bytearray(initial)
            if not error:
                expected[dest:dest+size]=initial[source:source+size]
            select(target);assert m.read(0x3000,len(initial))==expected,(caller,target,source,dest,size)
            rows.append({'entry':'MOVE','copy':[caller,target,source,dest,size],'error':error,
                         'ticks':state['elapsedTicks']})
        (out/'result.json').write_text(json.dumps({'runtimeBytes':len(runtime),'windowBytes':len(window),'cases':rows},indent=2)+'\n')
        print(json.dumps(rows))
    finally:
        matrix.stop()

if __name__=='__main__':
    main()
