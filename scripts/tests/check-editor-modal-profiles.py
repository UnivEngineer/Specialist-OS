"""8080: MAIN reuses document slack in HEX; BANK restores only the saved rectangle."""
from pathlib import Path
import re,subprocess
from relocated_component import load_graphics
R=Path(__file__).resolve().parents[2];O=R/'.local/editor-modal-check';O.mkdir(parents=True,exist_ok=True)
def symbols(name):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/name).read_text(),re.M)}
gfx,S=load_graphics(0x1b3d);C=symbols('EDITCORE.sym');M=symbols('EDIT.sym');W=lambda n:n.to_bytes(2,'little')
assert 'hexSaveUnderlay' in M and 'uiBitmapSave' in M and 'ebSnapshotModal' not in M
assert M['EDITOR_TEXT_BEGIN']&255==0
# Resident block operations share editing code without using the overlay space.
assert C['EDITOR_MEMORY_END']-C['EDITOR_TEXT_BEGIN']-1>=38_900
assert M['EDITOR_MEMORY_END']-M['EDITOR_TEXT_BEGIN']-1>=14_500
assert 'uiHelpLine' not in C and 'uiHelpLine' in S
assert C['EDIT_MAIN_PIXELS']==C['EDIT_MAIN_PACKET']+C['EDIT_QUEUE_BYTES']
assert C['EDIT_MAIN_CACHE']==C['EDIT_MAIN_END']
assert S['ngFileEnd']-S['entry']==S['IMAGE_MEM_END']
assert 'EDIT_MAIN_CACHE' not in S and 'ngPanelColors' not in S
assert C['EDIT_MAIN_END']<=0x6400 and C['EDIT_MAIN_END']-C['EDIT_MAIN_PIXELS']==32*148
core=(R/'bin/EDITCORE.i80').read_bytes();base=bytearray(65536)
base[S['entry']:S['ngFileEnd']]=gfx
for name,at in [('BIOS.sys',0xc000),('IO.main.bin',0xf000)]:
 b=(R/'bin'/name).read_bytes();base[at:at+len(b)]=b
base[0x9000:0xc000]=bytes((i*37+(i>>8)*19+5)&255 for i in range(12288))
base[C['EDIT_MAIN_END']:0x6400]=bytes([0xa5])*(0x6400-C['EDIT_MAIN_END'])
count=0
def run(m,entry,hl=0,bc=0,de=0,a=0xb1,entries=S):
 global count
 m=bytearray(m[:65536]);code=b'\x31\x00\x8f\x21'+W(hl)+b'\x01'+W(bc)+b'\x11'+W(de)+b'\x3e'+bytes([a])+b'\xcd'+W(entries[entry])+b'\xc3\x40\x8c'
 m[0x8c00:0x8c00+len(code)]=code;(O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8c00','-end','8c40','-counter','10000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True)
 out=(O/'out.bin').read_bytes();assert out[65542:65544]==W(0x8c40),out[65536:].hex()
 assert out[C['EDIT_MAIN_END']:0x6400]==base[C['EDIT_MAIN_END']:0x6400]
 count+=1;return bytearray(out[:65536])
for index,descriptor in enumerate(['uiHelpDescriptor','editorErrorDescriptor','editorFileDescriptor','editorFindDescriptor','editorSaveDescriptor','editorProgressDescriptor']):
 if descriptor=='uiHelpDescriptor':
  address=S[descriptor]
  window=base[address+7:address+12]
 else:
  address=C[descriptor]-0x100
  window=core[address+7:address+12]
 _,y,x,h,w=window;h+=6;w+=2;screen=(x<<8)|y
 assert w*h<=(C['EDIT_MAIN_END']-C['EDIT_MAIN_PIXELS']) and 13<=y and y+h<=243,(descriptor,window)
 m=bytearray(base);palette=0x31 if index&1 else 0xb1
 m=run(m,'uiBitmapSave',hl=screen,bc=(w<<8)|h,de=C['EDIT_MAIN_PIXELS'])
 packed=b''.join(base[screen+i*256:screen+i*256+h] for i in range(w))
 assert m[C['EDIT_MAIN_PIXELS']:C['EDIT_MAIN_PIXELS']+w*h]==packed
 for i in range(w):m[screen+i*256:screen+i*256+h]=bytes([0x77])*h
 # An outside marker proves restore does not repaint the full document.
 m[0x9000]^=0x33;want=bytearray(base[0x9000:0xc000]);want[0]^=0x33
 m[S['IO_COLOR']]=palette
 m=run(m,'uiBitmapRestore',hl=screen,bc=(w<<8)|h,de=C['EDIT_MAIN_PIXELS'])
 assert m[0x9000:0xc000]==want,descriptor
 assert m[S['IO_COLOR']]==palette

# CORE must keep the document palette after issuing its bitmap restore.
# Replace only the transport calls so the model can observe the queued palette.
m=bytearray(base)
m[0x100:0x100+len(core)]=core
m[C['ebColor']]=0x07
m[C['ebSnapshotActive']]=1
latched=0x8c80
m[C['ebQueueCommand']:C['ebQueueCommand']+7]=b'\x3a'+W(C['ebColor'])+b'\x32'+W(latched)+b'\xc9'
m[C['ebFlush']]=0xc9
m=run(m,'ebRestoreModal',entries=C)
assert not m[C['ebSnapshotActive']]
assert m[C['ebQOperation']]==C['GFX_BITMAP_RESTORE']
assert m[latched]==m[C['ebColor']]==C['COLOR_TEXT']
print('EDITOR MODAL PROFILES PASS',count)
# Check each SDK orientation, extreme ranges, both edges and short tracks.
cases=[(97,0,0,0),(97,4,9,7),(97,33,9,0),(97,33,9,12),(97,33,9,24),
       (97,65535,1,0),(97,65535,1,32000),(97,65535,1,65535),(5,65535,0,65535)]
for mode in range(4):
 for length,total,visible,position in cases:
  x,y,thickness=(16,41,8) if mode>=2 else (19,41,5)
  if mode==2:length=min(length,43)
  descriptor=W(x)+bytes((y,length,thickness,mode))+W(total)+W(visible)+W(position)
  m=bytearray(base);m[0x800:0x800+len(descriptor)]=descriptor
  wanted=bytearray(m[0x9000:0xc000])
  span=max(0,total-visible)
  unit=8 if mode==2 else 1
  thumb=length if not span else min(length,max(8//unit,length*visible//total))
  first=0 if not span else (length-thumb)*min(position,span)//span
  for along in range(length*unit):
   for across in range(thickness):
    px,py=(x+along,y+across) if mode in (1,2) else (x+across,y+along)
    cell=(px//8)*256+py;bit=0x80>>(px&7)
    if mode>=2:
     pixel=across==4 or (across!=0 and first*unit<=along<(first+thumb)*unit)
    else:
     pixel=first<=along<first+thumb or 0x55&bit
    if pixel:wanted[cell]|=bit
    else:wanted[cell]&=~bit
  actual=run(m,'uiDrawScrollbar',hl=0x800)
  assert actual[0x9000:0xc000]==wanted,(mode,length,total,visible,position)
print('SDK SCROLLBAR PASS',4*len(cases))
