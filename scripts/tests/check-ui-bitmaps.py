"""8080: arbitrary bitmap save/restore, RLE attributes and NC modal overlays."""
from pathlib import Path
import runpy,subprocess

R=Path(__file__).resolve().parents[2];O=R/'.local/ui-bitmap-cpu';O.mkdir(exist_ok=True)
h=runpy.run_path(str(R/'scripts/tests/benchmark-nc-color-scroll.py'))
n=h['symbols'](R/'bin/NC.sym');W=lambda x:int(x).to_bytes(2,'little')
base=bytearray(65536)
for at,name in [(0,'NC.i80'),(0xc000,'BIOS.sys'),(0xf000,'IO.sys')]:
    data=(R/'bin'/name).read_bytes();base[at:at+len(data)]=data


def call(m,entry,hl=0,de=0,bc=0,trace=False):
    m=bytearray(m)
    code=(b'\x21'+W(hl)+b'\x11'+W(de)+b'\x01'+W(bc)+b'\xcd'+W(n[entry])+
          b'\xf5\xe1\x22'+W(0x8300)+b'\xc9')
    m[0x8000:0x8000+len(code)]=code
    return h['execute'](m,0x8000,O,trace=trace)


for col,y,width,height in [(0,0,48,9),(0,1,1,255),(47,255,1,1),(3,80,32,64),(9,13,2,9),
                            (2,7,3,8),(0,0,0,17),(0,0,22,0)]:
    m=bytearray(base)
    for x in range(48):
        for py in range(256):m[0x9000+x*256+py]=(x*31+py*17)&255
    packed=b''.join(m[0x9000+(col+x)*256+y:0x9000+(col+x)*256+y+height] for x in range(width))
    m[0x5000:0x5000+len(packed)+1]=bytes([0xa5])*(len(packed)+1)
    saved=call(m,'uiBitmapSave',hl=0x9000+col*256+y,de=0x5000,bc=width*256+height)[0]
    assert saved[0x5000:0x5000+len(packed)]==packed,(col,y,width,height)
    assert saved[0x5000+len(packed)]==0xa5
    assert saved[0x9000:0xc000]==m[0x9000:0xc000]
    for x in range(width):saved[0x9000+(col+x)*256+y:0x9000+(col+x)*256+y+height]=bytes(height)
    result=call(saved,'uiBitmapRestore',hl=0x9000+col*256+y,de=0x5000,bc=width*256+height)[0]
    assert result[0x9000:0xc000]==m[0x9000:0xc000],(col,y,width,height)
print('Raw bitmap: arbitrary dimensions, bottom/right edge, 8-byte tails and zero dimensions PASS',flush=True)

# Worst-case color changes: one run per byte, every possible 8-bit palette.
for capacity,success in [(4096,True),(4095,False),(1,False)]:
    m=bytearray(base);screen=0x9850;width,height=32,64
    m[0x8100:0x810c]=W(screen)+bytes([width,height])+W(0x5000)+W(0x6800)+W(capacity)+W(0x8200)
    m[0x8200:0x8208]=bytes.fromhex('7c 87 87 87 ad 0e 01 c9') # A=(H*8) xor L, C=1.
    attrs=bytearray([0xf1])*12288
    for x in range(width):
        for y in range(height):
            at=screen+x*256+y;m[at]=(at*19+7)&255;attrs[at-0x9000]=((at>>8)*8&255)^(at&255)
    assert len({attrs[screen-0x9000+x*256+y] for x in range(width) for y in range(height)})==256
    saved=call(m,'uiBitmapSaveColors',hl=0x8100)[0]
    assert bool(saved[0x8300]&1)!=success,(capacity,'carry')
    assert saved[0x9000:0xc000]==m[0x9000:0xc000]
    if not success:continue
    current=bytearray(attrs)
    for x in range(width):
        for y in range(height):
            at=screen+x*256+y;saved[at]=0xaa;current[at-0x9000]=7
    saved[n['IO_COLOR']]=7
    result,_,trace,initial=call(saved,'uiBitmapRestoreColors',hl=0x8100,trace=True)
    assert result[0x9000:0xc000]==m[0x9000:0xc000]
    h['replay_trace'](trace,initial,current,result,attrs)
print('Color bitmap: 256 palettes, exact RLE capacity, guarded overflow and color-register write-only trace PASS',flush=True)


