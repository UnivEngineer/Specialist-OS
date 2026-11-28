"""C SDK: адреса публичных служб совпадают с собранным ABI mxos.inc."""
from pathlib import Path
import os
import json
import re
import runpy
import subprocess
import tempfile
R=Path(__file__).resolve().parents[2]
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/DOS.sym').read_text(encoding='utf-8'),re.M)}
header=(R/'sdk/include/mxos.h').read_text(encoding='utf-8')
ids_header=(R/'sdk/include/mxos_syscall_ids.h').read_text(encoding='utf-8')
assert 'MXOS_BIOS_ABI_' not in header+ids_header
assert 'bios_setRAMDDriver' not in s and 'MXOS_BIOS_SET_RAMD_DRIVER' not in s
assert s['MAX_PATH']==128
assert '#define MXOS_MAX_PATH 128' in header
c_entries={name:int(value,16) for name,value in re.findall(r'^#define MXOS_MEM_(\w+)\s+(0x[0-9A-Fa-f]+)$',header,re.M)}
for c_name, asm_name in dict(PRINT_CHAR='printChar',GET_CURSOR_POS='getCursorPos',SET_CURSOR_POS='setCursorPos',
                            PRINT_HEX_BYTE='printHexByte',PRINT_HEX_WORD='printHexWord',
                            GET_TOP='getMemTop',SET_TOP='setMemTop').items():
 assert c_entries[c_name]==s['mx_'+asm_name],c_name
for name,value in dict(FILE_HANDLE=64,FILE_DESCR_SIZE=32,FILE_INFO=16,DISK_INFO=48,FILE_COPY_REQUEST=86,FILE_PATH_IO=40).items():assert s[name]==value,(name,s[name],value)
runpy.run_path(str(R/'scripts/build/build-syscalls.py'))['generate'](check=True)
for name,value in dict(mx_callPacket=0xF860,mx_callCPM=0xF863,mx_callRegs=0xF866).items():assert s[name]==value
sdk_asm=(R/'sdk/src/mxos_syscall_ids.inc').read_text(encoding='utf-8')
direct={name:int(value,16) for name,value in re.findall(r'^defc (mx_\w+)\s*= 0([0-9A-F]{4})h$',sdk_asm,re.M)}
assert direct['mx_callPacket']==0xF860 and direct['mx_callRegs']==0xF866
assert 'mx_call' not in s and 'mx_call' not in direct
assert not re.search(r'\b(?:mxos_call|MXOS_CALL)\b',header)
for name,value in direct.items():
 assert s[name]==value,(name,value)
fast_entries=dict(mx_printChar=0xF87B,mx_getCursorPos=0xF87E,mx_setCursorPos=0xF881,
                  mx_printCharRaw=0xF884,mx_printHexByte=0xF887,mx_printHexWord=0xF88A,
                  mx_getMemTop=0xF88D,mx_setMemTop=0xF890)
for name,value in fast_entries.items():
 assert direct[name]==value,(name,direct[name],value)
for name in ('PRINT_CHAR','PRINT_HEX_BYTE','GET_CURSOR_POS','SET_CURSOR_POS',
             'PRINT_HEX_WORD','SET_MEM_TOP','GET_MEM_TOP','PRINT_CHAR_RAW'):
 service='MXOS_BIOS_'+name
 command='MXOS_COMMAND_BIOS_'+name
 assert s[service] == s[command]+0x100, name
assert 'MXOS_BIOS_PRINT_CHAR_REAL' not in s and 'MXOS_BIOS_PRINT_CHAR_REAL' not in ids_header
errors=re.findall(r'^#define MXOS_ERR_(\w+) (\d+)',(R/'sdk/include/mxos_file.h').read_text(encoding='utf-8'),re.M)
for name,value in errors:assert int(value)==s['DOS_ERR_'+name]
print('C SDK ABI PASS',len(c_entries),'MEM definitions;',len(errors),'error codes')

