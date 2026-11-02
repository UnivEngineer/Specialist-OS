"""A: прямой верхний источник Raw WRITE, все режимы и размеры 1/8/24 КиБ."""
import json, runpy, struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
n=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
Matrix=n['Matrix']; out=ROOT/'docs/diskbench/write-high-001';out.mkdir(exist_ok=True)
Matrix.__init__.__globals__['OUT']=out
x=Matrix();rows=[]
try:
    x.start('write-high');x.install('FlashSys')
    for kib in [1,8,24]:
        for mode in [1,5,6]:
            x.init_app(0,mode);m,s=x.m,x.s
            m.write(s['v_budget'],b'\1');m.write(s['v_fileKiB'],bytes([kib]))
            m.write(s['v_fileSectors'],bytes([kib*4]));m.write(s['v_bufferEndHi'],bytes([0x20+kib*4]))
            m.write(s['tmpDescriptor']+28,(kib*1024-1).to_bytes(4,'little'))
            disk=n['RUNTIME']/'specialist-os/system-flash.bin';before=disk.read_bytes()
            x.traps(s['file_cleanup']);m.call('regs_set',pc=s['file_write_case'],sp=0x8FDF);x.until(s['file_cleanup'])
            assert m.read(0x2000,kib*1024)==bytes(1^(a>>8)^(a&255) for a in range(0x2000,0x2000+kib*1024))
            low=m.read(s['resultsLow'],40);high=m.read(s['resultsHigh'],40)
            metrics={}
            for name,offset in [('rawWrite',0),('rawRead',10),('fileWrite',20),('fileRead',30)]:
                metrics[name]=struct.unpack_from('<IIH',low,offset)
                if name!='fileWrite':metrics[name+'High']=struct.unpack_from('<IIH',high,offset)
            assert all(metrics[k][0]>0 for k in metrics)
            x.traps(s['file_results']);x.until(s['file_results'])
            m.stop_at(s['exit_wait']);m.until(s['exit_wait']);m.screenshot(out/f'{kib}-{mode}.png')
            assert m.read(s['v_fileOwned'],1)==b'\0';x.check_integrity(before,disk.read_bytes())
            row=dict(sizeKiB=kib,mode=mode,metrics=metrics,integrity=True)
            rows.append(row);(out/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
            print(kib,mode,{k:round(kib*1024*7812.5/1000/v[0],2) for k,v in metrics.items()},flush=True)
finally:x.stop()
