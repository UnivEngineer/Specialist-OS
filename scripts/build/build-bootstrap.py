"""Сборка локального bootstrap, без вызовов BIOS и зависимости от COPYBEN."""
from pathlib import Path
import argparse, subprocess, runpy, re, json, sys
from build_cache import main as cached_build

ROOT = Path(__file__).absolute().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--probe', action='store_true')
p.add_argument('--font-body', type=Path)
p.add_argument('--font-name')
p.add_argument('--force', action='store_true')
p.add_argument('--run-build', action='store_true', help=argparse.SUPPRESS)
a = p.parse_args()
if a.font_body is None:
 lines=(ROOT/'scripts/images/manifests/A-System.txt').read_text(encoding='utf-8-sig').splitlines()
 names=[line.split()[1] for line in lines if line.strip().lower().startswith('@bootfont ')]
 if len(names)!=1:raise ValueError('System manifest requires exactly one @bootfont')
 entries=[line.split() for line in lines if line.strip() and not line.lstrip().startswith(('#','@'))]
 sources=[fields[1] for fields in entries if fields[0].lower()==names[0].lower()]
 if len(sources)!=1:raise ValueError('Boot font must name exactly one manifest entry')
 a.font_body=ROOT/'assets'/sources[0]
 if a.font_name is None:a.font_name=Path(names[0]).stem
font_path=a.font_body.with_suffix('.i80') if a.font_body.suffix.lower()=='.cpu' else a.font_body
out = ROOT / 'bin/bootstrap'
if not a.run_build:
    subprocess.run([sys.executable, str(ROOT / 'scripts/build/build-font-engines.py')], check=True)
    options = ['Bootstrap', '--scan-output', str(out), '--output', str(out / 'sector.bin')]
    for name in ('source/bootstrap', 'source/include', 'tasm/sjasmplus.exe', 'scripts/build/build-boot-splash.py',
                 'scripts/images/executable.py', 'scripts/images/cpu.py', 'scripts/images/manifests/A-System.txt',
                 'assets/system-rom/boot-system.bin', 'bin/BIOS.sys', 'bin/BIOS.sym', 'bin/IO.sys', 'bin/DOS.sys',
                 'bin/System.layout.json', 'bin/banked/TRANSPORT.sym', 'bin/banked/KERNEL.sym', 'bin/MEM.sym'):
        options.extend(('--input', str(ROOT / name)))
    options.extend(('--input', str(font_path.absolute())))
    if a.force:
        options.append('--force')
    command = [__file__, '--run-build', '--font-body', str(a.font_body.absolute())]
    if a.font_name is not None:
        command.extend(('--font-name', a.font_name))
    if a.probe:
        command.append('--probe')
    sys.exit(cached_build([*options, '--', *command]))

out.mkdir(parents=True, exist_ok=True)
module = (ROOT / 'bin/DOS.sys').read_bytes()
io = json.loads((ROOT / 'bin/System.layout.json').read_text('utf8'))['io']
assert io['bytes'] + io['bankBytes'] == (ROOT / 'bin/IO.sys').stat().st_size
font=font_path.read_bytes()
if font[:4]==b'\xcd MX':
 font=runpy.run_path(str(ROOT/'scripts/images/executable.py'))['decode'](font,'fnt').data
abi_source=(ROOT/'source/include/mxos.inc').read_text(encoding='utf-8')
font_version=int(re.search(r'^FONT_ENGINE_VERSION\s*=\s*(\d+)',abi_source,re.M).group(1))
if len(font)<48 or font[32:37]!=b'FNT1'+bytes([font_version]) or int.from_bytes(font[40:42],'little')!=len(font):
 raise ValueError('Invalid boot font module descriptor')
if int.from_bytes(font[44:46],'little'):
 raise ValueError('Bootstrap font must be self-contained: MAIN prerender reservation is not supported; use Rotate/Compact and install another engine in AUTOEXEC')
