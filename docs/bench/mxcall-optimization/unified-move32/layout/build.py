"""Исследовательская сборка четырёх сегментов MEM ревизии 65 с общей развёрткой."""

from pathlib import Path
import argparse
import re
import shutil
import struct
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
REPORT = HERE.parent
ROOT = REPORT.parents[3]
OUT = HERE / 'out'


def read(path):
    return path.read_text(encoding='utf-8-sig')


def put(path, data):
    path.write_text(data, encoding='utf-8')


def change(path, old, new):
    data = read(path)
    assert data.count(old) == 1, (path, old)
    put(path, data.replace(old, new))


def symbols(path):
    return {k: int(v, 16) for k, v in
            re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)', read(path), re.M)}


def build(stage, checkout=None):
    mem = stage / 'source/mem'
    inc = stage / 'source/include'
    bank = stage / 'bin/banked'
    shutil.copytree(ROOT / 'source/mem', mem)
    shutil.copytree(ROOT / 'source/include', inc)
    shutil.copytree(ROOT / 'source/common', stage / 'source/common')
    bank.mkdir(parents=True)
    service_import = (checkout or ROOT) / 'bin/banked/main-io-import.inc'
    shutil.copyfile(service_import, bank / 'main-io-import.inc')
    for name in ('copy.inc', 'copy-resident.inc', 'copytail.inc', 'remote-move.inc',
                 'remote-move-prepare.inc'):
        shutil.copyfile(REPORT / 'transport' / name, mem / name)

    change(inc / 'mxos.inc', '= 0FE71h', '= 0F4E6h')
    change(inc / 'mxos.inc', '= 0FE87h', '= 0FE00h')
    change(inc / 'mxos.inc', 'MEM_RUNTIME_REVISION = 65', 'MEM_RUNTIME_REVISION = 66')
    change(inc / 'mxos.inc', 'RAMD_PAGE_END        = 0FFBEh', 'RAMD_PAGE_END        = 0FF4Dh')
    change(mem / 'abi.inc', '= 0FE62h', '= 0F4D7h')
    copy = mem / 'copy.inc'
    change(copy, '    xor a\n    ld (bankCopyMove),a\nbankCopyEnter:', 'bankCopyEnter:')
    change(copy, '    ld a,(bankCopyMove)\n    or a\n    jp nz,MEM_MOVE_SELECT_ADDR\n', '')
    change(copy, '    ld a,0FFh\n    cp h', '    ld a,high MEM_DATA_LIMIT\n    cp h\n    ld a,BANK_ERROR_RANGE')
    change(copy, 'bankCopyMove: DB 0', '')
    change(copy, '    jp z,bankCopyBulkDone', '    jp m,bankCopyRemoteDone\n    jp z,bankCopyBulkDone')
    change(copy, '''    ld hl,(bankCopySP)
    ld sp,hl
    ld a,BANK_PROFILE_COPY
    ld (BANK_PROFILE),a''', '''    ld a,BANK_PROFILE_COPY
    ld (BANK_PROFILE),a
bankCopyRemoteDone:
    ld hl,(bankCopySP)
    ld sp,hl''')
    fast = read(mem / 'TRANSPORT.asm')
    start = fast.index('bankFastPrepare:')
    end = fast.index('    ASSERT $ <= MEM_DATA_START', start)
    aux = fast[start:end]
    fast = fast[:start] + fast[end:]
    fast = fast.replace('    ASSERT $ <= MEM_DATA_START', '''bankMoveToMain:
    ld (BANK_IO_MAIN),a
    jp bankCopyMoveRemote
    ASSERT $ <= MEM_DATA_START-3
    IF $ < MEM_DATA_START-3
    DS MEM_DATA_START-3-$,0
    ENDIF
bankMoveToRam:
    ld (BANK_IO_RAM),a
    ASSERT $ == MEM_DATA_START
    ASSERT $ <= MEM_DATA_START''')
    # Бинарник после второй ORG служит входом INCBIN для LOCALBLOCK. В TRANSPORT
    # входит только часть до bankDataTailEnd. Символы fast разрешены в одном проходе.
    fast += '\n    ORG 0F475h ; fileorg-ok\n' + aux + 'movedEnd:\n'
    fast = fast.replace('    INCLUDE "copy-resident.inc"',
                        '    INCLUDE "remote-move.inc"\n'
                        '    INCLUDE "compact223.inc"\n    compact223Upper\n'
                        'coldEnd:\n    INCLUDE "copy-resident.inc"')
    fast = fast.replace('    ASSERT $ <= MEM_EXEC_READY',
                        '    compact223Hot\nhotEnd:\n    ASSERT $ <= MEM_EXEC_READY')
    put(mem / 'TRANSPORT.asm', fast)
    shutil.copyfile(REPORT / 'core-test/compact223.inc', mem / 'compact223.inc')
    shutil.copyfile(HERE / 'remote-registers.inc', mem / 'remote-move.inc')
    shutil.copyfile(HERE / 'remote-registers-prepare.inc', mem / 'remote-move-prepare.inc')
    change(mem / 'copytail.inc', 'DS 6,0', 'DS 8,0')
    change(mem / 'copytail.inc', 'bankCopyStack:\n', 'bankCopyStack:\n    ASSERT $ == RAMD_PAGE_END\n')

    data = read(mem / 'memory.inc')
    start = data.index('memMove:\n')
    end = data.index('memScalar:\n', start)
    data = data[:start] + read(HERE / 'select.inc') + '\n' + data[end:]
    put(mem / 'memory.inc', data)
    # Внешний указатель MEM_NEAR_COPY и его исторический формат ещё присутствуют
    # в загрузочных шаблонах. В едином публичном FAR-пути используются прямые адреса.
    change(mem / 'near-copy.inc', '    call memNearInvoke', '    call bankUnifiedCore')
    data = read(mem / 'near-copy.inc').split('memNearInvoke:')[0]
    put(mem / 'near-copy.inc', data)
    for name in ('queue-hook.inc', 'TRANSPORT.asm'):
        path = mem / name
        data = re.sub(r'(?m)^(\s*)DS (\w+)-\$,0$',
                      r'\1IF $ < \2\n\1DS \2-$,0\n\1ENDIF', read(path))
        put(path, data)

    def assemble(name, target):
        subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow', '-Wno-shortblock',
                        f'--raw={target}.bin', f'--sym={target}.sym', f'--lst={target}.lst',
                        (mem / f'{name}.asm').as_posix()], cwd=mem, check=True)
        return symbols(target.with_suffix('.sym'))

    ts = assemble('TRANSPORT', bank / 'TRANSPORT')
    tran = bank / 'TRANSPORT.bin'
    size = ts['bankDataTailEnd'] - ts['BANK_LANDING']
    payload = tran.read_bytes()
    assert len(payload) - size == ts['movedEnd'] - 0xf475
    (bank / 'fast.bin').write_bytes(payload[size:])
    tran.write_bytes(payload[:size])
    oldimports = read(ROOT / 'bin/banked/transport-imports.inc')
    names = re.findall(r'^(\w+) = ', oldimports, re.M)
    count = len(payload[:size])
    exports = {k: ts[k] for k in names if k in ts}
    for k in ('bankCopyMoveRemote', 'bankMoveToRam', 'bankMoveToMain'):
        exports[k] = ts[k]
    exports['bankUnifiedCore'] = ts['moveCore']
    exports.update(BANK_COPY_DATA_BYTES=ts['bankDataTailEnd']-ts['MEM_DATA_START'],
                   BANK_TRANSPORT_BYTES=count, BANK_COPY_GUARD_ADDR=ts['bankKeyboardSignature'])
    imp = bank / 'transport-imports.inc'
    put(imp, ''.join(f'{k} = 0{v:04X}h\n' for k, v in exports.items()))
    ls = assemble('LOCALBLOCK', bank / 'LOCALBLOCK')
    blob = (bank / 'LOCALBLOCK.bin').read_bytes()
    split = ls['mx_call'] - ls['MEM_IMAGE_LOCAL_START']
    (bank / 'LOCALBLOCK.bin').write_bytes(blob[:split])
    (bank / 'CLIENT.bin').write_bytes(blob[split:])
    with imp.open('a', encoding='utf-8') as f:
        f.write(f"BANK_MAIN_CLIENT_BYTES = {ls['bankClientEnd']-ls['mx_call']}\n")
        f.write(f"BANK_MAIN_LOCAL_BYTES = {ls['MEM_IO_SYSTEM']-ls['MEM_IMAGE_LOCAL_START']}\n")
    assemble('WINDOW', bank / 'WINDOW')
    ms = assemble('MEM', bank / 'MEM')
    OUT.mkdir(exist_ok=True)
    shutil.copyfile(bank / 'MEM.bin', OUT / 'MEM.sys')
    for name in ('TRANSPORT', 'MEM', 'LOCALBLOCK'):
        for suffix in ('sym', 'lst'):
            shutil.copyfile(bank / f'{name}.{suffix}', OUT / f'{name}.{suffix}')
    for name in ('CLIENT', 'LOCALBLOCK', 'TRANSPORT', 'WINDOW'):
        shutil.copyfile(bank / f'{name}.bin', OUT / f'{name}.bin')
    print('MEM.sys', (OUT / 'MEM.sys').stat().st_size,
          'segments', [struct.unpack_from('<HHH', (OUT/'MEM.sys').read_bytes(), 6+i*6)[:2]
                        for i in range(4)], 'data', ms['MEM_DATA_SIZE'],
          'cold end', hex(ts['coldEnd']), 'hot end', hex(ts['hotEnd']),
          'aux end', hex(ts['movedEnd']))
    if checkout is not None:
        export(stage, checkout)


