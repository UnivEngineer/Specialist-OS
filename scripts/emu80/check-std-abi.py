"""Стек и общие ABI в настоящем STD-отображении отдельного Emu80.

Прямой вызов ABI; ввод через NC этим тестом не проверяется.
Экземпляр запускает и закрывает вызывающий сценарий, runtime должен быть своим.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
Machine = runpy.run_path(str(ROOT/'scripts/emu80/check-flash.py'))['Machine']
S = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', type=Path, required=True)
    p.add_argument('--report-directory', type=Path, default=ROOT/'docs/dos/std-002')
    args = p.parse_args()
    runtime = args.runtime.resolve()
    query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    owner = subprocess.check_output(['powershell.exe','-NoProfile','-Command',query],text=True).strip()
    assert Path(owner).resolve() == runtime/'Emu80qt-mcp.exe'
    m = Machine('http://127.0.0.1:19781/mcp')
    m.call('bp_clear')
    m.call('emu_run_for',ms=5000)
    m.call('debug_break')
    assert m.read(0,32) == (ROOT/'bin/NC.i80').read_bytes()[:32]
    sp = m.call('cpu_state')['registers']['sp']['dec']
    assert S['STACK_BOTTOM'] <= sp < S['STACK_ADDR'],sp
    report = dict(nc_sp=sp,stack_bottom=S['STACK_BOTTOM'],stack_top=S['STACK_ADDR'])
    font = m.read(S['FONT_ADDR'],2048)
    cmd = bytes((i*17+5)&255 for i in range(S['CMD_LINE_SIZE']))
    source = bytes((i*37+11)&255 for i in range(65))

    def word(n):return n.to_bytes(2,'little')
    def call(label):return b'\xCD'+word(S[label])

    def execute(std_mode=None):
        m.call('bp_clear')
        m.write(0x9000,bytes(0x3000))
        m.write(S['CMD_LINE'],cmd)
        m.write(S['STACK_BOTTOM'],bytes([0xA5])*(S['STACK_ADDR']-S['STACK_BOTTOM']))
        m.write(0x8500,source)
        m.write(0x8600,bytes(80))
        m.write(0x8400,b'STD ABI: PRINTCHAR / MEMCPY\0')
        m.write(0x8800,bytes.fromhex('05 C4 00 88 C9'))  # 64 вложенных CALL/RET.
        m.write(S['bios_vars.cursorY'],bytes((40,4)))
        m.write(S['bios_vars.inverse'],bytes(2))
        m.write(S['bios_vars.escMode'],bytes([255]))
        code = b''
        if std_mode is not None:
            code += bytes((0x3E,std_mode,0x32))+word(S['IO_COLOR'])+b'\x32'+word(S['IO_PAGE_STD'])
        code += bytes.fromhex('06 40 CD 00 88')
        code += b'\x21\x00\x84'+call('bios_printString')
        code += b'\x0E!'+call('bios_printChar')
        for start,end,dest in [(0x8500,0x8541,0x8600),(0x8600,0x8641,0x8601),(0x8601,0x8642,0x8600)]:
            code += b'\x21'+word(start)+b'\x11'+word(end)+b'\x01'+word(dest)+call('bios_memcpy_bc_hl')
        code += b'\x21\x28\x91\x01\x0A\x0A'+call('bios_uiInverseRect')
        code += b'\x21\x28\x91\x01\x0A\x0A'+call('bios_uiInverseRect')
        code += bytes.fromhex('C3 00 83')
        assert len(code)<256
        m.write(0x8200,code)
        m.call('bp_set',addr=0x8300,type='exec')
        m.call('regs_set',pc=0x8200,sp=S['STACK_ADDR'])
        m.call('emu_run_for',ms=500)
        state = m.call('cpu_state')
        assert state['registers']['pc']['dec']==0x8300,state
        assert state['registers']['sp']['dec']==S['STACK_ADDR'],state
        assert m.read(0x8600,65)==source
        assert m.read(S['CMD_LINE'],len(cmd))==cmd
        assert m.read(S['FONT_ADDR'],2048)==font
        stack = m.read(S['STACK_BOTTOM'],S['STACK_ADDR']-S['STACK_BOTTOM'])
        used = len(stack)-next(i for i,b in enumerate(stack) if b!=0xA5)
        assert stack[:128]==bytes([0xA5])*128
        return m.read(0x9000,0x3000),used

    def leave_std():
        # Выход через нижнее зеркало регистра режима, а не через ВВ55 FFFCh.
        m.write(0x8200,bytes.fromhex('3E 00 32 FC F7 C3 00 83'))
        m.call('regs_set',pc=0x8200,sp=S['STACK_ADDR'])
        m.call('emu_run_for',ms=500)
        assert m.call('cpu_state')['registers']['pc']['dec']==0x8300

    try:
        marker_low,marker_high=bytes.fromhex('25 D9 78 B1'),bytes.fromhex('A6 51 39 E8')
        m.write(0xF100,marker_low)
        m.write(0xF800,marker_high)
        reference,mx_used = execute()
        expected_core=m.read(0xC000,len((ROOT/'bin/DOS.sys').read_bytes()))
        modes=[]
        for std_mode in [3,2]:  # Нижние 32 КБ: RAM, затем ROM.
            actual,std_used = execute(std_mode)
            assert actual==reference and any(actual)
            # Пока CPU в STD, код, данные ядра и шрифт доступны целиком.
            assert m.read(0xC000,len(expected_core))==expected_core
            assert m.read(0xF800,4)==m.read(0xF820,4)==m.read(0xFFFC,4)
            assert m.read(0xF100,4)!=marker_low and m.read(0xF800,4)!=marker_high
            modes.append(dict(lower='RAM' if std_mode==3 else 'ROM',stack_used=std_used))
            leave_std()
            assert m.read(0xF100,4)==marker_low and m.read(0xF800,4)==marker_high
        report.update(std_screen_equals_mx=True,screen_sha256=hashlib.sha256(actual).hexdigest(),
                      command_preserved=True,font_preserved=True,memcpy_overlap_both_directions=True,
                      nested_call_depth=64,mx_stack_used=mx_used,std_modes=modes,
                      std_keyboard_mirrors_equal=True,upper_4k_ram_hidden=True)
        report['returned_to_mx']=True
    finally:
        m.call('bp_clear')
        m.call('emu_reset')
        m.call('emu_run_for',ms=5000)
    assert m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]
    args.report_directory.mkdir(parents=True,exist_ok=True)
    (args.report_directory/'emulator.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
