"""8080 alignment geometry and modal drawing bounds, including padding."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2];O=R/'.local/ui-alignment';O.mkdir(exist_ok=True)
h=runpy.run_path(str(R/'scripts/tests/benchmark-nc-color-scroll.py'));n=h['symbols'](R/'bin/NC.sym');W=lambda v:v.to_bytes(2,'little')
base=bytearray(65536);data=(R/'bin/NC.i80').read_bytes();base[:len(data)]=data
count=0
for width in (4,6,8):
 for length in (0,1,13,30,61):
  for align in (0,1,2):
   m=bytearray(base);m[0x8100:0x8100+length+1]=b'A'*length+b'\0'
   stub=b'\x21'+W(0x8100)+b'\x01'+W(36*256+120)+b'\x11'+W(width*256+111)+b'\x3e'+bytes([align])+b'\xcd'+W(n['uiAlignText'])+b'\xf5\xe1\x22'+W(0x8300)+b'\xeb\x22'+W(0x8302)+b'\xc9'
   m[0x8000:0x8000+len(stub)]=stub
   out=h['execute'](m,0x8000,O)[0];fits=length*width//2<=120
   assert bool(out[0x8300]&1)==(not fits),(width,length,align)
   gap=max(0,120-length*width//2);delta=0 if align==0 or not fits else gap//2 if align==1 else gap
   assert int.from_bytes(out[0x8302:0x8304],'little')==(36+delta)*256+111,(width,length,align,out[0x8302:0x8304])
   count+=1
for modal in (False,True):
 m=bytearray(base);m[0x9000:0xc000]=bytes([0xa5])*12288
 m[0x8100:0x8106]=bytes([0x92 if modal else 0x82,88,0x98,74,30,0])
 stub=b'\x21'+W(0x8100)+b'\xaf\xcd'+W(n['uiDraw'])+b'\xc9';m[0x8000:0x8000+len(stub)]=stub
 out=h['execute'](m,0x8000,O)[0]
 for col in range(48):
  for y in range(256):
   at=0x9000+col*256+y
   if not (8<=col<40 and 88<=y<168):assert out[at]==0xa5,('overdraw',modal,col,y)
   elif modal and y in (88,89,166,167):assert out[at]==0,('padding',col,y)
print('UI alignment PASS',count,'cases; plain/modal exact bounds and 2-pixel vertical padding')

# Draw/focus share one widget API. Stub only the BIOS text/cursor callbacks;
# real text layout is exercised by the visible emulator tests.
m=bytearray(base);m[0x9000:0xc000]=bytes(12288)
m[n['bios_setCursorPos']]=0xc9;m[n['bios_printStringReal']]=0xc9
m[0x8100:0x810a]=W(0x8180)+bytes([36,56,130,14,6,1,7,3]);m[0x8180:0x8188]=b'Yes [Y]\0'
def widget(memory,name,a=0):
 stub=b'\x21'+W(0x8100)+b'\x3e'+bytes([a])+b'\xcd'+W(n[name])+b'\xc9'
 memory[0x8000:0x8000+len(stub)]=stub
 return h['execute'](memory,0x8000,O)[0]
selected=widget(m,'uiDrawButton');plain=widget(bytearray(selected),'uiSelectButton',0)
again=widget(bytearray(plain),'uiSelectButton',0);back=widget(bytearray(plain),'uiSelectButton',1)
assert selected[0x9000:0xc000]==back[0x9000:0xc000]
assert plain[0x9000:0xc000]==again[0x9000:0xc000]
assert selected[0x9000:0xc000]==plain[0x9000:0xc000], 'focus changed bitmap'
assert plain[0x8107]==0 and back[0x8107]==1
print('UI button PASS: opaque frame, focus without bitmap inversion, idempotent selection and exact restore')
