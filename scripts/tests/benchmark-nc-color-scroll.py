"""Model-8080 experiment: NC bitmap scrolling with software row colors.

Prototypes only: current NC and installed font binaries are reused unchanged.
Outputs a reproducible report under ignored docs/nc/, not an installed program.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
TICKS = Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')
W = lambda x: int(x).to_bytes(2, 'little')
TRACE_STOP = 0x8cff


def symbols(path):
    return {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(encoding='utf-8-sig'), re.M)}


def unrolled_core(name, glyph_only, inline_color=False):
    pairs = 4 if glyph_only else 5
    asm = f'''
    MODULE {name}
entry:
    ld a,e
    or a
    ret z
    ld (source),hl
    ld h,b
    ld l,c
    ld (destination),hl
    ld a,d
    ld (width),a
    ld a,e
    ld (rows),a
    ld hl,0
    add hl,sp
    ld (restoreSP+1),hl
    ld hl,(source)
    ld a,b
    cp h
    jp c,forward
    jp nz,backward
    ld a,c
    cp l
    jp c,forward
backward:
    ld a,(rows)
    add a,a
    ld c,a
    add a,a
    add a,a
    add a,c
    ld c,a
    ld hl,(destination)
    add a,l
    ld l,a
    {'dec l' if glyph_only else ''}
    ld sp,hl
    ld hl,(source)
    ld a,c
    add a,l
    ld l,a
    {'dec l' if glyph_only else ''}
    ld a,(rows)
    ld b,a
backRow:
    DUP {pairs}
        dec l
        ld d,(hl)
        dec l
        ld e,(hl)
        push de
    EDUP
    {('dec l\n    dec l\n    dec sp\n    dec sp') if glyph_only else ''}
    dec b
    jp nz,backRow
    jp nextColumn
forward:
    ld hl,(source)
    {'inc l' if glyph_only else ''}
    ld sp,hl
    ld hl,(destination)
    {'inc l' if glyph_only else ''}
    ld a,(rows)
    ld b,a
forwardRow:
    DUP {pairs}
        pop de
        ld (hl),e
        inc l
        ld (hl),d
        inc l
    EDUP
    {('inc l\n    inc l\n    inc sp\n    inc sp') if glyph_only else ''}
    dec b
    jp nz,forwardRow
nextColumn:
restoreSP:
    ld sp,0
    ld hl,(source)
    inc h
    ld (source),hl
    ld hl,(destination)
    inc h
    ld (destination),hl
    ld a,(width)
    dec a
    ld (width),a
    ret z
    ld a,(direction)
    or a
    jp z,forward
    jp backward
source: DW 0
destination: DW 0
rows: DB 0
width: DB 0
direction: DB 0
end:
    ENDMODULE
'''.replace('    jp c,forward\n    jp nz,backward',
            '    jp c,setForward\n    jp nz,setBackward').replace(
                '    jp c,forward\nbackward:', '''    jp c,setForward
setBackward:
    ld a,1
    ld (direction),a
    jp backward
setForward:
    xor a
    ld (direction),a
    jp forward
backward:''')
    if inline_color:
        asm = asm.replace('setBackward:\n', '''setBackward:
    ld hl,(curColors)
    ld a,(rows)
    dec a
    ld e,a
    ld d,0
    add hl,de
    ld (colorBack),hl
''')
        asm = asm.replace('forward:\n    ld hl,(source)', '''forward:
    ld hl,(curColors)
    ld b,h
    ld c,l
    ld hl,(source)''')
        asm = asm.replace('    ld a,(rows)\n    ld b,a\nbackRow:', '''    ld (sourcePos),hl
    ld hl,(colorBack)
    ld b,h
    ld c,l
    ld hl,(sourcePos)
    ld a,(rows)
    ld (backCount+1),a
backRow:
    ld a,(bc)
    ld (IO_COLOR),a
    dec bc''')
        asm = asm.replace('    ld a,(rows)\n    ld b,a\nforwardRow:', '''    ld a,(rows)
    ld (forwardCount+1),a
forwardRow:
    ld a,(bc)
    ld (IO_COLOR),a
    inc bc''')
        asm = asm.replace('    dec b\n    jp nz,backRow', '''backCount:
    ld a,0
    dec a
    ld (backCount+1),a
    jp nz,backRow''')
        asm = asm.replace('    dec b\n    jp nz,forwardRow', '''forwardCount:
    ld a,0
    dec a
    ld (forwardCount+1),a
    jp nz,forwardRow''')
        asm = asm.replace('direction: DB 0', 'direction: DB 0\ncolorBack: DW 0\nsourcePos: DW 0')
    return asm


def source(n):
    return f'''
IO_COLOR EQU {n['IO_COLOR']}
CURSOR EQU {n['bios_vars.cursorY']}
    ORG 4000h
empty: ret
lookupShift:
    ld a,(walkDown)
    or a
    ld hl,5801h
    ld bc,5800h
    jp z,lookupReady
    dec hl
    inc bc
lookupReady:
    ld a,(visibleCount)
    dec a
    ld e,a
    ld d,0
    add hl,de
    ex de,hl
    ld hl,5801h
    ld a,(walkDown)
    or a
    jp z,lookupCopy
    dec hl
lookupCopy:
    jp {n['bios_memcpy_bc_hl']}
bulk:
    ld a,0F1h
    ld (IO_COLOR),a
    ld hl,bulkJob
    jp walkJobs
redraw:
    call bulk
    ld hl,textJob
    jp walkJobs
recolor:
    call bulk
    ld hl,touchJob
    jp walkJobs
rowColors:
    ld hl,rowJob
    jp walkJobs
runColors:
    ld hl,runJob
    jp walkJobs
inlineColors:
    ld hl,inlineJob
    jp walkJobs
walkJobs:
    ld (jobCall+1),hl
    ld a,(jobCount)
    ld (jobsLeft),a
    ld hl,5000h
nextJob:
    call wordIn
    ex de,hl
    ld (curSource),hl
    ex de,hl
    call wordIn
    ex de,hl
    ld (curDestination),hl
    ex de,hl
    ld a,(hl)
    inc hl
    ld (curWidth),a
    ld a,(hl)
    inc hl
    ld (curRows),a
    call wordIn
    ex de,hl
    ld (curColors),hl
    ex de,hl
    call wordIn
    ex de,hl
    ld (curRecord),hl
    ex de,hl
    ld (jobNext),hl
jobCall: call 0
    ld hl,jobsLeft
    dec (hl)
    ret z
    ld hl,(jobNext)
    jp nextJob
wordIn:
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ret
loadRect:
    ld hl,(curDestination)
    ld b,h
    ld c,l
    ld hl,(curSource)
    ld a,(curWidth)
    ld d,a
    ld a,(curRows)
    ld e,a
    ret
bulkJob:
    call loadRect
copyCall: call generic
    ret
inlineJob:
    call loadRect
inlineCall: jp colored8.entry
; Adapter: common prototype ABI uses E=text rows, current UI uses pixels.
generic:
    push bc
    ld a,e
    add a,a
    ld c,a
    add a,a
    add a,a
    add a,c
    ld e,a
    pop bc
    jp {n['uiCopyRect']}
genericEnd:
takeColor:
    ld hl,(curColors)
    ld a,(hl)
    inc hl
    ld (curColors),hl
    ret
textJob:
    ld hl,renderRow
    jp repairJob
touchJob:
    ld hl,touchCall
repairJob:
    ld (repairCall+1),hl
repairLoop:
    call takeColor
    cp 0F1h
    jp z,repairNext
    ld (IO_COLOR),a
repairCall: call 0
repairNext:
    call advanceOne
    ld hl,curRows
    dec (hl)
    jp nz,repairLoop
    ret
renderRow:
    ld hl,(curDestination)
    ld a,h
    sub 90h
    add a,a
    add a,a
    ld h,a
    ld a,l
    add a,8
    ld l,a
    ld (CURSOR),hl
    ld hl,(curRecord)
    jp {n['printFileNamOnPanel']}
touchCall: jp touch10
touch10:
    ld hl,(curDestination)
    ld a,(curWidth)
    ld b,a
touch10Column:
    push hl
    DUP 10
        ld a,(hl)
        ld (hl),a
        inc l
    EDUP
    pop hl
    inc h
    dec b
    jp nz,touch10Column
    ret
touch10End:
touch8:
    ld hl,(curDestination)
    ld a,(curWidth)
    ld b,a
touch8Column:
    push hl
    inc l
    DUP 8
        ld a,(hl)
        ld (hl),a
        inc l
    EDUP
    pop hl
    inc h
    dec b
    jp nz,touch8Column
    ret
touch8End:
rowJob:
    call prepareBottom
rowLoop:
    call takeDirectionalColor
    ld (IO_COLOR),a
    call loadRect
    ld e,1
    call copyCall
    call advanceDirectionalOne
    ld hl,curRows
    dec (hl)
    jp nz,rowLoop
    ret
runJob:
    call prepareBottom
runLoop:
    call takeDirectionalColor
    ld (IO_COLOR),a
    ld (curColor),a
    ld a,(curRows)
    dec a
    ld b,a
    ld c,1
scanRun:
    ld a,b
    or a
    jp z,runReady
    ld hl,(curColors)
    ld a,(curColor)
    cp (hl)
    jp nz,runReady
    call stepColor
    inc c
    dec b
    jp scanRun
runReady:
    ld a,b
    ld (curRows),a
    ld a,c
    ld (curBatch),a
    ld a,(walkDown)
    or a
    jp z,runTopReady
    ld a,(curBatch)
    dec a
    call rowsToPixels
    cpl
    inc a
    call advanceRect
runTopReady:
    call loadRect
    ld a,(curBatch)
    ld e,a
    call copyCall
    ld a,(walkDown)
    or a
    jp z,runAdvanceUp
    ld a,-10
    jp runAdvance
runAdvanceUp:
    ld a,(curBatch)
    call rowsToPixels
runAdvance:
    call advanceRect
    ld a,(curRows)
    or a
    jp nz,runLoop
    ret
rowsToPixels:
    add a,a
    ld e,a
    add a,a
    add a,a
    add a,e
    ret
prepareBottom:
    ld a,(walkDown)
    or a
    ret z
    ld a,(curRows)
    dec a
    call rowsToPixels
    call advanceRect
    ld hl,(curColors)
    ld a,(curRows)
    dec a
    ld e,a
    ld d,0
    add hl,de
    ld (curColors),hl
    ret
takeDirectionalColor:
    ld hl,(curColors)
    ld a,(hl)
    push af
    call stepColor
    pop af
    ret
stepColor:
    ld a,(walkDown)
    or a
    jp z,stepColorForward
    dec hl
    jp stepColorSave
stepColorForward:
    inc hl
stepColorSave:
    ld (curColors),hl
    ret
advanceDirectionalOne:
    ld a,(walkDown)
    or a
    ld a,10
    jp z,advanceRect
    ld a,-10
advanceRect:
    ld e,a
    ld hl,(curSource)
    add a,l
    ld l,a
    ld (curSource),hl
    ld hl,(curDestination)
    ld a,e
    add a,l
    ld l,a
    ld (curDestination),hl
    ret
advanceOne:
    ld a,10
    call advanceRect
    ld hl,(curRecord)
    ld de,16
    add hl,de
    ld (curRecord),hl
    ret
jobCount: DB 0
walkDown: DB 0
visibleCount: DB 0
jobsLeft: DB 0
jobNext: DW 0
curSource: DW 0
curDestination: DW 0
curWidth: DB 0
curRows: DB 0
curColors: DW 0
curRecord: DW 0
curColor: DB 0
curBatch: DB 0
driverEnd:
''' + unrolled_core('copy10', False) + unrolled_core('copy8', True) + \
        unrolled_core('colored10', False, True) + unrolled_core('colored8', True, True) + \
        '\nbenchEnd:\n    ASSERT $ < 5000h\n'


def execute(memory, entry, out, start=None, trace=False):
    m = bytearray(memory)
    # RUN's reserved area is outside NC's resizable scratch/catalog storage.
    code = b'\x31' + W(0x8f00) + b'\xcd' + W(entry) + b'\xc3' + W(TRACE_STOP)
    m[0x8c00:0x8c00 + len(code)] = code
    inp, dst = out / 'input.bin', out / 'output.bin'
    inp.write_bytes(m)
    command = [str(TICKS), '-m8080', '-pc', '8c00', '-end', f'{TRACE_STOP:x}',
               '-counter', '100000000', '-output', str(dst)]
    if start is not None:
        command += ['-start', f'{start:x}']
    if trace:
        command += ['-trace']
    p = subprocess.run(command + [str(inp)], check=True, capture_output=True, timeout=30)
    result = dst.read_bytes()
    assert result[65542:65544] == W(TRACE_STOP), (entry, p.stdout[-500:])
    cycles = int(p.stdout.strip().splitlines()[-1])
    return bytearray(result[:65536]), cycles, p.stdout if trace else None, m


def jobs_for(width, down, delta, n):
    columns, step, cell = {4: (3, 8, 6), 6: (2, 12, 10), 8: (1, 22, 22)}[width]
    rows, top = n['P_FILE_LIST_H'], n['SCROLL_TOP']
    jobs = []
    order = range(columns - 1, -1, -1) if down else range(columns)
    for col in order:
        x = n['SCROLL_LEFT'] + col * step
        if down:
            jobs.append(dict(src=x*256+top, dst=x*256+top+delta*10,
                             width=cell, rows=rows-delta, slot=col*rows+delta))
            if col:
                jobs.append(dict(src=(x-step)*256+top+(rows-delta)*10,
                                 dst=x*256+top, width=cell, rows=delta, slot=col*rows))
        else:
            jobs.append(dict(src=x*256+top+delta*10, dst=x*256+top,
                             width=cell, rows=rows-delta, slot=col*rows))
            if col < columns - 1:
                jobs.append(dict(src=(x+step)*256+top, dst=x*256+top+(rows-delta)*10,
                                 width=cell, rows=delta, slot=(col+1)*rows-delta))
    return [j for j in jobs if j['rows']], columns, step, cell


def palette(pattern, count):
    colors = [0xf1, 0xe1, 0xa1, 0xb1, 0xd1]
    if pattern == 'mono': return [0xf1] * count
    if pattern == 'sparse10': return [0xe1 if i % 10 == 3 else 0xf1 for i in range(count)]
    if pattern == 'grouped50': return [0xf1 if i < count//2 else 0xe1 for i in range(count)]
    if pattern == 'grouped100': return [colors[1 + min(3, i*4//count)] for i in range(count)]
    if pattern == 'alternating50': return [0xf1 if i%2 == 0 else 0xe1 for i in range(count)]
    if pattern == 'alternating100': return [colors[1+i%4] for i in range(count)]
    if pattern.startswith('runs'):
        length = int(pattern[4:])
        return [colors[1+(i//length)%4] for i in range(count)]
    state, result = 0x5a, []
    for _ in range(count):
        state = (state >> 1) ^ (0xb8 if state & 1 else 0)
        result.append(0xf1 if state % 4 else colors[1+(state>>2)%4])
    return result


def replay_trace(trace, initial, initial_attrs, final, wanted_attrs, write_observer=None):
    """Track writes from actual executed 8080 opcodes/registers, no color reads."""
    memory, attrs = bytearray(initial), bytearray(initial_attrs)
    color = memory[0xfff8]
    stats = dict(screen_reads=0, screen_writes=0, color_reads=0, color_writes=0,
                 wrong_visible_writes=0)
    regs = None

    def read(addr):
        if 0x9000 <= addr < 0xc000: stats['screen_reads'] += 1
        if addr == 0xfff8: stats['color_reads'] += 1
        return memory[addr]

    def write(addr, value):
        nonlocal color
        if write_observer is not None:
            write_observer(addr,value,memory[addr])
        memory[addr] = value
        if addr == 0xfff8:
            color = value
            stats['color_writes'] += 1
        if 0x9000 <= addr < 0xc000:
            at = addr-0x9000
            attrs[at] = color
            stats['screen_writes'] += 1
            if value and color != wanted_attrs[at]: stats['wrong_visible_writes'] += 1

    for line in trace.decode('ascii').splitlines():
        if line.startswith('pc='):
            v = re.match(r'pc=([0-9A-F]+), \[pc\]=([0-9A-F]+),\s+bc=([0-9A-F]+),\s+de=([0-9A-F]+),\s+hl=([0-9A-F]+),\s+af=([0-9A-F]+)', line)
            assert v, line
            pc, opcode, bc, de, hl, af = [int(s, 16) for s in v.groups()]
            regs = pc, opcode, bc, de, hl, af
        elif line.startswith('sp='):
            sp = int(line[3:7], 16)
            pc, op, bc, de, hl, af = regs
            assert memory[pc] == op, (hex(pc), hex(op), hex(memory[pc]))
            a, f = af >> 8, af & 255
            r = [bc >> 8, bc & 255, de >> 8, de & 255, hl >> 8, hl & 255, 0, a]
            imm = memory[pc+1] | memory[pc+2] << 8
            if op == 0x32: write(imm, a)
            elif op == 0x22:
                write(imm, hl & 255); write((imm+1)&65535, hl >> 8)
            elif op in (0x02, 0x12): write(bc if op == 2 else de, a)
            elif 0x70 <= op <= 0x77 and op != 0x76: write(hl, r[op & 7])
            elif op == 0x36: write(hl, memory[pc+1])
            elif op in (0x34, 0x35): write(hl, (read(hl)+(1 if op == 0x34 else -1))&255)
            elif op in (0xc5, 0xd5, 0xe5, 0xf5):
                value = [bc, de, hl, af][(op >> 4)&3]
                write((sp-1)&65535, value >> 8); write((sp-2)&65535, value & 255)
            elif op == 0xe3:
                read(sp); read((sp+1)&65535)
                write(sp, hl&255); write((sp+1)&65535, hl >> 8)
            elif op == 0xcd or op & 0xc7 == 0xc4:
                flags = [not f&0x40, bool(f&0x40), not f&1, bool(f&1),
                         not f&4, bool(f&4), not f&0x80, bool(f&0x80)]
                if op == 0xcd or flags[(op>>3)&7]:
                    write((sp-1)&65535, (pc+3) >> 8); write((sp-2)&65535, (pc+3)&255)
            if op in (0xc1, 0xd1, 0xe1, 0xf1): read(sp); read((sp+1)&65535)
            elif op == 0x3a: read(imm)
            elif op == 0x2a: read(imm); read((imm+1)&65535)
            elif op in (0x0a, 0x1a): read(bc if op == 0x0a else de)
            elif op & 0xc7 == 0x46 or op in (0x86,0x8e,0x96,0x9e,0xa6,0xae,0xb6,0xbe): read(hl)
    assert memory[0x9000:0xc000] == final[0x9000:0xc000], 'trace write model missed a VRAM opcode'
    for i, bitmap in enumerate(final[0x9000:0xc000]):
        assert attrs[i]&15 == wanted_attrs[i]&15, ('background attribute', i)
        if bitmap:
            assert attrs[i] == wanted_attrs[i], ('visible foreground attribute', i, attrs[i], wanted_attrs[i])
    assert stats['color_reads'] == 0
    return stats


def write_report(out, report):
    data = report['results']
    def find(font, method, core='copy8.entry', pattern='grouped50', direction='up'):
        return next(x for x in data if x['font']==font and x['method']==method and
                    x['core']==core and x['pattern']==pattern and x['direction']==direction)
    fmt = lambda value: f'{value:.2f}'.replace('.', ',')
    labels = {'redraw':'1a: общий перенос + повторный текст цветных строк',
              'recolor':'1b: общий перенос + чтение/запись цветных строк',
              'rowColors':'2a: отдельный вызов копировщика для каждой строки',
              'runColors':'2b: диапазоны одинакового цвета',
              'inlineColors':'2c: цвет внутри развёрнутого вертикального цикла'}
    text = '''# Цветная битмап-прокрутка NC: сравнение прототипов

Дата: 15.11.2026. Измерения модели Intel 8080 при 2 МГц, z88dk-ticks `-m8080`, без аппаратных задержек и IRQ. Это тактовая модель, не замер на физическом «Специалисте». Рабочий код NC, BIOS и движков не изменён; собраны отдельные экспериментальные ядра.

Вывод: атрибуты можно восстанавливать по программной таблице без чтения регистра цвета. Однопроходное копирование сразу с нужным цветом сохраняет цвет на каждом записанном байте. Для длинных одноцветных групп эффективен перенос диапазона, для частого чередования — установка цвета внутри развёрнутого цикла. Варианты с последующим восстановлением имеют промежуточный белый цвет; повторная печать текста особенно дорога.

## Границы измерения

Сдвиг на один файл в обоих направлениях, высота колонки 19 строк, ячейка 10 пикселей. Проверены три колонки по 6 байт (4×8), две по 10 байт (6×8) и одна по 22 байта (8×8). Копируется сохраняемая часть списка: 56, 37 и 18 строк соответственно, включая перенос между колонками. Одна новая строка, DOS, получение каталога, сортировка, обновление статуса и операции с выделением исключены; их стоимость общая для вариантов. Такты чтения lookup-таблицы, поиска диапазонов, установки цвета и вызовов копировщика включены.

Исходный экран сформирован настоящей `printFileNamOnPanel` из снимка NC и настоящими движками Cpm4, Compact, Rotate, Packed6K, Fast12K и Fast8. Для движков с пререндером сырые `.pre` загружены и attach выполнен до измерения. Подготовка шрифта и формирование исходного экрана в интервал не входят. Кольцо истории, файлы и диски не используются. Метаданные и код стенда размещены ниже 6000h, подготовленные плоскости — с 6400h, стек — с 8F00h.

Пять восьмибитных цветов в стенде используют одинаковый синий фон. Базовый цвет F1h — белый на синем; в текущем NC константа `COLOR_FILE=B1h`, замена этой константы не меняет тактов. Таблица цветов синтетическая: исследуется перенос уже известных атрибутов, классификация по расширению не измеряется. `grouped50` имитирует длинные одноцветные группы при сортировке по расширению; `alternating100` меняет цвет после каждого файла. Сами FILE_INFO во всех опытах одни и те же.

## Варианты

`generic` использует текущий `uiCopyRect` NC и текущий `bios_memcpy_bc_hl` из снимка BIOS. `copy10` — специальный копировщик текстовых ячеек с развёрнутыми пятью парами байтов; `copy8` — четыре пары глифа с пропуском двух пустых строк. Во всех таблицах ниже, кроме отдельного сравнения копировщиков, используется `copy8`.

1a сначала переносит сохраняемую часть базовым цветом, затем заново вызывает реальную печать имени/размера для каждой цветной строки. 1b вместо рендера устанавливает цвет и выполняет развёрнутые `MOV A,M / MOV M,A` над строкой. 2a устанавливает цвет и вызывает прямоугольный копировщик отдельно для каждой строки. 2b ищет подряд идущие одинаковые атрибуты в пределах физического прямоугольника и переносит диапазон одним вызовом. Прототип 2c сохраняет SP и выполняет весь прямоугольник внутри одного вызова; перед каждым восьмибайтным вертикальным фрагментом читает атрибут из lookup-таблицы и записывает регистр цвета. Цвет задаётся повторно для каждого байтового столбца, зато отсутствуют повторные входы в копировщик для каждой текстовой строки.

При движении вниз 2a/2b обходят строки и диапазоны снизу вверх. Верхний проход с отдельными строками затёр бы ещё не прочитанный источник; это проверяется побайтовым сравнением с эталоном.

## Результат для 6×8, Compact, движение вперёд

Все значения — мс модели 2 МГц. В группе 50% сохраняются 37 строк, из них 19 цветных; в чередовании все 37 цветные. Количество физических диапазонов — 3 и 37.

| Вариант | Редкие цветные строки (~10%) | Длинные группы (50%) | Чередование (100%) |
|---|---:|---:|---:|
'''
    for method in labels:
        text += '| '+labels[method]+' | '+' | '.join(fmt(find('Compact',method,pattern=p)['ms']) for p in ('sparse10','grouped50','alternating100'))+' |\n'
    text += '''
Одноцветный контроль `copy8` без чтения цветов: '''+fmt(find('Compact','bulk',pattern='mono')['ms'])+''' мс. Он служит нижней границей и сам по себе не сохраняет цветной список.

## Влияние движка на повторную печать

Вариант 1a, движение вперёд, все сохраняемые строки цветные. У 4×8 сохраняется 56 имён, у 6×8 — 37, у 8×8 — 18 строк с размером или `<DIR>`; сравнение разных геометрий не означает одинаковый объём работы.

| Движок | Шрифт | 1a: общий перенос + рендер, мс |
|---|---|---:|
'''
    for font in ('Cpm4','Compact','Rotate','Packed6K','Fast12K','Fast8'):
        x = find(font,'redraw',pattern='alternating100')
        text += f"| {font} | {x['width']}×8 | {fmt(x['ms'])} |\n"
    text += '''
Подготовленные шрифты ускоряют 1a, но это не устраняет лишний проход и затраты на построение/печать строк. Такты прочих вариантов от выбранного движка в рамках одной геометрии не зависят.

## Развёрнутый цикл и пропуск межстрочных полей

Одноцветный контроль, без lookup-таблицы и восстановления цветов. Все варианты переносят одинаковое изображение при нулевых межстрочных полях.

| Шрифт / направление | Текущий UI/memcpy, мс | Развёрнутые 10 байт, мс | Развёрнутые 8 + пропуски, мс | Выигрыш только пропуска |
|---|---:|---:|---:|---:|
'''
    for font in ('Cpm4','Compact','Fast8'):
        for direction in ('up','down'):
            a,b,c = [find(font,'bulk',core=core,pattern='mono',direction=direction)['ms'] for core in ('generic','copy10.entry','copy8.entry')]
            width = find(font,'bulk')['width']
            text += f"| {width}×8 / {'вперёд' if direction=='up' else 'назад'} | {fmt(a)} | {fmt(b)} | {fmt(c)} | {fmt(100*(1-c/b))}% |\n"
    text += '''
При 6×8 движение вперёд: текущее копирование 59,37 мс → развёрнутые 10 байт 38,42 мс → 8 байт 35,98 мс. Из общего ускорения около 39% значительная часть связана с разворачиванием короткого хвоста текущего memcpy, а сам пропуск даёт дополнительно 6,35%. Снижение числа обращений к экрану на 20% не даёт 20% экономии тактов: добавляются два продвижения SP, два продвижения указателя и управление строкой.

Внутренние циклы не заменены побайтовым условным проходом: на каждую строку явно развёрнуты четыре или пять пар. Вперёд используются `POP DE` и две записи, назад — два чтения и `PUSH DE`. SP восстанавливается перед выходом; прерывания должны оставаться запрещёнными.

## Длина цветной группы

6×8, движение вперёд, все строки цветные. Диапазоны разрываются на границах прямоугольников, поэтому число фактических диапазонов больше числа логических групп.

| Заданная длина группы | Диапазонов | 2b: диапазоны, мс | 2c: цвет в цикле, мс |
|---:|---:|---:|---:|
'''
    for length in (2,4,8,16):
        a,b = [find('Compact',method,pattern='runs'+str(length)) for method in ('runColors','inlineColors')]
        text += f"| {length} | {a['runs']} | {fmt(a['ms'])} | {fmt(b['ms'])} |\n"
    text += '''
Одного универсального режима «всегда объединять» нет. Для внедрения целесообразны два ядра: одноцветный диапазон и цвет из таблицы внутри цикла. Диспетчер выбирает их по числу/длине диапазонов конкретного прямоугольника. Точный порог следует измерить вместе с окончательным диспетчером NC; здесь гибрид не реализован и его время не выдается за измеренное.

## Таблица и размер кода

На одну видимую ячейку достаточно одного байта базового атрибута: в обычной раскладке максимум 57 строк на панель, то есть 114 байт на обе панели; при FULL_PANELS=1 максимум 120 байт. После сдвига таблица перемещается в том же логическом порядке, что список индексов, и добавляется цвет новой строки. Выделение переопределяет базовый атрибут отдельно. Ни цветовой регистр, ни аппаратная память атрибутов не читаются.

Измеренный отдельный сдвиг lookup-таблицы на один элемент через штатный memcpy (без классификации нового файла):

| Шрифт | Байтов | Вперёд, мс | Назад, мс |
|---|---:|---:|---:|
'''
    for font in ('Cpm4','Compact','Fast8'):
        a,b = [next(x for x in report['lookup_shift'] if x['font']==font and x['direction']==d) for d in ('up','down')]
        width = find(font,'bulk')['width']
        text += f"| {width}×8 | {a['count']} | {fmt(a['ms'])} | {fmt(b['ms'])} |\n"
    text += '\n| Экспериментальное ядро | Байт кода и локального состояния |\n|---|---:|\n'
    for key,value in report['sizes'].items(): text += f'| {key} | {value} |\n'
    text += '''
Сравнительный shared_driver обслуживает все способы и в рабочем NC целиком не нужен. 198-байтное одноцветное и 256-байтное цветное ядра «8 из 10» — размеры самостоятельных прототипов с разрушаемыми регистрами и локальным состоянием. Они не подменяют общий ABI uiCopyRect; для NC возможен локальный вызов из существующего bitmapCopy, который уже сохраняет регистры. Выбор цветов по расширению, взаимодействие с выделением и окончательный диспетчер потребуют своего небольшого кода.

## Корректность цвета и ограничения

К модели записи VRAM добавлен программный атрибут каждого экранного байта. Трасса реальных выполненных 8080-команд задаёт значение регистра цвета при каждой записи, включая записи через стек. Итоговые записи трассы сверяются с дампом CPU; проверяются фон всех байтов и передний цвет каждого байта с ненулевой битмапой. Цвет переднего плана совершенно пустого байта визуально несущественен при общем синем фоне, и его равенство не требуется. Чтений регистра цвета в проверенных трассах нет.

Пример 6×8 с длинными группами 50%, «8 из 10»:

| Вариант | Чтения VRAM | Записи VRAM | Записи регистра цвета | Промежуточные записи с неправильным передним цветом |
|---|---:|---:|---:|---:|
'''
    for method,label in labels.items():
        x = next(x for x in report['color_traces'] if x['font']=='Compact' and x['direction']=='up' and x['method']==method)
        text += f"| {label} | {x['screen_reads']} | {x['screen_writes']} | {x['color_writes']} | {x['wrong_visible_writes']} |\n"
    text += '''
Для 1a/1b окончательный цвет верен, но общий проход сначала делает цветные строки белыми. Это реальное промежуточное состояние записей, хотя физическое мерцание в данном опыте не измерялось. 2a/2b/2c пишут правильный цвет сразу; у 2c больше записей регистра, зато существенно меньше накладных расходов, чем при отдельном вызове на строку.

Глифы занимают строки 1–8 ячейки, а строки 0 и 9 пусты: шаблон — пропустить 1, перенести 8, пропустить 1. Прототипы проверяют этот контракт на исходном экране всех движков. В отдельной отрицательной проверке ненулевой байт в пустом поле намеренно делает перенос «8 из 10» неверным. Поэтому оптимизация допустима только при нулевых полях и одинаковом фоне обычных файлов. Инверсное выделение заполняет также поля: его нужно предварительно снять/исключить из копирования, как уже делает особый путь NC 6×8, а итоговое выделение рисовать на полную высоту ячейки. Для разноцветных фонов потребуется восстанавливать и два пропускаемых атрибута.

Для внедрения раскраски понадобится заменить постоянный COLOR_FILE не только в копировании, но и в печати/очистке строк, снятии выделения, пересечении колонок, восстановлении сохранённого курсора и полной перерисовке. Снимок NC в этом опыте не изменён. Отдельная проверка на реальном железе нужна для экранных задержек и видимости промежуточных состояний.

## Воспроизведение

```powershell
python scripts/tests/benchmark-nc-color-scroll.py
```

Сначала должны существовать актуальные bin/NC.i80, BIOS.sys, IO.sys и шрифтовые движки с `.pre`. `--quick` запускает сокращённый набор, `--report-only` пересобирает только этот текст из results.json. Получение файлов, установка шрифта, подготовка таблиц и начальная печать не входят в счётчик. Проверяется пустая граница измерения RET + JP = 20 тактов, затем эти общие 20 тактов вычитаются.

Исходники эксперимента: [benchmark-nc-color-scroll.py](../../../scripts/tests/benchmark-nc-color-scroll.py), сгенерированные [bench.asm](bench.asm), [листинг](bench.lst), [точные результаты](results.json). Каталог inputs содержит неизменные снимки бинарников, символов и исходников; SHA-256 записаны в JSON. Полные многомегабайтные трассы разбираются в памяти и не сохраняются.

'''
    text += f"Итого: {len(data)} сравнений битмапы с эталоном, {len(report['color_traces'])} трасс с проверкой цветов, отрицательные проверки ненулевого межстрочного поля во всех геометриях и направлениях. Все перечисленные проверки пройдены.\n"
    (out/'report.md').write_text(text,encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT/'docs/nc/color-scroll-2026-11-15')
    parser.add_argument('--quick', action='store_true')
    parser.add_argument('--report-only', action='store_true')
    args = parser.parse_args()
    out = args.out.resolve(); out.mkdir(parents=True, exist_ok=True)
    if args.report_only:
        write_report(out,json.loads((out/'results.json').read_text(encoding='utf-8')))
        return
    inputs = out/'inputs'; inputs.mkdir(exist_ok=True)
    input_paths = [ROOT/'bin/NC.i80', ROOT/'bin/NC.sym', ROOT/'bin/BIOS.sys',
                   ROOT/'bin/BIOS.sym', ROOT/'bin/IO.sys', ROOT/'source/include/ui.inc',
                   ROOT/'source/common/move8080.inc', ROOT/'source/nc/scrollBitmap.inc']
    input_paths += [ROOT/f'bin/fonts/{font}.i80' for font in ('Cpm4','Compact','Rotate','Packed6K','Fast12K','Fast8')]
    input_paths += [ROOT/f'bin/fonts/{font}.pre' for font in ('Packed6K','Fast12K')]
    input_paths += [Path(__file__).resolve()]
    frozen = {str(p.relative_to(ROOT)):p.read_bytes() for p in input_paths}
    for name, data in frozen.items():
        target = inputs/name; target.parent.mkdir(parents=True,exist_ok=True); target.write_bytes(data)
    n, bios = symbols(inputs/'bin/NC.sym'), symbols(inputs/'bin/BIOS.sym')
    asm = out/'bench.asm'; asm.write_text(source(n), encoding='utf-8')
    assembled = subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                    '--raw='+str(out/'bench.bin'), '--sym='+str(out/'bench.sym'),
                    '--lst='+str(out/'bench.lst'), str(asm)], check=True, capture_output=True,text=True)
    (out/'build.log').write_text(assembled.stdout+assembled.stderr,encoding='utf-8')
    assert not re.search(r'Errors: [1-9]|warnings: [1-9]', assembled.stdout+assembled.stderr)
    s = symbols(out/'bench.sym')
    rows = n['P_FILE_LIST_H']
    methods = ['bulk', 'redraw', 'recolor', 'rowColors', 'runColors', 'inlineColors']
    cores = ['generic', 'copy10.entry', 'copy8.entry']
    patterns = ['mono', 'sparse10', 'random25', 'grouped50', 'grouped100', 'alternating50', 'alternating100', 'runs2', 'runs4', 'runs8', 'runs16']
    fonts = [('Cpm4',4), ('Compact',6), ('Rotate',6), ('Packed6K',6), ('Fast12K',6), ('Fast8',8)]
    if args.quick:
        fonts, patterns = [('Compact',6)], ['mono','grouped50','alternating100']
    results, traces, lookup = [], [], []
    prep_base = (n['varsEnd']+255)&~255
    for font, width in fonts:
        for down in (False, True):
            delta = 1
            jobs, columns, step, cell = jobs_for(width, down, delta, n)
            visible = rows*columns
            first, new_first = (delta, 0) if down else (0, delta)
            m = bytearray(65536)
            for addr, path in [(0, inputs/'bin/NC.i80'), (0xc000, inputs/'bin/BIOS.sys'),
                               (0xd800, inputs/f'bin/fonts/{font}.i80'), (0xf000, inputs/'bin/IO.sys'),
                               (0x4000, out/'bench.bin')]:
                data = path.read_bytes(); m[addr:addr+len(data)] = data
            m[bios['bios_vars']:bios['bios_vars']+bios['BIOS_VARIABLES']] = m[bios['initVars']:bios['initVarsEnd']]
            m[bios['bios_upper.escMode']] = 255
            m[n['ncFontWidth']] = width
            prepared = font in ('Packed6K','Fast12K')
            if prepared:
                data = (inputs/f'bin/fonts/{font}.pre').read_bytes()
                m[0x6400:0x6400+len(data)] = data
            m[0xfff8] = 0xf1
            m[0x9000:0xc000] = bytes([0x55])*12288
            for i in range(visible+delta):
                record = f'F{i:07d}TXT'.encode()+bytes([16 if i%11==0 else 0,0,0])+W(i*937 % 65536)
                m[0x5200+i*16:0x5210+i*16] = record
            # Prepare the old screen outside the measured interval, with real NC/font code.
            prep = b'\x11'+W(0x6400 if prepared else 0)+b'\xcd'+W(n['FONT_ENGINE_INIT'])
            for slot in range(visible):
                col, row = divmod(slot, rows)
                x, y = (n['SCROLL_LEFT']-0x90+col*step)*4, n['P_FILE_LIST_Y']+row*10
                prep += b'\x21'+bytes([y,x])+b'\x22'+W(n['bios_vars.cursorY'])
                prep += b'\x21'+W(0x5200+(first+slot)*16)+b'\xcd'+W(n['printFileNamOnPanel'])
                base = (n['SCROLL_LEFT']+col*step)*256+n['SCROLL_TOP']+row*10
                for byte in range(cell): m[base+byte*256:base+byte*256+10] = bytes(10)
            prep += b'\xc9'
            assert prep_base+len(prep) < 0x4000, 'preparation fixture overlaps benchmark code'
            m[prep_base:prep_base+len(prep)] = prep
            m, _, _, _ = execute(m, prep_base, out)
            _, overhead, _, _ = execute(m,s['empty'],out,start=s['empty'])
            assert overhead == 20, 'counter boundary must contain only RET + JP'
            for slot in range(visible):
                col, row = divmod(slot, rows)
                for byte in range(cell):
                    at = (n['SCROLL_LEFT']+col*step+byte)*256+n['SCROLL_TOP']+row*10
                    assert m[at] == m[at+9] == 0, (font, slot, 'glyph is not in rows 1..8')
            for j, job in enumerate(jobs):
                slot = job['slot']
                descriptor = W(job['src'])+W(job['dst'])+bytes([cell,job['rows']])+W(0x5800+slot)+W(0x5200+(new_first+slot)*16)
                m[0x5000+j*10:0x5000+(j+1)*10] = descriptor
            m[s['jobCount']] = len(jobs)
            m[s['walkDown']] = down
            m[s['visibleCount']] = visible
            m[0x5800:0x5800+visible] = bytes(range(visible))
            shifted, shift_cycles, _, _ = execute(m,s['lookupShift'],out,start=s['lookupShift'])
            actual_shift = shifted[0x5800:0x5800+visible]
            wanted_shift = bytes([0]+list(range(visible-1))) if down else bytes(list(range(1,visible))+[visible-1])
            assert actual_shift == wanted_shift, 'row attribute lookup shift'
            lookup.append(dict(font=font,direction='down' if down else 'up',
                               count=visible-1,cycles=shift_cycles-20,ms=(shift_cycles-20)/2000))
            expected = bytearray(m)
            for job in jobs:
                for byte in range(cell):
                    src, dst = job['src']+byte*256, job['dst']+byte*256
                    expected[dst:dst+job['rows']*10] = m[src:src+job['rows']*10]
            for pattern in patterns:
                colors = palette(pattern, visible+delta)
                m[0x5800:0x5800+visible] = bytes(colors[new_first:new_first+visible])
                initial_attrs = bytearray([0xf1])*12288
                wanted_attrs = bytearray(initial_attrs)
                for slot in range(visible):
                    col, row = divmod(slot, rows)
                    for byte in range(cell):
                        at = (n['SCROLL_LEFT']-0x90+col*step+byte)*256+n['SCROLL_TOP']+row*10
                        initial_attrs[at:at+10] = bytes([colors[first+slot]])*10
                wanted_attrs[:] = initial_attrs
                for job in jobs:
                    for row in range(job['rows']):
                        color = colors[new_first+job['slot']+row]
                        for byte in range(cell):
                            at = job['dst']-0x9000+byte*256+row*10
                            wanted_attrs[at:at+10] = bytes([color])*10
                colored = sum(colors[new_first+j['slot']+r] != 0xf1 for j in jobs for r in range(j['rows']))
                runs = sum(1+sum(colors[new_first+j['slot']+r] != colors[new_first+j['slot']+r-1] for r in range(1,j['rows'])) for j in jobs)
                for core in cores:
                    m[s['copyCall']+1:s['copyCall']+3] = W(s[core])
                    m[s['inlineCall']+1:s['inlineCall']+3] = W(s['colored8.entry' if core=='copy8.entry' else 'colored10.entry'])
                    m[s['touchCall']+1:s['touchCall']+3] = W(s['touch8' if core=='copy8.entry' else 'touch10'])
                    for method in methods:
                        if core=='generic' and method=='inlineColors': continue
                        result, cycles, _, _ = execute(m, s[method], out, start=s[method])
                        assert result[0x9000:0xc000] == expected[0x9000:0xc000], (font,down,pattern,core,method,'bitmap')
                        item = dict(font=font,width=width,columns=columns,direction='down' if down else 'up',
                                    delta=delta,pattern=pattern,colored=colored,kept=visible-delta,runs=runs,
                                    core=core,method=method,cycles=cycles-20,ms=(cycles-20)/2000)
                        results.append(item)
                        # Color correctness for every width/direction/method, with both
                        # grouped and alternating input. Trace is parsed, not retained.
                        chosen = 'alternating100' if down else 'grouped50'
                        if core=='copy8.entry' and method!='bulk' and pattern==chosen and font in ('Cpm4','Compact','Fast8'):
                            final, _, trace, initial = execute(m, s[method], out, start=s[method], trace=True)
                            stats = replay_trace(trace, initial, initial_attrs, final, wanted_attrs)
                            if method in ('rowColors','runColors','inlineColors'):
                                assert stats['wrong_visible_writes']==0, 'one-pass copy temporarily lost a row color'
                            else:
                                assert stats['wrong_visible_writes']>0, 'two-pass whitening was not captured'
                            traces.append(dict(**item, **stats))
            # An inverted selection has nonzero padding. Skipping its spacing
            # is unsafe: production must erase/exclude it or use a full copy.
            probe = bytearray(m); probe[jobs[0]['src']] = 255
            full = bytearray(probe)
            for j in jobs:
                for byte in range(cell):
                    src, dst = j['src']+byte*256, j['dst']+byte*256
                    full[dst:dst+j['rows']*10] = probe[src:src+j['rows']*10]
            probe[s['copyCall']+1:s['copyCall']+3] = W(s['copy8.entry'])
            bad, _, _, _ = execute(probe,s['bulk'],out,start=s['bulk'])
            assert bad[0x9000:0xc000] != full[0x9000:0xc000], 'nonzero padding must reject 8/10 optimization'
            print(font, 'down' if down else 'up', 'bitmap, colors and padding contract PASS', flush=True)
    sizes = {name:s[end]-s[begin] for name,begin,end in [
        ('shared_driver','bulk','driverEnd'),('copy10','copy10.entry','copy10.end'),
        ('copy8','copy8.entry','copy8.end'),('touch10','touch10','touch10End'),
        ('touch8','touch8','touch8End'),('colored10','colored10.entry','colored10.end'),
        ('colored8','colored8.entry','colored8.end')]}
    report = dict(cpu='Intel 8080 model (z88dk-ticks)',cpu_hz=2000000,screen='48x256 bytes, MX 16 colors',
                  source_hashes={name:hashlib.sha256(data).hexdigest() for name,data in frozen.items()},
                  bench_sha256=hashlib.sha256(asm.read_bytes()).hexdigest(),
                  sizes=sizes,results=results,color_traces=traces,lookup_shift=lookup)
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    if not args.quick:
        write_report(out,report)
    print('COLOR SCROLL BENCH PASS:',len(results),'bitmap comparisons,',len(traces),'color traces',sizes)


if __name__ == '__main__':
    main()
