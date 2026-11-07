"""Встроенный блочный RAMD, сервисный байтовый ABI и устанавливаемые CP/M и R4."""
from pathlib import Path
import argparse,json,re,runpy

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime',type=Path,default=ROOT/'.local/monitor2-001')
parser.add_argument('--report',type=Path,default=ROOT/'docs/drivers/ramd-modes-2026-11-05')
parser.add_argument('--compare-r4',action='store_true')
args=parser.parse_args();args.report.mkdir(parents=True,exist_ok=True)
helpers=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
globals_=helpers['Matrix'].__init__.__globals__
globals_['OUT']=args.report;globals_['RUNTIME']=args.runtime

def symbols(name):
 return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/f'bin/{name}.sym').read_text(),re.M)}

d,n,r=[symbols(name) for name in ['DOS','NC','RAMDISK']]
driverLabel='CPM16' if r.get('RAMD_USE_CPM',0) else 'R4'
Base=globals_['Bench'];stages=[];cold=True
targets={d[name]:name for name in ['reboot3_1','fileExec','skipFormat','reboot3_2']}

class Profile(Base):
 def stop_at(self,pc):
  super().stop_at(pc)
  if cold:
   for address in targets:self.call('bp_set',addr=address,type='exec')
 def call(self,name,**arguments):
  result=super().call(name,**arguments)
  if cold and name=='emu_run_for':
   state=super().call('cpu_state');pc=state['registers']['pc']['dec']
   if pc in targets or pc==n['mainLoop']:
    row=dict(stage=targets.get(pc,'nc-main-loop'),ms=state['cpu_ticks']/2000)
    if pc==d['fileExec']:
     ptr=state['registers']['hl']['dec']
     row['command']=self.read(ptr,min(128,65536-ptr)).split(b'\0')[0].split(b'\r')[0].decode('ascii','replace')
     if row['command'].startswith('A:\\FORMAT'):
      assert self.read(0xFFC0,32)==self.read(d['driverBlockStart'],32)
      row['window']='builtin-block'
    stages.append(row)
  return result

globals_['Bench']=Profile;m=helpers['Matrix']()
report={'status':'emulator-only','cpuHz':2000000,'stages':stages,'checks':[],'sectorMs':{}}

def select(mode,hl=0x5678):
 state=m.func(d['bios_setRAMDDriver'],af=0x5AD7,bc=(mode<<8)|0xA7,de=0x1234,hl=hl)
 for key,value in dict(af=0x5AD7,bc=(mode<<8)|0xA7,de=0x1234,hl=hl,sp=0x8FDF).items():
  assert state['registers'][key]['dec']==value,(mode,key,state)

def window(label):assert m.m.read(0xFFC0,32)==m.m.read(label,32)

def io(label):
 for sector in [0xFE,0x100,0xFFE]:
  for buffer in [0x4001,0x40FD,0xFE00]:
   original=m.m.read(buffer-1,258)
   payload=bytes((i*37+sector+buffer)&255 for i in range(256))
   guard=original[-1:] if buffer==0xFE00 else b'\xAD'
   m.m.write(buffer-1,b'\xDA'+payload+guard)
   for mode in [1,2]:
    if mode==2:m.m.write(buffer,bytes(256))
    ticks=m.m.call('cpu_state')['cpu_ticks']
    state=m.func(d['bios_diskDriver'],af=0x5A02,bc=(mode<<8)|0xA7,de=sector,hl=buffer)
    elapsed=state['cpu_ticks']-ticks
    assert not state['flags']['C'],(label,sector,buffer,state)
    assert state['registers']['af']['dec']>>8==0x5A
    for key,value in dict(bc=(mode<<8)|0xA7,de=sector,hl=buffer,sp=0x8FDF).items():
     assert state['registers'][key]['dec']==value,(label,key,state)
    assert m.m.read(buffer-1,258)==b'\xDA'+payload+guard,(label,sector,buffer)
    if sector==0x100 and buffer==0x4001:report['sectorMs'].setdefault(label,{})[str(mode)]=elapsed/2000
   m.m.write(buffer-1,original)
 for sector,buffer in [(0xFF,0x4001),(0xFFF,0x4001),(0x1000,0x4001),(0x100,0xFE01)]:
  for mode in [1,2]:assert m.func(d['bios_diskDriver'],bc=mode<<8,de=sector,hl=buffer)['flags']['C'],(label,sector,buffer)
 report['checks'].append(label+': read/write, page boundaries, unaligned/high buffers, guards, registers, invalid ranges')
 print(label,'I/O PASS',flush=True)

