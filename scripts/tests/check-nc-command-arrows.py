"""8080: NC returns vertical keys, preserves prompt text and edit position."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'.local/nc-command-arrows-cpu'


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    n = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
                                        (ROOT/'bin/NC.sym').read_text(encoding='utf-8'),re.M)}
    word = lambda x:x.to_bytes(2,'little')
    memory = bytearray(65536)
    for address,name in ((0,'NC.i80'),(0xc000,'BIOS.sys'),(0xf000,'IO.main.bin')):
        data = (ROOT/'bin'/name).read_bytes()
        memory[address:address+len(data)] = data
    for name in ('ncInputHookBegin','ncInputHookEnd','ncPrintField','ncPrintChar'):
        memory[n[name]] = 0xc9
    memory[n['bios_getch']:n['bios_getch']+3] = bytes.fromhex('c3 00 85')
    reader = bytes.fromhex('2a 00 86 7e 23 22 00 86 b7 c9')
    memory[0x8500:0x8500+len(reader)] = reader
    buffer = n['vars.cmdLine']
    memory[buffer:buffer+6] = b'ECHO \0'
    memory[n['vars.cmdLineEnd']:n['vars.cmdLineEnd']+2] = word(buffer+5)
    memory[n['vars.cmdTextPos']:n['vars.cmdTextPos']+2] = bytes([237,8])
    memory[n['vars.cmdCapacity']] = 50

    def run(state,keys,entry='ncReadKey',flags=15):
        state = bytearray(state)
        state[0x8600:0x8602] = word(0x8700)
        state[0x8700:0x8700+len(keys)] = bytes(keys)
        code = (b'\x31\x00\x8b\x21'+word(buffer)+b'\x11'+word(buffer+5)+
                b'\x01'+word((flags<<8)+50)+b'\xcd'+word(n[entry])+
                bytes.fromhex('f5 e1 22 20 86 c3 ff 80'))
        state[0x8000:0x8000+len(code)] = code
        source,destination = OUT/'input.bin',OUT/'output.bin'
        source.write_bytes(state)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080','-pc','8000','-end','80ff','-counter','2000000',
                        '-output',str(destination),str(source)],check=True,capture_output=True)
        result = destination.read_bytes()
        assert int.from_bytes(result[65542:65544],'little')==0x80ff
        return bytearray(result[:65536])

    for key,home_end in ((0x19,0),(0x1a,5)):
        result = run(memory,[8,8,key,13],entry='lineEdit',flags=1)
        assert result[n['lineEditPosition']]==home_end
        assert result[0x8621]==13
    result = run(memory,[8,8,0x1a])
    assert result[0x8621]==0x1a and not result[0x8620]&1
    for key in (0x19,0x1a,0x19):
        result = run(result,[key])
        assert result[0x8621]==key and not result[0x8620]&1
        assert result[n['lineEditPosition']]==3
        assert result[buffer:buffer+6]==b'ECHO \0'
    result = run(result,[ord('X'),13])
    assert result[buffer:buffer+7]==b'ECHXO \0'
    long_text = b'X'*70
    state = bytearray(memory)
    state[buffer:buffer+71] = long_text+b'\0'
    state[n['vars.cmdLineEnd']:n['vars.cmdLineEnd']+2] = word(buffer+70)
    result = run(state,[8,8,8,0x1a])
    offset = result[n['lineEditOffset']]
    assert offset>0 and result[n['lineEditPosition']]==67
    result = run(result,[0x19])
    assert result[n['lineEditOffset']]==offset and result[n['lineEditPosition']]==67
    assert result[buffer:buffer+71]==long_text+b'\0'
    print('NC COMMAND ARROWS CPU PASS: vertical return, edit resume, insertion position, long-line offset, dialog Home/End.')


if __name__=='__main__':
    main()
