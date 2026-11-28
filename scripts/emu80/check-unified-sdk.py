"""C SDK FILEHANDLE on the real banked DOS; clear test console and restore NC."""
from pathlib import Path
import argparse,re,runpy,struct
R=Path(__file__).resolve().parents[2];O=R/'.local/unified-sdk';O.mkdir(parents=True,exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text('utf-8'),re.M)}
k=sy(R/'bin/banked/KERNEL.sym');b=sy(R/'bin/BIOS.sym')
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--runtime',type=Path,default=R/'.local/rom-pio-19782')
p.add_argument('--port',type=int,choices=range(19781,19797),default=19782)
args=p.parse_args()
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
g=h['Matrix'].__init__.__globals__;g.update(OUT=O,RUNTIME=args.runtime.resolve())
g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{args.port}/mcp'
word=lambda n:struct.pack('<H',n)
def page(v):m.write(0xfffd if v else 0xfffc,bytes([v-1 if v else 0]))
def call(op,hl=0,de=0,bc=0,a=0,error=False):
 page(0);m.write(0x3000,struct.pack('<4H',hl,de,bc,a*256+2));m.write(0x2000,b'\xcd\x60\xf8\xc3\x00\x8b')
 m.stop_at(0x8b00);m.call('regs_set',pc=0x2000,sp=0x8fdf,bc=op,de=0x3000)
 z=m.until(0x8b00,90);r={key:v['dec'] for key,v in z['registers'].items()}
 assert r['sp']==0x8fdf,r
 assert bool(r['af']&1)==error,(hex(op),r)
 assert m.read(b['FONT_ADDR'],b['FONT_BYTES'])==font,'font damaged'
 return r
try:
 fixture=runpy.run_path(str(R/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,O)
 x.start('C-SDK',disk=R/'bin/nested-Games2M.bin',**fixture)
 m.call('debug_break');m.call('bp_clear');font=m.read(b['FONT_ADDR'],b['FONT_BYTES'])
 call(k['MXOS_DOS_FILE_SELECT_DRIVE'],a=1)
 h['main_bios'](m,k['mx_printChar'],bc=31)
 data=(R/'docs/file-api-001/stdlib/stdio_code_crt_init.bin').read_bytes()
 assert len(data)+0x2000<0x5e80
 saved={address:m.read(address,3) for address in (b['getch'],b['keyScan2'])}
 m.write(0x6200,bytes.fromhex('3E 51 37 C9'));m.write(0x6210,bytes.fromhex('3E FF B7 C9'))
 for address in saved:m.write(address,b'\xc3'+word(0x6200 if address==b['getch'] else 0x6210))
 m.write(0x2000,data);m.stop_at(0x6100);m.call('regs_set',pc=0x2000,sp=0x8fdf)
 m.until(0x6100,90,page=0)
 for address,code in saved.items():m.write(address,code)
 result=int.from_bytes(m.read(0x5e80,2),'little')
 assert result==0,result
 assert m.read(b['FONT_ADDR'],b['FONT_BYTES'])==font
 n=sy(fixture['nc_symbols'])
 m.stop_at(n['mainLoop']);m.call('regs_set',pc=b['mon2_init'],sp=0x8fdf)
 m.until(n['mainLoop'],90,page=0);m.call('bp_clear');m.call('emu_run_for',ms=500)
 print('C SDK FILEHANDLE PASS',len(data),'bytes',flush=True)
finally:x.stop()
