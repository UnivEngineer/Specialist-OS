"""Автономное сравнение исторических, CP/M и FAR-копировщиков на одной модели MX2, 2 МГц."""
from pathlib import Path
import argparse,hashlib,json,re,runpy,statistics,subprocess

ROOT=Path(__file__).resolve().parents[2]
def symbols(path):
    return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',path.read_text(),re.M)}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,default=ROOT/'.local/monitor2-001')
    p.add_argument('--report',type=Path,default=ROOT/'docs/bench/copybench-001')
    a=p.parse_args();out=a.report.resolve();out.mkdir(parents=True,exist_ok=True)
    if (out/'result.json').exists():p.error('Choose a new report directory')
    subprocess.run(['python',str(ROOT/'scripts/build/build-banked-transport.py')],check=True)
    subprocess.run(['python',str(ROOT/'source-apps/copybench/build.py')],check=True)
    s=symbols(ROOT/'bin/banked/TRANSPORT.sym')
    h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    g=h['Matrix'].__init__.__globals__;g['OUT']=out;g['RUNTIME']=a.runtime.resolve()
    matrix=h['Matrix']();rows=[]
    def assemble(text):
        path=out/'harness.asm';path.write_text(text,encoding='utf-8')
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080',f'--raw={out}/harness.bin',str(path)],check=True,capture_output=True)
        return (out/'harness.bin').read_bytes()
    try:
        matrix.start('copy-baseline');m=matrix.m;m.call('debug_break');m.call('bp_clear')
        def select(page):m.write(0xFFFD if page else 0xFFFC,bytes([page-1 if page else 0]))
        def sample(caller,srcPage,src,dstPage,dst,size,code):
            data=bytes((i*19+37)&255 for i in range(size))
            select(srcPage);m.write(src,data)
            select(dstPage);m.write(dst-1,b'\xa5'+bytes(size)+b'\x5a')
            select(caller);m.write(0x1000,code)
            m.stop_at(0x1800);m.call('regs_set',pc=0x1000,sp=0x8FDF)
            before=m.call('cpu_state')['cpu_ticks'];state=m.until(0x1800,10)
            ticks=state['cpu_ticks']-before
            assert state['registers']['sp']['dec']==0x8FDF
            select(dstPage);assert m.read(dst,size)==data
            assert m.read(dst-1,1)==b'\xa5' and m.read(dst+size,1)==b'\x5a'
            return ticks
        def save_row(name,caller,srcPage,src,dstPage,dst,size,code):
            samples=[sample(caller,srcPage,src,dstPage,dst,size,code) for _ in range(3)]
            assert len(set(samples))==1, (name,samples)
            ticks=statistics.median(samples);ms=ticks/2000
            rows.append(dict(name=name,caller=caller,source=[srcPage,src],destination=[dstPage,dst],
                             size=size,ticks=samples,ms=ms,kB_s=size/ms,verified=True))
            print(rows[-1],flush=True)
        # Старые драйверы работают только с main и сектором 256 байт.
        # Время установки режима исключено; цикл вызовов и адресация включены.
        for name in ('Builtin block',):
            select(0);m.write(0xFFC0,(ROOT/'bin/copybench/builtin_block.bin').read_bytes())
            for size in (256,8192):
                for write in (False,True):
                    code=assemble(f'''    ORG 01000h
    ld hl,04000h
    ld de,02000h
    ld b,{size//256}
again:
    ld c,2
    push bc
    push de
    push hl
    call 0{0xFFD0 if write else 0xFFC0:04X}h
    pop hl
    pop de
    pop bc
    inc h
    inc d
    dec b
    jp nz,again
    jp 01800h
''')
                    save_row(name+('-write' if write else '-read'),0,0 if write else 3,
                             0x4000 if write else 0x2000,3 if write else 0,
                             0x2000 if write else 0x4000,size,code)
        select(0);m.write(0xFFC0,(ROOT/'bin/copybench/builtin_byte.bin').read_bytes())
        for size in (256,8192):
            for write in (False,True):
                body = ('ld a,(de)\n    ld c,a\n    ld a,2\n    call 0FFD0h' if write else
                        'ld a,2\n    call 0FFC0h\n    ld a,c\n    ld (de),a')
                code=assemble(f'''    ORG 01000h
    ld hl,02000h
    ld de,04000h
    ld bc,{size}
byteLoop:
    push bc
    {body}
    pop bc
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,byteLoop
    jp 01800h
''')
                save_row('Builtin byte'+('-write' if write else '-read'),0,
                         0 if write else 3,0x4000 if write else 0x2000,
                         3 if write else 0,0x2000 if write else 0x4000,size,code)
        # Исторические алгоритмы загружаются независимо от старого DOS.
        for name in ('stack','loop','unrolled','direct','regs2','regs4','regs6'):
            sy=symbols(ROOT/f'bin/copybench/{name}.sym')
            raw=(ROOT/f'bin/copybench/{name}.bin').read_bytes()
            tail=raw[sy['ramdTailImage']-0x2000:sy['ramdTailImageEnd']-0x2000]
            select(0);m.write(0x2000,raw)
            for page in (0,3):
                select(page);m.write(0xFF00,tail)
            select(0);m.write(0xFFC0,raw[sy['driverBufferedStart']-0x2000:sy['driverBufferedEnd']-0x2000])
            for size in (256,8192):
                for write in (False,True):
                    code=assemble(f'''    ORG 01000h
    ld hl,04000h
    ld de,02000h
    ld b,{size//256}
again:
    ld c,2
    push bc
    push de
    push hl
    call 0{0xFFD0 if write else 0xFFC0:04X}h
    pop hl
    pop de
    pop bc
    inc h
    inc d
    dec b
    jp nz,again
    jp 01800h
''')
                    save_row(('RAMD R4' if name=='regs4' else name)+('-write' if write else '-read'),0,0 if write else 3,
                             0x4000 if write else 0x2000,3 if write else 0,
                             0x2000 if write else 0x4000,size,code)
        cpm=(ROOT/'bin/copybench/CPMCOPY.bin').read_bytes()
        for page in (0,1,2,3):
            select(page);m.write(0xFF00,cpm)
        for size in (256,8192):
            for caller,srcPage,dstPage in [(0,3,0),(0,0,3),(1,3,2)]:
                src=0x4000 if srcPage==0 else 0x2000
                dst=0x4000 if dstPage==0 else 0x5000
                code=assemble(f'''    ORG 01000h
    ld hl,0{src:04X}h
    ld (0FFD0h),hl
    ld hl,0{dst:04X}h
    ld (0FFD2h),hl
    ld hl,{size//16}
    ld (0FFD4h),hl
    ld a,{srcPage-1 if srcPage else 255}
    ld (0FFD6h),a
    ld a,{dstPage-1 if dstPage else 255}
    ld (0FFD7h),a
    ld a,{caller-1 if caller else 255}
    ld (0FFD8h),a
    call 0FF00h
    jp 01800h
''')
                save_row('CPM16',caller,srcPage,src,dstPage,dst,size,code)
        # После замены хвостов штатный DOS больше не вызывается.
        runtime=(ROOT/'bin/banked/TRANSPORT.bin').read_bytes()
        window=(ROOT/'bin/banked/WINDOW.bin').read_bytes()
        for page in (0,1,2,3):
            select(page);m.write(0xFC00,runtime);m.write(s['bankPage'],bytes([page]))
        select(0);m.write(0xFFC0,window)
        for size in (1,4,20,256,512,8192):
            for caller,srcPage,dstPage in [(0,3,0),(0,0,3),(1,3,2)]:
                src=0x4000 if srcPage==0 else 0x2000
                dst=0x4000 if dstPage==0 else 0x5000
                code=assemble(f'''    ORG 01000h
    ld hl,request
    call 0{s['bankCopyFar']:04X}h
    jp c,01801h
    jp 01800h
request:
    DB {srcPage}
    DW 0{src:04X}h
    DB {dstPage}
    DW 0{dst:04X}h
    DW {size}
''')
                save_row('FAR',caller,srcPage,src,dstPage,dst,size,code)
        report=dict(cpuHz=2000000,repeats=3,units='decimal kB/s; emulated CPU milliseconds',rows=rows,
                    binaries={name:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for name,path in
                              [('transport','bin/banked/TRANSPORT.bin'),('RAMD','bin/RAMD.i80'),('DOS','bin/DOS.sys')] + [(n,'bin/copybench/'+n+'.bin') for n in ('stack','loop','unrolled','direct','regs2','regs4','regs6','builtin_byte','builtin_block','CPMCOPY')]})
        (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
        subprocess.run(['python',str(ROOT/'source-apps/copybench/report.py'),str(out)],check=True)
    finally:matrix.stop()

if __name__=='__main__':main()
