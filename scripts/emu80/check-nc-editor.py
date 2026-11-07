"""Клавиатура NC/CLI: вставка пути, запуск E, NUL-маркер и сохранение FFh."""
from pathlib import Path
import runpy,json,re
R=Path(__file__).resolve().parents[2];O=R/'docs/nc-fixes-048';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001')
def symbols(p):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(),re.M)}
x=h['Matrix']();m=x.m;n=symbols(R/'bin/NC.sym');e=symbols(R/'bin/E.sym');c=symbols(R/'bin/CLI.sym');rows=[]
def ready():
 m.call('bp_clear');m.call('emu_run_for',ms=500)
def reset():
 m.stop_at(n['mainLoop']);m.call('emu_reset');m.until(n['mainLoop'],120);ready()
def text(s):
 for ch in s:m.tap({' ':'space',':':'colon','.':'period','/':'slash'}.get(ch,ch.lower()))
def cmd(s,target):
 ready();m.tap('esc');text(s);m.screenshot(O/'command.png');m.stop_at(target);m.tap('enter');m.until(target,120,page=0)
def choose(stem,ext):
 cache=m.read(n['FILE_CACHE_A'],n['FILE_CACHE_BYTES'])
 name=(stem.ljust(8)+ext.ljust(3)).encode();i=next(i for i in range(len(cache)//16) if cache[i*16:i*16+11].upper()==name)
 cur=m.read(n['state']+10,1)[0]
 for j in range(abs(i-cur)):m.tap('down' if i>cur else 'up')
 print('SELECT',name,i,flush=True)
def editor(label,empty):
 start=int.from_bytes(m.read(e['v_textBeginLo'],2),'little');end=int.from_bytes(m.read(e['v_textEnd'],2),'little')
 assert (end==start+1) if empty else end>start+1,(label,start,end,m.read(start,32))
 if empty:assert m.read(start,2)==b'\r\0'
 m.screenshot(O/(label+'.png'));rows.append(dict(case=label,empty=empty,bytes=end-start));print(rows[-1],flush=True)
# Собственный H: с текстом, содержащим FFh. Рабочие образы пользователя не меняются.
import struct
raw=bytearray((R/'bin/nested-Games2M.bin').read_bytes());w=lambda at:struct.unpack_from('<H',raw,at)[0]
sector=w(11);fat=w(14)*sector;root=(w(14)+raw[16]*w(22))*sector;data=root+((w(17)*32+sector-1)//sector)*sector
slots=[a for a in range(root,data,32) if raw[a] in (0,0xe5,0xff)];free=[a for a in range(2,(len(raw)-data)//sector+2) if w(fat+a*2)==0]
for i,(name,payload) in enumerate([('MARKER.TXT',b'A\xffB\rC\xffD\r'),('APPEND.TXT',b'X\xffY\r')]):
 cluster=free[i];struct.pack_into('<H',raw,fat+2*cluster,0xffff);offset=data+(cluster-2)*sector;raw[offset:offset+len(payload)]=payload
 stem,ext=name.split('.');record=bytearray((stem.ljust(8)+ext.ljust(3)).encode()+bytes(21));struct.pack_into('<H',record,26,cluster);struct.pack_into('<I',record,28,len(payload));raw[slots[i]:slots[i]+32]=record
fixture=O/'fixtures.bin';fixture.write_bytes(raw)
try:
 x.start('nc-regressions',disk=fixture);m.screenshot(O/'toolbar.png');ready();choose('README','TXT')
 m.call('kbd_send',keys=['rpt'],action='press');m.call('emu_run_for',ms=200);m.tap('enter');m.call('kbd_send',keys=['rpt'],action='release');m.call('emu_run_for',ms=500)
 line=m.read(n['vars.cmdLine'],128).split(b'\0')[0];assert line.upper()==b'A:'+bytes([92])+b'README.TXT',line
 rows.append(dict(case='ctrl-enter',line=line.decode()));m.screenshot(O/'ctrl-enter.png');m.tap('esc')
 m.stop_at(e['editorStatusLoop']);m.tap('f4');m.until(e['editorStatusLoop'],120,page=0);editor('f4-readme',False)
 reset();choose('E','COM');m.stop_at(e['editorStatusLoop']);m.tap('enter');m.until(e['editorStatusLoop'],120,page=0);editor('enter-e',True)
 reset();cmd('E.COM',e['editorStatusLoop']);editor('nc-empty',True)
 reset();m.stop_at(c['cliPrompt']);m.tap('f10');m.until(c['cliPrompt'],120,page=0);cmd('E.COM',e['editorStatusLoop']);editor('cli-empty',True)
 m.stop_at(c['cliPrompt']);m.call('emu_reset');m.until(c['cliPrompt'],120,page=0);cmd('E.COM A:/README.TXT',e['editorStatusLoop']);editor('cli-readme',False)
 # Открытие FFh-текста, изменение строки, добавление файла и сохранение без NUL.
 m.stop_at(c['cliPrompt']);m.call('emu_reset');m.until(c['cliPrompt'],120,page=0);cmd('E.COM H:/MARKER.TXT',e['editorStatusLoop'])
 assert m.read(0x1800,9)==b'A\xffB\rC\xffD\r\0'
 ready();m.tap('z');m.tap('f6');text('H:/APPEND.TXT');m.stop_at(e['editorStatusLoop']);m.tap('enter');m.until(e['editorStatusLoop'],120,page=0)
 expected=b'z\xffB\rC\xffD\rX\xffY\r';assert m.read(0x1800,len(expected)+1)==expected+b'\0'
 ready();m.tap('f5');text('B:/MARKER.TXT');m.stop_at(e['editorStatusLoop']);m.tap('enter');m.until(e['editorStatusLoop'],120,page=0);m.screenshot(O/'marker-saved.png')
 m.write(0x7600,b'B:/MARKER.TXT\0');assert not x.func(0xC85A,hl=0x7600,de=0x7800)['flags']['C']
 assert not x.func(0xC851,hl=0x7800)['flags']['C'];assert int.from_bytes(m.read(0x781c,4),'little')==len(expected)
 assert not x.func(0xC866,hl=0x7800,de=0x7000)['flags']['C'];assert m.read(0x7000,len(expected))==expected
 rows.append(dict(case='nul-marker',hardSignPreserved=True,appendAfterEditing=True,savedBytes=len(expected),nulNotSaved=True))
 (O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
finally:x.stop()

