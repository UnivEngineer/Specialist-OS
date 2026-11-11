"""C SDK: адреса публичных служб совпадают с собранным ABI mxos.inc."""
from pathlib import Path
import re
import runpy
R=Path(__file__).resolve().parents[2]
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/DOS.sym').read_text(encoding='utf-8'),re.M)}
macros={k:int(v,16) for k,v in re.findall(r'^#define MXOS_BIOS_ABI_(\w+) (0x[0-9A-Fa-f]+)',(R/'sdk/include/mxos.h').read_text(encoding='utf-8'),re.M)}
mapping=dict(MEMCPY='memcpy_bc_hl',GETCH='getch',PUTCH='printChar',INPUT='input',PUTS='printString',KEY_SCAN='keyScan',PRINT_HEX8='printHexByte',PRINT_HEX16='printHexWord',CURSOR_GET='getCursorPos',CURSOR_SET='setCursorPos',CHECKSUM='calcCS',MEMTOP_SET='setMemTop',MEMTOP_GET='getMemTop',FILE_LIST='fileList',DRIVE='fileGetSetDrive',FILE_CREATE='fileCreate',FILE_DELETE='fileDelete',FILE_RENAME='fileRename',FILE_STAT='fileLoadInfo',FILE_ATTR='fileGetSetAttr',FILE_NAME='fileNamePrepare',FILE_LOAD='fileLoad2',DISK_DRIVER='diskDriver',PRINT_DEC='printDecWord',UPPER='upperCase',STRCMP='strcmp',STRICMP='stricmp',FLUSH='flushVolume',ARG_NEXT='argNext',DRIVE_SELECT='fileSelectDrive',FILE_COPY='fileCopy',ERROR='getError',SECTOR='sectorIO')
mapping['PUTS_REAL']='printStringReal'
mapping.update(PRINT_CHAR_RAW='printCharRaw',PRINT_STRING_Z='printStringZ',PRINT_STRING_RAW_Z='printStringRawZ',PRINT_STRING_RAW_LIM='printStringRawLim')
mapping['DIRECTORY']='directory'
mapping['CLI']='cli'
mapping['SHELL']='setShell'
mapping['INPUT_HOOK']='setInputHook'
mapping['RAMD_DRIVER']='setRAMDDriver'
for mode,value in dict(RESTORE=0,BYTE=1,REGISTER=2,BLOCK=3,USER=4).items():
 assert s['RAMD_DRIVER_'+mode]==value
 assert f'#define MXOS_RAMD_DRIVER_{mode} {value}' in (R/'sdk/include/mxos.h').read_text(encoding='utf-8')
assert s['MAX_PATH']==128
assert '#define MXOS_MAX_PATH 128' in (R/'sdk/include/mxos.h').read_text(encoding='utf-8')
assert macros.keys()==mapping.keys(),(macros.keys()-mapping.keys(),mapping.keys()-macros.keys())
for name,value in macros.items():assert value==s['bios_'+mapping[name]],(name,value,s['bios_'+mapping[name]])
for name,value in dict(FILE_HANDLE=64,FILE_DESCR_SIZE=32,FILE_INFO=16,DISK_INFO=48,FILE_COPY_REQUEST=86,FILE_PATH_IO=40).items():assert s[name]==value,(name,s[name],value)
runpy.run_path(str(R/'scripts/build/build-syscalls.py'))['generate'](check=True)
for name,value in dict(mx_call=0xF800,mx_cpmCall=0xF803,mx_callRegs=0xF806,bios_fileLoadPath=0xF809,bios_fileSavePath=0xF80C).items():assert s[name]==value
errors=re.findall(r'^#define MXOS_ERR_(\w+) (\d+)',(R/'sdk/include/mxos_file.h').read_text(encoding='utf-8'),re.M)
for name,value in errors:assert int(value)==s['DOS_ERR_'+name]
print('C SDK ABI PASS',len(macros),'addresses;',len(errors),'error codes')