if len(font)>0x1480:raise ValueError('Boot font exceeds resident slot')
(out/'config.inc').write_text(f'DOS_BYTES = {(ROOT/"bin/BIOS.sys").stat().st_size}\nMODULE_SECTORS = {(len(module)+255)//256}\nBOOT_PROBE = {int(a.probe)}\n',encoding='utf-8')
symbols=dict(re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/BIOS.sym').read_text(),re.M))
with (out/'config.inc').open('a',encoding='utf-8') as f:
 f.write(f'BOOT_STAGE_BASE = 08000h\nBOOT_STAGE_LIMIT = 0{int(symbols["STACK_BOTTOM"],16):04X}h\nBOOT_MEM_ROM = 07000h\n')
 f.write(f'MEM_DATA_START = 0{int(symbols["MEM_DATA_START"],16):04X}h\n')
 f.write(f'BOOT_COLOR_PORT = 0{int(symbols["IO_COLOR"],16):04X}h\n')
 f.write(f'BANK_MAIN_CLIENT_BYTES = {int(symbols["BANK_MAIN_CLIENT_BYTES"],16)}\n')
 f.write(f'BANK_MAIN_LOCAL_BYTES = {int(symbols["BANK_MAIN_LOCAL_BYTES"],16)}\n')
 f.write(f"BOOT_IO_BYTES = {io['bytes']}\nBOOT_IO_KEYS = {io['bankAddress']}\nBOOT_IO_KEY_BYTES = {io['bankBytes']}\nBOOT_IO_ROM = 06000h\n")
 for alias,key in [('BOOT_IO_RAM','BIOS_IO_ADDR'),('BOOT_LOCAL_BLOCK','MEM_IMAGE_LOCAL_START'),('BOOT_MEM_KIND','MEM_BANK_KIND'),('BOOT_DOS_STACK_TOP','DOS_BANK_STACK_TOP'),('BOOT_DOS_STACK_BOTTOM','DOS_BANK_STACK_BOTTOM'),('BOOT_BIOS_STACK_TOP','BIOS_BANK_STACK_TOP')]:
  f.write(f'{alias} = 0{int(symbols[key],16):04X}h\n')
 f.write(f'BOOT_BIOS_STACK_FLOOR = 0{int(symbols["BIOS_BANK_STACK_BOTTOM"],16)+64:04X}h\n')
 f.write(f'BOOT_FONT_ROM = 04820h\nBOOT_FONT_BYTES = {len(font)}\n')
 for alias,key in [('BOOT_FONT_RAM','FONT_ENGINE_BASE')]:
  f.write(f'{alias} = 0{int(symbols[key],16):04X}h\n')
 name=(a.font_name or font_path.stem).encode('ascii')
 if not 1<=len(name)<=8:raise ValueError('Boot font basename must fit FAT 8.3')
 f.write(f'BOOT_FONT_NAME_RAM = 0{int(symbols["io_vars.fontName"],16):04X}h\n')
 f.write('    DEFINE BOOT_SELECTED_BASENAME '+','.join(str(n) for n in name.ljust(8,b' '))+'\n')
 f.write(f'BIOS_BOOT_LOAD_DOS = 0{int(symbols["biosLoadDosImage"],16):04X}h\n')
ts=dict(re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/banked/TRANSPORT.sym').read_text(),re.M))
with (out/'config.inc').open('a',encoding='utf-8') as f:
 for alias,key in [('BIOS_CHECK_RAMD','checkRAMD'),('BIOS_PAGE_COUNT','bios_upper.ramPageCount'),('BIOS_BOOT_PAGES','biosBootPages'),('BIOS_BOOT_WARM','biosBootWarm'),('BIOS_BANKED','v_dosBanked'),('BIOS_DOS_READY','v_dosModuleReady')]:
  f.write(f'{alias} = 0{int(symbols[key],16):04X}h\n')
 for alias,key in [('BANK_PAGE','bankPage'),('BANK_PAGE_COUNT','bankPageCount'),('BANK_SERVICE_SP','bankServiceSP'),('BANK_STACK_FLOOR','bankStackFloor')]:
  f.write(f'{alias} = 0{int(ts[key],16):04X}h\n')
ks=dict(re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/banked/KERNEL.sym').read_text(),re.M))
with (out/'config.inc').open('a',encoding='utf-8') as f:
 f.write(f'BOOT_MEM_LOCAL_A = 0{int(symbols["MEM_LOCAL_A"],16):04X}h\n')
 f.write(f'BOOT_MEM_NEAR_COPY = 0{int(symbols["MEM_NEAR_COPY"],16):04X}h\n')
 f.write(f'BOOT_DOS_NEAR_COPY = 0{int(ks["memcpy_bc_hl"],16):04X}h\n')
 f.write(f'BOOT_DOS_LOCAL_A = 0{int(ks["dosSystemDriver"],16):04X}h\n')
 f.write(f'DOS_READY = 0{int(ks["v_dosModuleReady"],16):04X}h\n')
 f.write(f'BOOT_RAM_FRESH = 0{int(ks["memoryRamFresh"],16):04X}h\n')
 f.write(f'BOOT_MEM_REVISION_ADDR = 0{int(symbols["MEM_RUNTIME_REVISION_ADDR"],16):04X}h\n')
 f.write(f'BOOT_MEM_REVISION = {int(symbols["MEM_RUNTIME_REVISION"],16)}\n')
 f.write(f'MODULE_LOAD = 0{int(ks["DOS_BANK_CODE_ADDR"],16):04X}h\n')
ms=dict(re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/MEM.sym').read_text(),re.M))
with (out/'config.inc').open('a',encoding='utf-8') as f:
 for part in ('TRANSPORT','CLIENT','LOCAL','WINDOW','DATA'):
  for field in ('OFFSET','SIZE'):
   key=f'MEM_{part}_{field}'
   f.write(f'{key} = {int(ms[key],16)}\n')
subprocess.run(['python', str(ROOT/'scripts/build/build-boot-splash.py')], check=True)
for name,source,defines in [('BOOT','BOOTBANK',[]),('legacy' ,'SECTOR',[]),('sector','SECTOR',['-DSTAGED_BOOT=1'])]:
 if name=='sector':
  (out/'sector-config.inc').write_text(f'BOOT_COLOR_PORT = 0{int(symbols["IO_COLOR"],16):04X}h\nBOOT_STAGE_BASE = 08000h\nBOOT_STAGE_BYTES = {(out/"BOOT.bin").stat().st_size}\n',encoding='utf-8')
 subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',*defines,f'--raw={out/name}.bin',f'--sym={out/name}.sym',f'--lst={out/name}.lst',str(ROOT/f'source/bootstrap/{source}.asm')],check=True)
assert (out/'legacy.bin').read_bytes()==(ROOT/'assets/system-rom/boot-system.bin').read_bytes()
