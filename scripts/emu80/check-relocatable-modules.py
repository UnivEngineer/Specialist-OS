"""Проверка MODULE_LOAD/CALL/UNLOAD на файлах в B: и замер тактов в Emu80."""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT/'scripts/images'))
from executable import checksum, encode, Image


def check(machine, out):
    symbols = runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))['symbols']
    k = symbols(ROOT/'bin/banked/KERNEL.sym')
    bank_nc = symbols(ROOT/'bin/NC_CORE.sym')
    cli = symbols(ROOT/'bin/CLI.sym')
    path_io = runpy.run_path(str(TOOLS/'file-api.py'))['path_io']
    m = machine
    def reset_to_cli():
        m.stop_at(bank_nc['ncBankInputPoll'])
        m.call('emu_reset')
        for _ in range(120):
            state = m.until(bank_nc['ncBankInputPoll'],120)
            if m.read(k['bankPage'],1)[0] > 1:
                break
            m.call('debug_step',mode='into')
        else:
            raise RuntimeError(('NC boot', state))
        # Выходим из NC, чтобы тестовые адреса MAIN не затрагивали живую оболочку.
        m.stop_at(cli['cliPrompt'])
        m.tap('f10')
        m.until(cli['cliPrompt'],120,page=0)

    reset_to_cli()
    m.screenshot(out/'cli.png')
    observations = []

    def page(n):
        m.write(0xfffd if n else 0xfffc, bytes([n-1 if n else 0]))

    def syscall(op,hl=0,a=0,de=0,bc=0,error=None):
        page(0)
        m.write(0x3100,struct.pack('<4H',hl,de,bc,a<<8|2))
        m.write(0x2800,b'\xcd\x60\xf8')
        m.stop_at(0x2803)
        m.call('regs_set',pc=0x2800,sp=0x2700,bc=op,de=0x3100)
        start = m.call('cpu_state')['cpu_ticks']
        state = m.until(0x2803,100,page=0)
        r = {name:reg['dec'] for name,reg in state['registers'].items()}
        assert r['sp']==0x2700 and (r['af'] & 1)==int(error is not None), (hex(op),error,r)
        if error is not None:
            assert r['af']>>8 == error,(hex(op),error,r)
        return r['hl'],state['cpu_ticks']-start

    def filename(name):
        m.write(0x3200,('B:/'+name).encode().ljust(k['MAX_PATH'],b'\0'))

    def save(name, data):
        page(0);filename(name)
        m.write(0x4000,data)
        m.write(0x3300,struct.pack('<3H',0x3200,0x4000,len(data))+bytes(34))
        path_io(m,syscall,k,0x3300,True)

    def load(name,target,error=None):
        page(0);filename(name)
        return syscall(k['MXOS_DOS_MODULE_LOAD'],hl=0x3200,a=target,error=error)

    def unload(handle):
        syscall(k['MXOS_DOS_MODULE_UNLOAD'],hl=handle)

    def records():
        page(1)
        raw=m.read(k['MODULE_TABLE'],k['MODULE_RECORD_SIZE']*k['MODULE_MAX'])
        page(0)
        return {struct.unpack_from('<H',raw,i)[0]:
                (raw[i+3],*struct.unpack_from('<2H',raw,i+10))
                for i in range(0,len(raw),k['MODULE_RECORD_SIZE']) if any(raw[i:i+2])}

    def occupancy():
        page(1)
        raw=m.read(k['MEM_ALLOC_TABLE'],k['MEM_ALLOC_RECORDS']*6)
        return records(),tuple(raw[i:i+6] for i in range(0,len(raw),6) if raw[i]!=255)

    def invoke(handle,expected,warm=0):
        m.write(0x3400,bytes(8))
        _,ticks=syscall(k['MXOS_DOS_MODULE_CALL'],hl=handle,de=0x3400)
        value,pointer,status,_=struct.unpack('<4H',m.read(0x3400,8))
        bank,base,end=records()[handle]
        assert (value,status)==(expected,bank<<8|warm),(value,status,expected,bank,warm)
        assert base<=pointer<end-1,(pointer,base,end)
        page(bank)
        assert m.read(pointer,2)==struct.pack('<H',expected)
        page(0)
        return dict(base=base,end=end,bank=bank,callTicks=ticks)

    directory=ROOT/'bin/moduletest'
    for name in ('RELOCA','RELOCB','RELOCD','RELOCE'):
        save(name+'.DLL',(directory/(name+'.DLL')).read_bytes())
    original=bytearray((directory/'RELOCA.DLL').read_bytes())
    table_at=40+struct.unpack_from('<H',original,10)[0]
    for name,offset,value in (('DUP',table_at+2,struct.unpack_from('<H',original,table_at)[0]),
                              ('OUTSIDE',len(original)-2,table_at-41),('OVER',table_at+2,2)):
        bad=bytearray(original)
        if name=='OVER':
            value=struct.unpack_from('<H',original,table_at)[0]+1
        struct.pack_into('<H',bad,offset,value)
        struct.pack_into('<H',bad,36,checksum(bad[table_at:]))
        struct.pack_into('<H',bad,30,0)
        struct.pack_into('<H',bad,30,checksum(bad[:40]))
        save(name+'.DLL',bad)
    bad=bytearray(original);bad[-1]^=1
    save('CHECKSUM.DLL',bad)
    save('SHORT.DLL',original[:-1])
    reject=bytes((0x3e,k['MEM_ERROR_RANGE'],0x37,0xc9))
    save('REJECT.DLL',encode(Image(reject,flags=4,options=3,reloc_kind=0),'dll'))
    # Холодная активация успешна, а тёплая возвращает CF=1. Последний байт —
    # собственные изменяемые данные модуля, пригодные для проверки перечитывания.
    save('REFRESH.DLL',encode(Image(bytes.fromhex('79 b7 c8 37 c9 a5'),flags=4,
                                    options=3,reloc_kind=0),'dll'))
    baseline=occupancy()
    for name in ('DUP','OUTSIDE','OVER','CHECKSUM','SHORT'):
        load(name+'.DLL',0,k['DOS_ERR_BAD_IMAGE'])
        assert occupancy()==baseline,name
    a,load_ticks=load('RELOCA.DLL',0)
    b,_=load('RELOCB.DLL',0)
    ra=invoke(a,0x1101)
    rb=invoke(b,0x2201)
    assert ra['base'] and rb['base'] and rb['base']%256==0
    assert rb['end']<=ra['base'] or ra['end']<=rb['base']
    invoke(a,0x1102);invoke(b,0x2202)
    observations.append(dict(target='MAIN',activation='cold',loadTicks=load_ticks,**ra))
    unload(b);unload(a)
    assert occupancy()==baseline
    a,load_ticks=load('RELOCA.DLL',0)
    reused=invoke(a,0x1103,1)
    assert reused['base']==ra['base']
    observations.append(dict(target='MAIN',activation='warm',loadTicks=load_ticks,**reused))
    b,_=load('RELOCB.DLL',0)
    unload(a);unload(b)
    assert occupancy()==baseline
    load('REJECT.DLL',0,k['MEM_ERROR_RANGE'])
    assert occupancy()==baseline
    refresh,_=load('REFRESH.DLL',0)
    _,base,end=records()[refresh]
    m.write(end-1,b'\x5a')
    unload(refresh)
    refresh,_=load('REFRESH.DLL',0)
    assert records()[refresh][1:]==(base,end)
    assert m.read(end-1,1)==b'\xa5'
    unload(refresh)
    assert occupancy()==baseline
    # Зафиксировать v2 DLL на прежней базе. При холодной загрузке обе DLL
    # должны сохранить свои данные в непересекающихся интервалах.
    guard=bytes([0xc9])+bytes(range(1,ra['end']-ra['base']))
    save('GUARD.DLL',encode(Image(guard,ra['base'],ra['base'],4),'dll'))
    guard_handle,_=load('GUARD.DLL',0)
    a,_=load('RELOCA.DLL',0)
    moved=invoke(a,0x1101)
    assert moved['base']!=ra['base']
    assert m.read(ra['base'],len(guard))==guard
    unload(a);unload(guard_handle)
    assert occupancy()==baseline
    for name,target,seed in (('RELOCD',2,0x3300),('RELOCE',1,0x4400)):
        for run in (1,2):
            h,ticks=load(name+'.DLL',target)
            actual=invoke(h,seed+1)
            repeat=invoke(h,seed+2)
            observations.append(dict(target=('EXEC' if target==1 else 'DOS'),loadNumber=run,
                                     loadTicks=ticks,repeatCallTicks=repeat['callTicks'],**actual))
            unload(h)
            assert occupancy()==baseline
    # Полный реестр живых DOS DLL. Последний отказ сохраняет все записи.
    handles=[load('RELOCD.DLL',2)[0] for _ in range(k['MODULE_MAX']-len(baseline[0]))]
    full=occupancy()
    load('RELOCD.DLL',2,k['MEM_ERROR_NOMEM'])
    assert occupancy()==full
    for handle in reversed(handles):
        invoke(handle,0x3301)
        unload(handle)
    assert occupancy()==baseline
    # RETAIN предъявляется от имени работающей DLL. FIND возвращает тот же handle.
    retained,_=load('RELOCD.DLL',2)
    m.write(0x3400,struct.pack('<4H',0,0,0,0x0102))
    syscall(k['MXOS_DOS_MODULE_CALL'],hl=retained,de=0x3400)
    assert not m.read(0x3406,1)[0]&1
    found,_=syscall(k['MXOS_DOS_RESIDENT'],a=k['RESIDENT_FIND'],de=0x524c)
    assert found==retained
    retained_record=records()[retained]
    # Прерываем вторую загрузку на уже зарезервированном и прочитанном теле,
    # до публикации handle. После Reset следует заново загрузить NC и выйти в CLI.
    filename('RELOCB.DLL')
    m.write(0x3100,struct.pack('<4H',0x3200,0,0,2))
    m.write(0x2800,b'\xcd\x60\xf8')
    m.stop_at(k['relocReadTable'])
    m.call('regs_set',pc=0x2800,sp=0x2700,bc=k['MXOS_DOS_MODULE_LOAD'],de=0x3100)
    m.until(k['relocReadTable'],100,page=1)
    pending = dict((field, m.read(k[field],length).hex())
                   for field,length in (('MODULE_TABLE',128),('MEM_ALLOC_TABLE',192),
                                        ('appOwner',1),('appDepth',1),('appImage',2),
                                        ('residentKeys',16),('mdRecord',2)))
    (out/'before-reset.json').write_text(json.dumps(pending,indent=2)+'\n','utf-8')
    reset_to_cli()
    assert syscall(k['MXOS_DOS_RESIDENT'],a=k['RESIDENT_FIND'],de=0x524c)[0]==retained
    assert records()[retained]==retained_record
    invoke(retained,0x3301)
    unload(retained)
    assert occupancy()==baseline
    a,_=load('RELOCA.DLL',0)
    invoke(a,0x1101)
    unload(a)
    assert occupancy()==baseline
    (out/'result.json').write_text(json.dumps(observations,indent=2)+'\n','utf-8')
    print(json.dumps(observations,indent=2),flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',type=int,choices=range(19781,19797),default=19782)
    parser.add_argument('--report',type=Path,default=ROOT/'docs/dos/relocatable-test/mmx')
    args=parser.parse_args()
    out=args.report.resolve();out.mkdir(parents=True,exist_ok=True)
    subprocess.run([sys.executable,str(ROOT/'source-apps/moduletest/build.py')],check=True)
    bench=runpy.run_path(str(TOOLS/'benchmark-disks.py'))
    runtime=Path(os.environ['LOCALAPPDATA'])/f'Specialist-OS/emu80/{args.port}'
    reply=bench['command'](['powershell.exe','-NoProfile','-File',str(TOOLS/'run-mcp-slot.ps1'),'-Port',str(args.port)])
    pid=int(re.search(r'\(PID (\d+)\)',reply)[1])
    machine=bench['Bench'](f'http://127.0.0.1:{args.port}/mcp')
    try:
        exe=Path(bench['shell'](f'(Get-Process -Id {pid}).Path')).resolve()
        assert exe==(runtime/'Emu80qt-mcp.exe').resolve()
        bench['set_benchmark_speed'](machine,pid)
        check(machine,out)
    except Exception:
        print('last CPU state',machine.call('cpu_state'),flush=True)
        machine.screenshot(out/'failure.png')
        raise
    finally:
        bench['shell'](f'$p=Get-Process -Id {pid} -ErrorAction SilentlyContinue; '
                       f'if ($p -and $p.Path -eq {bench["ps"](runtime/"Emu80qt-mcp.exe")}) {{Stop-Process -InputObject $p}}; exit 0')


if __name__=='__main__':
    main()
