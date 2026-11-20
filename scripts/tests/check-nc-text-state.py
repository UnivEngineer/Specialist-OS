"""8080 text-state serialization, bounded decimal output and saved leaf lookup."""
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'.local/nc-text-state-cpu'
W=lambda value:int(value).to_bytes(2,'little')


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    sy=lambda path:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',path.read_text('utf-8-sig'),re.M)}
    n=sy(ROOT/'bin/NC.sym')
    memory=bytearray(65536)
    for address,file in [(0,'NC.i80'),(0xc000,'BIOS.sys'),(0xf000,'IO.main.bin')]:
        data=(ROOT/'bin'/file).read_bytes();memory[address:address+len(data)]=data

    def call(base,entry,a=0,de=0,hl=0):
        result=bytearray(base)
        code=b'\x31'+W(0x8b00)+b'\x3e'+bytes([a])+b'\x11'+W(de)+b'\x21'+W(hl)+b'\xcd'+W(n[entry])+b'\xc3'+W(0x81ff)
        result[0x8100:0x8100+len(code)]=code
        (OUT/'input.bin').write_bytes(result)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080','-pc','8100','-end','81ff','-counter','30000000',
                        '-output',str(OUT/'output.bin'),str(OUT/'input.bin')],check=True,capture_output=True,timeout=20)
        data=(OUT/'output.bin').read_bytes()
        assert data[65542:65544]==W(0x81ff),(entry,data[65542:65544].hex())
        assert data[65544:65546]==W(0x8b00),(entry,'stack')
        return bytearray(data[:65536])

    for value in range(256):
        result=call(memory,'ncStateWriteNumber',a=value,de=0x8500)
        want=str(value).encode()+b'\r\n'
        assert result[0x8500:0x8500+len(want)]==want,value
    for leaf,want in [(b'FILE.TXT',b'FILE.TXT'),(b'#FILE.TXT',b'"#FILE.TXT"'),(b'',b'')]:
        base=bytearray(memory);base[0x8500:0x8500+len(leaf)+1]=leaf+b'\0'
        base[0x8600:0x8620]=bytes([0xa5])*32
        result=call(base,'ncStateWriteSelected',hl=0x8500,de=0x8600)
        assert result[0x8600:0x8600+len(want)+1]==want+b'\xa5'

    def patch(base,entry,code):base[n[entry]:n[entry]+len(code)]=code
    def choose(left,right):
        return b'\x3a'+W(n['state.activePanel'])+b'\xb7\x21'+W(left)+b'\xc8\x21'+W(right)+b'\xc9'

    for active in (0,1):
        base=bytearray(memory)
        # Filesystem writes are intercepted; the production serializer/capture,
        # packet setup, pointer restoration and decimal conversion run unchanged.
        patch(base,'mx_callRegs',b'\xe3\x23\x23\xe3\xaf\xc9')
        patch(base,'loadFiles',b'\xc9')
        patch(base,'getSelectedFile',choose(0x8000,0x8010))
        patch(base,'getPanelPath',choose(0x8400,0x8500))
        base[0x8000:0x8010]=b'Mixed   txt'+bytes(5)
        base[0x8010:0x8020]=b'Dir     ext'+bytes([16])+bytes(4)
        for at,value in [(0x8400,b'A:\\TEST\0'),(0x8500,b'H:\\GAMES\0')]:base[at:at+len(value)]=value
        for name,value in [('activePanel',active),('panelA_sort',2),('panelB_sort',0),
                           ('panelA_info',0),('panelB_info',1),('panelA_firstFile',255),('panelB_firstFile',7)]:
            base[n['state.'+name]]=value
        result=call(base,'saveState')
        size=int.from_bytes(result[n['ncStateIO']+n['FILE_PATH_IO.count']:n['ncStateIO']+n['FILE_PATH_IO.count']+2],'little')
        text=bytes(result[n['NC_SORT_KEYS']:n['NC_SORT_KEYS']+size])
        expected=(f'ACTIVEPANEL {active}\r\nLEFTPATH A:\\TEST\r\nLEFTSORT 2\r\nLEFTINFO 0\r\n'
                  'LEFTSELECTED Mixed.txt\r\nLEFTFIRST 255\r\nRIGHTPATH H:\\GAMES\r\nRIGHTSORT 0\r\n'
                  'RIGHTINFO 1\r\nRIGHTSELECTED Dir.ext\r\nRIGHTFIRST 7\r\n').encode()
        assert text==expected,(text,expected)
        assert result[n['state.activePanel']]==active
        index=n['FILE_INDEX_A']+active*256+(255 if active==0 else 7)
        assert result[n['vars.fileListPtr']:n['vars.fileListPtr']+2]==W(index)
        expected_path=b'B:\\NC-STATE.CFG\0'
        path=result[n['vars.pathArg']:n['vars.pathArg']+len(expected_path)]
        assert path==expected_path,path
    print('Text state PASS: 256 decimal values, both active panels, paths/selected leaves, CRLF, IO packet, list pointer and SP')


if __name__=='__main__':main()
