"""8080: NUL завершает документ, FFh сохраняется при просмотре и правках."""
from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[2];O=R/'docs/editor-zero-049/cpu';O.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/E.sym').read_text(),re.M)}
W=lambda n:n.to_bytes(2,'little');base=bytearray(65536)
for name,addr in [('E.i80',0),('BIOS.sys',0xc000),('IO.sys',0xf000)]:
 data=(R/'bin'/name).read_bytes();base[addr:addr+len(data)]=data
font=(R/'assets/system-rom/FONTKOI.FNT').read_bytes();base[s['FONT_ADDR']:s['FONT_ADDR']+len(font)]=font
base[s['bios_vars.charGen']:s['bios_vars.charGen']+2]=W(s['FONT_ADDR']//8)
rows=0
def run(memory,entry,hl=0,de=0,bc=0,end=0x8c20+25):
 global rows
 stub=b'\x31'+W(s['STACK_ADDR'])+b'\x21'+W(hl)+b'\x11'+W(de)+b'\x01'+W(bc)+b'\xcd'+W(s[entry])+b'\x22\x00\x8c'
 # LD (8C00),HL; 8080 HALT после записи регистров через PUSH.
 stub=stub+b'\xf5\xc1\x60\x69\x22\x02\x8c\x76'
 memory[0x8c20:0x8c20+len(stub)]=stub;(O/'in.bin').write_bytes(memory)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8c20','-end',f'{end:x}','-counter','3000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True)
 got=(O/'out.bin').read_bytes();assert int.from_bytes(got[65542:65544],'little')==end,(entry,got[65536:].hex());rows+=1;return got
for payload in [b'A\xffB\rC\xffD\0',b'\xff\0',b'\0']:
 m=bytearray(base);m[0x1800:0x1800+len(payload)]=payload;m[s['v_textLimitLo']:s['v_textLimitLo']+2]=W(0x8000)
 g=run(m,'scanDocument',end=s['clearAndRedrawDocument']);expected=payload[:-1] if len(payload)>1 else b'\r'
 assert g[0x1800:0x1800+len(expected)+1]==expected+b'\0'
 assert int.from_bytes(g[s['v_textEnd']:s['v_textEnd']+2],'little')==0x1800+len(expected)
m=bytearray(base);m[0x1800:0x1808]=b'A\xffB\rC\xffD\0'
g=run(m,'nextLine',hl=0x1800,end=0x8c20+25);assert int.from_bytes(g[0x8c00:0x8c02],'little')==0x1804
# Буфер строки должен включать FFh, а не заканчивать копирование на нём.
m[s['v_currentLine']:s['v_currentLine']+2]=W(0x1800)
g=run(m,'beginLineEdit',end=0x8c20+25);assert g[s['startupBannerAndLineBuffer']:s['startupBannerAndLineBuffer']+3]==b'A\xffB'
# Подсчёт размера для диска исключает NUL и включает оба FFh.
g=run(m,'getCRC',hl=0x1800,end=s['buildSaveDescriptor']);assert int.from_bytes(g[65548:65550],'little')==7
# Добавление файла ставит нуль строго за точным N, не трогая payload.
m[s['bios_fileLoad2']:s['bios_fileLoad2']+2]=b'\xb7\xc9';m[s['fileDescSize']:s['fileDescSize']+2]=W(7)
g=run(m,'loadTextFile',de=0x1800,end=0x8c20+25);assert g[0x1800:0x1808]==b'A\xffB\rC\xffD\0'
# Сдвиг хвоста в обе стороны переносит FFh как обычные байты и сохраняет NUL.
m=bytearray(base);payload=b'A\xffB\rC\xffD\r\0';m[0x1800:0x1809]=payload
m[s['v_textEnd']:s['v_textEnd']+2]=W(0x1808);m[s['v_textLimitLo']:s['v_textLimitLo']+2]=W(0x8000)
g=run(m,'resizeTextRange',hl=0x1804,de=0x1805);assert g[0x1805:0x180a]==payload[4:]
g=run(bytearray(g[:65536]),'resizeTextRange',hl=0x1805,de=0x1804);assert g[0x1800:0x1809]==payload
# Чтение с ленты ограничено длиной заголовка, FFh в середине не обрывает его.
for payload in [b'A\xffB\rC\xffD\r',b'']:
 m=bytearray(base);m[0x5500:0x5500+len(payload)]=payload;m[0x5400:0x5402]=W(0x5500)
 reader=b'\xe5\x2a\x00\x54\x7e\x23\x22\x00\x54\xe1\xc9'
 m[0x5000:0x5000+len(reader)]=reader;m[s['readTapeByte']:s['readTapeByte']+3]=b'\xc3'+W(0x5000)
 m[s['readTapeWord']:s['readTapeWord']+4]=b'\x01'+W(sum(payload))+b'\xc9'
 start=s['readTapeTextLoop'] if payload else s['readTapeTextDone']
 m[0x5300:0x5305]=b'\xd1\xe5\xc3'+W(start);s['tapeFixture']=0x5300
 g=run(m,'tapeFixture',hl=0x1800,bc=len(payload),end=s['keyHandler_Esc_Home']);assert g[0x1800:0x1800+len(payload)+1]==payload+b'\0'
 assert int.from_bytes(g[s['v_textEnd']:s['v_textEnd']+2],'little')==0x1800+len(payload)
print('EDITOR NUL PASS',rows)
