"""Матрица DISKBEN: реальные файловые вызовы и Raw, только отдельный Emu80-flash-test."""
import argparse,hashlib,json,os,runpy,shutil,struct,subprocess,time
from datetime import datetime,timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
HELPERS=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))
Bench,symbols=HELPERS['Bench'],HELPERS['symbols']
def app_symbols(path):
    result=symbols(path)
    for name,alias in [('sectorBuffer','diskbench_source'),('highBuffer','diskbench_high'),('fatBuffer','diskbench_fat')]:
        if name not in result and alias in result:result[name]=result[alias]
    return result
RUNTIME=Path('D:/Projects/RetroPC/Specialist/Emulator/Emu80-flash-test')
OUT=ROOT/'docs/diskbench/matrix-001'
SIZE=8192
PASSES=3

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def ps(text):return "'"+str(text).replace("'","''")+"'"
def shell(script):
    return subprocess.run(['powershell.exe','-NoProfile','-Command',script],capture_output=True,text=True,check=True).stdout.strip()
def mcp_owner():
    return shell("$listener = Get-NetTCPConnection -State Listen -LocalPort 19781 -ErrorAction SilentlyContinue; if ($listener) { $listener[0].OwningProcess }; exit 0")
def command(args):
    if str(args[0]).lower()=='powershell.exe' and '-File' in args:
        i=args.index('-File');tail=[str(x) for x in args[i+2:]]
        # Из PowerShell 7 в Windows PowerShell передаётся PSModulePath другой редакции.
        script="Import-Module 'C:/Windows/System32/WindowsPowerShell/v1.0/Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1'; & "+ps(args[i+1])+' '+' '.join(x if x.startswith('-') else ps(x) for x in tail)
        args=['powershell.exe','-NoProfile','-Command',script]
    p=subprocess.run([str(x) for x in args],cwd=ROOT,capture_output=True,text=True)
    if p.returncode:raise RuntimeError(p.stdout+'\n'+p.stderr)
    return p.stdout

def prepare_fixtures():
    OUT.mkdir(parents=True,exist_ok=True)
    command(['powershell.exe','-NoProfile','-File','scripts/build/build-project.ps1','OS'])
    command(['powershell.exe','-NoProfile','-File','scripts/images/build-image.ps1','SystemEmuWritable'])
    command(['powershell.exe','-NoProfile','-File','scripts/images/build-image.ps1','Games2M'])
    payload=OUT/'readonly.tmp';payload.write_bytes(bytes(1^(a>>8)^(a&255) for a in range(0x2000,0x4000)))
    entry='DISKBEN.TMP '+os.path.relpath(payload,ROOT/'assets')+' 0x2000\n'
    base=(ROOT/'scripts/images/manifests/A-System.txt').read_text(encoding='utf-8-sig')
    (OUT/'system-ro-full.txt').write_text(base+'\n'+entry,encoding='utf-8')
    keep={'DOS.SYS','DOSMOD.COM','NC.COM','FONT.FNT','FONTKOI.FNT','RAMD.COM','FORMAT.BAT','FORMAT.COM','AUTOEXEC.BAT','ROM.COM'}
    minimal='@label SYSTEM ROM\n'+'\n'.join(line for line in base.splitlines() if line.split() and line.split()[0] in keep)+'\n'+entry
    (OUT/'system-ro-small.txt').write_text(minimal,encoding='utf-8')
    (OUT/'disk-ro.txt').write_text('@label ROM BENCH\n'+entry,encoding='utf-8')
    builder=(ROOT/'scripts/images/build-image.ps1').read_text(encoding='utf-8-sig')
    def build(name,manifest,target):
        folder=OUT/name;folder.mkdir(exist_ok=True)
        text=builder.replace('$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)','$repositoryRoot = '+ps(ROOT))
        text=text.replace(". (Join-Path $PSScriptRoot 'executable.ps1')",". "+ps(ROOT/'scripts/images/executable.ps1'))
        text=text.replace('$imageDirectory = Join-Path $repositoryRoot "images"','$imageDirectory = '+ps(folder))
        for key in ['A-System.txt','H-ROM64k.txt']:
            text=text.replace('Join-Path $PSScriptRoot "manifests\\'+key+'"',ps(OUT/manifest))
        script=folder/'build-fixture.ps1';script.write_text(text,encoding='utf-8-sig')
        command(['powershell.exe','-NoProfile','-File',script,target])
    build('rom512','system-ro-full.txt','SystemEmuWritable')
    build('rom64','system-ro-small.txt','SystemEmu')
    build('external-rom64','disk-ro.txt','Games64k')

