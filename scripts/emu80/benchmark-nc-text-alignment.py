"""PIT and model-8080 comparison: real H: names at byte-aligned vs centered X.

run(machine, nc_symbols, output) is invoked only after normal NC keyboard input
has selected a 6x8 font module and H: root. This is an isolated renderer benchmark;
it patches only the owned emulator RAM and does not resume NC afterwards.
"""
from pathlib import Path
import hashlib
import json
import runpy
import statistics
import subprocess

ROOT=Path(__file__).resolve().parents[2]
W=lambda x:int(x).to_bytes(2,'little')


def run(m,n,out,font='Compact',dataset=None):
    out=Path(out);out.mkdir(parents=True,exist_ok=True)
    tools=runpy.run_path(str(ROOT/'scripts/tests/benchmark-nc-color-scroll.py'))
    m.call('debug_break')
    panel=m.read(n['state.activePanel'],1)[0]
    assert m.read(n['ncFontWidth'],1)==b'\x06'
    assert m.read(n['state.panelA_drive']+panel,1)==b'\x07'
    cache_count=m.read(n['state.panelA_filesCnt']+panel,1)[0]
    count=cache_count
    visible=n['P_FILE_LIST_H']*2
    assert count>=visible
    slot=m.read(n['vars.cacheSlotA']+panel,1)[0]
    base=int.from_bytes(m.read(n['ncCatalogBases']+2*slot,2),'little')
    records=m.read(base,count*16)
    indices=m.read(n['FILE_INDEX_A']+panel*256,count)
    if dataset:
        records=(Path(dataset)/'H-records.bin').read_bytes()
        indices=(Path(dataset)/'H-index.bin').read_bytes()
        count=len(indices)
    ordered=[records[i*16:(i+1)*16] for i in indices]
    assert all(len(row)==16 for row in ordered)
    (out/'H-records.bin').write_bytes(records)
    (out/'H-index.bin').write_bytes(indices)
    # No hardware registers are read for the snapshot, especially IO_COLOR.
    snapshot=bytearray(m.read(0,0xff00)+bytes(256))
    (out/'machine.bin').write_bytes(snapshot)
    origin=n['NC_SORT_KEYS'];strings=origin+0x400;rows=origin+0x600
    assert rows+visible*16<=n['FILE_CACHE_A']
    asm=f'''    INCLUDE "{(ROOT/'source/include/mxos.inc').as_posix()}"
    ORG {origin}
entry:
    ld sp,08F00h
    call clearPanel
    call pitTimerInit
    jp c,timerError
    xor a
    ld (error),a
    call pitTimerRead
    ld (started),hl
    call renderPanel
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
done:
    jp done
timerError:
    ld a,1
    ld (error),a
    jp done
clearPanel:
    ld a,{n['COLOR_FILE']}
    ld (IO_COLOR),a
    ld ({n['ncCurrentColor']}),a
    ld hl,{8+panel*192}
    ld bc,80
    ld de,{n['SCROLL_TOP']*256+n['SCROLL_HEIGHT']}
    xor a
    call {n['uiFillRectPixels']}
    ld hl,{104+panel*192}
    ld bc,80
    ld de,{n['SCROLL_TOP']*256+n['SCROLL_HEIGHT']}
    xor a
    jp {n['uiFillRectPixels']}
renderPanel:
    ld a,(scope)
    cp 2
    ret z
    ld hl,{strings}
    or a
    jp z,sourceReady
    ld hl,{rows}
sourceReady:
    ld de,{n['P_FILE_LIST_X1']*256+n['P_FILE_LIST_Y']}
    ld b,{visible}
renderRow:
    push bc
    push de
    push hl
    ex de,hl
    ld a,(xShift)
    add a,h
    ld h,a
    call {n['setCursorPosPanel']}
    ex de,hl
    ld a,(scope)
    or a
    jp nz,formatRecord
    call text12
    jp rowDone
formatRecord:
    call {n['printFileNamOnPanel']}
rowDone:
    pop hl
    pop de
    pop bc
    push de
    ld de,13
    ld a,(scope)
    or a
    jp z,strideReady
    ld de,16
strideReady:
    add hl,de
    pop de
    ld a,e
    add a,10
    ld e,a
    cp {n['P_FILE_LIST_Y_MAX']}
    jp c,nextRow
    ld de,{n['P_FILE_LIST_X2']*256+n['P_FILE_LIST_Y']}
nextRow:
    dec b
    jp nz,renderRow
    ret
text12:
    ld bc,12
    jp bios_printStringRawLim
xShift: DB 0
scope: DB 0
error: DB 0
started: DW 0
elapsed: DW 0
    INCLUDE "{(ROOT/'source/include/timing-pit.inc').as_posix()}"
    ASSERT $<{strings}
'''
    (out/'bench.asm').write_text(asm,encoding='utf-8')
    p=subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',
        '--raw='+str(out/'bench.bin'),'--sym='+str(out/'bench.sym'),
        '--lst='+str(out/'bench.lst'),str(out/'bench.asm')],capture_output=True)
    if p.returncode:raise RuntimeError(p.stdout+p.stderr)
    s=tools['symbols'](out/'bench.sym');code=(out/'bench.bin').read_bytes()
    patched=bytearray(snapshot);patched[origin:origin+len(code)]=code
    # Both layouts use exactly the same 12-byte BIOS call. Exclude the cursor
    # padding pixel; only the actual name text belongs to this comparison.
    at=patched.index(b'\xcd'+W(n['bios_printStringReal']),n['printFileNamOnPanel'],n['ncPrintDetailedFile'])
    patched[at+1:at+3]=W(s['text12'])
    patched[n['ncNamePadding']]=0xc9
    m.write(origin,code);m.write(at+1,W(s['text12']));m.write(n['ncNamePadding'],b'\xc9')
    starts=sorted(set((0,(count-visible)//2,count-visible)))
    report=dict(cpu_hz=2000000,pit_tick_ms=.128,repeats=5,drive='H:',panel=panel,
        root_records=count,source_cache_count=cache_count,visible_rows=visible,font=font,results=[],
        font_sha256=hashlib.sha256(m.read(n['FONT_ENGINE_BASE'],0x1800)).hexdigest(),
        nc_sha256=hashlib.sha256((ROOT/'bin/NC.i80').read_bytes()).hexdigest(),
        notes='Text only; no DOS, sorting, prerender preparation, frame, status or selection. All rows use BIOS RawLim length 12.')

    def pit(scope,shift):
        m.write(s['scope'],bytes([scope]));m.write(s['xShift'],bytes([shift]))
        m.stop_at(s['done']);m.call('regs_set',pc=s['entry'],sp=0x8f00)
        m.until(s['done'],60,page=0)
        assert m.read(s['error'],1)==b'\0','PIT did not stabilize'
        return int.from_bytes(m.read(s['elapsed'],2),'little')

    empty=[pit(2,0) for _ in range(5)]
    report['empty_ticks']=empty;overhead=statistics.median(empty)
    for first in starts:
        subset=ordered[first:first+visible]
        prepared=[]
        for row in subset:
            clean=lambda data:bytes(c if c>=32 else 32 for c in data)
            sep=b'\\' if row[11]&16 and row[0]!=ord('\\') else b' '
            prepared.append(clean(row[:8])+sep+clean(row[8:11])+b'\0')
        data=b''.join(prepared);raw=b''.join(subset)
        m.write(strings,data);m.write(rows,raw)
        local=bytearray(patched);local[strings:strings+len(data)]=data;local[rows:rows+len(raw)]=raw
        screens={}
        for scope,label in [(0,'prepared'),(1,'formatted')]:
            for shift,name in [(0,'aligned'),(2,'centered')]:
                local[s['scope']]=scope;local[s['xShift']]=shift
                clean=tools['execute'](local,s['clearPanel'],out)[0]
                final,cycles,_,_=tools['execute'](clean,s['renderPanel'],out,start=s['renderPanel'])
                cycles-=20 # Common RET + harness JP.
                screens[(scope,shift)]=bytes(final[0x9000:0xc000])
                ticks=[pit(scope,shift) for _ in range(5)]
                actual=m.read(0x9000,12288)
                # Only the active file fields were cleared/repainted in the
                # model snapshot; all bitmap bytes must match the emulator.
                assert actual==screens[(scope,shift)],(first,label,name,'Emu80/model bitmap mismatch')
                result=dict(first=first,scope=label,layout=name,cycles=cycles,
                    model_ms=cycles/2000,pit_ticks=ticks,
                    pit_ms=(statistics.median(ticks)-overhead)*.128)
                report['results'].append(result)
                if first==0:m.screenshot(out/f'{label}-{name}.png')
                print(result,flush=True)
        for shift in (0,2):assert screens[(0,shift)]==screens[(1,shift)],'prepared/formatter mismatch'
        a,c=screens[(0,0)],screens[(0,2)]
        bit=lambda screen,x,y:bool(screen[x//8*256+y]&(128>>(x%8)))
        for left in (8+panel*192,104+panel*192):
            for y in range(n['SCROLL_TOP'],n['SCROLL_BOTTOM']):
                for x in range(left,left+80):
                    expected=bit(a,x-4,y) if left+4<=x<left+76 else False
                    assert bit(c,x,y)==expected,(first,x,y,'centered text shifted incorrectly')
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    lines=['# NC: выравнивание текста панели H:', '',
        'Измерено на видимом Emu80 с ВИ53 и отдельно на модели Intel 8080, 2 МГц. Это не замер на физическом компьютере.', '',
        f'Движок {font}. Каталог H: получен обычной командой NC до начала опыта: {count} записей, сортировка панели сохранена. Один вывод — {visible} имён в двух колонках по {n["P_FILE_LIST_H"]} строк. Для каждого имени выводятся ровно 12 байт через bios_printStringRawLim. Начала колонок относительно панели: X=8/104 и X=12/108 (смещение +4 пикселя, центр в поле шириной 80 пикселей).', '',
        'prepared измеряет готовые поля 8+1+3, позиционирование и ограниченный вывод. formatted дополнительно включает штатное построение такого поля из FILE_INFO. Не включены DOS, сортировка, подготовка шрифта, очистка области, рамки, статус и выделение. ВИ53 стабилизируется до начальной отметки; вычтена пустая граница измерения. На каждой строке таблицы медиана пяти прогонов.', '',
        '| Первое имя | Подготовка | Положение | ВИ53, мс | Модель, мс |',
        '|---:|---|---|---:|---:|']
    for row in report['results']:
        lines.append(f'| {row["first"]} | {row["scope"]} | {row["layout"]} | {row["pit_ms"]:.3f} | {row["model_ms"]:.3f} |')
    lines+=['', 'Битмапы Emu80 и модели совпали. Подготовленные и сформированные на лету поля дают одинаковое изображение; центрированный вариант проверен как точный сдвиг на четыре пикселя. Исходные строки каталога, индекс, снимок памяти, код стенда, листинг, снимки экрана и точные значения сохранены рядом. Рабочая геометрия текста NC этим опытом не менялась.']
    (out/'report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    return report


def main():
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fonts',default='Rotate,Compact,Packed6K,Fast12K')
    parser.add_argument('--out',type=Path,default=ROOT/'docs/nc/text-align-2026-11-15')
    args=parser.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    h['Matrix'].__init__.__globals__.update(RUNTIME=ROOT/'.local/nc-sort-benchmark-runtime',OUT=args.out)
    n=runpy.run_path(str(ROOT/'scripts/tests/benchmark-nc-color-scroll.py'))['symbols'](ROOT/'bin/NC.sym')
    reports=[];reference=None
    for font in args.fonts.split(','):
        assert font in ('Rotate','Compact','Packed6K','Fast12K')
        machine=h['Matrix']()
        try:
            machine.start('text-alignment-'+font,disk=ROOT/'images/FlashDisk_emu80_seed.bin')
            m=machine.m
            def command(value):
                m.tap('esc');m.call('emu_run_for',ms=250)
                for char in value:m.tap({' ':'space',':':'colon','/':'slash'}.get(char,char.lower()))
                assert m.read(n['vars.cmdLine'],len(value)).upper()==value.upper().encode()
                m.tap('enter')
                if value.startswith('FONT '):
                    m.stop_at(n['ncReadKey'])
                    for _ in range(120):
                        m.until(n['ncReadKey'],120,page=0)
                        if (m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32] and
                            int.from_bytes(m.read(n['vars.cmdLineEnd'],2),'little')==n['vars.cmdLine']):break
                        m.call('debug_step',mode='into')
                    else:raise AssertionError('FONT return timeout')
                    m.call('bp_clear')
                m.call('emu_run_for',ms=1500)
            command('FONT '+font);command('H:');command('CD /')
            report=run(m,n,args.out/font,font,reference)
            reports.append(report)
            if reference is None:reference=args.out/font
        finally:
            machine.stop()
    (args.out/'all-results.json').write_text(json.dumps(reports,indent=2)+'\n')
    lines=['# NC: цена центрирования имён 6×8', '',
           '38 реальных имён H: в двух колонках; один и тот же каталог для всех движков. Вывод ровно 12 байт через bios_printStringRawLim. Выравнивание X=8/104, центрирование X=12/108 относительно панели. ВИ53 в Emu80, 2 МГц; медиана пяти прогонов. DOS, сортировка, подготовка пререндера, очистка, рамки и выделение исключены. Это не физическое железо.', '',
           '| Движок | Готовые строки: ровно, мс | Со сдвигом, мс | Изменение | С формированием: ровно, мс | Со сдвигом, мс |',
           '|---|---:|---:|---:|---:|---:|']
    for report in reports:
        find=lambda scope,layout:next(row for row in report['results'] if row['first']==0 and row['scope']==scope and row['layout']==layout)
        a,c=find('prepared','aligned')['pit_ms'],find('prepared','centered')['pit_ms']
        f,g=find('formatted','aligned')['pit_ms'],find('formatted','centered')['pit_ms']
        lines.append(f'| {report["font"]} | {a:.3f} | {c:.3f} | {(c/a-1)*100:+.1f}% | {f:.3f} | {g:.3f} |')
    lines+=['', 'Compact всегда использует скалярный вывод, поэтому для него выравнивание не меняет число тактов. В Rotate/Packed6K/Fast12K есть пакетный путь по четыре символа и скалярные голова/хвост. Подробные результаты трёх окон каталога, сверка с моделью 8080, снимки экрана и исходные входы находятся в подкаталогах движков. Все битмапы проверены: центрированный вариант совпадает со сдвигом ровно на четыре пикселя; готовые поля и форматирование FILE_INFO дают одинаковый результат. Геометрия рабочего NC этим экспериментом не менялась.']
    (args.out/'report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print('ALL FONT TEXT ALIGNMENT PASS',flush=True)


if __name__=='__main__':main()