assert s['IO_KEY_STATE'] == 11
keyboard_wrappers = (R/'sdk/src/mxos_z88dk.asm').read_text(encoding='utf-8')
assert re.search(r'^\s+PUBLIC _mxos_call_packet$',keyboard_wrappers,re.M)
assert not re.search(r'\b_mxos_call\b',keyboard_wrappers)
for wrapper, entry in [('getch','io_bankGetCh'), ('key_poll','io_bankKeyPoll'), ('getch_cursor','io_bankGetChCursor'),
                       ('key_modifiers','io_bankKeyModifiers'), ('key_shift','io_bankKeyShift')]:
    assert re.search(rf'^_mxos_bank_{wrapper}:\s+call {entry}$',keyboard_wrappers,re.M), entry
    assert re.search(rf'^defc {entry}\s*= 0{s[entry]:04X}h$',sdk_asm,re.M), entry
for name in ('KEY_MOD_SHIFT','KEY_MOD_CTRL'):
    assert int(re.search(rf'^#define MXOS_{name}\s+(\d+)$',header,re.M)[1]) == s['IO_'+name]

assert s['MEM_COPY_REQUEST']==8
assert direct['mx_memcpyFar']==s['mx_memcpyFar']==0xF86F and s['mx_memInfo']==0xF872
assert s['MEM_ABI_VERSION']==int(re.search(r'^#define MXOS_MEM_ABI_VERSION (0x[0-9A-Fa-f]+)',(R/'sdk/include/mxos.h').read_text(encoding='utf-8'),re.M).group(1),16)
assert re.search(r'^_mxos_memcpy_far:\s+call mx_memcpyFar$',keyboard_wrappers,re.M)
assert re.search(r'^_mxos_memmove_far:\s+xor a\s+call mx_memory$',keyboard_wrappers,re.M)
for wrapper,entry in dict(putch='mx_printChar',cls='mx_printChar',memtop='mx_getMemTop',
                          set_memtop='mx_setMemTop',cursor_get='mx_getCursorPos',
                          cursor_set='mx_setCursorPos').items():
 assert re.search(rf'^_mxos_{wrapper}:\s+(?:ld c,\w+\s+)?jp {entry}$',keyboard_wrappers,re.M),wrapper
print('Direct register entries PASS:',len(fast_entries),'entry addresses, 6 C wrappers')
print('MEM ABI PASS: FAR copy packet and common entry points')

# Единый контракт операций памяти ASM/C, включая ошибки и возможности.
cm={k:int(v,0) for k,v in re.findall(r'^#define (MXOS_(?:MEM_\w+|ALLOC_\w+)) (0x[0-9A-Fa-f]+|\d+)\s*$',header,re.M)}
for name in ('MOVE','GET8','GET16','PUT8','PUT16','PRINT','ALLOC','FREE','REALLOC','ARENA','RAMLIMIT','FILL','QUERY','DISKCLAIM','RAMDISK','RESIDENT','MAINCLAIM','MAINREGISTER'):
 assert cm['MXOS_MEM_OP_'+name]==s['MEM_OP_'+name],name
for name in ('BANK','STACK','RANGE','NOMEM','OWNER','HANDLE','ACTIVE','UNSUPPORTED'):
 assert cm['MXOS_MEM_ERR_'+name]==s['MEM_ERROR_'+name],name
for name in ('COPY','MEMORY_OPS','MODULES','ALLOCATOR','COMMAND_QUEUE','BIOS_REGISTERS'):
 assert cm['MXOS_MEM_CAP_'+name]==s['MEM_CAP_'+name],name
assert cm['MXOS_MEM_SERVICE']==s['mx_memory']==0xF875
assert s['MXOS_BIOS_PRINT_STRING_BANK']==s['MXOS_BIOS_PRINT_STRING']==0x107
assert s['MXOS_BIOS_PRINT_STRING_MAIN']==0x124
assert s['MXOS_BIOS_FONT_RENDER']==0x125
for name in ('FONT_PRERENDER_BYTES','FONT_RENDER_QUERY','FONT_RENDER_BUILD','FONT_RENDER_DISABLE'):
 assert re.search(r'^#define MXOS_'+name+r' '+str(s[name])+r'$',header,re.M),name
