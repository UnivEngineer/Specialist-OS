"""Стоимость DOSMOD, разрешения команды и FORMAT в собственном Emu80 (2 МГц)."""
from pathlib import Path
import runpy,json
R=Path(__file__).resolve().parents[2]
O=R/'docs/cli/cli-fixes-2026-11-05/performance';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
g=h['Matrix'].__init__.__globals__;g['OUT']=O;g['RUNTIME']=R/'.local/monitor2-001'
m=h['Matrix']();s=h['symbols'](R/'bin/DOS.sym');rows={}
def timed(name,entry,**regs):
    before=m.m.call('cpu_state')['cpu_ticks']
    state=m.func(entry,**regs)
    ticks=state['cpu_ticks']-before
    assert not state['flags']['C'],(name,state,m.m.read(s['v_dosError'],1).hex())
    rows[name]={'ticks':ticks,'ms':ticks/2000}
    print(name,rows[name],flush=True)
    return state
try:
    m.start('cli-cost',disk=R/'bin/nested-Games2M.bin')
    m.m.call('debug_break');m.m.call('bp_clear')
    # Проверка ABI, не проверка пользовательского ввода: PC/SP задаёт обвязка.
    m.select(0)
    timed('module-already-loaded',s['bios_loadDosModule'])
    timed('module-forced-reload',s['dosModuleLoad'])
    m.m.write(s['v_shell'],b'\x01')
    m.select(1)
    cmd=b'A:\\FORMAT.COM B: -R -Y\0'
    m.m.write(0x5000,cmd)
    timed('format-with-cli-resolver',s['bios_fileExec'],hl=0x5000)
    # Явное имя, старый прямой вход: та же программа, без PATH/ассоциаций.
    m.select(1);m.m.write(0x5000,cmd)
    timed('format-direct-native-loader',s['fileExecExternal'],hl=0x5000)
    m.select(1);m.m.write(0x5000,b'B: -R -Y\r\0')
    timed('format-body-only',0,de=0x5000)
    # Прежний хвост FORMAT возвращался сразу после записи корня, без mount.
    binary=(R/'bin/FORMAT.i80').read_bytes()
    tail=b'\xCD'+int(s['bios_mountVolume']).to_bytes(2,'little')+b'\xD8\x11\0\0'
    at=binary.index(tail);m.m.write(at,b'\xC9')
    m.select(1);m.m.write(0x5000,b'B: -R -Y\r\0')
    timed('format-body-with-old-return',0,de=0x5000)
    m.m.write(at,b'\xCD')
    timed('module-already-loaded-after-format',s['bios_loadDosModule'])
    (O/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
finally:
    m.stop()
