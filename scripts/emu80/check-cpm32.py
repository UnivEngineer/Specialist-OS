"""CPM32 в собственном видимом Emu80: реальные EXEC/DATA/ROM, без bootstrap."""
from pathlib import Path
import os
import argparse,json,re,runpy,struct,subprocess,time
R=Path(__file__).resolve().parents[2];W=lambda n:struct.pack('<H',n)
def sy(path):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',path.read_text(),re.M)}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True);p.add_argument('--port',type=int,default=19782);p.add_argument('--report',type=Path,default=R/'docs/bench/cpm32-system-2026-11-13/emulator');a=p.parse_args();a.report.mkdir(parents=True,exist_ok=True)
 owner=subprocess.run(['powershell.exe','-NoProfile','-Command',f'Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue | Select-Object -ExpandProperty OwningProcess'],capture_output=True,text=True)
 assert not owner.stdout.strip(),'Port is owned; no emulator started'
 launch=(a.runtime/'specialist-os/launch.conf').read_text();assert 'pitSoundSource.muted = yes' in launch and 'tapeMonitor.muted = yes' in launch
 B=runpy.run_path(str(R/'source-apps/diskbench/check-emulator.py'))['Bench'];m=B(f'http://127.0.0.1:{a.port}/mcp');rows=[]
 process=subprocess.Popen([str(a.runtime/'Emu80qt-mcp.exe'),'--conf-file','specialist-os/launch.conf','--platform','specialist-os'],cwd=a.runtime,env=dict(os.environ,SPECIALIST_EMU80_MCP_PORT=str(a.port)))
 try:
  for _ in range(60):
   try:m.call('debug_break');break
   except Exception:time.sleep(.2)
  else:raise RuntimeError('Owned emulator unavailable')
  def select(page):m.write(0xfffe if page&128 else (0xfffd if page else 0xfffc),bytes([page&15 if page&128 else (page-1 if page else 0)]))
  for variant,folder in [('CPM16',R/'docs/bench/cpm32-system-2026-11-13/before/bin'),('CPM32',R/'bin')]:
   ts=sy(folder/'banked/TRANSPORT.sym');image=(folder/'MEM.sys').read_bytes();parts=[struct.unpack_from('<HHH',image,6+i*6) for i in range(image[5])]
   for page in (0,1,3):
    select(page)
    for addr,size,offset in parts:m.write(addr,image[offset:offset+size])
    m.write(ts['bankPage'],bytes([page]));m.write(ts['bankPageCount'],b'\x10');m.write(ts['MEM_NEAR_COPY'],b'\0\0')
   for page in (2,4):
    select(page)
    addr,size,offset=next(x for x in parts if x[0]==0xfc00);begin=ts.get('MEM_DATA_START',0xff00);m.write(begin,image[offset+begin-0xfc00:offset+size])
   for caller,srcpage,dstpage in [(0,2,3),(3,0,2),(1,128,3),(3,2,4)]:
    for count in (1,31,32,33,255,256,257,8192):
     for phase in (0,1,255):
      src,dst=0x2000+phase,0x6000+phase
      select(srcpage)
      if srcpage&128:payload=m.read(src,count)
      else:payload=bytes((j*73+(j>>8)+count)&255 for j in range(count));m.write(src,payload)
      select(dstpage);m.write(dst-1,b'\xa5'+bytes(count)+b'\x5a')
      select(caller);m.write(0x8500,bytes([srcpage])+W(src)+bytes([dstpage])+W(dst)+W(count));m.write(0xffc0,image[parts[-1][2]:parts[-1][2]+32])
      stub=b'\x01'+W(0x1234)+b'\x11'+W(0x5678)+b'\x21'+W(0x8500)+b'\xcd'+W(ts['bankCopySector'] if srcpage&128 else ts['bankCopyFar'])+b'\xc3'+W(0x88ff)
      m.write(0x8800,stub);m.stop_at(0x88ff);m.call('regs_set',pc=0x8800,sp=0x8f00)
      if variant=='CPM32':
       m.call('bp_set',addr=0xffc0,type='exec',skipCount=1_000_000)
      start=m.call('cpu_state')['cpu_ticks'];state=m.until(0x88ff,10);ticks=state['cpu_ticks']-start
      assert not state['flags']['C'],(variant,caller,count,phase,state)
      assert state['registers']['sp']['dec']==0x8f00
      assert state['registers']['bc']['dec']==0x1234 and state['registers']['de']['dec']==0x5678 and state['registers']['hl']['dec']==0x8500
      if variant=='CPM32':
       entry,= [bp for bp in m.call('bp_list')['breakpoints'] if bp['addr']=='FFC0' and bp['type']=='exec']
       assert not entry['hitCount'],(caller,srcpage,count,phase,'executed common window')
       window=m.read(0xffc0,32)
       portion=count%32 or 32
       assert window[:portion]==payload[count-portion:]
      select(dstpage);actual=m.read(dst-1,count+2);assert actual==b'\xa5'+payload+b'\x5a',(variant,caller,srcpage,count,phase,'data')
      rows.append(dict(engine=variant,caller=caller,sourcePage=srcpage,destinationPage=dstpage,bytes=count,phase=phase,ticks=ticks,KiB_s=count*2000000/ticks/1024))
   print(variant,'PASS',sum(r['engine']==variant for r in rows),flush=True)
  (a.report/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
 finally:
  process.terminate();process.wait(timeout=15)
 print('CPM32 REAL BANKS PASS',len(rows),'cases, original bootstrap unchanged; owned emulator closed')
if __name__=='__main__':main()
