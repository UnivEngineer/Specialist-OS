import hashlib,json,re,struct,subprocess
from pathlib import Path
from tempfile import TemporaryDirectory

out=Path(__file__).resolve().parent
root=out.parents[4]
sym=(root/'bin/banked/TRANSPORT.sym').read_text(encoding='utf8')
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',sym,re.M)}
bios=(root/'bin/BIOS.sys').read_bytes()
assert s['MEM_RUNTIME_REVISION']==64
word=struct.Struct('<H').pack
def run(cmd):
    return subprocess.run(cmd,check=True,text=True,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
rows=[]
with TemporaryDirectory() as temp:
    work=Path(temp)
    origin=0xfd80
    prefix=[f'    ORG 0{origin:04X}h']
    for name in ('BANK_IO_MAIN','BANK_IO_RAM','bankPage','bankServiceSP','bios_getCursorPos'):
        prefix.append(f'{name} = 0{s[name]:04X}h')
    prefix.append(f"cursorAddress = 0{s['bios_vars.cursorY']:04X}h")
    asm=work/'replica.asm'
    asm.write_text('\n'.join(prefix)+'\n'+(out/'replica.asm').read_text(encoding='utf-8-sig'))
    raw=work/'replica.bin'
    listing=work/'replica.sym'
    run([str(root/'tasm/sjasmplus.exe'),'--i8080',f'--raw={raw}',f'--sym={listing}',str(asm)])
    labels={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',listing.read_text(),re.M)}
    read_addr=labels['cursorReplicaRead']
    bound_addr=labels['cursorReplicaReadBound']
    replica=raw.read_bytes()
    base=bytearray(65536)
    base[0xc000:0xc000+len(bios)]=bios
    base[origin:origin+len(replica)]=replica
    assert base[0xc81e]==0xc3
    before=bytes(range(0x20))
    base[0xffc0:0xffe0]=before
    base[0xfffc:0xfffe]=b'\xee\xdd'
    caller_sp=0x8650
    for page in (2,6,16):
        for af,xy,main_sp in ((0x7193,0x1234,0xed80),(0x5546,0x0123,0xed40),(0xa5d7,0x4567,0xed60)):
            memory=bytearray(base)
            assert memory[bound_addr+1]==0x3e
            memory[bound_addr+2]=page-1
            memory[s['bankPage']]=page
            memory[s['bankServiceSP']:s['bankServiceSP']+2]=word(main_sp)
            cursor=s['bios_vars.cursorY']
            memory[cursor:cursor+2]=word(xy)
            memory[main_sp-16:main_sp+4]=b'\xa5'*20
            memory[caller_sp-16:caller_sp+4]=b'\x5a'*20
            setup=b'\x31'+word(caller_sp)+b'\x01'+word(af)+b'\xc5\xf1\x01'+word(0x3456)+b'\x11'+word(0x5678)+b'\x21'+word(0x9876)
            def measure(code):
                slot=0x8000
                memory[slot:slot+len(code)]=code
                infile=work/'in.bin'
                dump=work/'out.bin'
                infile.write_bytes(memory)
                end=slot+len(code)
                r=run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8000',
                       '-end',f'{end:x}','-counter','1000000','-output',str(dump),str(infile)])
                output=dump.read_bytes()
                assert output[65542:65546]==word(end)+word(caller_sp)
                assert output[0xffc0:0xffe0]==before
                assert struct.unpack_from('<2H',output,65536)==(af,0x3456)
                return int(r.stdout.strip()),output
            overhead,_=measure(setup)
            direct,result=measure(setup+b'\xcd'+word(s['bios_getCursorPos']))
            assert result[0xfffc:0xfffe]==b'\xee\xdd'
            assert result[main_sp-16:main_sp+4]==b'\xa5'*20
            indirect,state=measure(setup+b'\xcd'+word(origin))
            assert state[0xfffc:0xfffe]==bytes([page-1])*2
            assert state[65536:65542]==result[65536:65542]
            # Снимок z88dk содержит три пары до PC/SP; HL проверяем отдельной записью SHLD.
            for addr in (s['bios_getCursorPos'],origin,read_addr,bound_addr):
                _,captured=measure(setup+b'\xcd'+word(addr)+bytes.fromhex('220070 eb 220270 eb'))
                assert captured[0x7000:0x7002]==word(xy)
                assert captured[0x7002:0x7004]==word(0x5678)
            read_times=[]
            for addr in (read_addr,bound_addr):
                elapsed,raw_read=measure(setup+b'\xcd'+word(addr))
                assert raw_read[0xfffc:0xfffe]==bytes([page-1])*2
                assert raw_read[main_sp-16:main_sp+4]==b'\xa5'*20
                read_times.append(elapsed-overhead)
            assert state[main_sp-16:main_sp-2]==b'\xa5'*14 and state[main_sp:main_sp+4]==b'\xa5'*4
            assert state[main_sp-2:main_sp]!=b'\xa5\xa5'
            assert state[caller_sp-16:caller_sp-6]==b'\x5a'*10 and state[caller_sp:caller_sp+4]==b'\x5a'*4
            rows.append(dict(page=page,af=af,cursor=xy,main_sp=main_sp,direct=direct-overhead,replica=indirect-overhead,
                             read=read_times[0],read_bound=read_times[1]))
    print(rows)
    assert all((r['direct'],r['replica'],r['read'],r['read_bound'])==(53,224,108,97) for r in rows)
    (out/'result.json').write_text(json.dumps(dict(model='z88dk-ticks 8080 flat',bios_sha256=hashlib.sha256(bios).hexdigest(),
        replica_bytes=read_addr-origin,read_bytes=bound_addr-read_addr,read_bound_bytes=origin+len(replica)-bound_addr,
        replica_hex=replica.hex(),rows=rows),indent=2)+'\n')
