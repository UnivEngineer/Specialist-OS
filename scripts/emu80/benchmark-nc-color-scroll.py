"""Production NC scrolling: monochrome baseline, hybrid, runs and inline colors.

CPU campaign is standalone. run_pit is called by check-nc-info only in its
owned visible/muted emulator, after ordinary keyboard FONT Rotate / H: input.
"""
from pathlib import Path
import hashlib,json,random,runpy,statistics,struct,subprocess

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/nc/color-scroll-production-2026-11-15'
H=runpy.run_path(str(ROOT/'scripts/tests/benchmark-nc-color-scroll.py'))
W=lambda x:int(x).to_bytes(2,'little')
sy=H['symbols']


def call(m,n,entry,hl=0,bc=0,de=0,a=0):
    m=bytearray(m)
    code=b'\x3e'+bytes([a])+b'\x21'+W(hl)+b'\x01'+W(bc)+b'\x11'+W(de)+b'\xcd'+W(n[entry])+b'\xc9'
    m[0x8000:0x8000+len(code)]=code
    return H['execute'](m,0x8000,OUT)[0]


def datasets():
    image=(ROOT/'images/H-Flash-2M-Games.bin').read_bytes()
    word=lambda a:struct.unpack_from('<H',image,a)[0]
    root=(word(14)+image[16]*word(22))*word(11)
    records=[]
    for at in range(root,root+word(17)*32,32):
        row=image[at:at+32]
        if row[0]==0:break
        if row[0]==229 or row[11]&8:continue
        records.append(row[:12]+row[20:22]+row[28:30])
    key=lambda r:(not bool(r[11]&16),r[8:11].upper(),r[:8].upper())
    mixed=[]
    exts=['COM','TXT','SYS','ZIP','RKS','EXE','FNT','DLL','ARJ','ASM']
    for i in range(128):
        ext=('DIR' if i<8 else exts[(i-8)%len(exts)]).encode()
        mixed.append(f'FILE{i:04}'.encode()+ext+bytes([16 if i<8 else 0,0,0])+W(i*311&65535))
    shuffled=mixed[8:];random.Random(0x8080).shuffle(shuffled)
    return {'H-root':sorted(records,key=key),'mixed-extension':sorted(mixed,key=key),
            'mixed-unsorted':mixed[:8]+shuffled}


def seed(font,mono):
    n=sy(ROOT/('.local/nc-colors-bench/NC-mono.sym' if mono else 'bin/NC.sym'))
    m=bytearray(65536)
    files=[(0,ROOT/('.local/nc-colors-bench/NC-mono.i80' if mono else 'bin/NC.i80')),
           (0xc000,ROOT/'bin/BIOS.sys'),(0xf000,ROOT/'bin/IO.main.bin'),
           (0xd800,ROOT/f'bin/fonts/{font}.i80')]
    for at,path in files:
        data=path.read_bytes();m[at:at+len(data)]=data
    b=sy(ROOT/'bin/BIOS.sym')
    m[b['mon2_vars']:b['mon2_vars']+b['MON2_VARIABLES']]=(ROOT/'bin/BIOS.defaults.bin').read_bytes()[:b['MON2_VARIABLES']]
    m[b['bios_upper.escMode']]=255
    m=call(m,n,'FONT_ENGINE_INIT');m=call(m,n,'ncLayoutInit')
    m[n['loadFiles']]=0xc9
    m[n['state.activePanel']]=0;m[n['vars.cacheSlotA']]=0
    return m,n


def prepare(m,n,records,first,direction,kept):
    m=bytearray(m);count=len(records);base=n['FILE_CACHE_A'];index=n['FILE_INDEX_A']
    m[base:base+count*16]=b''.join(records)
    m[index:index+count+1]=bytes(range(count))+b'\xff'
    m[n['state.panelA_filesCnt']]=count;m[n['state.panelA_firstFile']]=first
    cursor=first+(m[n['ncVisibleCount']]-1 if direction==1 else 0)
    m[n['state.panelA_curFile']]=cursor
    m[n['vars.fileListPtr']:n['vars.fileListPtr']+2]=W(index+first)
    m[n['ncCurrentColor']]=m[n['IO_COLOR']]=n['COLOR_FILE']
    m=call(m,n,'ncPrintAdaptiveList')
    m[n['ncCurrentColor']]=m[n['IO_COLOR']]=n['COLOR_CURSOR']
    m=call(m,n,'drawCursor')
    m[n['vars.scrollRows']]=1;m[n['vars.scrollCursorKept']]=int(kept)
    return m


