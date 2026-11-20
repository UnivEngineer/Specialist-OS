"""Build the native on-machine executable metadata/conversion utility."""
from pathlib import Path
import os,re,runpy,subprocess
R=Path(__file__).resolve().parents[2];out=R/'bin';z=Path(os.environ.get('SPECIALIST_Z88DK_DIR','D:/Projects/RetroPC/Specialist/MySoft/z88dk'))
env=os.environ.copy();env['ZCCCFG']=str(z/'lib/config');env['PATH']=str(z/'bin')+os.pathsep+env['PATH']
files=['sdk/src/mxos_z88dk.asm','sdk/src/mxos.c','sdk/src/mxos_file_z88dk.asm','sdk/src/mxos_file.c','sdk/src/mxos_far.c','sdk/src/mxos_heap.c','source/exe/EXE.c']
subprocess.run([str(z/'bin/zcc.exe'),'+cpm','-SO3','-clib=8080','-compiler=sccz80','-crt0='+str(R/'sdk/runtime/crt0'),'-zorg=256','-I'+str(R/'sdk/include'),*[str(R/p) for p in files],'-o',str(out/'EXE.bin'),'-m'],env=env,cwd=R,check=True)
code=(out/'EXE_code_crt_init.bin').read_bytes()
map_text=(out/'EXE.map').read_text('utf-8')
head=int(re.search(r'^__BSS_head\s*=\s*\$([0-9A-Fa-f]+)',map_text,re.M)[1],16)
assert 0x100<=head<=0x100+len(code)
crt_limit=int(re.search(r'^crtLimit\s*=\s*\$([0-9A-Fa-f]+)',map_text,re.M)[1],16)
assert code[crt_limit-0x100]==0x11, 'CRT heap must use LXI D, not a memory load'
code=code[:head-0x100]
(out/'EXE.i80').write_bytes(code)
c=runpy.run_path(str(R/'scripts/images/cpu.py'));fields=c['fields_from_c_map']((out/'EXE.map').read_text('utf-8'),0x100,len(code))
(out/'EXE.cpu').write_text(c['format_cpu'](0x100,flags=4,fields=fields),encoding='ascii')
print('EXE utility:',len(code),'body bytes, BSS',fields['BSS'])
