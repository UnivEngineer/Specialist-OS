"""8080 checks for NC runtime font geometry, bounded fields and lazy toolbar."""
from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[2]; O=R/'.local/nc-layout-cpu'; O.mkdir(exist_ok=True)
def sy(p): return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(encoding='utf-8'),re.M)}
N=sy(R/'bin/NC.sym'); S=sy(R/'bin/BIOS.sym'); W=lambda n:n.to_bytes(2,'little')
def call(m,entry,a=0,bc=0,de=0,hl=0):
 global LAST_CYCLES
 m=bytearray(m)
 code=b'\x31'+W(0x8b00)+b'\x3e'+bytes([a])+b'\x01'+W(bc)+b'\x11'+W(de)+b'\x21'+W(hl)+b'\xcd'+W(N.get(entry,S.get(entry)))+b'\xc3'+W(0x81ff)
 m[0x8100:0x8100+len(code)]=code;(O/'in.bin').write_bytes(m)
 p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8100','-end','81ff','-counter','30000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True,timeout=20)
 b=(O/'out.bin').read_bytes();assert b[65542:65544]==W(0x81ff),(entry,p.stdout,b[65542:65544].hex())
 LAST_CYCLES=int(p.stdout.strip().splitlines()[-1])
 return bytearray(b[:65536])
def seed(name):
 m=bytearray(65536)
 for at,file in [(0,'bin/NC.i80'),(0xc000,'bin/BIOS.sys'),(0xd800,f'bin/fonts/{name}.i80'),(0xf000,'bin/IO.sys')]:
  data=(R/file).read_bytes();m[at:at+len(data)]=data
 m[S['bios_vars']:S['bios_vars']+S['BIOS_VARIABLES']]=m[S['initVars']:S['initVarsEnd']]
 m[S['bios_upper.escMode']]=255
 m=call(m,'FONT_ENGINE_INIT');m=call(m,'ncLayoutInit')
 return m
