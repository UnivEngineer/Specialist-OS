"""GET/PUT8/16 через COMMAND: реальные MAIN/EXEC/DOS/DATA."""
from pathlib import Path
import os
import argparse, itertools, json, re, runpy, struct, subprocess, time
R=Path(__file__).resolve().parents[2]
W=lambda n:struct.pack('<H',n)
def symbols(path):
    return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',path.read_text(),re.M)}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True);p.add_argument('--port',type=int,default=19782)
    p.add_argument('--report',type=Path,default=R/'docs/mem/command-pack-2026-11-13/scalar')
    a=p.parse_args();a.report.mkdir(parents=True,exist_ok=True)
    owner=subprocess.run(['powershell.exe','-NoProfile','-Command',f'Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue | Select-Object -ExpandProperty OwningProcess'],capture_output=True,text=True)
    assert not owner.stdout.strip(),'Port owned; test not started'
    conf=(a.runtime/'specialist-os/launch.conf').read_text()
    assert 'pitSoundSource.muted = yes' in conf and 'tapeMonitor.muted = yes' in conf
    Bench=runpy.run_path(str(R/'source-apps/diskbench/check-emulator.py'))['Bench']
    m=Bench(f'http://127.0.0.1:{a.port}/mcp');ts=symbols(R/'bin/banked/TRANSPORT.sym')
    image=(R/'bin/MEM.sys').read_bytes();parts=[struct.unpack_from('<HHH',image,6+i*6) for i in range(image[5])]
    rows=[]
    overlays=tuple(runpy.run_path(str(R/'scripts/tests/mem-image.py'))['read_bios_overlays'](R/'bin'))
    process=subprocess.Popen([str(a.runtime/'Emu80qt-mcp.exe'),'--conf-file','specialist-os/launch.conf','--platform','specialist-os'],cwd=a.runtime,env=dict(os.environ,SPECIALIST_EMU80_MCP_PORT=str(a.port)))
    try:
        for _ in range(60):
            try:m.call('debug_break');break
            except Exception:time.sleep(.2)
        else:raise RuntimeError('Own emulator unavailable')
        def select(page):m.write(0xfffd if page else 0xfffc,bytes([page-1 if page else 0]))
        for page in (0,1,3):
            select(page)
            for addr,size,offset in parts:
                if page==0 and addr==ts['MEM_BANK_CLIENT_START']:
                    skip=ts['mx_callPacket']-addr
                    addr+=skip;size-=skip;offset+=skip
                m.write(addr,image[offset:offset+size])
            if page==0:
                for address,body in overlays:
                    m.write(address,body)
            m.write(ts['bankPage'],bytes([page]));m.write(ts['bankPageCount'],b'\x10')
        for page in (2,4):
            select(page);addr,size,offset=parts[2];begin=ts['MEM_DATA_START']
            m.write(begin,image[offset+begin-addr:offset+size])
            # No executable client and no valid target stack: COMMAND is register-only.
            m.write(ts['mx_callPacket'],b'\x76'*32)
        def case(caller,target,seed,op,address=0x3001,error=0):
            select(target if target<=16 else 2)
            if not error:m.write(0x3000,bytes.fromhex('a51234565a'))
            window=bytes((seed+31*i)&255 for i in range(32))
            select(caller);m.write(0xffc0,window)
            stub=(b'\x01'+W((target<<8)|0x5a)+b'\x11'+W(0xabcd)+b'\x21'+W(address)
                  +b'\x3e'+bytes([op])+b'\xcd'+W(ts['mx_memory'])+b'\xc3'+W(0x88ff))
            m.write(0x8800,stub);m.stop_at(0x88ff);m.call('regs_set',pc=0x8800,sp=0x8f00)
            state=m.until(0x88ff,3);regs=state['registers'];status=regs['af']['dec']>>8
            assert state['flags']['C']==bool(error) and status==error,(caller,target,seed,op,state)
            assert regs['bc']['dec']==(target<<8)|0x5a and regs['hl']['dec']==address and regs['sp']['dec']==0x8f00
            assert m.read(0xffde,2)==window[-2:]
            if not error:
                if op in (1,2):assert regs['de']['dec']==(0x12 if op==1 else 0x3412)
                select(target);data=m.read(0x3000,5)
                assert data==(bytes.fromhex('a5cd34565a') if op==3 else bytes.fromhex('a5cdab565a') if op==4 else bytes.fromhex('a51234565a')),data
            rows.append(dict(caller=caller,target=target,seed=seed,operation=op,address=address,error=error))
        seeds=(0,77,154,231)
        for values in itertools.product((0,1,3),(0,1,2,3,4),seeds,range(1,5)):case(*values)
        for caller,seed in itertools.product((0,1,3),seeds):
            case(caller,2,seed,6,error=ts['MEM_ERROR_RANGE'])
        (a.report/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
        print('COMMAND GET/PUT PASS',len(rows),'cases; MAIN/DOS/EXEC/DATA, guards, BC/HL/SP, errors')
    finally:process.terminate();process.wait(timeout=15)
if __name__=='__main__':main()
