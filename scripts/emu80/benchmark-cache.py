"""Изолированный опыт размеров кэша метаданных, DOS/Raw, файл 1–8 КиБ."""
import argparse, json, os, runpy, struct
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ns = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
OUT = ROOT/'docs/diskbench/cache-001'
OUT.mkdir(parents=True, exist_ok=True)
Matrix = ns['Matrix']
Matrix.__init__.__globals__['OUT'] = OUT
command, ps = ns['command'], ns['ps']
FILE_KIB = 1

def fixture(variant='large'):
    folder = OUT/(variant+'-image'); folder.mkdir(exist_ok=True)
    manifest = (ROOT/'scripts/images/manifests/SystemRom.txt').read_text('utf-8-sig')
    manifest = manifest.replace('../bin/DOS.sys', os.path.relpath(OUT/variant/'DOS.sys', ROOT/'assets'))
    (OUT/(variant+'-manifest.txt')).write_text(manifest, encoding='utf-8')
    builder = (ROOT/'scripts/images/build-image.ps1').read_text('utf-8-sig')
    builder = builder.replace('$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)', '$repositoryRoot = '+ps(ROOT))
    builder = builder.replace('$imageDirectory = Join-Path $repositoryRoot "images"', '$imageDirectory = '+ps(folder))
    builder = builder.replace('Join-Path $PSScriptRoot "manifests\\SystemRom.txt"', ps(OUT/(variant+'-manifest.txt')))
    path = folder/'build.ps1'; path.write_text(builder, encoding='utf-8-sig')
    command(['powershell.exe','-NoProfile','-File',path,'SystemEmuWritable'])
    return folder/'SystemRom_emu80_writable.bin'

def metadata(x):
    # Один верхний сектор: нижний кэш и его таблица остаются нетронутыми.
    def read(i):
        assert not x.func(0xC863, bc=0x0200, de=i, hl=x.s['highBuffer'])['flags']['C']
        return x.m.read(x.s['highBuffer'],256)
    boot = read(0)
    word = lambda n: struct.unpack_from('<H',boot,n)[0]
    count = word(14)+boot[16]*word(22)+(word(17)*32+255)//256
    return boot+b''.join(read(i) for i in range(1,count))