def step_code(n,m,first,direction,kept):
    shift=1 if direction==1 else -1
    code=b''
    if not kept:
        code=(b'\x3e'+bytes([n['COLOR_FILE']])+b'\x32'+W(n['IO_COLOR'])+
              b'\x32'+W(n['ncCurrentColor'])+b'\xcd'+W(n['drawCursor']))
    for address,value in [(n['state.panelA_firstFile'],first+shift),
                          (n['state.panelA_curFile'],m[n['state.panelA_curFile']]+shift)]:
        code+=b'\x3e'+bytes([value])+b'\x32'+W(address)
    code+=b'\x21'+W(n['FILE_INDEX_A']+first+shift)+b'\x22'+W(n['vars.fileListPtr'])
    code+=b'\x06'+bytes([direction])+b'\xcd'+W(n['scrollBitmapPanel'])+b'\xc9'
    return code


def select_method(m,n,method):
    m=bytearray(m)
    if method in ('runs','inline'):
        at=m.index(b'\xfe\x03\xd2',n['uiTextColors.countRuns'],n['uiTextColors.selectDirection'])
        m[at+1]=255 if method=='runs' else 0
    return m


def model():
    OUT.mkdir(parents=True,exist_ok=True)
    report={'cpu_hz':2000000,'pit_tick_ms':.128,'results':[],
            'scope':'Сдвиг всего файлового списка панели на один файл, все колонки: снятие старого курсора, если он не сохранён, перенос битмапа, чтение и обновление таблицы цветов, печать новых и восстанавливаемых имён, новый курсор. DOS, подготовка списка, сортировка, пререндер, рамки и статус не входят.',
            'nc_sha256':hashlib.sha256((ROOT/'bin/NC.i80').read_bytes()).hexdigest()}
    report['mono_sha256']=hashlib.sha256((ROOT/'.local/nc-colors-bench/NC-mono.i80').read_bytes()).hexdigest()
    report['font_sha256']={font:hashlib.sha256((ROOT/f'bin/fonts/{font}.i80').read_bytes()).hexdigest()
                           for font in ('Cpm4','Rotate','Compact','Fast8')}
    data=datasets()
    for font in ('Cpm4','Rotate','Compact','Fast8'):
        for mono in (True,False):
            original,n=seed(font,mono)
            for pattern,records in data.items():
                for first in (1,40):
                    for direction in (1,2):
                        kept=original[n['ncFontWidth']]==6
                        prepared=prepare(original,n,records,first,direction,kept)
                        for method in (('mono',) if mono else ('hybrid','runs','inline')):
                            m=select_method(prepared,n,method)
                            code=step_code(n,m,first,direction,kept)
                            m[0x8000:0x8000+len(code)]=code
                            result,cycles,_,_=H['execute'](m,0x8000,OUT)
                            reference=call(result,n,'ncPrintAdaptiveList')
                            reference[n['ncCurrentColor']]=reference[n['IO_COLOR']]=n['COLOR_CURSOR']
                            reference=call(reference,n,'drawCursor')
                            assert result[0x9000:0xc000]==reference[0x9000:0xc000],(font,pattern,method,direction)
                            report['results'].append(dict(font=font,pattern=pattern,first=first,direction=direction,
                                retained_cursor=kept,method=method,cycles=cycles,ms=cycles/2000))
        print(font,'full-panel model and redraw equivalence PASS',flush=True)
    (OUT/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    write_report(report)
    return report


def write_report(report):
    text=['# Цветная прокрутка NC: рабочий код','',report['scope'],'',
          'Модель 8080, 2 МГц. Сдвиг на один файл в обоих направлениях, два окна списка (first=1/40). Для 6×8 сохранённый курсор, как в обычной навигации NC; для 4×8 и 8×8 включено снятие старого курсора. Все варианты сверены с полной перерисовкой. Mono — отдельная сборка без раскраски, остальные различаются только выбором ядра переноса.','',
          'H-root — реальные записи штатного H:, отсортированные по расширению. Mixed — 128 записей FILE_INFO: восемь каталогов с расширением и 120 файлов COM/TXT/SYS/ZIP/RKS/EXE/FNT/DLL/ARJ/ASM; данные одинаковы для сортированного и перемешанного списков. Получение списка и сортировка в замер не входят.','',
          '| Шрифт | Список | Mono, мс | Hybrid, мс | Изменение | Только группы | Только inline |',
          '|---|---|---:|---:|---:|---:|---:|']
    for font in ('Cpm4','Rotate','Compact','Fast8'):
        for pattern in datasets():
            rows=[r for r in report['results'] if r['font']==font and r['pattern']==pattern]
            means={method:statistics.mean(r['ms'] for r in rows if r['method']==method) for method in ('mono','hybrid','runs','inline')}
            text.append(f'| {font} | {pattern} | {means["mono"]:.3f} | {means["hybrid"]:.3f} | {100*(means["hybrid"]/means["mono"]-1):+.1f}% | {means["runs"]:.3f} | {means["inline"]:.3f} |')
    if report.get('pit'):
        text+=['','## ВИ53 в Emu80','',
               'Rotate, смешанный список, first=40, сохранённый курсор, пять повторов; вычтена медиана пустого интервала. Подготовка экрана и загрузка тестового состояния вне интервала. Битмапы модели и эмулятора совпадают. Это эмуляторная проверка, не физическое железо.','',
               '| Список | Метод | Направление | ВИ53, мс | Модель, мс |','|---|---|---|---:|---:|']
        for row in report['pit']:text.append(f'| {row["pattern"]} | {row["method"]} | {row["direction"]} | {row["ms"]:.3f} | {row["model_ms"]:.3f} |')
    (OUT/'report.md').write_text('\n'.join(text)+'\n',encoding='utf-8')


def run_pit(m):
    report=json.loads((OUT/'results.json').read_text())
    report['pit']=[]
    for mono in (True,False):
        original,n=seed('Rotate',mono)
        assert m.read(n['FONT_ENGINE_BASE'],8)==original[n['FONT_ENGINE_BASE']:n['FONT_ENGINE_BASE']+8]
        for pattern in ('mixed-extension','mixed-unsorted'):
            for direction in (1,2):
                prepared=prepare(original,n,datasets()[pattern],40,direction,True)
                for method in (('mono',) if mono else ('hybrid','runs','inline')):
                    memory=select_method(prepared,n,method);step=step_code(n,memory,40,direction,True)
                    (OUT/'step.bin').write_bytes(step)
                    asm=f''' INCLUDE "{(ROOT/'sdk/asm/mxos.inc').as_posix()}"
 ORG 8000h
entry:
 ld sp,8F00h
 ld a,{n['COLOR_FILE']}
 ld (IO_COLOR),a
 ld ({n['ncCurrentColor']}),a
 call {n['ncPrintAdaptiveList']}
 ld a,{n['COLOR_CURSOR']}
 ld (IO_COLOR),a
 ld ({n['ncCurrentColor']}),a
 call {n['drawCursor']}
 call pitTimerInit
 jp c,failed
 call io_pitRead
 ld (started),hl
 ld a,(scope)
 or a
 call nz,step
 call io_pitRead
 ex de,hl
 ld hl,(started)
 ld a,l
 sub e
 ld l,a
 ld a,h
 sbc a,d
 ld h,a
 ld (elapsed),hl
done: jp done
failed: ld a,1
 ld (error),a
 jp done
scope: DB 0
error: DB 0
started: DW 0
elapsed: DW 0
step: INCBIN "{(OUT/'step.bin').as_posix()}"
 INCLUDE "{(ROOT/'sdk/asm/lib/timing-pit.inc').as_posix()}"
 ASSERT $<8B00h
'''
                    (OUT/'pit.asm').write_text(asm)
                    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',
                                    '--raw='+str(OUT/'pit.bin'),'--sym='+str(OUT/'pit.sym'),str(OUT/'pit.asm')],check=True,capture_output=True)
                    s=sy(OUT/'pit.sym');code=(OUT/'pit.bin').read_bytes()
                    memory[0x8000:0x8000+len(code)]=code
                    def sample(scope):
                        m.call('debug_break');m.write(0,memory[:0x8c00]);m.write(0x9000,memory[0x9000:0xc000])
                        b=sy(ROOT/'bin/BIOS.sym');m.write(b['mon2_vars'],memory[b['mon2_vars']:b['mon2_vars']+b['MON2_VARIABLES']])
                        m.write(s['scope'],bytes([scope]));m.stop_at(s['done']);m.call('regs_set',pc=s['entry'],sp=0x8f00)
                        m.until(s['done'],60,page=0)
                        assert m.read(s['error'],1)==b'\0'
                        return int.from_bytes(m.read(s['elapsed'],2),'little')
                    empty=statistics.median(sample(0) for _ in range(3))
                    ticks=[sample(1) for _ in range(5)]
                    model_memory=bytearray(memory);model_memory[0x8000:0x8000+len(step)]=step
                    result,cycles,_,_=H['execute'](model_memory,0x8000,OUT)
                    assert m.read(0x9000,12288)==result[0x9000:0xc000],(pattern,direction,method,'PIT bitmap')
                    row=dict(pattern=pattern,direction=direction,method=method,ticks=ticks,empty_ticks=empty,
                             ms=(statistics.median(ticks)-empty)*.128,model_ms=cycles/2000)
                    report['pit'].append(row)
                    print('PIT',row,flush=True)
    (OUT/'results.json').write_text(json.dumps(report,indent=2)+'\n');write_report(report)


if __name__=='__main__':model()