assert s['MXOS_DOS_MEMORY']==0x225
for name,value in [('Z',0x124),('RAW_Z',0x127),('RAW_LIM',0x129),('LIM',0x12a)]:
 symbol='MXOS_BIOS_PRINT_STRING_'+name+'_MAIN'
 assert s[symbol]==value and f'#define {symbol} 0x{value:04X}' in ids_header
assert 'MXOS_BIOS_TEXT_PRINT' not in s and 'MXOS_TEXT_PRINT_REQUEST' not in header
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
public_asm=(R/'sdk/asm/abi/bios.inc').read_text(encoding='utf-8')
addresses={int(address,16) for address in re.findall(r'^\w+\s*=\s*0([0-9a-fA-F]{4})h',public_asm,re.M)}
assert not addresses.intersection({0xC88D,0xC8D8,0xC8DB})
assert not any(0xD700<=address<0xD78E for address in addresses)
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
for macro,symbol in [('MXOS_RELOC_IMAGE_HEADER_SIZE','IMAGE_RELOC_HEADER'),
                     ('MXOS_RELOC_IMAGE_VERSION','IMAGE_RELOC_VERSION'),
                     ('MXOS_RELOC_WORD16','IMAGE_RELOC_WORD16'),('MXOS_RELOC_HIGH8','IMAGE_RELOC_HIGH8')]:
 assert int(re.search(rf'^#define {macro} (\d+)$',header,re.M)[1])==s[symbol]

# Размер и смещения вычисляет целевой C-компилятор с реальным заголовком SDK.
checks={'sizeof(MXOS_IMAGE_HEADER)':s['IMAGE_HEADER_SIZE'],
        'sizeof(MXOS_RELOC_IMAGE_HEADER)':s['IMAGE_RELOC_HEADER'],
        'sizeof(MXOS_CALL_PACKET)':10}
for field,symbol in [('reloc_kind','MODE'),('align_log2','ALIGN'),('reloc_count','COUNT'),
                     ('table_checksum','CHECKSUM'),('reserved','RESERVED')]:
 checks[f'((char *)&hdr.{field} - (char *)&hdr)']=s['IMAGE_RELOC_'+symbol]
for offset,field in enumerate(('operation','hl','de','bc','af')):
 checks[f'((char *)&packet.{field} - (char *)&packet)']=2*offset
