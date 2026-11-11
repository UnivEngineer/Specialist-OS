"""FONTBEN: compare row-paged and 32-glyph block layouts on 8080."""
from pathlib import Path
import json,re,subprocess
R=Path(__file__).resolve().parents[2];O=R/'docs/bench/block32';O.mkdir(parents=True,exist_ok=True)
W=lambda n:n.to_bytes(2,'little')
font=(R/'assets/system-rom/FONT.FNT').read_bytes()
codes=list(range(32,128))+list(range(192,256))
source=(R/'assets/system-rom/README.TXT').read_bytes().replace(b'\r\n',b' ').replace(b'\r',b' ').replace(b'\n',b' ')
corpus=bytes(c if c>=32 else 32 for c in source)[:1152].ljust(1152,b' ')
corpus=bytes(c if c in codes else 32 for c in corpus)
ops=[lambda b:b<<2,lambda b:b>>2,lambda b:(b<<6)&255,lambda b:b>>4,lambda b:(b<<4)&255]
results={};checks=0
for compact,name in [(False,'FONTCOM'),(True,'FONTBLK'),(False,'FONTRMP')]:
 s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/f'bin/{name}.sym').read_text(),re.M)}
 base=bytearray(65536);binary=(R/f'bin/{name}.i80').read_bytes();base[:len(binary)]=binary
 data=bytes(font[c*8+y] for c in codes for y in range(8)) if compact else bytes(font[c*8+y] for y in range(8) for c in range(256))
 base[0x7e00:0x7e00+len(data)]=data
 for i,op in enumerate(ops):
  start=s[f'PAGED_PHASE{i}'];base[start:start+len(data)]=bytes(op(b) for b in data)
 # Exercise the actual standalone preparation routine, including chained phases.
 prepared=bytearray(base)
 for binary_name,address in [('BIOS',0xc000),('IO',0xf000)]:
  body=(R/f'bin/{binary_name}.sys').read_bytes();prepared[address:address+len(body)]=body
 prepared[s['FONT_ADDR']:s['FONT_ADDR']+2048]=bytes(font[c*8+y] for y in range(8) for c in range(256))
 prepared[s['output']+1:s['output']+3]=W(s['pagedRaw'])
 setup=b'\x31'+W(0x8f00)+b'\xcd'+W(s['pagedSelect'])+b'\xc3'+W(0x8bf0)
 prepared[0xba00:0xba00+len(setup)]=setup
 (O/'input.bin').write_bytes(prepared)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','ba00','-end','8bf0','-counter','20000000','-output',str(O/'output.bin'),str(O/'input.bin')],capture_output=True,check=True)
 built=(O/'output.bin').read_bytes()
 assert built[0x7e00:0x7e00+len(data)]==data,(name,'prepared font')
 for phase,op in enumerate(ops):
  start=s[f'PAGED_PHASE{phase}']
  assert built[start:start+len(data)]==bytes(op(b) for b in data),(name,'prepared phase',phase)
 checks+=1
 for koi in ([False,True] if compact or name=='FONTRMP' else [False]):
  for label in ('raw6','pre6'):
   samples=[corpus,bytes(range(256))*4+bytes(range(128))]
   for sample in samples:
    if compact:sample=bytes(c if c in codes else 32 for c in sample)
    mem=bytearray(base);mem[0x8600:0x8a80]=sample
    if koi:
     if compact:mem[s['blockPages']+3]=0x82
     else:mem[s['paged.blockMap']+3]=0xe0
    expected=bytearray(12288)
    code=b'\x31'+W(0x8f00)
    for row in range(24):
     code+=b'\x21'+W(0x8600+48*row)+b'\x0e\x30\x11'+W(0x9000+8*row)+b'\xcd'+W(s['paged.'+label])
     for x,c in enumerate(sample[row*48:row*48+48]):
      if compact:c=c if c in codes else 32
      if koi and 0x60<=c<0x80:c+=128
      for y,b in enumerate(font[c*8:c*8+8]):
       for bit in range(6):
        px=x*6+bit;addr=(px//8)*256+row*8+y
        if b&(32>>bit):expected[addr]|=128>>(px%8)
    code+=b'\xc3'+W(0x8bf0)
    # Harness in unused video tail; outputs occupy only the first 36 byte columns.
    mem[0xba00:0xba00+len(code)]=code
    (O/'input.bin').write_bytes(mem)
    z=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','ba00','-end','8bf0','-counter','20000000','-output',str(O/'output.bin'),str(O/'input.bin')],capture_output=True,check=True)
    actual=(O/'output.bin').read_bytes()
    assert actual[0x9000:0xb400]==expected[:0x2400],(name,koi,label)
    assert actual[0x7e00:0x7e00+len(data)]==mem[0x7e00:0x7e00+len(data)]
    for phase in range(5):
     start=s[f'PAGED_PHASE{phase}']
     assert actual[start:start+len(data)]==mem[start:start+len(data)],'prerender modified'
    checks+=1
    if sample==corpus:results[f'{name}-{label}-koi{7 if koi else 8}']={'cycles':int(z.stdout),'ms':int(z.stdout)/2000}
(O/'cpu.json').write_text(json.dumps(dict(checks=checks,timings=results),indent=2)+'\n')
print(results)
