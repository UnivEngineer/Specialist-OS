"""8080 timer startup with delayed 8253 reload and millisecond formatting."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'.local/sortbench-timing'


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    symbols = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',
               (ROOT/'bin/SORTBEN.sym').read_text(encoding='utf-8'),re.M)}
    image = (ROOT/'bin/SORTBEN.i80').read_bytes()
    word = lambda x:x.to_bytes(2,'little')

    def run(memory,code):
        memory[0x8000:0x8000+len(code)] = code
        source,destination = OUT/'input.bin',OUT/'output.bin'
        source.write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080','-pc','8000','-end','80ff','-counter','2000000',
                        '-output',str(destination),str(source)],check=True,capture_output=True)
        result = destination.read_bytes()
        assert int.from_bytes(result[65542:65544],'little')==0x80ff
        return result

    def fresh():
        memory = bytearray(65536)
        memory[:len(image)] = image
        return memory

    cases = 0
    for module in ('raw','upper'):
        for method,divider in (('benchTimerInit',1),('benchTimerInit',16),
                               ('pitTimerInit',1),('pitTimerWaitReady',1),
                               ('pitTimerWaitReady',16)):
            for samples in ((59000,59000,0,65535),
                            (12000,12000,65535,65534),
                            (0,0,65535,65534),
                            (65535,65535,65534,65533),
                            (59000,)*257):
                memory = fresh()
                memory[symbols[f'{module}.benchDivisorHigh']] = divider
                reader = symbols[f'{module}.pitTimerRead']
                memory[reader:reader+3] = b'\xc3\x00\x82'
                # Return scripted HL while preserving BC/DE, like pitTimerRead.
                stub = bytes.fromhex('2a 00 84 7e 23 f5 7e 23 22 00 84 67 f1 6f c9')
                memory[0x8200:0x8200+len(stub)] = stub
                data = b''.join(word(x) for x in samples)
                memory[0x8500:0x8500+len(data)] = data
                memory[0x8400:0x8402] = word(0x8500)
                code = (bytes.fromhex('31 00 8b 01 34 12 11 78 56 cd')+
                        word(symbols[f'{module}.{method}'])+
                        bytes.fromhex('f5 e1 22 40 84 60 69 22 42 84 eb 22 44 84 c3 ff 80'))
                result = run(memory,code)
                assert int.from_bytes(result[0x8400:0x8402],'little')==0x8500+len(data),(module,method,divider)
                assert bool(result[0x8440]&1)==(len(samples)==257),(module,method,divider)
                assert bool(result[0x8440]&0x40)==(len(samples)==257),(module,method,divider)
                assert result[0x8442:0x8446]==bytes.fromhex('34 12 78 56'),(module,method,divider)
                cases += 1

    for ticks in (0,1,945,32768,59109,65535):
        for divider in (1,16):
            memory = fresh()
            memory[symbols['bios_printStringZ']] = 0xc9
            code = (b'\x31\x00\x8b\x21'+word(ticks)+b'\x3e'+bytes([divider])+
                    b'\xcd'+word(symbols['milliseconds'])+b'\xc3\xff\x80')
            result = run(memory,code)
            end = int.from_bytes(result[symbols['reportEnd']:symbols['reportEnd']+2],'little')
            actual = result[symbols['REPORT']:end].decode()
            microseconds = ticks*divider*128
            assert actual==f'{microseconds//1000}.{microseconds%1000:03} ms',actual
            cases += 1
    print('PIT/SORTBEN TIMING PASS:',cases,'cases; init/wait, delayed reload, stopped clock, BC/DE, U32 formatting.')


if __name__=='__main__':
    main()