with tempfile.TemporaryDirectory() as temp:
 directory=Path(temp)
 source=directory/'layout.c'
 source.write_text('#include "mxos.h"\nstatic const char text[] = "A\\0B";\n'
     'int main(void) { MXOS_RELOC_IMAGE_HEADER hdr; mxos_u16 pit_base;\n'
     '  MXOS_CALL_PACKET packet;\n'+''.join(
     f'  if ({expr} != {value}) return {i};\n'
     for i,(expr,value) in enumerate(checks.items(),1))
     +'  if (mxos_pit_get_port(&pit_base) || pit_base != 0xffec) return 35;\n'
     +'  if (mxos_pit_read() != 0x4b4b) return 36;\n'
     +'  if (mxos_printStringLim(text, 0) != text) return 30;\n'
     +'  if (mxos_printStringLim(text, 3) != text+3) return 31;\n'
     +'  packet.operation = MXOS_BIOS_PRINT_STRING_LIM; packet.hl = (mxos_u16)text;\n'
     +'  packet.de = 0; packet.bc = 3; packet.af = 0;\n'
     +'  if ((mxos_call_packet(&packet) & MXOS_CF) || packet.hl != (mxos_u16)(text+3)) return 32;\n'
     +'  packet.operation = 0x01fe; packet.de = 0x5678; packet.af = 0x1234;\n'
     +'  if ((mxos_call_packet(&packet) & 0xff01) != ((MXOS_ERR_UNSUPPORTED << 8) | MXOS_CF)) return 33;\n'
     +'  if (packet.operation != 0x01fe || packet.hl != (mxos_u16)(text+3) ||\n'
     +'      packet.de != 0x5678 || packet.bc != 3 || packet.af != 0x1234) return 34;\n'
     +'  packet.operation = 0x0116; packet.de = 0x5678; packet.af = 0x1234;\n'
     +'  if ((mxos_call_packet(&packet) & 0xff01) != ((MXOS_ERR_UNSUPPORTED << 8) | MXOS_CF)) return 33;\n'
     +'  if (packet.operation != 0x0116 || packet.hl != (mxos_u16)(text+3) ||\n'
     +'      packet.de != 0x5678 || packet.bc != 3 || packet.af != 0x1234) return 34;\n'
     +'  return 0;\n}\n',encoding='ascii')
 z88dk=runpy.run_path(str(R/'scripts/build/build_cache.py'))['z88dk_directory']()
 environment={**os.environ,'ZCCCFG':str(z88dk/'lib/config'),
              'PATH':str(z88dk/'bin')+os.pathsep+os.environ['PATH']}
 subprocess.run([str(z88dk/'bin/zcc.exe'),'+cpm','-clib=8080','-compiler=sccz80',
                 '-I'+str(R/'sdk/include'),'-crt0='+str(R/'sdk/tests/file-crt'),'-zorg=8192',
                 str(source),str(R/'sdk/src/mxos_z88dk.asm'),'-o','layout.bin'],cwd=directory,env=environment,check=True)
 body=(directory/'layout_code_crt_init.bin').read_bytes()
 assert bytes([0xcd])+s['io_pitRead'].to_bytes(2,'little') in body, 'C: прямой CALL входа IO.SYS'
 memory=bytearray(65536)
 bios={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/BIOS.sym').read_text(encoding='utf-8'),re.M)}
 ramfos_start,ramfos_end=bios['MEM_BANK_CLIENT_START'],bios['mx_callPacket']
 assert ramfos_start==0xF800 and ramfos_end==0xF860
 ramfos_sentinel=bytes([0x76])*(ramfos_end-ramfos_start)
 memory[ramfos_start:ramfos_end]=ramfos_sentinel
 runpy.run_path(str(R/'scripts/tests/mem-image.py'))['install'](memory,bios,main=True)
 assert memory[ramfos_start:ramfos_end]==ramfos_sentinel
 for address in range(ramfos_end,bios['mx_setMemTop']+3,3):
  assert memory[address]==0xc3,hex(address)
 # CPU-стенд возвращает один байт из MMIO при каждом из двух чтений канала.
 memory[0xffee] = 0x4b
 # Штатные MAIN F860h и F866h вызывают bios_printStringLim. Перехватчик mon2_printCharOld считает байты.
 putch=bios['mon2_printCharOld']
 memory[putch:putch+3]=bytes.fromhex('c3 00 60')
 hook=bytes.fromhex('f5 79 32 83 5e 3a 82 5e 3c 32 82 5e f1 c9')
 memory[0x6000:0x6000+len(hook)]=hook
 memory[0x2000:0x2000+len(body)]=body
 memory[0x5e80:0x5e82]=b'\xff\xff'
 (directory/'input.bin').write_bytes(memory)
 subprocess.run([str(z88dk/'bin/z88dk-ticks.exe'),'-m8080','-pc','2000','-end','6100',
                 '-counter','100000','-output','result.bin','input.bin'],cwd=directory,check=True,capture_output=True)
 result=(directory/'result.bin').read_bytes()
 assert result[65542:65544]==b'\x00\x61' and result[0x5e80:0x5e82]==b'\0\0','v3 C layout'
 assert result[0x5e82:0x5e84]==b'\x06B','sccz80 printStringLim via F860h and F866h'
print('EXE v2/v3, C packet calls and layout, module record and FONT restoration ABI PASS')

assert s['mx_queueBind']==0xF869 and s['mx_queueFlush']==0xF86C
assert s['MXOS_BIOS_COMMAND_BATCH']==0x12c
assert s['MEM_CAP_COMMAND_QUEUE']==32 and '#define MXOS_MEM_CAP_COMMAND_QUEUE 32' in header
assert '#define MXOS_QUEUE_BIND_ADDR 0xF869' in header and '#define MXOS_QUEUE_FLUSH_ADDR 0xF86C' in header
assert s['mx_routeCall']==0xF878 and s['MXOS_DOS_MODULE_SCOPE']==0x22e and s['MXOS_DOS_COMMAND_BATCH']==0x22f
print('MEM 1.3 queue, direct DLL and BIOS/DOS batch entries PASS')