for name,width,cols in [('Cpm4',4,3),('Compact',6,2),('Fast8',8,1)]:
 m=seed(name); assert m[N['ncFontWidth']]==width and m[N['ncColumns']]==cols
 # Compact names are plain ASCIIZ, including the delete-dialog source.
 for record,leaf in [(b'BENCH      ',b'BENCH'),(b'FILE    TXT',b'FILE.TXT'),
                     (b'12345678ABC',b'12345678.ABC'),(b'..         ',b'..')]:
  q=bytearray(m);q[0x8500:0x8510]=record+bytes(5)
  q[0x8600:0x8620]=bytes([0xa5])*32
  result=call(q,'compactName',hl=0x8500,de=0x8600)
  assert result[0x8600:0x8620]==leaf+b'\0'+bytes([0xa5])*(31-len(leaf)),(name,leaf,'compact terminator')
  result=call(q,'printInvFile',de=0x8500)
  assert result[N['vars.input']:N['vars.input']+len(leaf)+1]==leaf+b'\0',(name,leaf,'delete dialog name')
  expected=bytearray(q);expected[0x8600:0x8600+len(leaf)+1]=leaf+b'\0'
  expected[N['bios_vars.cursorY']:N['bios_vars.cursorY']+2]=W(0x3c6e)
  expected=call(expected,'printStringInv',hl=0x8600)
  assert result[0x9000:0xc000]==expected[0x9000:0xc000],(name,leaf,'delete dialog bitmap')
 if width==8:
  # RAW8 padding follows the installed glyphs, not the historical font shape.
  for bitmap,padding in [(0xfe,(0,1)),(0x7f,(1,0)),(0xff,(0,0))]:
   q=bytearray(m)
   for row in range(8):
    for char in range(ord('A'),ord('H')+1):q[N['FONT_ADDR']+row*256+char]=bitmap
   q=call(q,'ncLayoutInit')
   assert (q[N['ncDrivePadLeft']],q[N['ncDrivePadRight']])==padding
 # Raw glyph copy: 200px FULL_PANELS height, overlap, identical ranges, no-op.
 if width==4:
  # Byte rectangles keep their direct, unmasked fill path for large windows.
  for col,y,w,h in [(0,0,48,1),(47,255,1,1),(1,13,3,12),(1,13,1,12),
                    (1,13,3,2),(1,13,3,0),(1,13,0,12)]:
   for pattern in (0,255,0xaa):
    q=bytearray(m);q[0x9000:0xc000]=bytes([0x55])*12288
    expected=bytearray(q[0x9000:0xc000])
    for bx in range(col,col+w):
     for py in range(y,y+h):expected[bx*256+py]=pattern
    result=call(q,'uiFillRectBytes',a=pattern,hl=(0x90+col)*256+y,bc=w*256+h)
    assert result[0x9000:0xc000]==expected,('byte fill',col,y,w,h,pattern)
    expected=bytearray(q[0x9000:0xc000])
    for px in range(col*8,(col+w)*8):
     for py in range(y,y+h):
      if px in (col*8,(col+w)*8-1) or py in (y,y+h-1):
       bit=128>>(px%8);at=px//8*256+py
       expected[at]=(expected[at]&~bit)|(pattern&bit)
    result=call(q,'uiFrameRectBytes',a=pattern,hl=(0x90+col)*256+y,bc=w*256+h)
    assert result[0x9000:0xc000]==expected,('byte outline',col,y,w,h,pattern)
  # Pixel fills/inversion preserve neighbouring bits at every byte phase,
  # including X>=256, the right/bottom edges, and empty rectangles.
  cases=[(x,13,w,3) for x in range(8) for w in (1,7,8,9,16,25)]
  cases += [(255,254,9,2),(383,255,1,1),(0,0,384,1),(0,255,384,1),
            (0,0,0,7),(383,255,1,0)]
  for x,y,w,h in cases:
   for pattern in (0,255,0xaa):
    q=bytearray(m);q[0x9000:0xc000]=bytes([0x55])*12288
    expected=bytearray(q[0x9000:0xc000])
    for px in range(x,x+w):
     bit=128>>(px%8)
     for py in range(y,y+h):
      at=px//8*256+py
      expected[at]=(expected[at]&~bit)|(pattern&bit)
    result=call(q,'uiFillRectPixels',a=pattern,hl=x,bc=w,de=y*256+h)
    assert result[0x9000:0xc000]==expected,('pixel fill',x,y,w,h,pattern)
   expected=bytearray(q[0x9000:0xc000])
   for px in range(x,x+w):
    for py in range(y,y+h):expected[px//8*256+py]^=128>>(px%8)
   result=call(q,'uiInverseRectPixels',hl=x,bc=w,de=y*256+h)
   assert result[0x9000:0xc000]==expected,('pixel inverse',x,y,w,h)
   assert call(result,'uiInverseRectPixels',hl=x,bc=w,de=y*256+h)[0x9000:0xc000]==q[0x9000:0xc000]
   expected=bytearray(q[0x9000:0xc000])
   for px in range(x,x+w):
    for py in range(y,y+h):
     if px in (x,x+w-1) or py in (y,y+h-1):expected[px//8*256+py]|=128>>(px%8)
   result=call(q,'uiFrameRectPixels',a=255,hl=x,bc=w,de=y*256+h)
   assert result[0x9000:0xc000]==expected,('pixel outline',x,y,w,h)
  for src,dst,cell,height in [(0x910a,0x9114,22,200),(0x9114,0x910a,22,200),
                              (0x950a,0x910a,8,200),(0x910a,0x910a,22,200),
                              (0x910a,0x9114,22,0)]:
   q=bytearray(m)
   for x in range(48):
    for y in range(256):q[0x9000+x*256+y]=0 if y%10 in (0,9) else (x*37+y*19)&255
   expected=bytearray(q[0x9000:0xc000])
   for x in range(cell):
    for y in range(height):expected[dst-0x9000+x*256+y]=q[src+x*256+y]
   result=call(q,'uiCopyTextRect',hl=src,bc=dst,de=cell*256+height)
   assert result[0x9000:0xc000]==expected,('raw glyph copy',src,dst,cell,height)
 # Disk free clusters and physical blocks can use different unit sizes.
 N['testInfoKiB']=0x8300
 for unit in (256,512,1024,2048,32768):
  for count in (0,1,3,4,19,2048,65535):
   expected=count*unit//1024
   if expected>65535:continue
   q=bytearray(m)
   stub=b'\xcd'+W(N['pip_unitsKiB'])+b'\xeb\x22'+W(0x8406)+b'\xc9'
   q[0x8300:0x8300+len(stub)]=stub
   q=call(q,'testInfoKiB',de=count,hl=unit)
   assert q[0x8406:0x8408]==W(expected),(unit,count,'KiB conversion')
 # Every visible slot fits the panel interior, aligned to screen bytes and glyphs.
 for slot in range(m[N['ncVisibleCount']]):
  q=call(m,'ncSlotPosition',a=slot)
  # capture DE through tiny direct store on a second harness isn't needed: cursor test.
  q[N['state.panelA_filesCnt']]=255;q[N['state.panelA_curFile']]=slot
  before=bytes(q[0x9000:0xc000]);q=call(q,'drawCursor')
  changed=[i for i,(a,b) in enumerate(zip(before,q[0x9000:0xc000])) if a!=b]
  col,row=divmod(slot,N['P_FILE_LIST_H'])
  x=8+col*m[N['ncColumnStep']]*2
  text_width={4:48,6:72,8:176}[width]
  left=x;right=x+text_width+(width==6)
  expected=bytearray(before)
  for px in range(left,right):
   for py in range(N['SCROLL_TOP']+row*10,N['SCROLL_TOP']+row*10+10):expected[px//8*256+py]^=128>>(px%8)
  assert q[0x9000:0xc000]==expected,(name,slot,'cursor does not enter frame bytes')
 # First toolbar paint and replay are identical and never scroll/touch other rows.
 for shift in [2,0]:
  q=bytearray(m);q[N['IO_KEYB_B']]=shift;q[0x9000:0xc000]=bytes([0x55])*12288
  first=call(q,'drawToolbar')
  assert all(first[0x9000+x*256:0x9000+x*256+247]==bytes([0x55])*247 for x in range(48)),(name,shift,'toolbar overflow')
  valid=N['ncToolbarValid']+(shift==0);assert first[valid]==1
  q=bytearray(first)
  for x in range(48):q[0x9000+x*256+247:0x9000+x*256+256]=bytes(9)
  replay=call(q,'drawToolbar');assert replay[0x9000:0xc000]==first[0x9000:0xc000],(name,shift,'replay')
  reset=call(first,'ncLayoutInit');assert reset[N['ncToolbarValid']:N['ncToolbarValid']+2]==b'\0\0'
 # Full name with extension, file size/DIR: cannot overwrite any other cell.
 for directory in [False,True]:
  q=bytearray(m);q[0x8000:0x8010]=b'ABCDEFGHXYZ'+bytes([16 if directory else 0,0,0,255,255])
  q[N['bios_vars.cursorY']:N['bios_vars.cursorY']+2]=bytes([N['P_FILE_LIST_Y'],4])
  q[0x9000:0xc000]=bytes([0x55])*12288
  result=call(q,'printFileNamOnPanel',hl=0x8000)
  changed=[i for i,(a,b) in enumerate(zip(q[0x9000:0xc000],result[0x9000:0xc000])) if a!=b]
  assert all(1<=i//256<1+m[N['ncCellBytes']] and N['SCROLL_TOP']<=i%256<N['SCROLL_TOP']+10 for i in changed),(name,'filename overflow')
  if width==4:
   assert m[N['ncCellBytes']]==6 and {i//256 for i in changed}==set(range(1,7)), '12 narrow characters occupy exactly 6 screen bytes'
 # The editing cursor uses the selected width, including a scrolled window.
 q=bytearray(m);q[N['lineEditOrigin']:N['lineEditOrigin']+2]=bytes([243,8]);q[N['lineEditPosition']]=13;q[N['lineEditOffset']]=3
 q=call(q,'lineEditCursor');assert q[N['bios_vars.cursorX']]==8+10*width//2
 # BIOS blinking cursor: width, engine alignment, XOR restoration, right edge.
 for x in [0,1,2,3,4,186,188,190]:
  if width==6 and x==190:continue
  q=bytearray(m);q[N['bios_vars.cursorY']:N['bios_vars.cursorY']+2]=bytes([243,x])
  q[N['bios_upper.cursorCfg']]=1
  q[0x9000:0xc000]=bytes([0x55])*12288
  result=call(q,'bios_drawCursorOld')
  xp=x*2 if width==6 else (x*2//width)*width
  expected=bytearray(q[0x9000:0xc000])
  for pixel in range(xp,xp+width):expected[pixel//8*256+245]^=128>>(pixel%8)
  assert result[0x9000:0xc000]==expected,(name,x,'BIOS cursor mask')
  assert call(result,'bios_drawCursorOld')[0x9000:0xc000]==q[0x9000:0xc000]
 # Path display budgets include the full 8.3 leaf and never mutate the source.
 for path in [r'A:\Folder\Middle\File.txt',r'A:\FOLDER01\FOLDER02\FOLDER03\FILENAME.TXT',r'A:\12345678.123','A:\\',r'A:\ABC\DEF\GHI\JKL\FILE.TXT']:
  for budget in [19,20,22,24,28,36,40,44]:
   q=bytearray(m);raw=path.encode()+b'\0';q[0x8000:0x8000+len(raw)]=raw
   result=call(q,'compactPath',hl=0x8000,bc=budget<<8)
   at=N['vars.displayPath'];shown=bytes(result[at:at+128]).split(b'\0')[0]
   assert len(shown)<=budget,(name,path,budget,shown)
   assert shown.startswith(b'A:\\') and shown.endswith(path.split('\\')[-1].encode()),(path,budget,shown)
   assert result[0x8000:0x8000+len(raw)]==raw
   if path==r'A:\Folder\Middle\File.txt' and budget==24:assert shown==rb'A:\Folder\...\File.txt',shown
 # Entire F1/F2 dialog, including title and 1..8 drive letters, stays in its box.
 for panel in (0,1):
  for count in range(1,9):
   q=bytearray(m);q[N['state.activePanel']]=panel
   q[N['buildDriveList']]=0xc9
   q[N['vars.driveCount']]=count
   q[N['vars.driveList']:N['vars.driveList']+8]=bytes(range(8))
   q[N['bios_getch']:N['bios_getch']+4]=bytes.fromhex('3e 1b b7 c9')
   q[0x9000:0xc000]=bytes([0x55])*12288
   result=call(q,'chooseDrive')
   changed=[i for i,(a,b) in enumerate(zip(q[0x9000:0xc000],result[0x9000:0xc000])) if a!=b]
   assert all(5+panel*24<=i//256<19+panel*24 and 85<=i%256<135 for i in changed),(name,panel,count,'drive dialog overflow')
 # Every scroll step, including the 6px retained highlight, must match redraw.
 for panel in (0,1):
  for direction in (1,2):
   for delta in range(1,N['P_FILE_LIST_H']+1):
    for kept in ((False,True) if width==6 and delta==1 else (False,)):
     q=bytearray(m);q[N['state.activePanel']]=panel
     q[N['loadFiles']]=0xc9
     q[N['vars.cacheSlotA']+panel]=0
     q[N['state.panelA_filesCnt']+panel]=190
     base=N['FILE_CACHE_A'];index=N['FILE_INDEX_A']
     for i in range(190):
      record=f'FILE{i:04d}TXT'.encode()+bytes([16 if i%7==0 else 0,0,0])+W(i*311)
      q[base+i*16:base+(i+1)*16]=record
     q[index:index+191]=bytes(range(190))+b'\xff'
     q[N['vars.fileListPtr']:N['vars.fileListPtr']+2]=W(index+30)
     q[0x9000:0xc000]=bytes([0x55])*12288
     q=call(q,'ncPrintAdaptiveList')
     if kept:
      q[N['state.panelA_firstFile']+panel]=30
      q[N['state.panelA_curFile']+panel]=30+(q[N['ncVisibleCount']]-1 if direction==1 else 0)
      q=call(q,'drawCursor')
     first=30+(delta if direction==1 else -delta)
     q[N['state.panelA_firstFile']+panel]=first
     q[N['state.panelA_curFile']+panel]=first+(q[N['ncVisibleCount']]-1 if direction==1 else 0)
     q[N['vars.fileListPtr']:N['vars.fileListPtr']+2]=W(index+first)
     q[N['vars.scrollRows']]=delta
     q[N['vars.scrollCursorKept']]=kept
     # Independently redraw from a clean list, then invert the new selection.
     reference=bytearray(q)
     if kept:
      reference[N['state.panelA_firstFile']+panel]=30
      reference[N['state.panelA_curFile']+panel]=30+(reference[N['ncVisibleCount']]-1 if direction==1 else 0)
      reference=call(reference,'drawCursor')
      reference[N['state.panelA_firstFile']+panel]=first
      reference[N['state.panelA_curFile']+panel]=q[N['state.panelA_curFile']+panel]
     expected=call(reference,'ncPrintAdaptiveList');expected=call(expected,'drawCursor')
     result=call(q,'scrollBitmapPanel',bc=direction<<8)
     cycles=LAST_CYCLES
     assert result[0x9000:0xc000]==expected[0x9000:0xc000],(name,panel,direction,delta,kept,'bitmap scroll vs full redraw')
     if N['ENABLE_COLOR']:
      colors=N['vars.rowColors']+panel*3*N['P_FILE_LIST_H']
      size=cols*N['P_FILE_LIST_H']
      assert result[colors:colors+size]==expected[colors:colors+size],(name,panel,direction,delta,kept,'row color scroll vs full redraw')
     if panel==0 and delta==1:
      old=bytearray(q)
      copy_entry=N['ncCopyColoredText'] if N['ENABLE_COLOR'] else N['uiCopyTextRect']
      at=old.index(b'\xcd'+W(copy_entry),N['bitmapCopy'],N['bitmapCopy']+64)
      old[at+1:at+3]=W(N['uiCopyRect'])
      baseline=call(old,'scrollBitmapPanel',bc=direction<<8)
      assert baseline[0x9000:0xc000]==result[0x9000:0xc000]
      if not N['ENABLE_COLOR']:assert cycles<LAST_CYCLES,(name,direction,'scroll speed')
      print(f'{name} scroll {direction}, kept={kept}: full step {LAST_CYCLES/2000:.3f} -> {cycles/2000:.3f} ms (model 2 MHz)',flush=True)
 # Every drive button has one glyph cell of padding on both sides.
 if width in (4,6,8):
  N['testDrivePosition']=0x8300
  for panel in (0,1):
   for count in range(1,9):
    for selected in range(count):
     q=bytearray(m);q[N['state.activePanel']]=panel
     q[N['vars.driveCount']]=count;q[N['vars.chooseDrive']]=selected
     stub=b'\xcd'+W(N['driveCellPosition'])+b'\x22'+W(0x8402)+b'\xc9'
     q[0x8300:0x8300+len(stub)]=stub
     pos=call(q,'testDrivePosition',a=selected)
     y,x=pos[0x8402:0x8404]
     q[0x9000:0xc000]=bytes([0x55])*12288
     result=call(q,'chooseDrive_draw')
     expected=bytearray(q[0x9000:0xc000])
     for pixel in range(x*2+panel*192-width+q[N['ncDrivePadLeft']],x*2+panel*192+2*width-q[N['ncDrivePadRight']]):
      for row in range(y-8,y+2):expected[pixel//8*256+row]^=128>>(pixel%8)
     assert result[0x9000:0xc000]==expected,(count,selected,panel,'pixel drive cursor padding')
     assert call(result,'chooseDrive_draw')[0x9000:0xc000]==q[0x9000:0xc000]
 # Real main-loop no-op navigation must not invoke either cursor inversion.
 # All four overwrite answers are centered in their own pixel rectangles.
 for choice,length in enumerate((3,3,2,4)):
  q=bytearray(m);q[N['ncOverwriteChoice']]=choice
  q[0x9000:0xc000]=bytes([0x55])*12288
  result=call(q,'ncOverwriteInvert')
  text_x=(48+32*choice-(length*width//2)//2)
  if width==4:text_x&=~1
  if width==8:text_x&=~3
  left=text_x*2-width+q[N['ncDrivePadLeft']]+1
  right=text_x*2+(length+1)*width-q[N['ncDrivePadRight']]-1
  expected=bytearray(q[0x9000:0xc000])
  for px in range(left,right):
   for py in range(141,151):expected[px//8*256+py]^=128>>(px%8)
  assert result[0x9000:0xc000]==expected,(name,choice,'overwrite centering')
  assert 68<=left<right<=316,(name,choice,'overwrite button outside window')
  assert call(result,'ncOverwriteInvert')[0x9000:0xc000]==q[0x9000:0xc000]
 q=bytearray(m);q[0x8000:0x8010]=b'FILE    TXT'+bytes(5)
 q[N['ncModalGetch']:N['ncModalGetch']+3]=bytes.fromhex('3e 1b c9')
 q=call(q,'ncOverwriteBody',hl=0x8000)
 assert q[N['ncOverwriteChoice']]==2,(name,'default overwrite answer')
 for count,selected,keys in [(20,0,[8,12,25]),(20,19,[10,24,26]),(0,0,[8,10,12,24,25,26])]:
  for key in keys:
   q=bytearray(m);q[N['state.panelA_filesCnt']]=count;q[N['state.panelA_curFile']]=selected
   reader=bytes.fromhex('3a 00 84 3c 32 00 84 fe 02 ca ff 81 3e')+bytes([key])+bytes.fromhex('b7 c9')
   q[N['ncReadKey']:N['ncReadKey']+len(reader)]=reader
   trace=bytes.fromhex('3a 01 84 3c 32 01 84 c9')
   q[N['drawCursor']:N['drawCursor']+len(trace)]=trace
   result=call(q,'mainLoop_cursorReady')
   assert result[0x8400]==2 and result[0x8401]==0,(name,count,selected,key,'boundary blink')
   assert result[N['state.panelA_curFile']]==selected
 # Exact directory notifications preserve unrelated path/list caches.
 for directory,scope,expected in [(0x1234,0,0),(0,0,1),(0x1234,1,1)]:
  q=bytearray(m);q[N['state.panelA_drive']]=1;q[N['state.panelB_drive']]=7
  q[N['vars.pathValidA']]=1;q[N['vars.pathValidB']]=1
  result=call(q,'ncDiskChanged',a=1,bc=scope,de=directory)
  assert result[N['ncDirtyMask']]==expected
  assert result[N['vars.pathValidA']]==1-expected
  assert result[N['vars.pathValidB']]==1
 # A quiet command without visible changes returns with the highlight intact.
 q=bytearray(m);q[N['ncCommandCursorKept']]=1
 for label in ('updateCommandPath','clearCmdLine'):q[N[label]]=0xc9
 q[N['ncReadKey']:N['ncReadKey']+3]=bytes.fromhex('c3 ff 81')
 trace=bytes.fromhex('3a 01 84 3c 32 01 84 c9')
 q[N['drawCursor']:N['drawCursor']+len(trace)]=trace
 result=call(q,'ncRefreshDirty')
 assert result[0x8401]==0 and result[N['ncCommandCursorKept']]==0,(name,'quiet command blink')
 print(name,'layout, cursor, toolbar capture/replay, file fields and editor PASS',flush=True)
print('NC adaptive layout CPU PASS')
