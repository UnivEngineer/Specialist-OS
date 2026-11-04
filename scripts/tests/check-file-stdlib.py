"""C/sccz80 -> мост ABI -> настоящее ядро/модуль 8080; без вызовов CP/M."""
from pathlib import Path
import argparse,json,os,runpy,subprocess

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--z88dk',type=Path,default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk'))
args=parser.parse_args();out=ROOT/'docs/file-api-001/stdlib';out.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();env['ZCCCFG']=str(args.z88dk/'lib/config');env['PATH']=str(args.z88dk/'bin')+os.pathsep+env['PATH']
command=[str(args.z88dk/'bin/zcc.exe'),'+cpm','-clib=8080','-compiler=sccz80','-crt0='+str(ROOT/'sdk/tests/file-crt'),'-zorg=8192','-I'+str(ROOT/'sdk/include'),str(ROOT/'sdk/src/mxos_file.c'),str(ROOT/'sdk/src/mxos_file_z88dk.asm'),str(ROOT/'sdk/src/mxos.c'),str(ROOT/'sdk/src/mxos_z88dk.asm'),str(ROOT/'sdk/tests/file-stdio.c'),'-o',str(out/'stdio.bin'),'-m']
subprocess.run(command,cwd=ROOT,env=env,check=True)
h=runpy.run_path(str(ROOT/'scripts/tests/check-file-handle.py'));memory=h['memory']
s=h['S'];n=s['initVarsEnd']-s['initVars'];memory[s['bios_vars']:s['bios_vars']+n]=memory[s['initVars']:s['initVars']+n]
font=(ROOT/'assets/system-rom/FONTKOI.FNT').read_bytes();memory[s['FONT_ADDR']:s['FONT_ADDR']+len(font)]=font
# Клавиатуру моделируем, а регистровый мост и прочие функции исполняются реально.
memory[s['bios_getch']:s['bios_getch']+3]=b'\xC3\x00\x7D';memory[0x7D00:0x7D04]=bytes.fromhex('3E 51 37 C9')
memory[s['bios_keyScan']:s['bios_keyScan']+3]=b'\xC3\x10\x7D';memory[0x7D10:0x7D14]=bytes.fromhex('3E FF B7 C9')
data=(out/'stdio_code_crt_init.bin').read_bytes();assert 0x2000+len(data)<0x6000
memory[0x2000:0x2000+len(data)]=data;(out/'input.bin').write_bytes(memory)
subprocess.run([str(args.z88dk/'bin/z88dk-ticks.exe'),'-m8080','-pc','2000','-end','6100','-counter','50000000','-output',str(out/'output.bin'),str(out/'input.bin')],check=True,capture_output=True)
got=(out/'output.bin').read_bytes();result=int.from_bytes(got[0x7E00:0x7E02],'little')
assert int.from_bytes(got[65542:65544],'little')==0x6100
assert result==0,result
(out/'result.json').write_text(json.dumps(dict(cpu='8080',compiler='sccz80 -clib=8080',programBytes=len(data),exitCode=result)),encoding='utf-8')
print('C STDIO PASS; compiled 8080 executable:',len(data),'bytes')
