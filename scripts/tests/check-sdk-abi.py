"""C SDK: адреса публичных служб совпадают с собранным ABI mxos.inc."""
from pathlib import Path
import re
R=Path(__file__).resolve().parents[2]
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/DOS.sym').read_text(encoding='utf-8'),re.M)}
macros={k:int(v,16) for k,v in re.findall(r'^#define MXOS_BIOS_(\w+) (0x[0-9A-Fa-f]+)',(R/'sdk/include/mxos.h').read_text(encoding='utf-8'),re.M)}
mapping=dict(MEMCPY='memcpy_bc_hl',GETCH='getch',PUTCH='printChar',INPUT='input',PUTS='printString',KEYSCAN='keyScan',PRINT_HEX8='printHexByte',PRINT_HEX16='printHexWord',CURSOR_GET='getCursorPos',CURSOR_SET='setCursorPos',CHECKSUM='calcCS',MEMTOP_SET='setMemTop',MEMTOP_GET='getMemTop',FILE_LIST='fileList',DRIVE='fileGetSetDrive',FILE_CREATE='fileCreate',FILE_DELETE='fileDelete',FILE_RENAME='fileRename',FILE_STAT='fileLoadInfo',FILE_ATTR='fileGetSetAttr',FILE_NAME='fileNamePrepare',FILE_LOAD='fileLoad2',DISK_DRIVER='diskDriver',PRINT_DEC='printDecWord',UPPER='upperCase',STRCMP='strcmp',STRICMP='stricmp',FLUSH='flushVolume',TIMER_INIT='timerInit',TIMER_READ='timerRead',UI_DRAW='uiDraw',UI_INVERSE='uiInverseRect',UI_COPY='uiCopyRect',ARG_NEXT='argNext',DRIVE_SELECT='fileSelectDrive',FILE_COPY='fileCopy',ERROR='getError',SECTOR='sectorIO',MODULE_RELEASE='releaseDosModule')
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
for name,value in dict(FILE_HANDLE=64,FILE_DESCR_SIZE=32,FILE_INFO=16,DISK_INFO=48,FILE_COPY_REQUEST=86).items():assert s[name]==value,(name,s[name],value)
errors=re.findall(r'^#define MXOS_ERR_(\w+) (\d+)',(R/'sdk/include/mxos_file.h').read_text(encoding='utf-8'),re.M)
for name,value in errors:assert int(value)==s['DOS_ERR_'+name]
print('C SDK ABI PASS',len(macros),'addresses;',len(errors),'error codes')
