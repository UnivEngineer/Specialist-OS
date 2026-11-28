"""Счётчики системных вызовов в отдельном прогоне пользовательского сценария."""


class UiCounters:
    def __init__(self, machine, transport, kernel):
        self.m = machine
        self.t = transport
        self.k = kernel

    def until(self, target, page, *, graphics_submit=None, graphics_page=None, target_test=None):
        m, t, k = self.m, self.t, self.k
        m.call('bp_clear')
        m.call('bp_set', addr=target, type='exec')
        observed = {
            'F860': (t['mx_callPacket'], 'exec'),
            'F866': (t['mx_callRegs'], 'exec'),
            'F878': (t['mx_routeCall'], 'exec'),
            'selectMAIN': (t['IO_PAGE_RAM'], 'write'),
            'selectBank': (t['IO_PAGE_RAMD'], 'write'),
        }
        # Считаем входы и выбор банка без остановки CPU на каждом событии.
        for name, (address, kind) in observed.items():
            m.call('bp_set', addr=address, type=kind, skipCount=1_000_000, comment=name)
        traps = {k['moduleCall']: 'MODULE_CALL', k['moduleScope']: 'MODULE_SCOPE',
                 t['mx_memcpyFar']: 'farCopy'}
        if graphics_submit is not None:
            traps[graphics_submit] = 'graphicsSubmit'
        if graphics_page is None:
            graphics_page = page
        for address in traps:
            m.call('bp_set', addr=address, type='exec')
        counts = dict(MODULE_CALL=0, MODULE_SCOPE=0, farCopy=0, farCopyBytes=0,
                      graphicsPacketBytes=[])
        for _ in range(5000):
            m.call('emu_run_for', ms=1000)
            state = m.call('cpu_state')
            regs = state['registers']
            pc = regs['pc']['dec']
            active = m.read(t['bankPage'], 1)[0]
            if pc == target and (page is None or active == page) and (target_test is None or target_test(active)):
                break
            op = traps.get(pc)
            if op in ('MODULE_CALL', 'MODULE_SCOPE') and active == 1:
                counts[op] += 1
            elif op == 'farCopy':
                counts[op] += 1
                address = regs['hl']['dec']
                counts['farCopyBytes'] += int.from_bytes(m.read(address+6, 2), 'little')
            elif op == 'graphicsSubmit' and ((graphics_page is None and active > 1) or active == graphics_page):
                counts['graphicsPacketBytes'].append(regs['bc']['dec'])
            if op or pc == target:
                m.call('debug_step', mode='into')
        else:
            raise RuntimeError(('UI trace timeout', hex(target), state, counts))
        items = m.call('bp_list')['breakpoints']
        counts.update({bp['comment']: bp['hitCount'] for bp in items if bp['comment'] in observed})
        m.call('bp_clear')
        return state, counts