assert s['MEM_COPY_REQUEST']==8
assert s['mx_memcpyFar']==0xF80F and s['mx_memInfo']==0xF812
assert s['MEM_ABI_VERSION']==0x101
assert 'call 0F80Fh' in (R/'sdk/src/mxos_z88dk.asm').read_text(encoding='utf-8')
print('MEM ABI PASS: FAR copy packet and common entry points')

# Единый контракт операций памяти ASM/C, включая ошибки и возможности.
header=(R/'sdk/include/mxos.h').read_text(encoding='utf-8')
cm={k:int(v,0) for k,v in re.findall(r'^#define (MXOS_(?:MEM_\w+|ALLOC_\w+)) (0x[0-9A-Fa-f]+|\d+)\s*$',header,re.M)}
for name in ('MOVE','GET8','GET16','PUT8','PUT16','PRINT','ALLOC','FREE','REALLOC','ARENA','RAMLIMIT','FILL','QUERY','DISKCLAIM','RAMDISK'):
 assert cm['MXOS_MEM_OP_'+name]==s['MEM_OP_'+name],name
for name in ('BANK','STACK','RANGE','NOMEM','OWNER','HANDLE','ACTIVE'):
 assert cm['MXOS_MEM_ERR_'+name]==s['MEM_ERROR_'+name],name
for name in ('COPY','LAZY_WINDOW','MEMORY_OPS','MODULES','ALLOCATOR','DWORD'):
 assert cm['MXOS_MEM_CAP_'+name]==s['MEM_CAP_'+name],name
assert cm['MXOS_MEM_SERVICE']==s['mx_memory']==0xF815
assert cm['MXOS_MEM_GET32']==s['mx_get32']==0xF7A8
assert cm['MXOS_MEM_PUT32']==s['mx_put32']==0xF7AB
assert s['MXOS_BIOS_PRINT_STRING_BANK']==s['MXOS_BIOS_PRINT_STRING']==0x107
assert s['MXOS_BIOS_PRINT_STRING_MAIN']==0x124
assert s['MXOS_BIOS_FONT_RENDER']==0x125
for name in ('FONT_PRERENDER_BYTES','FONT_RENDER_QUERY','FONT_RENDER_BUILD','FONT_RENDER_DISABLE'):
 assert re.search(r'^#define MXOS_'+name+r' '+str(s[name])+r'$',header,re.M),name
assert s['MXOS_DOS_MEMORY']==0x225
print('MEM operations, capabilities, errors and string routes ASM/C PASS')

assert s["MXOS_DOS_MODULE_LOAD"]==0x226 and s["MXOS_DOS_MODULE_CALL"]==0x227 and s["MXOS_DOS_MODULE_UNLOAD"]==0x228

assert s["MODULE_TARGET_MAIN"]==0 and s["MODULE_TARGET_BANK"]==1
assert "#define MXOS_MODULE_MAIN 0" in header and "#define MXOS_MODULE_BANK 1" in header

assert s["MXOS_DOS_FILE_EXEC_NATIVE"]==0x229
for name,value in dict(DRIVER=1,BANK=2,MAIN=4).items():
 assert s["IMAGE_FLAG_"+name]==value
 assert f"#define MXOS_IMAGE_FLAG_{name} 0x{value:02X}" in header

# Removed development-only ABI entries must not return as compatibility aliases.
for removed in ('bios_reservedC88D','bios_loadDosModule','bios_releaseDosModule',
                'MXOS_DOS_RESERVEDC88D','MXOS_BIOS_LOADDOSMODULE','MXOS_BIOS_RELEASEDOSMODULE'):
 assert removed not in s,removed
image=(R/'bin/BIOS.sys').read_bytes()
assert image[0x88D:0x890]==bytes(3)
assert image[0x8D8:0x8DE]==bytes(6)
print('Removed BIOS development entries absent; fixed-address gaps preserved')
