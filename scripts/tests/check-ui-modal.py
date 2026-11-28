"""Assemble real modal descriptors and check border/padding/grid invariants."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/ui-modal-layout-check'
OUT.mkdir(parents=True, exist_ok=True)
source = [f'    INCLUDE "{(ROOT / "sdk/asm/lib/ui.inc").as_posix()}"',
          '    ORG 0']
cases = []
for body in (0, 12, 36):
    for rows in (1, 2):
        for cols in (1, 2, 3, 4):
            cases.append((body, rows, cols, rows*cols, False, True))
            source.append(f'    UI_MODAL_BEGIN {body},{rows},{cols}')
            for row in range(rows):
                for col in range(cols):
                    source.append(f'    UI_MODAL_BUTTON 0,{row},{col},0')
            source.append('    UI_MODAL_END')
for actual in (5, 6, 7):
    cases.append((0, 2, 4, actual, False, True))
    source.append('    UI_MODAL_BEGIN 0,2,4')
    for col in range(4):
        source.append(f'    UI_MODAL_BUTTON 0,0,{col},0')
    source.append(f'    UI_MODAL_ROW {actual-4}')
    for col in range(actual-4):
        source.append(f'    UI_MODAL_BUTTON 0,1,{col},0')
    source.append('    UI_MODAL_END')
for rows in (1, 2):
    for cols in (1, 2, 3, 4):
        cases.append((0, rows, cols, rows*cols, True, True))
        source.append(f'    UI_MODAL_COMPACT_BEGIN 0,{rows},{cols}')
        for row in range(rows):
            for col in range(cols):
                source.append(f'    UI_MODAL_BUTTON 0,{row},{col},0')
        source.append('    UI_MODAL_END')
for actual in (5, 6, 7):
    cases.append((0, 2, 4, actual, True, True))
    source.append('    UI_MODAL_COMPACT_BEGIN 0,2,4')
    for col in range(4):
        source.append(f'    UI_MODAL_BUTTON 0,0,{col},0')
    source.append(f'    UI_MODAL_ROW {actual-4}')
    for col in range(actual-4):
        source.append(f'    UI_MODAL_BUTTON 0,1,{col},0')
    source.append('    UI_MODAL_END')
for compact in (False, True):
    for cols, last in ((4,4),(3,2),(3,3),(4,3)):
        cases.append((0,2,cols,cols+last,compact,False))
        preset = 'UI_MODAL_COMPACT_BUTTONS_BEGIN' if compact else 'UI_MODAL_BUTTONS_BEGIN'
        source.append(f'    {preset} 2,{cols}')
        for col in range(cols):
            source.append(f'    UI_MODAL_BUTTON 0,0,{col},0')
        if last != cols:
            source.append(f'    UI_MODAL_ROW {last}')
        for col in range(last):
            source.append(f'    UI_MODAL_BUTTON 0,1,{col},0')
        source.append('    UI_MODAL_END')
asm = OUT / 'layout.asm'
binary = OUT / 'layout.bin'
asm.write_text('\n'.join(source) + '\n', encoding='utf-8')
result = subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080',
                         '-Wno-rdlow', '--raw=' + str(binary), str(asm)],
                        capture_output=True, text=True)
assert result.returncode == 0, result.stdout + result.stderr
data = binary.read_bytes()
offset = 0
for body, rows, cols, actual, compact, has_body in cases:
    title, body_top, body_height, buttons_top, count = data[offset:offset+5]
    pointer = int.from_bytes(data[offset+5:offset+7], 'little')
    assert pointer == offset + (25 if has_body else 19) and count == rows*cols
    frame = data[offset+7:offset+12]
    x, y, width, height = (frame[2]-0x90)*8, frame[1], (frame[4]+2)*8, frame[3]+6
    header_line = data[offset+13]
    separator = header_line
    if has_body:
        separator = data[offset+19]
        assert separator-(body_top+body) == 4
    else:
        assert data[offset+18] == 0
    assert x == (24 if compact else 64) and width == (144 if compact else 256) and y*2+height in (255, 256)
    assert title-7 >= y+9 and header_line-title == 5
    assert body_top-header_line == 5 and body_height == body
    assert buttons_top-separator == 5
    boxes = []
    for index in range(actual):
        button = data[pointer+index*10:pointer+(index+1)*10]
        left, size, top, high = button[2]*2, button[3]*2, button[4], button[5]
        assert left >= x+8 and left+size <= x+width-8
        assert top >= separator+5 and top+high <= y+height-9
        assert left % 8 == size % 8 == 0
        boxes.append((left, size, top, high))
    for row in range(rows):
        group = boxes[row*cols:(row+1)*cols]
        assert group[0][0]+group[-1][0]+group[-1][1] == (192 if compact else 384)
        for first, second in zip(group, group[1:]):
            assert first[0]+first[1]+8 <= second[0]
    offset = pointer+actual*10
assert offset == len(data)
print(f'UI modal layout PASS: {len(cases)} assembled presets; borders, padding, centering, byte alignment')

# Exercise the assembled focus engine on the 8080 model with real NC descriptors.
import runpy

model = runpy.run_path(str(ROOT / 'scripts/tests/check-nc-bank-graphics.py'))
run = model['run']
run.__globals__['OUT'] = OUT
symbols = model['symbols']('NC')
nc = (ROOT / 'bin/NC.i80').read_bytes()
w = lambda value: value.to_bytes(2, 'little')
checked = 0
for name, count, mode, initial, steps in (
        ('ncDeleteModal', 2, 'UI_FOCUS_PAIR', 0, [(8,0),(24,1),(24,1),(9,0),(26,1)]),
        ('ncOverwriteModal', 4, 'UI_FOCUS_GRID', 2, [(8,3),(25,1),(24,0),(26,2),(9,3),(9,0)]),
        ('ncDriveModal7', 7, 'UI_FOCUS_LINEAR', 6, [(24,0),(25,6),(9,0)]),
        ('ncDriveModal5', 5, 'UI_FOCUS_GRID', 2, [(26,2),(9,3),(25,0),(8,2)])):
    memory = bytearray(65536)
    memory[:len(nc)] = nc
    descriptor = symbols[name]
    button = int.from_bytes(memory[descriptor+5:descriptor+7], 'little')
    selected = symbols['UI_BUTTON.selected']
    stride = symbols['UI_BUTTON']
    memory[symbols['uiModalCurrent']:symbols['uiModalCurrent']+2] = w(descriptor)
    start = symbols['uiModalFocusInit']
    memory = run(memory,start,a=initial,bc=(symbols[mode]<<8)|count,hl=descriptor)
    for key, wanted in [(0xff,initial),*steps]:
        memory = run(memory,symbols['uiModalFocusKey'],a=key)
        assert memory[symbols['uiModalFocusIndex']] == wanted,(name,key,wanted)
        assert [memory[button+i*stride+selected] for i in range(count)] == [
            int(i==wanted) for i in range(count)],(name,key,wanted,'visual focus')
        checked += 1
print(f'UI modal focus PASS: {checked} navigation steps and selected flags')

# Test the actual drive descriptors, both positions and all supported fonts.
bios = model['symbols']('BIOS')
for font, width in (('Cpm4',4),('Compact',6),('Fast8',8)):
    prepared = bytearray(65536)
    for name, address in (('NC.i80',0), ('BIOS.sys',0xc000),
                          ('IO.main.bin',0xf000), (f'fonts/{font}.i80',0xd800)):
        module = (ROOT/'bin'/name).read_bytes()
        prepared[address:address+len(module)] = module
    variables = bios['mon2_vars']
    prepared[variables:variables+bios['MON2_VARIABLES']] = (ROOT/'bin/BIOS.defaults.bin').read_bytes()[:bios['MON2_VARIABLES']]
    prepared = run(prepared,bios['FONT_ENGINE_INIT'])
    prepared = run(prepared,symbols['ncLayoutInit'])
    assert prepared[symbols['ncFontWidth']]==width
    for count, expected_rows in ((4,[4]),(5,[3,2]),(6,[3,3]),(7,[4,3]),(8,[4,4])):
        image = bytearray(prepared)
        image[symbols['vars.driveCount']] = count
        at = symbols['vars.driveList']
        image[at:at+count] = bytes(range(count))
        image[symbols['vars.chooseDrive']] = count-1
        descriptor = symbols[f'ncDriveModal{count}']
        button = int.from_bytes(image[descriptor+5:descriptor+7], 'little')
        for panel in (1,0):
            image[symbols['state.activePanel']] = panel
            image = run(image,symbols['ncDriveModalOpen'])
            assert image[descriptor+9] == 0x93+24*panel
            assert image[descriptor+14] == image[descriptor+9]
            assert image[descriptor+18] == 0
            top = image[descriptor+13]
            column = (0x93-0x90)+1+24*panel
            assert image[0x9000+column*256+top] == 0xff
            assert image[0x9000+column*256+top+9] == 0
            placements = {}
            widths = set()
            for i in range(count):
                at = button+i*10
                assert image[at+7] == int(i==count-1)
                assert image[at+6] == width
                left, size, y = image[at+2:at+5]
                placements.setdefault(y,[]).append((left,size))
                widths.add(size)
            assert len(widths)==1
            assert [len(row) for row in placements.values()]==expected_rows
            first_row, *other_rows = placements.values()
            assert first_row[0][0]+first_row[-1][0]+first_row[-1][1]==96+192*panel
            for row in other_rows:
                assert row==first_row[:len(row)],(count,panel,font,'drive columns')
            image = run(image,symbols['ncDialogClose'])
print('NC drive dialogs PASS: 4-8 drives, both panels, three fonts, one separator')
