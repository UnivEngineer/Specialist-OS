"""Историческая платформа Specialist: каталог проверенных исходных RKS без отладочной записи."""
import csv
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import traceback

ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'scripts/games'),str(ROOT/'scripts/images')]
import importlib.util
spec=importlib.util.spec_from_file_location('monitor_pilot',ROOT/'scripts/games/check-monitor-pilot.py')
pilot=importlib.util.module_from_spec(spec);spec.loader.exec_module(pilot)
from mcp_pool import lease
from game_inventory import source_record, apply_lafan_headers, route_bytes

OUT=ROOT/'docs/compat/game-campaign/native-monitor'
TRIALS=ROOT/'docs/compat/game-campaign/trials'

def digest(data):
    return hashlib.sha256(data).hexdigest()

def save(path,obj):
    tmp=path.with_suffix('.tmp')
    tmp.write_text(json.dumps(obj,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    tmp.replace(path)

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    index=OUT/'catalog-index.json'
    catalog=json.loads(index.read_text(encoding='utf8')) if index.exists() else {}
    with (ROOT/'assets/games/catalog.csv').open(encoding='utf-8-sig',newline='') as stream:
        records=[source_record(row) for row in csv.DictReader(stream)]
    apply_lafan_headers(records)
    records={x['file']:x for x in records}
    candidates={}
    for p in sorted(TRIALS.glob('*/result.json')):
        d=json.loads(p.read_text(encoding='utf8'))
        if d.get('profile')!='MON' or not d['id'].startswith('rks-'):continue
        rec=records.get(d['file'])
        if not rec or rec.get('blocked'):continue
        try:
            data=route_bytes(rec,'rks',d['recipe'])
        except (ValueError,KeyError,TypeError,FileNotFoundError):
            continue
        h=digest(data)
        if h!=d['id'].split('-')[1]:continue
        candidates.setdefault(h,(d,ROOT/d['file'],data))
    active = {h:row for h,row in catalog.items() if h in candidates}
    if len(active)!=len(catalog):
        save(OUT/"catalog-index-superseded.json", {h:row for h,row in catalog.items() if h not in candidates})
        save(index,active)
    catalog=active
    todo=[(h,*v) for h,v in candidates.items() if h not in catalog]
    print('native catalog:',len(candidates),'pending:',len(todo),flush=True)
    if not todo:return
    with lease(size=16,port=19794,artifact_root=OUT/'catalog-slots') as slot:
        import shutil
        shutil.copytree(pilot.SOURCE,slot.runtime/'spec',dirs_exist_ok=True)
        conf=slot.runtime/'spec/spec.conf'
        conf.write_text(conf.read_text(encoding='utf-8-sig')+'\nkbdLayout.layout = smart\nbeepSoundSource.muted = yes\ntapeSoundSource.muted = yes\n',encoding='utf8')
        (slot.runtime/'spec/spec.opt').write_text('beepSoundSource.muted = yes\ntapeSoundSource.muted = yes\nkbdLayout.layout = smart\n',encoding='utf8')
        for i,(h,d,path,data) in enumerate(todo,1):
            folder=OUT/'catalog'/h;folder.mkdir(parents=True,exist_ok=True)
            result=dict(source_sha256=pilot.sha(path),rks_sha256=h,file=d['file'],mxos_trial=d['id'],mxos_evidence=d['evidence'],
                        scenario=d.get('scenario'),frames={},events=[],platform='spec',
                        rom_sha256=pilot.sha(pilot.SOURCE/'spec_m2.rom'),loader='SpecFileLoader: reset, load and set PC internally',
                        evidence=folder.relative_to(OUT).as_posix(),classification='unconfirmed')
            media=folder/'source.rks'
            media.write_bytes(data)
            result['rks_sha256']=digest(data)
            result['recipe']=d['recipe']
            g=pilot.Native(slot,folder,result)
            try:
                if len(data)<7:raise ValueError('Неполный заголовок RKS')
                start=int.from_bytes(data[:2],'little');end=int.from_bytes(data[2:4],'little')
                if end<start or end>=0x9000 or len(data)<4+end-start+1:
                    raise ValueError('Нет непрерывного RKS в доступной RAM 0000h–8FFFh')
                result.update(load=start,end=end,payload_sha256=digest(data[4:5+end-start]))
                g.call('debug_break');g.call('bp_clear');g.call('kbd_reset')
                if g.call('sys_info')['platform']=='spec':g.call('switch_platform',platform='specialist-os')
                state=g.call('switch_platform',platform='spec')
                if state.get('current')!='spec' or not slot.verify_owner():raise RuntimeError('Платформа или владелец не подтверждены')
                g.call('debug_break');g.call('emu_speed',factor=32)
                reply=g.call('emu_load',file=media.as_posix(),autorun=True,break_after_load=True)
                g.call('debug_break');g.call('bp_clear')
                state=g.regs();body=g.read(start,end-start+1)
                result.update(loader=reply,entry_pc=state['pc'],loaded_sha256=digest(body),
                              entry_body_matches=body==data[4:5+end-start],expected_entry=start)
                if state['pc']!=start or not result['entry_body_matches']:
                    raise RuntimeError('Точка входа или тело RKS не совпадают')
                g.frame('entry');g.advance(5500);g.frame('title')
                scenario=d.get('scenario') or {}
                for key in scenario.get('start_keys',[])[:8]:
                    g.key(key,150,160)
                g.advance(700);g.frame('before')
                for key in scenario.get('action_keys',[])[:10]:
                    g.key(key,160,150)
                g.advance(500);g.frame('after')
                result['classification']='input_unconfirmed'
                result['limitation']='Сценарий и кадры без независимой проверки управляемого объекта; автоматическим успехом игры не считать.'
            except Exception as e:
                result['error']=str(e);result['traceback']=traceback.format_exc()
                try:g.call('debug_break');g.frame('failure')
                except Exception:pass
            catalog[h]=result;save(index,catalog)
            print(i,'/',len(todo),path.name,result['classification'],result.get('error',''),flush=True)
    print('done:',len(catalog),flush=True)

if __name__=='__main__':main()