def scene(width,columns,info,active,kept):
    m=bytearray(base);attrs=bytearray([n['COLOR_BORDER']])*12288
    step={4:32,6:48,8:88}[width];cell={4:6,6:10,8:22}[width]
    m[n['ncFontWidth']]=width;m[n['ncColumnStep']]=step;m[n['ncCellBytes']]=cell
    m[n['state.activePanel']]=active;m[n['state.panelA_info']:n['state.panelA_info']+2]=bytes(info)
    m[n['pipHasVolume']]=1;m[n['ncCommandCursorKept']]=kept
    for panel in (0,1):
        m[n['state.panelA_filesCnt']+panel]=120;m[n['state.panelA_curFile']+panel]=9
        for x,pattern in [(24*panel,0x50),(24*panel+23,0x0a)]:
            for y in range(230):m[0x9000+x*256+y]=pattern
        if info[panel]:
            for y in (31,112,160):
                for x in range(24*panel+1,24*panel+23):m[0x9000+x*256+y]=255
            for baseline in (50,60,70,80,90,100,134,144):
                for x in range(24*panel+1,24*panel+22):
                    color=n['COLOR_INFONUMBER'] if x%24<7 else n['COLOR_INFOTEXT']
                    if x%24==7:continue
                    for y in range(baseline-7,baseline+1):m[0x9000+x*256+y]=0x5a;attrs[x*256+y]=color
            continue
        for column in range(columns):
            x=panel*24+1+column*step//4
            for row in range(n['P_FILE_LIST_H']):
                color=[0xf1,0xa1,0xd1,0x71,0xb1][(column+row)%5]
                slot=column*n['P_FILE_LIST_H']+row
                m[n['vars.rowColors']+panel*3*n['P_FILE_LIST_H']+slot]=color
                for bx in range(cell):
                    for py in range(10):
                        at=0x9000+(x+bx)*256+n['SCROLL_TOP']+row*10+py
                        m[at]=0x5a if 1<=py<=8 and (width!=6 or bx<9) else 0
                        attrs[at-0x9000]=color
                if kept and panel==active and slot==9:
                    for bx in range(cell if width!=6 else 9):
                        for py in range(10):
                            at=0x9000+(x+bx)*256+n['SCROLL_TOP']+row*10+py
                            m[at]^=255;attrs[at-0x9000]=n['COLOR_CURSOR']
                    if width==6:
                        for py in range(10):
                            at=0x9000+(x+9)*256+n['SCROLL_TOP']+row*10+py
                            m[at]^=128;attrs[at-0x9000]=(n['COLOR_CURSOR']&240)|1
            if column and width!=8:
                divider=panel*24+column*step//4
                for py in range(7,208):m[0x9000+divider*256+py]=128;attrs[divider*256+py]=n['COLOR_BORDER']
    return m,attrs


cases=0
for font,width,columns in [('Cpm4',4,3),('Compact',6,2),('Fast8',8,1)]:
    for info in ((0,0),(0,1),(1,0)):
        for active in (0,1):
            for window in ('g_window1','ncErrorWindow','g_chooseDrive','ncSortWindow','ncOverwriteWindow','ncProgressWindow'):
                kept=int(info==(0,0) and window=='ncOverwriteWindow')
                m,attrs=scene(width,columns,info,active,kept)
                original=bytes(m[0x9000:0xc000])
                saved=call(m,'ncDialogOpen',hl=n[window])[0]
                assert saved[n['ncDialogActive']]==1,(font,info,active,window,'snapshot fallback')
                assert saved[n['ncDamageMask']]==0
                assert saved[0x9000:0xc000]==original
                request=n['ncDialogRequest'];screen=int.from_bytes(saved[request:request+2],'little')
                w,ht=saved[request+2:request+4];current=bytearray(attrs)
                for x in range(w):
                    for y in range(ht):
                        at=screen+x*256+y;saved[at]=0xaa;current[at-0x9000]=7
                saved[n['ncCurrentColor']]=saved[n['IO_COLOR']]=7
                result,_,trace,initial=call(saved,'ncDialogClose',trace=True)
                assert result[0x9000:0xc000]==original,(font,info,active,window,'bitmap')
                h['replay_trace'](trace,initial,current,result,attrs)
                assert result[n['ncDialogActive']]==0
                cases+=1
    print(font,'file colors, F3 number/label colors, frames and retained cursor restored PASS',flush=True)
print('NC modal snapshots:',cases,'color traces PASS; no color register reads')

# Replacing a modal overlay restores its predecessor before taking the next
# snapshot. The caller never needs a second permanent bitmap buffer.
m,attrs=scene(6,2,(0,1),0,0);original=bytes(m[0x9000:0xc000])
m=call(m,'ncDialogOpen',hl=n['g_window1'])[0]
req=n['ncDialogRequest'];screen=int.from_bytes(m[req:req+2],'little');w,ht=m[req+2:req+4]
current=bytearray(attrs)
for x in range(w):
    for y in range(ht):m[screen+x*256+y]=0xaa;current[screen-0x9000+x*256+y]=7
m[n['ncCurrentColor']]=m[n['IO_COLOR']]=7
m,_,trace,initial=call(m,'ncDialogOpen',hl=n['ncOverwriteWindow'],trace=True)
assert m[0x9000:0xc000]==original
h['replay_trace'](trace,initial,current,m,attrs)
m=call(m,'ncDialogClose')[0];assert m[0x9000:0xc000]==original
assert m[n['ncDialogActive']]==0
print('Replacing input/progress/confirmation overlays restores the original scene PASS')
q,_=scene(6,2,(0,0),0,0);q[n['ncCommandScreen']]=1
fallback=call(q,'ncDialogOpen',hl=n['g_window1'])[0]
assert fallback[n['ncDialogActive']]==0 and fallback[n['ncDamageMask']]==3
assert fallback[0x9000:0xc000]==q[0x9000:0xc000]
print('Console palette safely falls back to panel reconstruction PASS')
