"""C SDK: адреса публичных служб совпадают с собранным ABI mxos.inc."""
from pathlib import Path
import re
import runpy
R=Path(__file__).resolve().parents[2]
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/DOS.sym').read_text(encoding='utf-8'),re.M)}
header=(R/'sdk/include/mxos.h').read_text(encoding='utf-8')
macros={k:int(v,16) for k,v in re.findall(r'^#define MXOS_BIOS_ABI_(\w+) (0x[0-9A-Fa-f]+)',header,re.M)}
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
for key in list(mapping):
 if key not in macros: del mapping[key]
assert macros.keys()==mapping.keys(),(macros.keys()-mapping.keys(),mapping.keys()-macros.keys())
for name,value in macros.items():assert value==s['bios_'+mapping[name]],(name,value,s['bios_'+mapping[name]])
for name,value in dict(FILE_HANDLE=64,FILE_DESCR_SIZE=32,FILE_INFO=16,DISK_INFO=48,FILE_COPY_REQUEST=86,FILE_PATH_IO=40).items():assert s[name]==value,(name,s[name],value)
runpy.run_path(str(R/'scripts/build/build-syscalls.py'))['generate'](check=True)
for name,value in dict(mx_call=0xF800,mx_cpmCall=0xF803,mx_callRegs=0xF806).items():assert s[name]==value
errors=re.findall(r'^#define MXOS_ERR_(\w+) (\d+)',(R/'sdk/include/mxos_file.h').read_text(encoding='utf-8'),re.M)
for name,value in errors:assert int(value)==s['DOS_ERR_'+name]
print('C SDK ABI PASS',len(macros),'addresses;',len(errors),'error codes')

assert s['IO_KEY_STATE'] == 11
keyboard_wrappers = (R/'sdk/src/mxos_z88dk.asm').read_text(encoding='utf-8')
for wrapper, entry in [('getch','io_bankGetCh'), ('key_poll','io_bankKeyPoll'), ('getch_cursor','io_bankGetChCursor'),
                       ('key_modifiers','io_bankKeyModifiers'), ('key_shift','io_bankKeyShift')]:
    assert re.search(rf'^_mxos_bank_{wrapper}:\s+call 0{s[entry]:04X}h$',keyboard_wrappers,re.M), entry
for name in ('KEY_MOD_SHIFT','KEY_MOD_CTRL'):
    assert int(re.search(rf'^#define MXOS_{name}\s+(\d+)$',header,re.M)[1]) == s['IO_'+name]

assert s['MEM_COPY_REQUEST']==8
assert s['mx_memcpyFar']==0xF80F and s['mx_memInfo']==0xF812
assert s['MEM_ABI_VERSION']==int(re.search(r'^#define MXOS_MEM_ABI_VERSION (0x[0-9A-Fa-f]+)',(R/'sdk/include/mxos.h').read_text(encoding='utf-8'),re.M).group(1),16)
assert 'call 0F80Fh' in (R/'sdk/src/mxos_z88dk.asm').read_text(encoding='utf-8')
print('MEM ABI PASS: FAR copy packet and common entry points')

# Единый контракт операций памяти ASM/C, включая ошибки и возможности.
cm={k:int(v,0) for k,v in re.findall(r'^#define (MXOS_(?:MEM_\w+|ALLOC_\w+)) (0x[0-9A-Fa-f]+|\d+)\s*$',header,re.M)}
for name in ('MOVE','GET8','GET16','PUT8','PUT16','PRINT','ALLOC','FREE','REALLOC','ARENA','RAMLIMIT','FILL','QUERY','DISKCLAIM','RAMDISK','RESIDENT','MAINCLAIM','MAINREGISTER'):
 assert cm['MXOS_MEM_OP_'+name]==s['MEM_OP_'+name],name
for name in ('BANK','STACK','RANGE','NOMEM','OWNER','HANDLE','ACTIVE'):
 assert cm['MXOS_MEM_ERR_'+name]==s['MEM_ERROR_'+name],name
for name in ('COPY','LAZY_WINDOW','MEMORY_OPS','MODULES','ALLOCATOR'):
 assert cm['MXOS_MEM_CAP_'+name]==s['MEM_CAP_'+name],name
assert cm['MXOS_MEM_SERVICE']==s['mx_memory']==0xF815
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
# The linker can place private code in the former slots.
for released in (0xC88D,0xC8D8,0xC8DB):
 assert released not in macros.values()
print('Removed BIOS development entries absent from ASM and C')

assert s['FILEOPS_JOB']==112
assert s['FILEOPS_JOB.request']==26 and s['FILEOPS_JOB.phase']==13
assert s['FILEOPS_JOB.total']==16 and s['FILEOPS_JOB.path']==22
for name,value in dict(BEGIN=0,STEP=1,ANSWER=2,CANCEL=3,CLOSE=4,SKIP_PREPARE=5,NO_PREPARE=1,PHASE_COPY=0,PHASE_PREPARE=1,PHASE_DELETE=2,DELETE=2,COPY=0,MOVE=1,RUNNING=0,CONFIRM=1,DONE=2,FAILED=3).items():
 assert f'#define MXOS_FILEOPS_{name} {value}' in header
 assert s['FILEOPS_'+name]==value
print('FILEOPS ASM/C job layout and operations PASS')

assert s['MXOS_DOS_ENUMERATE']==0x22c
assert s['ENUM_REQUEST']==12 and s['ENUM_ENTRY']==160
for name,value in dict(OPEN=0,NEXT=1,CLOSE=2,RECURSIVE=1,CHECK_CANCEL=2).items():
 assert s['ENUM_'+name]==value
 assert f'#define MXOS_ENUM_{name} {value}' in header
print('ENUM ASM/C request, record layout and operations PASS')

for name,value in dict(RETAIN=0,FIND=1,ENUM=2,SHELL_SET=3,SHELL_RESUME=4,
                       DIRECTORY_EPOCH=5,RELEASE=6,EPOCH_ALL=254).items():
 assert s['RESIDENT_'+name]==value
 assert int(re.search(r'^#define MXOS_RESIDENT_'+name+r' (\S+)',header,re.M)[1],0)==value
print('Resident shell operations ASM/C PASS')

assert s['IMAGE_HEADER_SIZE']==32 and s['MODULE_RECORD_SIZE']==16
assert '#define MXOS_IMAGE_HEADER_SIZE 32' in header
assert s['MXOS_DOS_FONT_CONFIG']==0x22d
for name,value in dict(EXCLUSIVE=0,PRESERVE=1,MANAGED=2).items():assert f'#define MXOS_IMAGE_POLICY_{name} {value}' in header
for name,value in dict(AUTO=0,MX=1,STD=2,MON=3,RF=4).items():assert f'#define MXOS_IMAGE_MODE_{name} {value}' in header
print('EXE v2, module record and FONT restoration ABI PASS')

assert s['mx_queueBind']==0xf809 and s['mx_queueFlush']==0xf80c
assert s['MXOS_BIOS_COMMAND_BATCH']==0x12c
assert s['MEM_CAP_COMMAND_QUEUE']==32 and '#define MXOS_MEM_CAP_COMMAND_QUEUE 32' in header
assert '#define MXOS_QUEUE_BIND_ADDR 0xF809' in header and '#define MXOS_QUEUE_FLUSH_ADDR 0xF80C' in header
assert s['mx_routeCall']==0xf818 and s['MXOS_DOS_MODULE_SCOPE']==0x22e and s['MXOS_DOS_COMMAND_BATCH']==0x22f
print('MEM 1.3 queue, direct DLL and BIOS/DOS batch entries PASS')
