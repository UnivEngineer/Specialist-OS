"""ROM Monitor-2: команды R/G через перенаправление ленты, без записей отладчиком."""
from concurrent.futures import ThreadPoolExecutor, as_completed
import argparse
import csv
import hashlib
import json
from pathlib import Path
import shutil
import sys
import traceback

ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'scripts/games'),str(ROOT/'scripts/emu80'),str(ROOT/'scripts/images')]
import importlib.util
sp=importlib.util.spec_from_file_location('monitor_pilot',ROOT/'scripts/games/check-monitor-pilot.py')
pilot=importlib.util.module_from_spec(sp);sp.loader.exec_module(pilot)
from executable import checksum
from game_inventory import source_record, apply_lafan_headers, route_bytes
from mcp_pool import lease
sys.path.insert(0,str(ROOT/'.local/game-campaign-python'))
from PIL import Image
ROM_ERROR_CROP_SHA256='a3446787a306d39529296c40308b7e9bec088408b49514af389422f7c7ca3806'
ROM_SUM_CROP_SHA256='b6cef86bf189bd473003388b2dce03eb929bb874a80e4de4e66d1f06351e7629'

OUT=ROOT/'docs/compat/game-campaign/native-monitor'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def save(path,obj):
    tmp=path.with_suffix('.tmp')
    tmp.write_text(json.dumps(obj,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    tmp.replace(path)

def work(port,jobs):
    output=[]
    try:
        with lease(size=16,port=port,artifact_root=OUT/'tape-catalog-slots') as slot:
            runtime=slot.runtime/'spec'
            shutil.copytree(pilot.SOURCE,runtime,dirs_exist_ok=True)
            conf=runtime/'spec.conf'
            conf.write_text(conf.read_text(encoding='utf-8-sig')+'\ntapeInFile.fileName = "spec/input.rks"\nkbdLayout.layout = smart\nbeepSoundSource.muted = yes\ntapeSoundSource.muted = yes\n',encoding='utf8')
            (runtime/'spec.opt').write_text('beepSoundSource.muted = yes\ntapeSoundSource.muted = yes\nkbdLayout.layout = smart\n',encoding='utf8')
            for h,d,data in jobs:
                folder=OUT/'tape-catalog'/h;folder.mkdir(parents=True,exist_ok=True)
                result=dict(rks_sha256=h,source=d['file'],source_sha256=d['source_sha256'],recipe=d['recipe'],
                            evidence=folder.relative_to(OUT).as_posix(),port=port,frames={},events=[],
                            rom_sha256=pilot.sha(pilot.SOURCE/'spec_m2.rom'),
                            source_format=('LIK' if data.startswith(bytes.fromhex('708f828f')) else 'named_or_tape' if data.startswith((b'\xd3'*3,b'\xd9'*3,b'\xe6')) else 'plain_rks'),
                            checksum_status='unconfirmed',
                            input_complete=False,classification='unconfirmed')
                if (folder/'result.json').exists():shutil.copyfile(folder/'result.json',folder/'previous-result.json')
                g=pilot.Native(slot,folder,result)
                try:
                    g.call('debug_break');g.call('bp_clear');g.call('kbd_reset')
                    if g.call('sys_info')['platform']=='spec':g.call('switch_platform',platform='specialist-os')
                    (runtime/'input.rks').write_bytes(data)
                    st=g.call('switch_platform',platform='spec')
                    if st.get('current')!='spec' or not slot.verify_owner():raise RuntimeError('Платформа или владелец не подтверждены')
                    g.call('debug_break');g.call('emu_speed',factor=32);g.call('emu_reset');g.advance(1200)
                    g.key('f1',100,120);g.key('enter',100,300);g.frame('monitor')
                    g.key('r',100,120);g.key('enter',100,0)
                    # Ожидаем следующий вход монитора после завершения R.
                    g.points(0xc190)
                    actual=g.advance(60000)
                    if actual!=0xc190:actual=g.advance(60000)
                    result['read_pc']=actual;result['input_complete']=actual==0xc190
                    g.points();g.frame('read')
                    crop=Image.open(folder/'read.png').convert('L').point(lambda x:255 if x else 0).crop((0,30,120,38))
                    result['rom_message_crop_sha256']=sha(crop.tobytes())
                    result['rom_input_error_visible']=result['rom_message_crop_sha256']==ROM_ERROR_CROP_SHA256
                    sumcrop=Image.open(folder/'read.png').convert('L').point(lambda x:255 if x else 0).crop((0,30,105,38))
                    result['rom_sum_crop_sha256']=sha(sumcrop.tobytes())
                    result['rom_checksum_message_visible']=result['rom_sum_crop_sha256']==ROM_SUM_CROP_SHA256
                    if result['source_format']=='LIK':
                        result['checksum_status']='неприменимо: первый заголовок принадлежит LIK, не RKS'
                    if len(data)>=4 and result['source_format']=='plain_rks':
                        start=int.from_bytes(data[:2],'little');end=int.from_bytes(data[2:4],'little')
                        result.update(header_start=start,header_end=end)
                        if start<=end<0x9000 and len(data)>=5+end-start:
                            body=data[4:5+end-start];tail=data[5+end-start:]
                            result.update(expected_body_sha256=sha(body),checksum_expected=checksum(body),
                                          checksum_tail=tail.hex(),checksum_matches=tail==checksum(body).to_bytes(2,'little'),
                                          checksum_status='valid' if tail==checksum(body).to_bytes(2,'little') else ('missing_or_truncated' if len(tail)<2 else 'invalid'))
                            actual_body=g.read(start,len(body))
                            result.update(read_body_sha256=sha(actual_body),body_matches=actual_body==body)
                    if result['input_complete'] and result['rom_input_error_visible']:
                        result['classification']='rom_read_error'
                    elif not result['input_complete']:
                        result['classification']='monitor_read_not_completed'
                    elif not result['rom_checksum_message_visible']:
                        result['classification']='rom_read_status_uncertain'
                    elif not result.get('body_matches'):
                        result['classification']='read_completed_body_mismatch'
                    else:
                        # Перевод управления происходит только командой G оригинального ROM.
                        g.advance(500)
                        g.points(result['header_start'])
                        g.key('g',100,160);g.frame('g-typed');g.key('enter',100,0)
                        pc=g.regs()['pc']
                        if pc!=result['header_start']:pc=g.advance(60000)
                        result.update(g_entry_pc=pc,g_entry_confirmed=pc==result['header_start']);g.frame('g-ended')
                        if not result['g_entry_confirmed']:
                            result['classification']='g_entry_unconfirmed'
                        else:
                            result['body_sha256_at_g']=sha(g.read(result['header_start'],len(body)))
                            result['body_matches_at_g']=result['body_sha256_at_g']==result['expected_body_sha256']
                            g.frame('entry');g.points();g.advance(1500);g.frame('running')
                            result['classification']='rom_read_and_g_entry'
                except Exception as e:
                    result.update(error=str(e),traceback=traceback.format_exc())
                    try:g.call('debug_break');g.frame('failure')
                    except Exception:pass
                save(folder/'result.json',result)
                output.append((h,result))
                print(port,len(output),'/',len(jobs),d['file'],result['classification'],flush=True)
                if not slot.verify_owner():break
    except Exception as e:
        print(port,'worker error',repr(e),flush=True)
    return output

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--limit',type=int)
    parser.add_argument('--file', help="Проверить один путь из каталога игр")
    parser.add_argument('--source',type=Path,default=pilot.SOURCE)
    args=parser.parse_args()
    pilot.SOURCE=args.source.resolve()
    OUT.mkdir(parents=True,exist_ok=True);index=OUT/'tape-catalog.json'
    existing=json.loads(index.read_text(encoding='utf8')) if index.exists() else {}
    for file in (OUT/'tape-catalog').glob('*/result.json'):
        saved=json.loads(file.read_text(encoding='utf8'))
        existing.setdefault(saved['rks_sha256'],saved)
    cases=json.loads((OUT/'catalog-index.json').read_text(encoding='utf8'))
    retired={h:row for h,row in existing.items() if h not in cases}
    if retired:
        save(OUT/'tape-index-superseded.json',retired)
    existing={h:row for h,row in existing.items() if h in cases}
    with (ROOT/'assets/games/catalog.csv').open(encoding='utf-8-sig',newline='') as stream:
        recs=[source_record(row) for row in csv.DictReader(stream)]
    apply_lafan_headers(recs);recs={r['file']:r for r in recs}
    jobs=[]
    for h,d in sorted(cases.items()):
        if args.file and d['file'].removeprefix('assets/games/')!=args.file:
            continue
        if h in existing:
            old=existing[h]
            retry=old.get('classification')=='g_entry_unconfirmed' or (old.get('classification')=='unconfirmed' and 'jet-set' not in old.get('source','').lower())
            if not retry:continue
        try:
            data=route_bytes(recs[d['file']],'rks',d['recipe'])
            if sha(data)!=h:raise ValueError('SHA маршрута не совпадает')
            jobs.append((h,d,data))
        except Exception as e:
            existing[h]=dict(rks_sha256=h,source=d['file'],classification='source_reconstruction_failed',error=str(e))
    if args.limit is not None:jobs=jobs[:args.limit]
    print('R/G pending',len(jobs),flush=True);save(index,existing)
    if not jobs:return
    ports=(19791,19792,19793,19794);groups=[jobs[i::len(ports)] for i in range(len(ports))]
    with ThreadPoolExecutor(max_workers=len(ports)) as executor:
        futures=[executor.submit(work,p,g) for p,g in zip(ports,groups) if g]
        for future in as_completed(futures):
            for h,result in future.result():
                existing[h]=result
            save(index,existing)
    print('R/G indexed',len(existing),flush=True)

if __name__=='__main__':main()
