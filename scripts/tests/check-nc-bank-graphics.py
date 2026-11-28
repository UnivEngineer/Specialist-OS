"""Execute both NC backends on an 8080 model and compare actual VRAM bytes."""
from pathlib import Path
import re
import subprocess
from relocated_component import load_graphics

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/nc-bank-graphics'
W = lambda value: int(value).to_bytes(2, 'little')


def symbols(name):
    return {key: int(value, 16) for key, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
        (ROOT / 'bin' / f'{name}.sym').read_text('utf-8-sig'), re.M)}


def run(memory, entry, a=0, bc=0, de=0, hl=0):
    code = b'\x31'+W(0x8f00)+b'\x3e'+bytes([a])+b'\x01'+W(bc)
    code += b'\x11'+W(de)+b'\x21'+W(hl)+b'\xcd'+W(entry)+b'\xc3'+W(0x08ff)
    memory = bytearray(memory)
    memory[0x0800:0x0800+len(code)] = code
    (OUT / 'input.bin').write_bytes(memory)
    subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                    '-m8080', '-pc', '0800', '-end', '08ff', '-counter', '30000000',
                    '-output', str(OUT / 'output.bin'), str(OUT / 'input.bin')],
                   check=True, capture_output=True, timeout=20)
    result = (OUT / 'output.bin').read_bytes()
    assert result[65542:65544] == W(0x08ff), (hex(entry), 'PC')
    assert result[65544:65546] == W(0x8f00), (hex(entry), 'SP')
    return bytearray(result[:65536])


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    main = symbols('NC'); core = symbols('NC_CORE')
    bios = symbols('BIOS')

    def seed(image, origin, font, body=None):
        memory = bytearray(65536)
        for name, base in [(image, origin), ('BIOS', 0xc000), ('IO', 0xf000),
                           ('fonts/'+font, 0xd800)]:
            ext = '.sys' if name in ('BIOS', 'IO') else '.i80'
            data = body if name == image and body is not None else (ROOT / 'bin' / (name+ext)).read_bytes()
            memory[base:base+len(data)] = data
        memory[bios['mon2_vars']:bios['mon2_vars']+bios['MON2_VARIABLES']] = (ROOT/'bin/BIOS.defaults.bin').read_bytes()[:bios['MON2_VARIABLES']]
        memory[bios['bios_upper.charGen']:bios['bios_upper.charGen']+2] = W(bios['FONT_ADDR'])
        memory[bios['bios_upper.escMode']] = 255
        return memory

    checked = 0
    body, gfx = load_graphics()
    base = gfx['entry']
    assert gfx['ngFileEnd'] <= core['NC_MAIN_PACKET'] < core['NC_MAIN_END'] <= 0x6400
    for font, width in [('Cpm4', 4), ('Rotate', 6), ('Fast8', 8), ('Fast12K', 6), ('Packed6K', 6), ('Compact', 6)]:
        value_x = 28 if width == 8 else 32
        value_chars = (184-2*value_x)//width
        label_end = 26 if width == 8 else 28
        native = seed('NC', 0, font)
        target = seed('UI_GFX', base, font, body)
        packet = 0x550d if width == 8 else core['NC_MAIN_PACKET']
        lower_base = 0x6400 if font == 'Fast12K' else 0x7c00
        if font in ('Fast12K', 'Packed6K'):
            # Place the lower tables above the graphics DLL, as FONT does.
            lower = (ROOT / f'bin/fonts/{font}.pre').read_bytes()
            for memory in (native, target):
                memory[lower_base:lower_base+len(lower)] = lower

        native = run(native, bios['FONT_ENGINE_INIT'], de=lower_base if font in ('Fast12K', 'Packed6K') else 0)
        target = run(target, bios['FONT_ENGINE_INIT'], de=lower_base if font in ('Fast12K', 'Packed6K') else 0)
        target = run(target, gfx['ngBind'], hl=packet, bc=core['NC_GFX_QUEUE_BYTES'])
        dest = gfx['ngTransfer']+4
        assert target[dest:dest+2] == W(packet)
        limit = gfx['ngPacketLimit']
        for at, length in [(base, 1), (base-1, 2), (0xffff, 2), (0x8bff, 2), (packet, 0), (packet, 3073)]:
            rejected = run(target, gfx['ngBind'], hl=at, bc=length)
            assert rejected[dest:dest+2] == W(packet)
            assert rejected[limit:limit+2] == W(core['NC_GFX_QUEUE_BYTES'])
        target = run(target, gfx['ngPadding'], hl=ord('A'), bc=8)
        assert target[gfx['ngResult']+15] == (ROOT / f'bin/fonts/{font}.i80').read_bytes()[39]
        # The idle cursor is a graphics-only XOR. A second request erases it.
        cursor_args = dict(a=gfx['GFX_CURSOR'], hl=0x041b, bc=0x0870)
        shown = run(target, gfx['entry'], **cursor_args)
        assert shown[0x9000:0xc000] != target[0x9000:0xc000], font
        hidden = run(shown, gfx['entry'], **cursor_args)
        assert hidden[0x9000:0xc000] == target[0x9000:0xc000], font
        assert hidden[bios['bios_upper.cursorCfg']] == target[bios['bios_upper.cursorCfg']]
        bank = bytearray(65536)
        data = (ROOT / 'bin/NC_CORE.i80').read_bytes()
        bank[0x100:0x100+len(data)] = data
        bank[core['ncBankFontHeader']+5:core['ncBankFontHeader']+7] = bytes([width, 8])
        size = gfx['GFX_RESULT_BYTES']
        bank[core['ncGfxResult']:core['ncGfxResult']+size] = target[gfx['ngResult']:gfx['ngResult']+size]
        bank[core['ncBankFontHeader']+7] = target[gfx['ngResult']+15]
        native = run(native, main['ncLayoutInit'])
        bank = run(bank, core['ncLayoutInit'])
        for entry in ('mx_queueBind', 'mx_queueFlush', 'mx_memInfo'):
            bank[core[entry]] = 0xc9
        bank = run(bank, core['commandQueueInit'], hl=core['ncBankQueue'],
                   bc=core['NC_GFX_QUEUE_BYTES'], de=core['NC_GFX_NORMAL_RECORD'])
        bank = run(bank, core['commandQueueSelect'], a=core['MX_QUEUE_DLL'], hl=1)
        for panel in (0, 1):
            for label in ('drawPanelHeader', 'ncUpdateCatalogFrame', 'drawCursor', 'ncClearCell', 'ncPrintField',
                          'drawToolbar', 'shift_toolbar', 'toolbar_replay', 'ncCopyColoredText', 'colored_back', 'printPanelFile', 'drawWindow1', 'dialog_roundtrip', 'pip_printFont', 'redrawCommandText', 'ncPrintPanelCentered', 'pip_printDescription'):
                # The old full NC cannot coexist with Fast12K's lower tables;
                # its dialog scratch overlaps them. Compare its primitives only.
                if font == 'Fast12K' and label in ('drawWindow1', 'dialog_roundtrip'):
                    continue
                left = bytearray(native); right = bytearray(bank); display = bytearray(target)
                screen = bytes((index*17+index//256*29)&255 for index in range(0x3000))
                left[0x9000:0xc000] = display[0x9000:0xc000] = screen
                for memory, sy in ((left, main), (right, core)):
                    memory[sy['state.activePanel']] = panel
                    memory[sy['state.panelA_firstFile']+panel] = 10
                    memory[sy['state.panelA_curFile']+panel] = 13
                    memory[sy['state.panelA_filesCnt']+panel] = 250
                    memory[sy['pipDetailX']] = value_x
                    memory[sy['pipDetailChars']] = value_chars
                    memory[sy['vars.cmdTextPos']:sy['vars.cmdTextPos']+2] = W(0x10f3)
                    memory[sy['vars.cmdCapacity']] = 12
                    memory[sy['vars.cmdLine']:sy['vars.cmdLine']+9] = b'FILE.EXE\0'
                    memory[sy['vars.cmdLineEnd']:sy['vars.cmdLineEnd']+2] = W(sy['vars.cmdLine']+8)
                    memory[sy['ncCurrentColor']] = 0x31
                    memory[sy['IO_COLOR']] = 0x31
                    if label == 'ncUpdateCatalogFrame':
                        memory[sy['vars.cacheSlotA']+panel] = 0
                        memory[sy['ncCatalogs']] = 1
                        memory[sy['ncCatalogs']+sy['CATALOG_OVERFLOW']] = 2
                    memory[sy['vars.rowColors']:sy['vars.rowColors']+120] = bytes([0xa1, 0xa1, 0xb1, 0xf1]*30)
                    memory[sy['mon2_vars.cursorY'] if sy is main else sy['ncBankCursor']:
                           (sy['mon2_vars.cursorY'] if sy is main else sy['ncBankCursor'])+2] = bytes([27, 4])
                params = {}
                if label in ('ncPrintPanelCentered', 'pip_printDescription'):
                    for memory in (left, right): memory[0x8d00:0x8d51] = b'ABCDEFGHIJ'*8+b'\0'
                    params = dict(hl=0x8d00, de=120)
                if label == 'ncPrintField':
                    for memory in (left, right): memory[0x8d00:0x8d05] = b'Test\0'
                    params = dict(hl=0x8d00, bc=12 << 8)
                if label == 'ncCopyColoredText':
                    params = dict(hl=0x910a, bc=0x9100, de=(6 << 8)+190)
                if label == 'colored_back':
                    params = dict(hl=0x9100, bc=0x910a, de=(6 << 8)+190)
                if label == 'printPanelFile':
                    for memory, sy in ((left, main), (right, core)):
                        memory[sy['ncCatalogBases']:sy['ncCatalogBases']+2] = W(0x8d00)
                        memory[sy['vars.cacheSlotA']+panel] = 0
                        memory[0x8d00:0x8d10] = b'EXAMPLE TXT'+bytes(5)
                        memory[0x8d20] = 0
                    params = dict(hl=0x8d20, de=(4 << 8)+22)
                action = 'drawWindow1' if label == 'dialog_roundtrip' else label
                if label == 'colored_back': action = 'ncCopyColoredText'
                if label in ('drawToolbar', 'shift_toolbar', 'toolbar_replay'):
                    action = 'drawToolbar'
                    shifted = int(label == 'shift_toolbar')
                    left[bios['IO_KEYB_B']] = 0 if shifted else 2
                    display[bios['IO_KEYB_B']] = 2 if shifted else 0
                    at = core['io_bankKeyShift']
                    right[at:at+3] = bytes([0x3e, shifted, 0xc9])
                if label == 'pip_printFont':
                    name = b'MyFont42'
                    address = bios['io_vars.fontName']
                    left[address:address+8] = name
                    address = core['pipFontName']
                    right[address:address+8] = name
                    right[core['ncGfxReadBytes8']] = 0xc9  # MAIN read supplied above.
                    font_bytes = {'Fast12K': 10240, 'Packed6K': 4096}.get(font, 0)
                    for memory, sy in ((left, main), (right, core)):
                        address = sy['pipFontBytes']
                        memory[address:address+2] = W(font_bytes)
                left = run(left, main[action], **params)
                right = run(right, core[action], **params)
                if label == 'toolbar_replay':
                    left = run(left, main['drawToolbar'])
                    right = run(right, core['drawToolbar'])
                if label == 'dialog_roundtrip':
                    left = run(left, main['ncDialogClose'])
                    right = run(right, core['ncDialogClose'])
                length = int.from_bytes(right[core['cqUsed']:core['cqUsed']+2], 'little')
                assert 0 < length < core['NC_GFX_QUEUE_BYTES'], (font, label, length)
                source = core['ncBankQueue']
                if action in ('pip_printFont', 'pip_printDescription'):
                    if action == 'pip_printFont':
                        units = 'B' if width == 8 or 16+len(str(font_bytes)) > value_chars else 'bytes'
                        title, value, baseline = b'Font', name+f', {font_bytes} {units}'.encode(), 219
                    else:
                        title, value, baseline = b'Info', (b'ABCDEFGHIJ'*8)[:value_chars], 37
                    pos = source
                    for text, color, x in ((title, 0xf1, label_end-len(title)*width//2),
                                           (value, 0xe1, value_x)):
                        assert right[pos:pos+4] == bytes([gfx['GFX_TEXT'], color, baseline, x+96*panel])
                        size = int.from_bytes(right[pos+14:pos+16], 'little')
                        pos += gfx['GFX_RECORD_HEADER']
                        assert right[pos:pos+size] == text, (font, action, text)
                        pos += size
                    assert pos == source+length
                display[packet:packet+length] = right[source:source+length]
                # Пакет уже перенесён в зарегистрированный буфер MAIN.
                display[gfx['mx_memcpyFar']:gfx['mx_memcpyFar']+2] = b'\xaf\xc9'
                display = run(display, gfx['entry'], a=2, bc=length, hl=source)
                if left[0x9000:0xc000] != display[0x9000:0xc000]:
                    at = next(index for index in range(0x9000, 0xc000) if left[index] != display[index])
                    raise AssertionError((font, panel, label, hex(at), left[at], display[at]))
                # Command painting pads the field, then moves the logical cursor
                # back to the text end. MAIN receives that position at KEY entry.
                cursor_source = left if action == 'redrawCommandText' else display
                assert cursor_source[bios['mon2_vars.cursorY']:bios['mon2_vars.cursorY']+2] == right[
                    core['ncBankCursor']:core['ncBankCursor']+2], (font, panel, label, 'cursor')
                if action == 'ncPrintPanelCentered':
                    assert right[core['ncBankCursor']] == 120, (font, panel, action, 'wrapped')
                    count = 176//width
                    assert right[core['ncTextBuffer']:core['ncTextBuffer']+count] == (b'ABCDEFGHIJ'*8)[:count]
                    assert right[core['ncTextBuffer']+count] == 0, 'unclipped description'
                if action == 'redrawCommandText':
                    assert left[bios['IO_COLOR']] == 0x70, 'command text inherited file color'
                    assert right[core['ncCurrentColor']] == 0x70, 'CORE command color'
                if action == 'ncUpdateCatalogFrame':
                    edge = 0x9000+panel*0x1800+208
                    assert (display[edge], display[edge+0x1700]) == (0x5f, 0xfa)
                    assert left[bios['IO_COLOR']] == display[bios['IO_COLOR']] == 0xc1
                if action in ('ncCopyColoredText', 'printPanelFile'):
                    at = core['vars.rowColors']
                    native_at = main['vars.rowColors']
                    assert left[native_at:native_at+114] == right[at:at+114]
                if action == 'ncCopyColoredText':
                    assert left[bios['IO_COLOR']] == display[bios['IO_COLOR']], 'copy palette latch'
                    assert left[main['ncCurrentColor']] == right[core['ncCurrentColor']], 'copy palette mirror'
                checked += 1
    print(f'NC graphics profiles PASS: {checked} VRAM comparisons, 4/6/8px, all six engines, font info and command color')


if __name__ == '__main__':
    main()
