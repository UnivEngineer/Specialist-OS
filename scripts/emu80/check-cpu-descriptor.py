"""Real SpecMxFileLoader: extra CPU fields do not affect loaded bytes."""
from pathlib import Path
import argparse,json,runpy,urllib.request
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--runtime',type=Path,default=R/'.local/monitor2-001')
p.add_argument('--port',type=int,choices=range(19781,19797),default=19781)
a=p.parse_args();O=R/'.local/cpu-descriptor-check';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__
g.update(OUT=O,RUNTIME=a.runtime.resolve())
g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
original_command=g['command'];original_config={}
def command(argv):
 result=original_command(argv)
 if '-PrepareOnly' in [str(x) for x in argv]:
  config=a.runtime/'specialist-os/specialist-mx2.conf'
  original_config[config]=config.read_bytes()
  with config.open('a',encoding='utf-8') as f:
   f.write('\nSpecMxFileLoader descriptorLoader\ndescriptorLoader.addrSpace = &addrSpace\ndescriptorLoader.ramDiskAddrSpace = &ramDisk0\ndescriptorLoader.pageMapper = &addrSpace\n')
 return result
g['command']=command
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp'
try:
 fixture=runpy.run_path(str(R/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,O)
 x.start('CPU descriptor',disk=R/'bin/nested-Games2M.bin',**fixture)
 request=urllib.request.Request(m.url,json.dumps({'jsonrpc':'2.0','id':1,'method':'tools/list','params':{}}).encode(),{'Content-Type':'application/json','Accept':'application/json, text/event-stream'})
 tools=json.load(urllib.request.urlopen(request))['result']['tools']
 spec=next(t for t in tools if t['name']=='emu_load');assert 'file' in spec['inputSchema']['properties']
 m.call('debug_break');m.call('bp_clear');payload=bytes(range(1,65));monitor=bytes(range(32))
 for dest in [a.runtime/'monC000-cpu-probe.mon',a.runtime/'specialist-os/monC000-cpu-probe.mon']:dest.write_bytes(monitor)
 snapshots=[]
 for name,extra in [('plain',''),('extended','FLAGS=04\nCUSTOM=Emulator must ignore this field\nBSS=0200\n')]:
  source=O/(name+'.cpu');source.write_text('4000\n4003\nmonC000-cpu-probe.mon\n'+extra,encoding='ascii');source.with_suffix('.i80').write_bytes(payload)
  m.write(0xfffd,b'\0');m.write(0,bytes(1024))
  result=m.call('emu_load',file=str(source.resolve()),autorun=False);assert result['loaded'],result
  data=m.read(0,1024);assert data[:3]==b'\xd3'*3 and data[30:30+len(payload)]==payload
  assert int.from_bytes(data[24:26],'little')==0x4000
  assert int.from_bytes(data[26:28],'little')==0x403f
  assert m.read(0xc000,32)==monitor
  snapshots.append(data)
 assert snapshots[0]==snapshots[1]
 print('Emu80 CPU PASS: extra fields ignored; load/entry metadata unchanged',flush=True)
 print('Emu80 monitor placement: page 1 (RAM-disk), not MAIN; native MXOS monitor needs a loader fix',flush=True)
finally:
 x.stop()
 for config,data in original_config.items():config.write_bytes(data)
 for dest in [a.runtime/'monC000-cpu-probe.mon',a.runtime/'specialist-os/monC000-cpu-probe.mon']:
  if dest.exists() and dest.read_bytes()==bytes(range(32)):dest.unlink()

