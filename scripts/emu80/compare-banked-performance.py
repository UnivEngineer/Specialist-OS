"""Сравнение двух контрольных прогонов: код 1 при замедлении выше порога."""
from pathlib import Path
import argparse,json

def read(root):
    result={}
    copy=json.loads((root/'copy/result.json').read_text())
    disk=json.loads((root/'disks/raw.json').read_text())
    for row in copy['rows']:
        name=row['name'].replace('R3-', 'Builtin block-').replace('R4-', 'RAMD R4-')
        key=('copy',name,row['caller'],*row['source'],*row['destination'],row['size'])
        result[key]=row['ms']
    for row in disk['rows']:
        for metric,value in row['metrics'].items():
            key=('disk',row['id'],metric)
            result[key]=value[0]*256000/disk['pitHz']/disk['passes']
    signature=(copy['cpuHz'],disk['cpuHz'],disk['pitHz'],disk['fileBytes'],disk['inventory'])
    return result,signature

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline',type=Path,required=True)
    p.add_argument('--current',type=Path,required=True)
    p.add_argument('--threshold-percent',type=float,default=5)
    a=p.parse_args()
    if a.threshold_percent<0:p.error('Threshold must be nonnegative')
    old,old_sig=read(a.baseline);new,new_sig=read(a.current)
    if old_sig!=new_sig:p.error('CPU/PIT, payload size or disk inventory differs; compare these runs explicitly')
    missing=set(old)-set(new)
    rows=[]
    for key in sorted(set(old)&set(new)):
        if old[key]<=0:continue
        delta=(new[key]/old[key]-1)*100
        rows.append({'case':key,'before_ms':old[key],'after_ms':new[key],
                     'change_percent':delta,'regression':delta>a.threshold_percent})
    print(json.dumps({'missing':[list(k) for k in sorted(missing)],'rows':rows},indent=2))
    raise SystemExit(1 if missing or any(r['regression'] for r in rows) else 0)

if __name__=='__main__':main()