class Matrix:
    def __init__(self):
        self.m=Bench('http://127.0.0.1:19781/mcp');self.pid=None
        self.appBinary=ROOT/'bin/DISKBEN.i80'
        self.s=app_symbols(ROOT/'source-apps/diskbench/build/DISKBEN.sym')
        self.report={'startedUtc':datetime.now(timezone.utc).isoformat(),'cpuHz':2000000,'pitHz':2000000,'fileBytes':SIZE,'passes':PASSES,'units':'decimal kB/s; ms','rows':[],'inventory':{},'phases':{}}
    def save(self):
        (OUT/'raw.json').write_text(json.dumps(self.report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    def stop(self):
        if self.pid:
            shell('$p=Get-Process -Id '+str(self.pid)+' -ErrorAction SilentlyContinue; if ($p -and $p.Path -eq '+ps(RUNTIME/'Emu80qt-mcp.exe')+') { Stop-Process -Id $p.Id; $p.WaitForExit() }')
            self.pid=None
    def start(self,phase,flash=True,system=None,disk=None,locked=False,dos=None,chip_count=None,disk_locked=False):
        self.stop()
        # Порт MCP фиксирован upstream. Не подключаемся к чужому эмулятору.
        owner=mcp_owner()
        if owner:raise RuntimeError('MCP port 19781 is already owned by process '+owner+'; test emulator was not started')
        command(['powershell.exe','-NoProfile','-File','scripts/emu80/run.ps1','-EmulatorDirectory',RUNTIME,'-PrepareOnly','-Mcp']+(['-Flash','-ResetFlash'] if flash else [])+(['-ProtectSystemFlash'] if locked else []))
        if system:shutil.copyfile(system,RUNTIME/'specialist-os'/('system-flash.bin' if flash else 'system.bin'))
        if disk:shutil.copyfile(disk,RUNTIME/'specialist-os'/('flash-disk.bin' if flash else 'rom-disk.bin'))
        if chip_count is not None or disk_locked:
            assert flash and (chip_count is None or 1<=chip_count<=4)
            config=RUNTIME/'specialist-os/specialist-mx2.conf'
            text=config.read_text(encoding='utf-8-sig')
            if chip_count is not None:
                text=text.replace('flashDisk.chipCount = 4','flashDisk.chipCount = '+str(chip_count))
                (RUNTIME/'specialist-os/flash-disk.bin.sdp').write_bytes(bytes(chip_count))
            if disk_locked:text+='\nflashDisk.readOnly = yes\n'
            config.write_text(text,encoding='utf-8')
        self.pid=int(shell('$p=Start-Process -FilePath '+ps(RUNTIME/'Emu80qt-mcp.exe')+" -ArgumentList '--conf-file','specialist-os/launch.conf','--platform','specialist-os' -WorkingDirectory "+ps(RUNTIME)+" -WindowStyle Normal -PassThru; $p.Id"))
        for _ in range(30):
            try:
                owner=mcp_owner()
                if owner!=str(self.pid):
                    time.sleep(.2);continue
                self.m.call('debug_break');break
            except Exception:time.sleep(.2)
        else:raise RuntimeError('Owned test emulator did not acquire MCP port 19781')
        nc=symbols(ROOT/'bin/NC.sym');self.m.stop_at(nc['mainLoop'])
        if 'moduleLoadError' in nc:self.m.call('bp_set',addr=nc['moduleLoadError'],type='exec')
        diagnostic=symbols(ROOT/'bin/DOS.sym')
        if 'dosModuleBad' in diagnostic:self.m.call('bp_set',addr=diagnostic['dosModuleBad'],type='exec')
        for _ in range(120):
            self.m.call('emu_run_for',ms=500)
            state=self.m.call('cpu_state')
            if state['registers']['pc']['dec']==diagnostic.get('dosModuleBad'):
                a=diagnostic['moduleImports'];n=diagnostic['moduleImportsEnd']-a
                raise RuntimeError(('module header/import mismatch',state,self.m.read(a,n).hex()))
            if state['registers']['pc']['dec']==nc.get('moduleLoadError'):
                ds=symbols(ROOT/'bin/DOS.sym')
                self.m.screenshot(OUT/'module-error.png')
                raise RuntimeError(('DOS module load failed',self.m.read(ds['v_dosError'],1).hex(),self.m.read(0x8000,16).hex()))
            if state['registers']['pc']['dec']==nc['mainLoop']:
                if self.m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]:break
                self.m.call('bp_clear');self.m.call('emu_run_for',ms=50);self.m.stop_at(nc['mainLoop'])
        else:
            self.m.call('debug_break');state=self.m.call('cpu_state')
            ds=symbols(ROOT/'bin/DOS.sym');self.m.screenshot(OUT/'boot-timeout.png')
            raise RuntimeError(('NC boot timeout',state,self.m.read(ds['v_dosError'],1).hex(),self.m.read(0x8000,16).hex()))
        self.m.call('bp_clear');self.m.call('emu_run_for',ms=500)
        self.ds=symbols(dos.with_suffix('.sym') if dos else ROOT/'bin/DOS.sym')
        expected=(dos or ROOT/'bin/DOS.sys').read_bytes()
        # Холодный старт патчит ядро; сверяем таблицу ABI, а не все изменяемые байты.
        assert self.m.read(0xC800,3)==expected[0x800:0x803]
        conf=(RUNTIME/'specialist-os/specialist-mx2.conf').read_text(encoding='utf-8-sig')
        assert 'pit.frequency = 2000000' in conf and 'cpu.frequency = 2000000' in conf
        self.phase=phase
        self.report['phases'][phase]={'pid':self.pid,'dosSha256':sha(dos or ROOT/'bin/DOS.sys'),'launchConf':(RUNTIME/'specialist-os/launch.conf').read_text(),'ramPages':self.m.read(0x8FF8,1)[0]}
        self.save();print('PHASE',phase,flush=True)
    def func(self,address,**regs):
        self.m.stop_at(0x8E10);self.m.write(0x8E00,bytes((0xCD,address&255,address>>8,0xC3,0x10,0x8E)))
        self.m.call('regs_set',pc=0x8E00,sp=0x8FDF,**regs)
        return self.m.until(0x8E10,120)
    def select(self,drive):
        assert not self.func(0xC842,af=drive<<8,de=1)['flags']['C']
        assert not self.func(0xC896)['flags']['C']
    def install(self,name):
        address=0 if name in ['Flash','FlashFst','FlashSys','RAMD'] else SYSTEM['DISK_DRIVER_ADDR']
        self.m.write(address,(ROOT/f'bin/{name}.i80').read_bytes());self.func(address)
        # Смена буквы обновляет рабочий JP после установки драйвера.
        self.func(0xC842,af=0x0100,de=1)
    def format_ram(self):
        self.m.write(0,(ROOT/'bin/FORMAT.i80').read_bytes());self.m.write(0x7E00,b'B: -R -Y\r\0')
        assert not self.func(0,de=0x7E00)['flags']['C']
        self.select(1)
    def init_app(self,drive,mode=1):
        self.func(0xC8DB) # Стенд HIGH явно освобождает загружаемые службы.
        self.m.write(0,self.appBinary.read_bytes());self.select(drive)
        info=self.func(0xC863,bc=0x0300);capacity=info['registers']['de']['dec'];pointer=info['registers']['hl']['dec']
        self.m.write(self.s['v_info'],pointer.to_bytes(2,'little'));self.m.write(self.s['v_capacity'],capacity.to_bytes(2,'little'))
        self.m.write(self.s['v_drive'],bytes([drive]));self.m.write(self.s['v_budget'],bytes([PASSES]));self.m.write(self.s['v_writeMode'],bytes([mode]))
        self.func(self.s['detect_driver']);self.func(0xC863,bc=0x0200,de=0,hl=self.s['sectorBuffer'])
        assert not self.func(self.s['geometry'])['flags']['C'];self.func(self.s['select_layout'])
        self.func(0xC8A2);assert not self.func(self.s['timer_probe'])['flags']['Z']
        self.m.write(self.s['v_operations'],(32*PASSES).to_bytes(2,'little'))
        ptr=int.from_bytes(self.m.read(self.s['v_driverInfo'],2),'little');name,desc=self.m.read(ptr,80).split(b'\0')[:2]
        self.func(self.s['puts'],hl=self.s['txtTitle']);self.func(self.s['print_summary'])
        return {'drive':chr(65+drive),'capacityBytes':capacity*256,'driver':name.decode(),'description':desc.decode(),'caps':self.m.read(self.s['v_caps'],1)[0]}
    def traps(self,target):
        self.m.stop_at(target)
        for key in ['error','file_exists']:self.m.call('bp_set',addr=self.s[key],type='exec')
    def until(self,target):
        for _ in range(180):
            state=self.m.call('cpu_state');pc=state['registers']['pc']['dec']
            if pc==target:return state
            if pc in [self.s['error'],self.s['file_exists']]:
                self.m.screenshot(OUT/'failure.png');raise RuntimeError(('DISKBEN error',state))
            self.m.call('emu_run_for',ms=1000)
        raise RuntimeError(('timeout',self.m.call('cpu_state')))
    def snapshot_metadata(self):
        # Читаем Boot/FAT/root одним циклом вне измерений; на B: нет host-файла.
        self.func(0xC863,bc=0x0200,de=0,hl=0x2000);boot=self.m.read(0x2000,256)
        word=lambda n:struct.unpack_from('<H',boot,n)[0]
        count=word(14)+boot[16]*word(22)+(word(17)*32+255)//256
        assert count<96
        a=0x8800;loop=a+8
        code=bytes.fromhex('1100002100200E')+bytes([count])+bytes.fromhex('0602CD63C813240DC2')+struct.pack('<H',loop)+bytes.fromhex('C30087')
        self.m.stop_at(0x8700);self.m.write(a,code);self.m.call('regs_set',pc=a,sp=0x8FDF);self.m.until(0x8700)
        return self.m.read(0x2000,count*256)
    def inventory(self):
        for drive in range(8):
            self.func(0xC842,af=drive<<8,de=1)
            info=self.func(0xC863,bc=0x0300)
            self.report['inventory'][chr(65+drive)]={'sectors':info['registers']['de']['dec'],'available':info['registers']['de']['dec']>0}
        self.save()
    def writable(self,drive,driver,mode,variant='Buffered'):
        key=f'{chr(65+drive)}-{driver}-{variant}-{mode}'
        if any(row['id']==key for row in self.report['rows']):
            print('SKIP COMPLETED',key,flush=True);return
        print('RUN',key,flush=True)
        info=self.init_app(drive,mode)
        assert info['caps']&2
        if mode==5:assert info['caps']&4
        disk=(RUNTIME/'specialist-os'/('system-flash.bin' if drive==0 else 'flash-disk.bin')) if drive in [0,7] else None
        before=disk.read_bytes() if disk else self.snapshot_metadata()
        self.traps(self.s['raw_read_high_done']);self.m.call('regs_set',pc=self.s['file_write_case'],sp=0x8FDF)
        previous=[(0,0,0)]*4;passes=[]
        for attempt in range(1,PASSES+1):
            self.until(self.s['raw_read_high_done'])
            assert self.m.read(self.s['v_pass'],1)==bytes([attempt])
            assert self.m.read(0x2000,SIZE)==bytes(attempt^(a>>8)^(a&255) for a in range(0x2000,0x4000))
            raw=self.m.read(self.s['statAll'],40);current=[struct.unpack_from('<IIH',raw,i*10) for i in range(4)]
            passes.append([[x-y for x,y in zip(c,p)] for c,p in zip(current,previous)]);previous=current
            if attempt==PASSES:self.traps(self.s['file_results'])
            else:self.m.call('debug_step',mode='into')
        self.until(self.s['file_results']);assert self.m.read(self.s['v_fileOwned'],1)==b'\0'
        self.m.stop_at(self.s['exit_wait']);self.m.until(self.s['exit_wait']);self.m.screenshot(OUT/(key+'.png'))
        after=disk.read_bytes() if disk else self.snapshot_metadata()
        self.check_integrity(before,after)
        total=current
        assert total[0][2]==(PASSES if info['caps']&8 else 0)
        assert total[2][2]>=(PASSES if info['caps']&8 else 0)
        metrics=dict(zip(['rawWrite','rawRead','fileWrite','fileRead'],total))
        if drive==0:
            high=self.m.read(self.s['resultsHigh'],40)
            metrics['rawWriteHigh']=struct.unpack_from('<IIH',high,0)
            metrics['rawReadHigh']=struct.unpack_from('<IIH',high,10)
            metrics['fileWriteHigh']=struct.unpack_from('<IIH',high,20)
            metrics['fileReadHigh']=struct.unpack_from('<IIH',high,30)
        verification=[int.from_bytes(self.m.read(self.s[k],4),'little') for k in ['statRawVerify','statFileVerify']]
        mode_name={1:'write',5:'write+verify',6:'write+file-verify'}[mode]
        self.report['rows'].append(dict(info,id=key,phase=self.phase,variant=variant,mode=mode_name,buffer='low',metrics=metrics,verifyTicks=verification,perPass=passes,integrity=True))
        self.save();print('DONE',key,flush=True)
    @staticmethod
    def check_integrity(before,after):
        word=lambda n:struct.unpack_from('<H',before,n)[0]
        fat=word(14)*256;root=fat+before[16]*word(22)*256;data=root+(word(17)*32+255)//256*256
        assert before[:root]==after[:root]
        for a in range(root,data,32):
            if before[a] not in (0,0xE5,0xFF):assert before[a:a+32]==after[a:a+32]
        if len(before)>data:
            for c in range(2,(len(before)-data)//256+2):
                if fat+c*2+2>root:break
                if before[fat+c*2:fat+c*2+2]!=b'\0\0':
                    a=data+(c-2)*256;assert before[a:a+256]==after[a:a+256]
    def readonly(self,drive,kind,image):
        info=self.init_app(drive);assert not info['caps']&2
        initial=sha(image)
        # Отказы записи подтверждаются вне замера.
        for op in [1,5]:assert self.func(0xC863,bc=op<<8,de=0,hl=0x2000)['flags']['C']
        for buf in range(2 if drive==0 else 1):
            self.m.write(self.s['v_bufferMode'],bytes([buf]));self.m.write(self.s['v_pass'],b'\1');self.m.write(self.s['statAll'],bytes(40))
            assert not self.func(0xC866,hl=self.s['tmpDescriptor'],de=0x2000)['flags']['C']
            assert not self.func(self.s['file_compare'])['flags']['C']
            assert not self.func(self.s['raw_build_chain'])['flags']['C']
            # Измерения используют те же процедуры DISKBEN, без пользовательского вывода.
            asm='    ORG 08800h\n    ld a,3\n    ld ('+str(self.s['v_index'])+'),a\nagain:\n'
            asm+='    ld a,0FFh\n    call '+str(self.s['fill_payload'])+'\n    call '+str(self.s['measure_begin'])+'\n    ld hl,'+str(self.s['tmpDescriptor'])+'\n    ld de,02000h\n    call 0C866h\n    call '+str(self.s['measure_end'])+'\n    jp c,failed\n    ld de,'+str(self.s['statFileRead'])+'\n    call '+str(self.s['add_stats'])+'\n    call '+str(self.s['file_compare'])+'\n    jp c,failed\n'
            asm+='    ld a,0FFh\n    call '+str(self.s['fill_payload'])+'\n    ld b,2\n    call '+str(self.s['raw_transfer'])+'\n    jp c,failed\n    ld de,'+str(self.s['statRawRead'])+'\n    call '+str(self.s['add_stats'])+'\n    call '+str(self.s['file_compare'])+'\n    jp c,failed\n    ld hl,'+str(self.s['v_index'])+'\n    dec (hl)\n    jp nz,again\n    jp 08700h\nfailed:\n    jp 08600h\n'
            if buf:
                asm='    ORG 08E00h\n    ld a,3\n    ld ('+str(self.s['v_index'])+'),a\nagain:\n'
                for stream_kind,stat in [(0,'statFileRead'),(1,'statRawRead')]:
                    asm+='    ld b,'+str(stream_kind)+'\n    call '+str(self.s['read_high'])+'\n    jp c,failed\n    ld de,'+str(self.s[stat])+'\n    call '+str(self.s['add_stats'])+'\n'
                asm+='    ld hl,'+str(self.s['v_index'])+'\n    dec (hl)\n    jp nz,again\n    jp 08E90h\nfailed:\n    jp 08E91h\n'
            else:asm=asm.replace('ORG 08800h','ORG 08E00h').replace('08700h','08E90h').replace('08600h','08E91h')
            source=OUT/'read-harness.asm';source.write_text(asm,encoding='utf-8');binary=OUT/'read-harness.bin'
            command([ROOT/'tasm/sjasmplus.exe','--i8080','-Wno-rdlow','--raw='+str(binary),source])
            assert len(binary.read_bytes())<0x90
            self.m.write(0x8E00,binary.read_bytes());self.m.stop_at(0x8E90);self.m.call('bp_set',addr=0x8E91,type='exec');self.m.call('regs_set',pc=0x8E00,sp=0x8FDF)
            for _ in range(120):
                state=self.m.call('cpu_state');pc=state['registers']['pc']['dec']
                if pc==0x8E90:break
                assert pc!=0x8E91,('read verification',info,buf)
                self.m.call('emu_run_for',ms=1000)
            else:raise RuntimeError('read timeout')
            raw=self.m.read(self.s['statAll'],40);total=[struct.unpack_from('<IIH',raw,i*10) for i in range(4)]
            assert total[1][0]>0 and total[3][0]>0 and total[0]==(0,0,0) and total[2]==(0,0,0)
            self.report['rows'].append(dict(info,id=kind,variant='Buffered',mode='read-only',buffer='high-direct' if buf else 'low',metrics=dict(zip(['rawWrite','rawRead','fileWrite','fileRead'],total)),integrity=True))
        assert sha(image)==initial
        self.save();print('DONE',kind,flush=True)

def main():
    global OUT,RUNTIME
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report-directory',type=Path,default=OUT)
    parser.add_argument('--runtime',type=Path,default=RUNTIME)
    parser.add_argument('--resume',action='store_true')
    args=parser.parse_args();OUT=args.report_directory.resolve();RUNTIME=args.runtime.resolve()
    if RUNTIME.name!='Emu80-flash-test' and not RUNTIME.is_relative_to(ROOT/'.local'):parser.error('Use isolated Emu80-flash-test or a workspace .local runtime')
    if (OUT/'raw.json').exists() and not args.resume:parser.error('Report already exists; choose a new --report-directory or --resume')
    if not args.resume:prepare_fixtures()
    matrix=Matrix()
    if args.resume:
        matrix.report=json.loads((OUT/'raw.json').read_text(encoding='utf-8'))
        expected=next(iter(matrix.report['phases'].values()))['dosSha256']
        assert sha(ROOT/'bin/DOS.sys')==expected,'DOS changed since the saved measurements'
    try:
        matrix.start('writable-buffered-resume' if args.resume else 'writable-buffered');matrix.inventory()
        matrix.install('FlashSys')
        for mode in [1,6]:matrix.writable(0,'FlashSys',mode)
        for name in ['Flash','FlashFst']:
            matrix.install(name)
            for mode in [1,6]:matrix.writable(7,name,mode)
        matrix.func(0xC860,af=0x0100,hl=matrix.ds['diskDriver'])
        matrix.func(0xC86F,bc=0x0200,hl=matrix.ds['driverByteStart'])
        matrix.func(0xC86F,bc=0)
        matrix.select(1);matrix.format_ram()
        for mode in [1,6]:matrix.writable(1,'RAMD',mode,'Per-byte')
        matrix.install('RAMD');matrix.select(1)
        for mode in [1,6]:matrix.writable(1,'RAMD',mode,'R4')
        matrix.start('rom512',system=OUT/'rom512/SystemRom_emu80_writable.bin',locked=True)
        matrix.func(0xC860,af=0,hl=matrix.ds['diskDriver']);matrix.func(0xC842,af=0x0100,de=1)
        matrix.readonly(0,'A-native-ROM-512K',RUNTIME/'specialist-os/system-flash.bin')
        matrix.start('rom64',flash=False,system=OUT/'rom64/SystemRom_emulator.bin',disk=OUT/'external-rom64/FlashDisk_emulator.bin')
        matrix.install('Rom');matrix.readonly(7,'H-ROM-64K',RUNTIME/'specialist-os/rom-disk.bin')
        matrix.report['completedUtc']=datetime.now(timezone.utc).isoformat()
        matrix.report['binaries']={name:sha(ROOT/'bin'/name) for name in ['DOS.sys','DISKBEN.i80','Flash.i80','FlashFst.i80','FlashSys.i80','Rom.i80','RAMD.i80']}
        matrix.save()
    finally:matrix.stop()

if __name__=='__main__':main()
