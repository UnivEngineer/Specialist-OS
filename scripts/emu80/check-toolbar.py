"""Тулбар NC: единая высота цветных полей и переключение Shift через клавиатуру."""
from pathlib import Path
from PIL import Image
import runpy,re,json

R=Path(__file__).resolve().parents[2]
O=R/'docs/nc/toolbar-2026-11-05/ui';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
g=h['Matrix'].__init__.__globals__;g['OUT']=O;g['RUNTIME']=R/'.local/monitor2-001'
m=h['Matrix']()
n={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text(),re.M)}
nc=(R/'bin/NC.i80').read_bytes()
def fields(label):
    at=n[label];x=0;out=[]
    while nc[at]:
        flag=nc[at];at+=1;end=nc.index(0,at);text=nc[at:end].decode();at=end+1
        if flag&0x80:out.append((text,x))
        x+=(flag&7)*8
    assert x==384
    return out
def bitmap():return b''.join(m.m.read(0x9000+x*256+247,9) for x in range(48))
def capture(name,label):
    path=O/(name+'.png');m.m.screenshot(path)
    im=Image.open(path).convert('RGB');assert im.size==(384,256)
    color=im.getpixel((16,247))
    assert color!=(0,0,0)
    for text,x in fields(label):
        assert all(im.getpixel((x,y))==color for y in range(247,256)),(text,x)
        assert im.getpixel((x,246))!=(color),('cell extends above toolbar',text)
    return bitmap()
try:
    m.start('toolbar',disk=R/'bin/nested-Games2M.bin')
    normal=capture('normal','aF1LeftF2RighF3');shifted=None
    panels=b''.join(m.m.read(0x9000+x*256,230) for x in range(48))
    for i in range(3):
        m.m.call('kbd_send',keys=['shift'],action='press');m.m.call('emu_run_for',ms=500)
        assert m.m.read(n['ncShiftState'],1)==b'\0'
        got=capture('shift-'+str(i),'aShiftToolbar')
        if shifted is None:shifted=got
        assert got==shifted and got!=normal
        m.m.call('kbd_send',keys=['shift'],action='release');m.m.call('emu_run_for',ms=500)
        assert capture('released-'+str(i),'aF1LeftF2RighF3')==normal
        assert b''.join(m.m.read(0x9000+x*256,230) for x in range(48))==panels
    (O/'result.json').write_text(json.dumps(dict(height=9,top=247,bottom=255,shiftCycles=3,stableBitmaps=True,panelsUnchanged=True),indent=2)+'\n')
    print('Toolbar Emu80 PASS: all label cells 9 pixels; three Shift cycles; stable panels',flush=True)
except Exception:
    if m.pid:m.m.screenshot(O/'failure.png')
    raise
finally:m.stop()
