"""Парный тест штатного, предварительно сдвинутого 5x7 и быстрого 8x8 рендера."""
import argparse
import hashlib
import json
from pathlib import Path
import runpy

ROOT=Path(__file__).resolve().parents[2]
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-session.py'))
session,symbols,screenshot=(h[n] for n in ['session','symbols','screenshot'])
font8=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-font8.py'))['font_bytes']
CODE=65


def shifted_tables():
    # Создаём варианты только A. Сохраняем адресацию полного 12-КБ генератора,
    # чтобы тест не убирал вычисление адреса глифа из ускоренного рендера.
    glyph=(ROOT/'assets/system-rom/FONT.FNT').read_bytes()[CODE*8:CODE*8+8]
    data=bytearray(0x3000)
    for y,v in enumerate(glyph):
        data[CODE*8+y]=(v<<2)&0xFC
        data[0x800+CODE*8+y]=v&0x3F
        data[0x1000+CODE*16+y*2:0x1000+CODE*16+y*2+2]=bytes([(v>>2)&0x0F,(v<<6)&0xC0])
        data[0x2000+CODE*16+y*2:0x2000+CODE*16+y*2+2]=bytes([(v>>4)&3,(v<<4)&0xF0])
    return bytes(data)


def reference(rows,mode):
    frame=bytearray(0x3000)
    if mode==2:
        glyph=font8()[CODE*8:CODE*8+8]
        for row in range(rows):
            for col in range(48):
                at=col*256+row*8
                frame[at:at+8]=glyph
    else:
        glyph=(ROOT/'assets/system-rom/FONT.FNT').read_bytes()[CODE*8:CODE*8+8]
        for row in range(rows):
            for col in range(48):
                for y,v in enumerate(glyph):
                    for bit in range(6):
                        x=col*6+bit
                        if v&(1<<(5-bit)):
                            frame[(x//8)*256+row*8+y]|=1<<(7-x%8)
    return bytes(frame)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--repeats',type=int,default=3)
    p.add_argument('--rows',type=int,nargs='+',default=[8,24])
    p.add_argument('--modes',type=int,nargs='+',choices=[0,1,2,3],default=[0,1,2,3])
    p.add_argument('--report',type=Path,default=ROOT/'docs/bench/shifted-font-001/results.json')
    args=p.parse_args()
    if args.repeats<1 or any(n<1 or n>31 for n in args.rows):
        p.error('repeats must be positive; rows must be 1..31')
    results=[]
    cache=shifted_tables()
    font=(ROOT/'assets/system-rom/FONT.FNT').read_bytes()
    names=['standard','shifted5x7','fast8x8','optimized_standard']
    glyph_entry=symbols('DOS')['dos_piece_glyph_1_start']
    original_entry=(ROOT/'bin/DOS.sys').read_bytes()[glyph_entry-0xC000:glyph_entry-0xC000+3]
    with session(args.runtime) as m:
        m.write(0x3000,cache);m.write(0x6000,font8());m.write(0xE300,font)
        def run(mode,batch,bank,rows):
            m.call('bp_clear');m.write(0xFFFC,b'\0')
            m.write(glyph_entry,original_entry)
            # Приводим полиморфные инструкции штатного рендера в normalVideo
            # одним нетаймируемым выводом, затем очищаем экран и позицию.
            m.write(0x8FE7,(0xE300//8).to_bytes(2,'little'))
            m.write(0x8FEA,b'\0\xff');m.write(0x8FFA,b'\0\0\x07\0')
            m.call('bp_set',addr=0x8100,type='exec')
            m.subroutine(0xC809,bc=CODE)
            m.call('bp_clear')
            m.write(0x9000,bytes(0x3000));m.write(0x8FFC,b'\x07\0');m.write(0xFFF8,b'\xF0')
            render=(ROOT/f'bin/SHIFT_M{mode}_B{int(batch)}.i80').read_bytes()
            m.write(0xEB00,render)
            if mode==3:
                m.write(0xED00,(ROOT/'bin/OPT_DRAW.i80').read_bytes())
                m.write(glyph_entry,bytes.fromhex('C3 00 ED'))
            m.write(0xF880,(ROOT/'bin/SHIFTDISP.i80').read_bytes())
            m.write(0xFFC0,(ROOT/'bin/BUFFERGATE.i80').read_bytes())
            name=f'SHIFTBEN{int(batch)}'
            sy=symbols(name)
            body=bytearray((ROOT/f'bin/{name}.i80').read_bytes())
            for label,target in [('output',0xFFC0 if bank else (0xEB03 if batch else 0xEB00)),
                                 ('lineOutput',0xFFC0 if bank else 0xEB06)]:
                offset=sy[label]-0x100+1
                body[offset:offset+2]=target.to_bytes(2,'little')
            offset=sy['text']-0x100
            body[offset:offset+49]=b'A'*48+b'\0'
            if bank:
                m.write(0xFFFD,b'\0')
            m.write(0x100,body)
            m.call('regs_set',pc=sy['entry'],sp=0x8000,de=rows,hl=0x5678,bc=0x1234,af=0x0097)
            m.call('bp_set',addr=sy['done'],type='exec')
            start=m.call('cpu_state')['cpu_ticks']
            for _ in range(120):
                m.call('emu_run_for',ms=500)
                state=m.call('cpu_state')
                if state['registers']['pc']['dec']==sy['done']:
                    break
            else:
                raise RuntimeError('Benchmark timeout')
            ticks=state['cpu_ticks']-start
            assert state['registers']['sp']['dec']==0x8000
            assert m.read(0x100,len(body))==body,'Caller corrupted or wrong return page'
            m.write(0xFFFC,b'\0')
            frame=m.read(0x9000,0x3000)
            assert frame==reference(rows,mode),(mode,batch,bank,'Bitmap differs from pixel reference')
            assert m.read(0x8FFC,2)==bytes([(rows-1)*8+7,192 if mode==2 else 144])
            assert m.read(0x3000,len(cache))==cache
            return dict(ticks=ticks,seconds_2mhz=ticks/2_000_000,
                        chars_per_second=rows*48*2_000_000/ticks,
                        screen_sha256=hashlib.sha256(frame).hexdigest())
        for rows in args.rows:
            for mode in args.modes:
                for batch in (False,True):
                    samples=[dict(main=run(mode,batch,False,rows),page0=run(mode,batch,True,rows)) for _ in range(args.repeats)]
                    assert len({(s['main']['ticks'],s['page0']['ticks']) for s in samples})==1
                    result=dict(renderer=names[mode],rows=rows,columns=48,characters=rows*48,
                                buffered=batch,glyph_calls=rows*(3 if batch else 48),newlines=rows-1,samples=samples)
                    results.append(result);print(json.dumps(result),flush=True)
        args.report.parent.mkdir(parents=True,exist_ok=True)
        screenshot(m,args.report.with_suffix('.png'))
        artifacts=['SHIFTDISP','SHIFTBEN0','SHIFTBEN1','BUFFERGATE','OPT_DRAW']+[f'SHIFT_M{mode}_B{batch}' for mode in range(4) for batch in range(2)]
        report=dict(clock_hz=2_000_000,glyph='A',glyph_rows=8,prepared_shift_bytes=48,
                    cache_address_span=len(cache),cache_phases=[0,2,4,6],scrolling=False,
                    raw_batch_no_control_parser_modes=[1,2],repeats=args.repeats,
                    optimized_standard_bios_frontend_retained=True,
                    artifact_sha256={n:hashlib.sha256((ROOT/f'bin/{n}.i80').read_bytes()).hexdigest() for n in artifacts},
                    dos_sha256=hashlib.sha256((ROOT/'bin/DOS.sys').read_bytes()).hexdigest(),results=results)
        args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')


if __name__=='__main__':
    main()
