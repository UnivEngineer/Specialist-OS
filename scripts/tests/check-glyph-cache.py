"""8080: динамическое начало, сохранение кэша при расширении, сброс при сужении."""
from pathlib import Path
import re,subprocess,json
R=Path(__file__).resolve().parents[2];O=R/'docs/font-arena-044/cache';O.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/BIOS.sym').read_text(),re.M)}
font=(R/'assets/system-rom/FONT.FNT').read_bytes();rows=[]
for old_limit,mode,limit,expected_invalid,expected_entries,expected_epoch,error in [(s['GLYPH_CACHE_END'],1,s['GLYPH_CACHE_END'],0,5,42,False),(0xe500,1,s['GLYPH_CACHE_END'],0,5,42,False),(s['GLYPH_CACHE_END'],1,s['GLYPH_CACHE_END']-1,1,0,43,False),(s['GLYPH_CACHE_END'],1,s['FONT_ADDR']+len(font)-1,0,5,42,True),(0xe500,2,0,0,5,42,False)]:
 m=bytearray(65536)
 for name,at in [('BIOS',0xc000),('IO',0xf000)]:
  data=(R/f'bin/{name}.sys').read_bytes();m[at:at+len(data)]=data
 m[s['FONT_ADDR']:s['FONT_ADDR']+len(font)]=font
 m[s['GLYPH_CACHE_START']:s['GLYPH_CACHE_START']+2]=(s['FONT_ADDR']+len(font)).to_bytes(2,'little')
 m[s['GLYPH_CACHE_LIMIT']:s['GLYPH_CACHE_LIMIT']+2]=old_limit.to_bytes(2,'little')
 m[s['GLYPH_CACHE_ENTRIES']:s['GLYPH_CACHE_ENTRIES']+2]=b'\5\0';m[s['GLYPH_CACHE_INVALID']]=0;m[s['GLYPH_CACHE_EPOCH']]=42
 code=b'\x31\xdf\x8f\x21'+limit.to_bytes(2,'little')+b'\x3e'+bytes([mode])+b'\xcd'+s['glyphCacheService'].to_bytes(2,'little')+b'\xc3\x00\x21';m[0x2000:0x2000+len(code)]=code
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','2000','-end','2100','-counter','1000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 got=(O/'out.bin').read_bytes();assert bool(got[65536]&1)==error
 assert got[s['GLYPH_CACHE_INVALID']]==expected_invalid
 assert int.from_bytes(got[s['GLYPH_CACHE_ENTRIES']:s['GLYPH_CACHE_ENTRIES']+2],'little')==expected_entries
 assert got[s['GLYPH_CACHE_EPOCH']]==expected_epoch
 rows.append(dict(mode=mode,limit=limit,error=error))
(O/'result.json').write_text(json.dumps(rows,indent=2));print('GLYPH CACHE PASS',len(rows))
