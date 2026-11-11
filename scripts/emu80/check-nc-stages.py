"""Полное обновление NC: фон обеих панелей, рамки, метаданные, списки."""
from pathlib import Path
import runpy,re,json
R=Path(__file__).resolve().parents[2];O=R/'docs/nc/stages-2026-11-09';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001')
x=h['Matrix']();m=x.m;n={a:int(b,16) for a,b in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text('utf-8'),re.M)};rows=[]
def pixels():return b''.join(m.read(0x9000+c*256,230) for c in range(48))
def ready():m.call('bp_clear');m.call('emu_run_for',ms=500)
def frame(info):
 data=[bytearray(230) for _ in range(24)]
 for col in range(24):
  a,mid,z=([0x7f,0x40,0x5f],0x50,[0x5f,0x40,0x7f]) if col==0 else ([0xfe,2,0xfa],0x0a,[0xfa,2,0xfe]) if col==23 else ([255,0,255],0,[255,0,255])
  data[col][4:7]=bytes(a);data[col][7:227]=bytes([mid])*220;data[col][227:230]=bytes(z)
 for y in ((31,136) if info else (208,)):
  data[0][y]|=15;data[23][y]|=240
  for c in range(1,23):data[c][y]=255
 if not info:data[12][7:208]=bytes([128])*201
 return b''.join(data)
def check(label):
 active=m.read(n['state.activePanel'],1)
 modes=[m.read(n['vars.panelA_info']+i,1)[0] for i in range(2)]
 # Обычный диалог F7 и Escape должны войти в общий полный путь.
 ready();m.tap('f7');m.stop_at(n['fullClearPanels']);m.tap('esc');m.until(n['fullClearPanels'],40)
 m.stop_at(n['fullDrawFrames']);m.until(n['fullDrawFrames'],40)
 assert pixels()==bytes(48*230),'background is not blank/blue'
 m.screenshot(O/(label+'-1-background.png'))
 m.stop_at(n['fullDrawMetadata']);m.until(n['fullDrawMetadata'],40)
 expected=frame(modes[0])+frame(modes[1])
 actual=pixels()
 assert actual==expected,('frame mismatch',[(i//230,i%230,a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:12])
 m.screenshot(O/(label+'-2-frames.png'))
 m.stop_at(n['fullDrawFiles']);m.until(n['fullDrawFiles'],40)
 for panel,info in enumerate(modes):
  if not info:
   for col in (*range(1,11),*range(13,23)):
    assert m.read(0x9000+(col+24*panel)*256+14,190)==bytes(190),'files drawn before metadata completed'
 m.screenshot(O/(label+'-3-metadata.png'))
 m.stop_at(n['mainLoop']);m.until(n['mainLoop'],60)
 assert m.read(n['state.activePanel'],1)==active
 m.screenshot(O/(label+'-4-files.png'));rows.append(dict(case=label,active=active[0],info=modes,passed=True));print('PASS',label,flush=True)
try:
 x.start('NC drawing stages')
 check('two-files')
 ready();m.tap('f3');m.call('emu_run_for',ms=1000)
 check('right-info')
 ready();m.tap('f3');m.call('emu_run_for',ms=500);m.tap('tab');m.tap('f3');m.call('emu_run_for',ms=1000)
 check('left-info-active-right')
 (O/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
finally:x.stop()
