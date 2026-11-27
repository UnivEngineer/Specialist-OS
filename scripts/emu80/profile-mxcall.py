"""Профиль синхронного вызова от CALL до следующей команды на CPU Emu80."""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
helpers = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
symbols = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-mxcall.py'))['symbols']
WORD = struct.Struct('<H').pack
REGS = struct.Struct('<4H')
# На время измерения машина остановлена. В DOS это буфер файлового обмена.
STUB, PACKET, STRING, HOOK, COUNT, NEST, STACK = 0x8000, 0x8040, 0x8060, 0x8090, 0x80b0, 0x80c0, 0x8600
PAGES = (0, 1, 6)
MAPPER_PORTS = (0xfffc, 0xfffd)
WATCH_SKIP = 1_000_000
LEAF_STACK_BYTES = 4
LEAF_GUARD_BYTES = 2
BIOS_VECTORS = {
    'BIOS_UPPERCASE': 'bios_upperCase', 'BIOS_SUB_HL_DE': 'bios_sub_hl_de',
    'BIOS_GET_CURSOR_POS': 'bios_getCursorPos', 'BIOS_SET_CURSOR_POS': 'bios_setCursorPos',
    'BIOS_PRINT_CHAR': 'bios_printChar', 'BIOS_PRINT_STRING_Z': 'bios_printStringZ',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True, type=Path)
    parser.add_argument('--port', required=True, type=int, choices=range(19781, 19797))
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--quick', action='store_true', help='Контрольные строки COMMAND из EXEC')
    parser.add_argument('--audit-only', action='store_true', help='Счётчики и стеки с минимумом точек останова')
    parser.add_argument('--inline-only', action='store_true', help='Только вход F806h и вложенный вызов')
    parser.add_argument('--direct-bios-only', action='store_true', help='Только прямые CALL векторов BIOS из MAIN')
    parser.add_argument('--operations', nargs='+')
    parser.add_argument('--pages', nargs='+', type=int, choices=PAGES)
    args = parser.parse_args()
    folder = args.out.resolve()
    folder.mkdir(parents=True, exist_ok=True)
    runtime = args.runtime.resolve()
    context = helpers['Matrix'].__init__.__globals__
    context.update(OUT=folder, RUNTIME=runtime)
    context['mcp_owner'] = lambda: helpers['shell'](
        f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue;'
        'if($p){$p[0].OwningProcess};exit 0')
    machine = helpers['Matrix']()
    m = machine.m
    m.url = f'http://127.0.0.1:{args.port}/mcp'
    banked = ROOT / 'bin/banked'
    k = symbols(banked / 'CLIENT.sym')
    t = symbols(banked / 'TRANSPORT.sym')
    b = symbols(ROOT / 'bin/BIOS.sym')
    io = symbols(banked / 'MAINBIOS.sym')
    dos = symbols(banked / 'KERNEL.sym')
    image = ROOT / 'images/C-Flash-512k-MMX-System-Emu80.bin'
    window = (banked / 'WINDOW.bin').read_bytes()
    stack_top = {0: io['BANK_BIOS_STACK_TOP'], 1: k['DOS_BANK_STACK_TOP'], 6: k['DOS_BANK_STACK_TOP']}
    operations = ['BIOS_UPPERCASE', 'BIOS_SUB_HL_DE', 'BIOS_GET_CURSOR_POS', 'BIOS_SET_CURSOR_POS', 'BIOS_PRINT_CHAR',
                  'BIOS_PRINT_STRING_Z', 'BIOS_COMMAND_BATCH', 'DOS_GET_ERROR']
    points = {}
    def mark(symbols_, names, pages):
        for name in names.split():
            if name in symbols_:
                for page in pages:
                    points[page, symbols_[name]] = name
    mark(k, 'mx_callPacket mx_callRegs bankMxCall bankMxRegsRoute bankMxResult memPrintMain memPrintReturn', PAGES)
    mark(t, 'bankQueueBefore bankCallPacket bankSwitch bankLanding bankServiceReturn bankResume', PAGES)
    mark(t, 'BANK_SELECT_GATE' if 'BANK_SELECT_GATE' in t else 'BANK_WINDOW', PAGES)
    mark(io, 'bbEntry bbScalarDone bbScalarFast', [0])
    mark(b, 'sysPacketReady sysBiosRegs sysReturned upperCase sub_hl_de getCursorPos printChar printStringZ bankText.printStringLim', [0])
    mark(dos, 'kernelDispatch kernelLocalDispatch kdScalarFind kdInvoke dosGetError kdFinish', [1])
    mark(k, 'bankFastDecode bankFastReady', PAGES)
    mark(t, 'bankFastPrepare bankFastLanding bankFastReturn bankFastRestore', PAGES)
    mark(io, 'bbFastLeaf', [0])
    mark(dos, 'kdLeafGetError', [1])
    mark(io, 'bbFastIndex', [0])
    slots = {}
    if 'bbFastSlots' in io:
        leaves = [('BIOS_UPPERCASE', 'bios_upperCase'), ('BIOS_SUB_HL_DE', 'bios_sub_hl_de')]
        if 'mx_getCursorPos' not in k:
            leaves[:0] = [('BIOS_GET_CURSOR_POS', 'bios_getCursorPos'), ('BIOS_SET_CURSOR_POS', 'bios_setCursorPos')]
        for index, (op, vector) in enumerate(leaves):
            slots[op] = io['bbFastSlots'] + 4 * index, io[vector]
            mark(io, vector, [0])
    if args.audit_only:
        leaf_points = {'bbFastLeaf', 'bbFastIndex', 'kdLeafGetError', 'bankFastLanding'}
        points = {key: name for key, name in points.items() if name in leaf_points}
    leaf_pc = t.get('bankFastReturnPC')
    stack_bytes = 0 if slots else LEAF_STACK_BYTES
    if leaf_pc is not None:
        leaf_stack = leaf_pc - stack_bytes
        leaf_guard = leaf_stack - LEAF_GUARD_BYTES
        leaf_return = WORD(t['bankFastReturn'])

    def watch_counts():
        counts = {}
        for bp in m.call('bp_list')['breakpoints']:
            if bp['type'] == 'write':
                addr = int(bp['addr'], 16)
                count = bp['hitCount']
                assert bp['skipCount'] == WATCH_SKIP and bp['remaining'] == WATCH_SKIP-count, bp
                counts[addr] = count
        assert set(counts) == set(MAPPER_PORTS), counts
        return counts
    def page(n):
        m.write(0xfffd if n else 0xfffc, bytes([n-1 if n else 0]))
    def run_code(n, code, registers):
        page(n)
        m.write(STUB, code)
        stop = STUB + len(code)
        m.stop_at(stop)
        m.call('regs_set', pc=STUB, sp=STACK, **registers)
        return m.until(stop, 5, page=n)
    def context_bytes():
        return tuple(m.read(t[name], size) for name, size in (
            ('bankOwner', 2), ('bankDepth', 1), ('bankServiceSP', 2), ('bankStackFloor', 2)))
    rows = []
    try:
        machine.start('mxcall-profile', system=image)
        m.call('debug_break')
        m.call('bp_clear')
        page(6)
        for addr, name in [(k['MEM_IMAGE_LOCAL_START'], 'LOCALBLOCK'), (k['mx_callPacket'], 'CLIENT'), (t['BANK_LANDING'], 'TRANSPORT')]:
            m.write(addr, (banked / (name + '.bin')).read_bytes())
        before = {}
        leaf_guards = {}
        if slots:
            page(0)
            slot_table = b''.join(WORD(vector) + leaf_return for _, vector in slots.values())
            assert m.read(io['bbFastSlots'], len(slot_table)) == slot_table
        for n in PAGES:
            page(n)
            m.write(t['bankPage'], bytes([n]))
            m.write(t['bankOwner'], bytes([7+n, n]))
            m.write(t['bankDepth'], bytes(1))
            m.write(t['bankServiceSP'], WORD(stack_top[n]))
            m.write(t['bankStackFloor'], WORD(stack_top[n] - (192 if n == 0 else 512)))
            if 'MEM_COMMAND_SHADOW' in k:
                m.write(k['MEM_COMMAND_SHADOW'], window)
            m.write(STRING, b'X\0' + bytes(24))
            m.write(NEST, b'\xcd' + WORD(k['mx_callRegs']) + WORD(k['MXOS_BIOS_UPPERCASE']) + b'\xc9')
            before[n] = context_bytes()
            if leaf_pc is not None:
                leaf_guards[n] = m.read(leaf_guard, LEAF_GUARD_BYTES)
                assert m.read(leaf_pc, 2) == leaf_return, n
        src = {'git': subprocess.check_output(['git','rev-parse','HEAD'], cwd=ROOT, text=True).strip(),
               'cpu': m.call('sys_info'), 'imageSha256': hashlib.sha256(image.read_bytes()).hexdigest(),
               'emulatorSha256': hashlib.sha256((runtime/'Emu80qt-mcp.exe').read_bytes()).hexdigest(),
               'memBytes': (ROOT/'bin/MEM.sys').stat().st_size, 'runtimeRevision': k['MEM_RUNTIME_REVISION'],
               'auditOnly': args.audit_only, 'leafReturnPC': leaf_pc}
        (folder/'system.json').write_text(json.dumps(src, indent=2)+'\n', encoding='utf-8')
        def measure(n, operation, inline, after_copy, repeat, queue, direct_vector=None):
            page(n)
            # Активный callback сам обращается к F80Ch: вложенный flush должен сразу вернуться.
            m.write(HOOK, b'\x21'+WORD(COUNT)+b'\x34\xcd'+WORD(k['mx_queueFlush'])+b'\xc9')
            run_code(n, b'\xcd'+WORD(k['mx_queueBind']), dict(hl=HOOK if queue else 0))
            m.write(COUNT, b'\0')
            if repeat == 0:
                m.write(0xffc0, window)
                if after_copy:
                    m.write(PACKET, struct.pack('<BHBHH', 6, STRING+16, 0, STRING+20, 1))
                    run_code(n, b'\xcd'+WORD(k['mx_memcpyFar']), dict(hl=PACKET))
            m.write(COUNT, b'\0')
            hl, de, bc, af = 0x1234, 0x2345, 0x3451, 0x7193
            if operation == 'BIOS_PRINT_STRING_Z':
                hl = STRING
                m.write(STRING, b'X\0'+bytes(20))
            if operation == 'BIOS_COMMAND_BATCH':
                hl, bc, af = STRING, 5, (n << 8) | 2
                cursor_command = k.get('MXOS_COMMAND_BIOS_SET_CURSOR_POS', 10)
                m.write(STRING, bytes([cursor_command, 5, 1, 11, 15]))
            page(0)
            m.write(b['bios_vars.cursorY'], WORD(0x100a))
            page(1)
            m.write(dos['v_dosError'], b'\0')
            page(n)
            input_packet = REGS.pack(hl,de,bc,af)
            m.write(PACKET, input_packet)
            regs = dict(hl=hl, de=de, bc=bc, af=af)
            if direct_vector is not None:
                code = b'\xcd'+WORD(direct_vector)
            elif operation in BIOS_VECTORS and 'MXOS_'+operation not in k:
                code = b'\xcd'+WORD(k[BIOS_VECTORS[operation].replace('bios_', 'mx_')])
            elif operation == 'NESTED':
                code = b'\xcd'+WORD(t['bankCallPacket'])
                regs = dict(af=0x0102, hl=t['bankCallback'], bc=NEST, de=PACKET)
            else:
                op = k['MXOS_'+operation]
                code = b'\xcd'+WORD(k['mx_callRegs'] if inline else k['mx_callPacket'])
                if inline:
                    code += WORD(op)
                else:
                    regs.update(bc=op,de=PACKET)
            # У каждого исполнителя свой стек. Два байта защиты ниже тестируемой области.
            guard = bytes([0xa5])*192
            for dest in PAGES:
                page(dest)
                m.write(stack_top[dest]-len(guard), guard)
                if leaf_pc is not None and stack_bytes:
                    m.write(leaf_stack, guard[:stack_bytes])
            page(n)
            m.write(STUB, code)
            stop = STUB+len(code)
            m.call('bp_clear')
            for address in sorted({addr for _,addr in points} | {stop}):
                m.call('bp_set', addr=address, type='exec')
            for address in MAPPER_PORTS:
                m.call('bp_set', addr=address, type='write', skipCount=WATCH_SKIP)
            m.call('regs_set', pc=STUB, sp=STACK, **regs)
            start = m.call('cpu_state')['cpu_ticks']
            events = []
            for _ in range(160):
                m.call('emu_run_for', ms=5)
                state = m.call('cpu_state')
                r = {key: val['dec'] for key,val in state['registers'].items()}
                pc = r['pc']
                current = m.read(t['bankPage'], 1)[0]
                elapsed = state['cpu_ticks']-start
                if pc == stop and current == n:
                    break
                name = points.get((current,pc))
                if name:
                    events.append(dict(at=name,page=current,t=elapsed,sp=r['sp']))
                m.call('debug_step', mode='into')
            else:
                raise RuntimeError((operation,n,events[-6:],state))
            # Счётчики читаются до записей тестового кода в регистры маппера.
            # Emu80 считает и чтения при ненулевом skipCount. Измеряемые
            # маршруты применяют FFFCh/FFFDh только для записи селектора.
            writes = watch_counts()
            m.call('bp_clear')
            mapper_writes = {f'{a:04X}': writes[a] for a in MAPPER_PORTS}
            assert r['sp'] == STACK, (operation,r)
            if operation in ('BIOS_UPPERCASE','NESTED'):
                assert (r['af']>>8,r['bc'],r['de'],r['hl']) == (ord('Q'),bc,de,hl), r
            elif operation == 'BIOS_PRINT_CHAR':
                assert (r['af'],r['bc'],r['de'],r['hl']) == (af,bc,de,hl), r
            elif operation == 'BIOS_GET_CURSOR_POS':
                assert (r['af'],r['bc'],r['de'],r['hl']) == (af,bc,de,0x100a), r
            elif operation == 'BIOS_SET_CURSOR_POS':
                assert (r['af'],r['bc'],r['de'],r['hl']) == (af,bc,de,hl), r
                page(0)
                assert m.read(b['bios_vars.cursorY'], 2) == WORD(hl)
                page(n)
            elif operation == 'BIOS_SUB_HL_DE':
                assert (r['bc'], r['de'], r['hl'], r['af'] & 1) == (bc, de, (hl-de)&65535, int(hl<de)), r
            elif operation == 'BIOS_PRINT_STRING_Z':
                assert r['hl'] == STRING+1, r
            elif operation == 'DOS_GET_ERROR':
                assert r['af']>>8 == 0 and not r['af']&1, r
            elif operation == 'BIOS_COMMAND_BATCH':
                assert not r['af'] & 1, r
                page(0)
                assert m.read(b['bios_vars.cursorY'], 2) == WORD(0x0f0b), r
                page(n)
            if operation != 'NESTED' and not inline and 'MXOS_'+operation in k:
                assert m.read(PACKET,8) == REGS.pack(*(r[v] for v in ('hl','de','bc','af'))), (operation,r)
            callbacks = m.read(COUNT,1)[0]
            used = {}
            leaf_used = {}
            leaf_returns = set()
            if leaf_pc is not None:
                for event in events:
                    if event['at'] not in ('bbFastLeaf', 'bankFastLanding', 'kdLeafGetError'):
                        continue
                    if slots and event['at'] == 'bankFastLanding':
                        bios_op = 'BIOS_UPPERCASE' if operation == 'NESTED' else operation
                        assert event['page'] == 0 and event['sp'] == slots[bios_op][0], event
                        leaf_returns.add(0)
                    elif event['sp'] == leaf_pc:
                        leaf_returns.add(event['page'])
                    else:
                        assert event['at'] == 'kdLeafGetError' and n == 1, event
            for dest in PAGES:
                page(dest)
                assert context_bytes() == before[dest], (operation,n,dest)
                stack = m.read(stack_top[dest]-len(guard),len(guard))
                assert stack[:2] == guard[:2]
                used[dest] = max((len(stack)-i for i,v in enumerate(stack) if v != 0xa5),default=0)
                if leaf_pc is not None:
                    leaf = m.read(leaf_guard, LEAF_GUARD_BYTES + stack_bytes + 2)
                    assert leaf[:LEAF_GUARD_BYTES] == leaf_guards[dest] and leaf[-2:] == leaf_return, (dest,leaf)
                    payload = leaf[LEAF_GUARD_BYTES:-2]
                    observed = max((stack_bytes-i for i,v in enumerate(payload) if v != 0xa5), default=0)
                    depth = max((leaf_pc-e['sp'] for e in events if e['page'] == dest and
                                 e['at'] == 'bbFastIndex'), default=0)
                    assert observed == depth in (0, stack_bytes), (dest,leaf,operation,n,events)
                    leaf_used[dest] = depth
                if slots and dest == 0:
                    assert m.read(io['bbFastSlots'], len(slot_table)) == slot_table
            page(n)
            run_code(n,b'\xcd'+WORD(k['mx_queueBind']),dict(hl=0))
            # Копии MX_REGS в общем окне, без заголовков и пользовательского текста.
            count = lambda name: sum(e['at']==name for e in events)
            pages = [n] + [e['page'] for e in events] + [n]
            fast_return = 4 if 'bankPacketSP' in t else 8
            return dict(page=n,operation=operation,inline=inline,afterCopy=after_copy,repeat=repeat,
                        queue=queue,ticks=elapsed,events=events,
                        registers={v:r[v] for v in ('hl','de','bc','af')},serviceStackBytes=used,
                        mapperWrites=sum(mapper_writes.values()),mapperWritesByAddress=mapper_writes,
                        leafStackBytes=leaf_used,leafStackMax=max(leaf_used.values(),default=0),
                        leafReturnPages=sorted(leaf_returns),
                        bankSwitches=None if args.audit_only else sum(a!=b for a,b in zip(pages,pages[1:])),
                        windowRegisterBytes=None if args.audit_only else
                        8*count('bankCallPacket')+fast_return*count('bankServiceReturn'), callbacks=callbacks)
        def measure_direct_bios():
            native = {}
            for op, vector in BIOS_VECTORS.items():
                if args.operations and op not in args.operations:
                    continue
                address = io[vector]
                row = measure(0, op, True, False, 0, False, direct_vector=address)
                native[op] = dict(vector=vector, address=address, ticks=row['ticks'], registers=row['registers'])
                print(f'MAIN CALL {address:04X}h {op} {row["ticks"]} T', flush=True)
            (folder/'direct-bios.json').write_text(json.dumps(native, indent=2)+'\n',encoding='utf-8')
            return native

        if args.direct_bios_only:
            measure_direct_bios()
            return
        for n in (args.pages or ((6,) if args.quick else PAGES)):
            for op in (args.operations or operations + (['NESTED'] if n == 6 else [])):
                for inline in ([True] if args.inline_only or 'MXOS_'+op not in k else [False,True]):
                    for after_copy in ((False,) if args.quick else (False, True)):
                        for queue in ([False,True] if op in ('BIOS_UPPERCASE','BIOS_COMMAND_BATCH','BIOS_PRINT_STRING_Z') else [False]):
                            for rep in (range(1) if args.quick else range(2)):
                                row = measure(n,op,inline,after_copy,rep,queue)
                                rows.append(row)
                                with (folder/'samples.jsonl').open('a' if len(rows)>1 else 'w',encoding='utf-8') as f:
                                    f.write(json.dumps(row)+'\n')
                print(n, op, rows[-1]['ticks'], flush=True)
        if not args.audit_only:
            native = measure_direct_bios()
            for row in rows:
                if row['operation'] in native:
                    row['directBiosTicks'] = native[row['operation']]['ticks']
        filename = 'audit.json' if args.audit_only else 'profile.json'
        (folder/filename).write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
    finally:
        machine.stop()


if __name__ == '__main__':
    main()
