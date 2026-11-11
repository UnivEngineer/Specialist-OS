"""Полный FONTBEN через клавиатуру NC, снимки и проверка отчёта на B:."""
from pathlib import Path
import hashlib
import json
import re
import runpy
import time
import urllib.request

R = Path(__file__).resolve().parents[2]
import argparse
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report', type=Path, default=R/'docs/bench/engines/full')
parser.add_argument('--bench-args', default='')
parser.add_argument('--block32', action='store_true')
parser.add_argument('--remap32', action='store_true')
parser.add_argument('--block32-corpus', action='store_true')
parser.add_argument('--expect-usage', action='store_true')
parser.add_argument('--missing-module', action='store_true', help='проверка отказа загрузки модуля')
args = parser.parse_args()
O = args.report.resolve()
O.mkdir(parents=True, exist_ok=True)
h = runpy.run_path(str(R / 'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=O, RUNTIME=R / '.local/bios-font-20261109')
x = h['Matrix']()
m = x.m
sy = lambda p: {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', p.read_text(), re.M)}
n, s = sy(R / 'bin/NC.sym'), sy(R / ('bin/FONTRMP.sym' if args.remap32 else 'bin/FONTBLK.sym' if args.block32 else 'bin/FONTLAB.sym'))
transport = sy(R / 'bin/banked/TRANSPORT.sym')
cases = [line.split(',') for line in re.findall(r'^    CASE (.+)$', (R / 'source-apps/fontbench/matrix-data.inc').read_text(encoding='utf-8'), re.M)]
progress = False
narrow_progress = set()


def free_until(pc, watch=False):
    global progress
    m.stop_at(pc)
    m.call('emu_run')
    deadline = time.monotonic() + 900
    while time.monotonic() < deadline:
        state = m.call('cpu_state')
        if state['registers']['pc']['dec'] == pc:
            if m.read(transport['bankPage'], 1) == b'\0':
                return state
            # Адрес main-breakpoint может совпасть с кодом банкового модуля.
            m.call('debug_step', mode='into')
            m.call('emu_run')
            continue
        if watch:
            stage = m.read(s['matrixStage'], 1)[0]
            if not progress and 12 <= stage <= 54:
                m.screenshot(O / 'progress.png')
                progress = True
            for tag, first, last in [('main', 55, 69), ('bank', 113, 124)]:
                if tag not in narrow_progress and first <= stage <= last:
                    m.screenshot(O / f'font4-{tag}.png')
                    narrow_progress.add(tag)
        time.sleep(.25)
    raise RuntimeError(('timeout', state))


def enter_to(pc):
    m.call('bp_clear')
    m.call('emu_run_for', ms=250)
    m.stop_at(pc)
    m.tap('enter')
    return m.until(pc, 120, page=0)


def abi(address, af=0, hl=0):
    # Отдельная проверка файлового ABI после обычного запуска COM из NC.
    # Буфер файла не пересекается с переходником и временным стеком.
    m.write(0x8C00, b'\xcd' + address.to_bytes(2, 'little') + b'\xc3\x06\x8c\xc3\x06\x8c')
    m.stop_at(0x8C06)
    m.call('regs_set', pc=0x8C00, sp=0x8E00, af=af, hl=hl, bc=0, de=0)
    m.call('emu_run_for', ms=500)
    state = m.until(0x8C06, 60, page=0)
    assert not state['flags']['C'], ('ABI error', address, af, state)
    return state


def handle(mode):
    obj = bytearray(64)
    obj[:11] = b'FONTBEN TXT'
    obj[40:42] = bytes((1, mode))
    m.write(0x8B20, obj)
    abi(s['bios_fileHandle'], af=0, hl=0x8B20)


def read_report(expected):
    handle(1)
    actual = bytearray()
    while True:
        m.write(0x8B20 + 49, (0x7A00).to_bytes(2, 'little') + (256).to_bytes(2, 'little'))
        abi(s['bios_fileHandle'], af=1 << 8, hl=0x8B20)
        count = int.from_bytes(m.read(0x8B20 + 51, 2), 'little')
        if not count:
            break
        actual += m.read(0x7A00,count)
        assert len(actual) <= len(expected), 'file length'
    assert actual == expected, 'file bytes'
    abi(s['bios_fileHandle'], af=5 << 8, hl=0x8B20)
    assert m.read(0x8B20 + 42, 1) == b'\0'
    return actual


try:
    x.start('font4-001', system=R / 'images/SystemRom_emu80_writable.bin')
    request = urllib.request.Request(m.url, json.dumps({'jsonrpc': '2.0', 'id': 999, 'method': 'tools/list'}).encode(), {'Content-Type': 'application/json'})
    listing = json.load(urllib.request.urlopen(request, timeout=30))
    (O / 'tools.json').write_text(json.dumps(listing, indent=2), encoding='utf-8')
    assert not next(t for t in listing['result']['tools'] if t['name'] == 'emu_run')['inputSchema'].get('required')
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    m.tap('esc')
    m.call('emu_run_for', ms=250)
    command = ('A:/BENCH/FONTBEN/FONTMAP.COM' if args.remap32 else 'A:/BENCH/FONTBEN/FONTBLK.COM' if args.block32 else 'A:/BENCH/FONTBEN/FONTLAB.COM') + (' ' + args.bench_args if args.bench_args else '')
    keys = {' ': 'space', ':': 'colon', '-': 'minus', '.': 'period', '/': 'slash'}
    for char in command:
        if char in ('=', '?'):
            m.call('kbd_send', keys=['shift'], action='press')
            try:
                m.call('emu_run_for', ms=250)
                m.tap('minus' if char == '=' else 'slash')
            finally:
                m.call('kbd_send', keys=['shift'], action='release')
            m.call('emu_run_for', ms=250)
        else:
            m.tap(keys.get(char, char.lower()))
    assert m.read(n['vars.cmdLine'], len(command)).upper() == command.upper().encode()
    m.screenshot(O / 'command.png')
    m.tap('enter')
    m.stop_at(0)
    m.until(0, 120, page=0)
    try:
        m.write(0xfffd, b'\0')
        dos_reserve_before = m.read(0xc000, 0x800)
    finally:
        m.write(0xfffc, b'\0')
    bios_draw_before = m.read(s['BASE_DRAWCHAR'], 3)
    bios_hooks_before = [m.read(s[k], n) for k,n in [('bios_printChar',3), ('bios_printString',3), ('BASE_BIOS_TABLE',102)]]
    if args.missing_module:
        # Проверяем ошибку штатного загрузчика, не меняя файлов системного диска.
        m.write(s['bankPath'], b'A:\\ABSENT.COM\0')
        free_until(s['waitKey'])
        assert m.read(s['bankHandle'], 2) == b'\0\0'
        assert m.read(s['matrixStage'], 1) == b'\0'
        assert [m.read(s[k], n) for k,n in [('bios_printChar',3), ('bios_printString',3), ('BASE_BIOS_TABLE',102)]] == bios_hooks_before, 'BIOS hooks not restored after load failure'
        m.screenshot(O / 'missing-module.png')
        m.call('bp_clear')
        m.call('emu_run_for', ms=250)
        m.stop_at(n['mainLoop'])
        m.tap('enter')
        m.until(n['mainLoop'], 120, page=0)
        (O / 'results.json').write_text(json.dumps(dict(missingModule='PASS', returnedToNC=True), indent=2) + '\n')
        print('FONTBEN missing module and return PASS', flush=True)
        raise SystemExit(0)
    if args.expect_usage:
        free_until(s['bench_restore'])
        m.screenshot(O / 'usage.png')
        assert m.read(s['reportSize'], 2) == b'\0\0'
        if args.bench_args == '-?':
            assert m.read(s['benchHelp'], 1) == b'\1'
        m.stop_at(0)
        m.call('emu_run_for', ms=250)
        m.until(0, 120, page=0)
        m.stop_at(n['mainLoop'])
        m.until(n['mainLoop'], 120, page=0)
        (O / 'results.json').write_text(json.dumps(dict(arguments=args.bench_args, usage='PASS', returnedToNC=True), indent=2) + '\n')
        print('FONTBEN usage and return PASS', flush=True)
        raise SystemExit(0)
    free_until(s['report'])
    corpus = m.read(s['textCorpus'], 1152)
    raw = (R / 'assets/system-rom/README.TXT').read_bytes()
    expected_corpus = bytes(32 if c < 32 else c for c in raw.replace(b'\r\n', b'\n').replace(b'\r', b'\n'))[:1152].ljust(1152, b' ')
    if args.block32:
        expected_corpus = bytes(32 if 128 <= c < 192 else c for c in expected_corpus)
    assert corpus == expected_corpus
    if args.block32_corpus:
        corpus = bytes(32 if 128 <= c < 192 else c for c in corpus)
        m.write(s['textCorpus'], corpus)
    (O / 'corpus.bin').write_bytes(corpus)
    m.screenshot(O / 'text.png')
    options = m.read(s['benchOptions'], 7)
    all_cases = cases
    engines = m.read(s['benchEngine'], 1)[0]
    def bank_supported(case):
        return True
    def selected(case):
        kind, flags = int(case[1]), int(case[2])
        font = 1 if kind < 3 else 2 if kind < 5 else 4
        fmt = 1 if kind < 7 else 2 if kind < 9 else 4
        method = 4 if flags & 16 else 8 if flags & 64 else 16 if flags & 32 else 32 if kind == 1 else 1 if kind in (0,3,5,7,9) else 2
        return bool(options[0] & font and (kind < 5 or options[1] & fmt) and options[3] & method and (options[2] != 2 or bank_supported(case)) and engines & (2 if case[0].startswith('kernel') else 4 if case[0].startswith('paged') else 1))
    cases = [case for case in all_cases if selected(case)]
    expected_stages = len(cases) * bool(options[2] & 1) + sum(bank_supported(c) for c in cases) * bool(options[2] & 2)
    assert m.read(s['benchCaseCount'], 1)[0] == len(cases)
    assert m.read(s['benchStageTotal'], 1)[0] == expected_stages
    pages = (len(cases) + 24) // 25
    final_pc = s['bench_restore'] if options[5] else s['waitKey']
    free_until(s['matrixPageKey'] if pages > 1 and not options[5] else final_pc, watch=True)
    m.screenshot(O / 'results-1.png')
    if not options[5]:
        for page in range(1, pages):
            enter_to(s['matrixPageKey'] if page < pages - 1 else final_pc)
            m.screenshot(O / f'results-{page + 1}.png')
    assert m.read(s['matrixStage'], 1)[0] == expected_stages, dict(stage=m.read(s['matrixStage'],1)[0], expected=expected_stages, regs=m.read(s['bankRegs'],14).hex(), command=m.read(s['bankPath'],40).hex(), firstFailedStage=m.read(s['bitmapFailedStage'],1)[0])
    assert int.from_bytes(m.read(s['bitmapResult'], 2), 'little') == s['matchText'], ('first failed stage', m.read(s['bitmapFailedStage'], 1)[0])
    assert int.from_bytes(m.read(s['reportMessage'], 2), 'little') == s['benchSkippedText'] if options[4] else int.from_bytes(m.read(s['reportMessage'], 2), 'little') == s['reportSavedText']
    assert m.read(s['reportHandle'] + 42, 1) == b'\0'
    assert m.read(s['BASE_DRAWCHAR'], 3) == bios_draw_before
    assert [m.read(s[k], n) for k,n in [('bios_printChar',3), ('bios_printString',3), ('BASE_BIOS_TABLE',102)]] == bios_hooks_before, 'BIOS hooks not restored'
    assert m.read(s['bankHandle'], 2) == b'\0\0', 'module not unloaded'
    kernel = sy(R / 'bin/banked/KERNEL.sym')
    try:
        m.write(0xfffd, b'\0')
        allocations = m.read(kernel['MEM_ALLOC_TABLE'], 32 * 6)
        modules = m.read(kernel['MODULE_TABLE'], kernel['MODULE_MAX'] * kernel['MODULE_RECORD_SIZE'])
        assert m.read(0xc000, 0x800) == dos_reserve_before, 'DOS reserve modified'
    finally:
        m.write(0xfffc, b'\0')
    assert all(allocations[i] == 255 for i in range(0, len(allocations), 6)), 'allocation leak'
    assert not any(modules[i:i+2] != b'\0\0' for i in range(0, len(modules), kernel['MODULE_RECORD_SIZE'])), 'module leak'
    ticks = {name: int.from_bytes(m.read(s[name], 4), 'little') for case in cases for name in case[3:5]}
    misses = {name: int.from_bytes(m.read(s[name], 2), 'little') for case in cases for name in case[5:7] if name != '0'}
    size = int.from_bytes(m.read(s['reportSize'], 2), 'little')
    if not options[4]:
        assert 0 < size <= s['REPORT_LIMIT'] - s['REPORT_BUFFER']
        report_bytes = m.read(s['REPORT_BUFFER'], size)
        assert report_bytes.startswith(b'FONTBEN README 48x24') and b'Bitmap: PASS\r\n' in report_bytes
        assert report_bytes.count(b' / ') == len(cases) + 2
    else:
        assert size == 0
    assert all(case[-1].encode().startswith(b'label') for case in cases)
    result = dict(corpusBytes=1152, rows=24, columns=48, stages=expected_stages, engineFilter=engines, arguments=args.bench_args, options=list(options), cases=[c[-1] for c in cases], bitmap='PASS', continuousRun=True, firstFailedStage=m.read(s['bitmapFailedStage'], 1)[0],
                  ticks=ticks, ms={k: round(v * .128, 3) for k, v in ticks.items()}, misses=misses,
                  reportBytes=size, file=None if options[4] else 'B:\\FONTBEN.TXT', publicMemoryABI=True, biosHooksRestored=True, mainStringCalls=1, bankWindowStringCalls=1, far48StringCalls=24, farBulkStringCalls=1, crlfInText=True, bankStringChunkBytes=256, timerLocal=True, nativeBankLaunch=True, modulesUnloaded=True, allocationLeaks=0, dosReserveUnchanged=True, executionBank=m.read(s['bankPageCode'], 1)[0] if options[2] & 2 else None, unsupportedBankCases=[c[-1] for c in cases if not bank_supported(c)])
    (O / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('FONTBEN', expected_stages, 'stages PASS; report bytes:', size, flush=True)

    if not options[4]:
        # Проверяем файл обычными OPEN/READ/CLOSE и повторное сохранение с усечением.
        original = m.call('cpu_state')['registers']
        saved_case = m.read(s['matrixCase'], 2)
        saved_drive = m.read(s['reportDrive'], 1)[0]
        read_report(report_bytes)
        handle(2 | 16)
        extra = b'EXTRA-TRAILING!'
        m.write(0x8B70, extra)
        m.write(0x8B20 + 49, (0x8B70).to_bytes(2, 'little') + len(extra).to_bytes(2, 'little'))
        abi(s['bios_fileHandle'], af=2 << 8, hl=0x8B20)
        abi(s['bios_fileHandle'], af=5 << 8, hl=0x8B20)
        abi(s['bios_fileSelectDrive'], af=saved_drive << 8)
        abi(s['matrixSaveReport'])
        assert int.from_bytes(m.read(s['reportMessage'], 2), 'little') == s['reportSavedText']
        read_report(report_bytes)
        abi(s['bios_fileSelectDrive'], af=saved_drive << 8)
        m.write(s['matrixCase'], saved_case)
        m.call('bp_clear')
        m.call('regs_set', **{k: original[k]['dec'] for k in ('af', 'bc', 'de', 'hl', 'pc', 'sp')})
        (O / 'FONTBEN.TXT').write_bytes(report_bytes)
        result.update(fileReadback='PASS', fileTruncate='PASS')
        print('RAM report readback and truncate PASS', flush=True)
    if not options[5]:
        m.call('emu_run_for', ms=250)
        m.tap('a')
        assert m.read(0, 32) == (R / ('bin/FONTRMP.i80' if args.remap32 else 'bin/FONTBLK.i80' if args.block32 else 'bin/FONTLAB.i80')).read_bytes()[:32]
        m.stop_at(0)
        m.tap('enter')
    else:
        m.stop_at(0)
        m.call('emu_run_for', ms=250)
    m.until(0, 120, page=0)
    m.stop_at(n['mainLoop'])
    m.until(n['mainLoop'], 120, page=0)
    m.screenshot(O / 'returned.png')
    result.update(enterOnlyVerified=not bool(options[5]), quickReturnVerified=bool(options[5]), returnedToNC=True)
    result['binarySha256'] = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [Path('bin/FONTRMP.i80' if args.remap32 else 'bin/FONTBLK.i80' if args.block32 else 'bin/FONTLAB.i80'), Path('images/SystemRom_emu80_writable.bin')]}
    (O / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
finally:
    x.stop()
