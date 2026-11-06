"""Сборка локального bootstrap, без вызовов BIOS и зависимости от COPYBEN."""
from pathlib import Path
import argparse, subprocess, runpy, re
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--probe',action='store_true');a=p.parse_args()
out=ROOT/'bin/bootstrap';out.mkdir(parents=True,exist_ok=True)
module=(ROOT/'bin/DOS.sys').read_bytes()
(out/'config.inc').write_text(f'DOS_BYTES = {(ROOT/"bin/BIOS.sys").stat().st_size}\nMODULE_SECTORS = {(len(module)+255)//256}\nBOOT_PROBE = {int(a.probe)}\n',encoding='utf-8')
symbols=dict(re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/BIOS.sym').read_text(),re.M))
with (out/'config.inc').open('a',encoding='utf-8') as f:
 f.write(f'BIOS_BOOT_LOAD_DOS = 0{int(symbols["biosLoadDosImage"],16):04X}h\n')
ts=dict(re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/banked/TRANSPORT.sym').read_text(),re.M))
with (out/'config.inc').open('a',encoding='utf-8') as f:
 for alias,key in [('BIOS_CHECK_RAMD','checkRAMD'),('BIOS_PAGE_COUNT','bios_vars.ramPageCount'),('BIOS_BOOT_PAGES','biosBootPages'),('BIOS_BOOT_WARM','biosBootWarm'),('BIOS_BANKED','v_dosBanked'),('BIOS_DOS_READY','v_dosModuleReady')]:
  f.write(f'{alias} = 0{int(symbols[key],16):04X}h\n')
 for alias,key in [('BANK_PAGE','bankPage'),('BANK_PAGE_COUNT','bankPageCount'),('BANK_SERVICE_SP','bankServiceSP'),('BANK_STACK_FLOOR','bankStackFloor')]:
  f.write(f'{alias} = 0{int(ts[key],16):04X}h\n')
ks=dict(re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/banked/KERNEL.sym').read_text(),re.M))
with (out/'config.inc').open('a',encoding='utf-8') as f:
 f.write(f'DOS_READY = 0{int(ks["v_dosModuleReady"],16):04X}h\n')
for name,source,defines in [('legacy' ,'SECTOR',[]),('sector','SECTOR',['-DSTAGED_BOOT=1']),('BOOT','BOOTBANK',[])]:
 subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',*defines,f'--raw={out/name}.bin',f'--sym={out/name}.sym',f'--lst={out/name}.lst',str(ROOT/f'source/bootstrap/{source}.asm')],check=True)
assert (out/'legacy.bin').read_bytes()==(ROOT/'assets/system-rom/boot-system.bin').read_bytes()