def export(stage, checkout):
    """Переносить только в отдельную исследовательскую рабочую копию."""
    assert checkout.resolve() != ROOT.resolve()
    dest = checkout / 'source/mem'
    for name in ('TRANSPORT.asm', 'abi.inc', 'memory.inc', 'near-copy.inc', 'copy.inc',
                 'copy-resident.inc', 'copytail.inc', 'remote-move.inc', 'compact223.inc',
                 'queue-hook.inc', 'remote-move-prepare.inc'):
        shutil.copyfile(stage / 'source/mem' / name, dest / name)
    shutil.copyfile(stage / 'source/include/mxos.inc', checkout / 'source/include/mxos.inc')
    path = checkout / 'scripts/build/build-banked-transport.py'
    data = read(ROOT / 'scripts/build/build-banked-transport.py')
    insert = data.index("    (OUT/'transport-imports.inc').write_text")
    data = data[:insert] + """    transport = OUT/'TRANSPORT.bin'
    payload = transport.read_bytes()
    length = sy['bankDataTailEnd'] - sy['BANK_LANDING']
    assert len(payload) - length == sy['movedEnd'] - sy['bankFastPrepare']
    (OUT/'fast.bin').write_bytes(payload[length:])
    transport.write_bytes(payload[:length])
""" + data[insert:]
    data = data.replace("('bankMxRegsDos','bankFastPrepare'", "('bankCopyMoveRemote','bankMoveToRam','bankMoveToMain','bankMxRegsDos','bankFastPrepare'")
    data = data.replace("'bankCopyMove',", '')
    anchor = "        stream.write(f'BANK_COPY_DATA_BYTES"
    data = data.replace(anchor, """        stream.write(f'bankUnifiedCore = 0{sy["moveCore"]:04X}h\\n')
""" + anchor)
    put(path, data)
    entries = checkout / 'source/common/move-mem-entries.inc'
    ts = symbols(OUT / 'TRANSPORT.sym')
    if entries.exists():
        put(entries, '; Экспорт входов из экспериментального TRANSPORT.\n'
            f'moveMemRevision = {ts["MEM_RUNTIME_REVISION"]}\n'
            f'moveMemCore     = 0{ts["moveCore"]:04X}h\n'
            f'moveMemForward  = 0{ts["moveBytesForward"]:04X}h\n')
    # Детектор ДОЗУ пишет в каждую страницу; в новом MEM по FFBE находится RET.
    # Для проверки используем нижний байт пустого служебного стека транспорта.
    path = checkout / 'source/bootstrap/BOOTBANK.asm'
    data = read(path).replace('ld hl,0FFBEh', 'ld hl,BOOT_RAM_PROBE')
    old = '    ld hl,0FF04h\n    ld de,memImage+MEM_DATA_OFFSET+(0FF04h-MEM_DATA_START)'
    new = '    ld hl,BOOT_RAM_SIGNATURE\n    ld de,memImage+MEM_DATA_OFFSET+(BOOT_RAM_SIGNATURE-MEM_DATA_START)'
    assert old in data or new in data
    data = data.replace(old, new)
    put(path, data)
    path = checkout / 'scripts/build/build-bootstrap.py'
    data = read(ROOT / 'scripts/build/build-bootstrap.py')
    line = next(l for l in data.splitlines(True) if "f'MEM_DATA_START =" in l)
    data = data.replace(line, line + line.replace('MEM_DATA_START', 'BOOT_RAM_PROBE', 1)
                        .replace('symbols["MEM_DATA_START"]', 'symbols["RAMD_PAGE_END"]')
                        + line.replace('MEM_DATA_START', 'BOOT_RAM_SIGNATURE', 1)
                        .replace('symbols["MEM_DATA_START"]', 'symbols["BANK_COMMAND_LANDING"]'))
    put(path, data)
    signature = ts['BANK_COMMAND_LANDING'] - ts['BANK_LANDING']
    old = (ROOT / 'bin/banked/TRANSPORT.bin').read_bytes()[signature:signature+8]
    new = (OUT / 'TRANSPORT.bin').read_bytes()[signature:signature+8]
    assert old == new and len(set(old)) > 3


if __name__ == '__main__':
    parse = argparse.ArgumentParser()
    parse.add_argument('--checkout', type=Path)
    args = parse.parse_args()
    with tempfile.TemporaryDirectory(prefix='mxos-unified-mem-') as tmp:
        build(Path(tmp), args.checkout)