def run_case(x, drive, driver, mode, variant):
    info = x.init_app(drive, mode)
    s,m = x.s,x.m
    if 'cacheBenchmarkSample' in x.ds:
        target=s['timer_sample']
        m.write(x.ds['cacheBenchmarkSample'],b'\xC3'+struct.pack('<H',target))
    source=s['sectorBuffer'];size=FILE_KIB*1024; end=source+size
    guard_start,guard_size=((0x15C0,0x40) if source==0x1600 else (0x1800,0x800)) if FILE_KIB>1 else (0x26C0,0x140)
    capacity=x.ds['FAT_CACHE_CAPACITY']
    table=x.ds['cacheData']; cache_start=x.ds.get('FAT_CACHE_ADDR',0xFB00)
    layout=OUT/variant/'DOS.layout.json'
    if layout.exists():
        for define in json.loads(layout.read_text())['defines']:
            if define.startswith('FAT_CACHE_ADDR='):
                value=define.split('=',1)[1];cache_start=int(value[:-1],16) if value.lower().endswith('h') else int(value)
    overlaps=lambda a,b,c,d:a<d and c<b
    assert not overlaps(table,table+8*capacity,source,end)
    assert not overlaps(cache_start,cache_start+256*capacity,source,end)
    assert not overlaps(table,table+8*capacity,guard_start,guard_start+guard_size)
    assert len(x.appBinary.read_bytes())<=guard_start
    assert len(x.appBinary.read_bytes())<=table
    m.write(s['v_fileKiB'],bytes([FILE_KIB])); m.write(s['v_fileSectors'],bytes([FILE_KIB*4]))
    m.write(s['v_bufferEndHi'],bytes([end//256]))
    m.write(s['tmpDescriptor']+28,(size-1).to_bytes(4,'little'))
    # Сигнальные байты между буфером, таблицей и кэшем.
    m.write(guard_start,b'\xA5'*guard_size)
    disk = ns['RUNTIME']/'specialist-os'/('system-flash.bin' if drive==0 else 'flash-disk.bin')
    before = disk.read_bytes() if drive != 1 else metadata(x)
    x.traps(s['file_cleanup']); m.call('regs_set',pc=s['file_write_case'],sp=0x8FDF)
    previous = [(0,0,0)]*4; samples=[]
    for attempt in range(1,4):
        x.until(s['file_cleanup'])
        assert m.read(source,size)==bytes(attempt^(a>>8)^(a&255) for a in range(source,end))
        assert m.read(guard_start,guard_size)==b'\xA5'*guard_size
        raw=m.read(s['statAll'],40)
        current=[struct.unpack_from('<IIH',raw,i*10) for i in range(4)]
        samples.append([[a-b for a,b in zip(c,p)] for c,p in zip(current,previous)])
        previous=current
        # На границе файла грязные слоты уже сохранены.
        cache = x.m.read(x.ds['cacheData'], 8*x.ds['FAT_CACHE_CAPACITY'])
        assert all(cache[i+6]==0 for i in range(0,len(cache),8))
        if attempt == 3: x.traps(s['file_results'])
        else: m.call('debug_step',mode='into')
    x.until(s['file_results']); assert m.read(s['v_fileOwned'],1)==b'\0'
    metrics=dict(zip(['rawWrite','rawRead','fileWrite','fileRead'],current))
    if drive==0:
        h=m.read(s['resultsHigh'],40)
        metrics.update(rawReadHigh=struct.unpack_from('<IIH',h,10),fileReadHigh=struct.unpack_from('<IIH',h,30))
        if struct.unpack_from('<I',h,0)[0]:metrics['rawWriteHigh']=struct.unpack_from('<IIH',h,0)
    verify=[int.from_bytes(m.read(s[k],4),'little') for k in ['statRawVerify','statFileVerify']]
    m.stop_at(s['exit_wait']);m.until(s['exit_wait'])
    key=f'{variant}-{drive}-{driver}-{mode}';m.screenshot(OUT/(key+'.png'))
    after=disk.read_bytes() if drive != 1 else metadata(x)
    x.check_integrity(before,after)
    row=dict(info,variant=variant,mode=mode,cacheSlots=capacity,cacheAddress=cache_start,tableAddress=table,sourceAddress=source,highAddress=s['highBuffer'],metrics=metrics,verifyTicks=verify,perPass=samples,integrity=True)
    print(key,{k:round(size*3*7812.5/1000/v[0],2) for k,v in metrics.items()},flush=True)
    return row

def main():
    global OUT,FILE_KIB
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--sampled',action='store_true')
    p.add_argument('--report-directory',type=Path,default=OUT)
    p.add_argument('--variants',nargs='+',default=['baseline','large'])
    p.add_argument('--benchmark-binary',type=Path)
    p.add_argument('--file-kib',type=int,choices=range(1,9),default=1)
    p.add_argument('--resume',action='store_true')
    args=p.parse_args();OUT=args.report_directory.resolve()
    FILE_KIB=args.file_kib
    Matrix.__init__.__globals__['OUT']=OUT
    if (OUT/'results.json').exists(): raise RuntimeError('Completed report already exists')
    images={k:fixture(k) for k in args.variants if k!='baseline' or args.sampled}
    x=Matrix(); result=dict(fileBytes=FILE_KIB*1024,passes=3,cpuHz=2000000,sampled=args.sampled,rows=[])
    if args.resume:
        result=json.loads((OUT/'partial.json').read_text())
        assert result['fileBytes']==FILE_KIB*1024 and result['sampled']==args.sampled
        x.report['phases']=json.loads((OUT/'raw.json').read_text())['phases']
    if args.benchmark_binary:
        x.appBinary=args.benchmark_binary.resolve()
        x.s=ns['app_symbols'](x.appBinary.with_suffix('.sym'))
    result['benchmarkBinary']=dict(path=str(x.appBinary),sha256=ns['sha'](x.appBinary))
    try:
        for variant in args.variants:
            if sum(r['variant']==variant for r in result['rows'])==8:continue
            dos = OUT/variant/'DOS.sys' if variant!='baseline' or args.sampled else None
            x.start(variant,system=images.get(variant),dos=dos)
            x.install('FlashSys')
            for mode in [1,6]:result['rows'].append(run_case(x,0,'FlashSys',mode,variant))
            for driver in ['Flash','FlashFst']:
                x.install(driver)
                for mode in [1,6]: result['rows'].append(run_case(x,7,driver,mode,variant))
            x.format_ram()
            for mode in [1,6]:result['rows'].append(run_case(x,1,'RAMD',mode,variant))
            (OUT/'partial.json').write_text(json.dumps(result,indent=2)+'\n')
        result['completedUtc']=datetime.now(timezone.utc).isoformat()
        (OUT/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    finally:x.stop()

if __name__=='__main__':main()