try:
 m.start('ramd-modes',disk=ROOT/'bin/nested-Games2M.bin');cold=False
 assert any(row.get('window')=='builtin-block' for row in stages)
 user=r['driverBufferedStart']
 select(d['RAMD_DRIVER_USER']);window(user)
 select(d['RAMD_DRIVER_BYTE']);window(d['driverByteStart'])
 saved=m.func(d['bios_RAMDRead'],af=0x0200,hl=0x1234)['registers']['bc']['dec']&255
 m.func(d['bios_RAMDWrite'],af=0x0200,bc=0x5A,hl=0x1234)
 assert m.func(d['bios_RAMDRead'],af=0x0200,hl=0x1234)['registers']['bc']['dec']&255==0x5A
 m.func(d['bios_RAMDWrite'],af=0x0200,bc=saved,hl=0x1234)
 select(d['RAMD_DRIVER_BLOCK']);window(d['driverBlockStart'])
 select(d['RAMD_DRIVER_RESTORE']);window(user)
 report['checks'].append('byte access and explicit block selection preserve registered user driver; all selector registers/flags preserved')
 # Для испытания встроенного физического пути заменяем только регистрацию B:.
 assert not m.func(d['bios_installDriver'],af=0x0100,hl=d['diskDriver'])['flags']['C']
 select(d['RAMD_DRIVER_BLOCK']);m.select(1);io('R3')
 # REGISTER не устанавливает окно, USER/RESTORE применяют новый шаблон.
 select(d['RAMD_DRIVER_REGISTER'],d['driverBlockStart']);window(d['driverBlockStart'])
 select(d['RAMD_DRIVER_BYTE']);window(d['driverByteStart'])
 select(d['RAMD_DRIVER_USER']);window(d['driverBlockStart'])
 select(d['RAMD_DRIVER_REGISTER'],user);window(d['driverBlockStart'])
 select(d['RAMD_DRIVER_RESTORE']);window(user)
 assert not m.func(d['bios_installDriver'],af=0x0100,hl=r['Driver'])['flags']['C']
 m.select(1);io(driverLabel)
 if args.compare_r4:
  r4=symbols('RAMDISK-R4')
  m.m.write(0,(ROOT/'bin/RAMDISK-R4.i80').read_bytes())
  assert not m.func(r4['Install'])['flags']['C']
  m.select(1);io('R4')
 report['checks'].append('REGISTER leaves current window intact; USER applies new template; user registration restored')
 # Аппаратный сброс сохраняет RAM, но заново устанавливает начальный шаблон BIOS.
 sentinel=bytes(range(256));m.m.write(0x4001,sentinel)
 assert not m.func(d['bios_diskDriver'],bc=0x0100,de=0x100,hl=0x4001)['flags']['C']
 m.m.call('bp_clear');m.m.call('emu_reset');m.m.stop_at(d['reboot3_1']);m.m.until(d['reboot3_1'],30)
 assert int.from_bytes(m.m.read(d['setRAMDBlockTemplate']+1,2),'little')==d['driverBlockStart']
 m.m.stop_at(d['skipFormat']);m.m.call('bp_set',addr=d['fileExec'],type='exec');m.m.until(d['skipFormat'],30)
 # До skipFormat не должен запускаться FORMAT.BAT: метка прошлого форматирования сохранена.
 m.m.stop_at(n['mainLoop']);m.m.until(n['mainLoop'],60)
 select(d['RAMD_DRIVER_USER']);window(user);m.select(1)
 m.m.write(0x4001,bytes(256))
 assert not m.func(d['bios_diskDriver'],bc=0x0200,de=0x100,hl=0x4001)['flags']['C']
 assert m.m.read(0x4001,256)==sentinel
 report['checks'].append('hardware reset restores BIOS default block template, skips format on existing RAM disk, AUTOEXEC reinstalls configured driver, data retained')
 (args.report/'result.json').write_text(json.dumps(report,indent=2)+'\n')
 print('RAMD MODES PASS',len(report['checks']),flush=True)
finally:m.stop()
