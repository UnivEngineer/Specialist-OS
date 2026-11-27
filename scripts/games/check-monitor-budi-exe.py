"""EXE BUDI: перенос точной нагрузки в RKS и проверка ROM R/G."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import sys
import traceback

ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'scripts/games'),str(ROOT/'scripts/emu80'),str(ROOT/'scripts/images')]
spec=importlib.util.spec_from_file_location('monitor_pilot',ROOT/'scripts/games/check-monitor-pilot.py')
pilot=importlib.util.module_from_spec(spec);spec.loader.exec_module(pilot)
from mcp_pool import lease
from executable import Image,decode,encode
from campaign_play import budi,gate,action_to
from campaign_controls import budi_dig_start

OUT=ROOT/'docs/compat/game-campaign/native-monitor/budi-exe'
SOURCE=ROOT/'assets/games/families/budi/patched'
NAMES=('Budi-std.exe','Budi2-std.exe','Budi2-std-fff8.exe')

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    results={}
    try:
        with lease(size=16,port=19794,artifact_root=OUT/'slots') as slot:
            runtime=slot.runtime/'spec';shutil.copytree(pilot.SOURCE,runtime,dirs_exist_ok=True)
            conf=runtime/'spec.conf'
            conf.write_text(conf.read_text(encoding='utf-8-sig')+'\ntapeInFile.fileName = "spec/input.rks"\nkbdLayout.layout = smart\nbeepSoundSource.muted = yes\ntapeSoundSource.muted = yes\n',encoding='utf8')
            for name in NAMES:
                source=SOURCE/name;raw=source.read_bytes();image=decode(raw,'exe')
                if image.load!=0 or image.entry!=0:raise ValueError('Ожидалась точка входа и загрузка 0000h')
                packed=encode(Image(image.data,image.load,image.entry),'rks')
                folder=OUT/sha(raw);folder.mkdir(exist_ok=True)
                (folder/'derived.rks').write_bytes(packed)
                result=dict(source=source.relative_to(ROOT).as_posix(),source_sha256=sha(raw),body_sha256=sha(image.data),
                            derived_rks_sha256=sha(packed),derived_rks='derived.rks',load=image.load,entry=image.entry,
                            method='Точное тело EXE упаковано в производный RKS; оригинальный Monitor-2, команды R и G через RkTapeInHook.',
                            caveat='Байтовый перехват ленты; исходный EXE напрямую монитором не загружается.',
                            frames={},events=[])
                results[name]=result;g=pilot.Native(slot,folder,result)
                try:
                    g.call('debug_break');g.call('bp_clear');g.call('kbd_reset')
                    if g.call('sys_info')['platform']=='spec':g.call('switch_platform',platform='specialist-os')
                    (runtime/'input.rks').write_bytes(packed)
                    state=g.call('switch_platform',platform='spec')
                    if state.get('current')!='spec' or not slot.verify_owner():raise RuntimeError('Платформа или владелец не подтверждены')
                    g.call('debug_break');g.call('emu_speed',factor=32);g.call('emu_reset');g.advance(1200)
                    g.key('f1');g.key('enter');g.key('r');g.key('enter')
                    g.advance(60000);g.advance(60000);g.frame('read')
                    loaded=g.read(image.load,len(image.data))
                    result.update(read_body_sha256=sha(loaded),body_matches_before_g=loaded==image.data)
                    if loaded!=image.data:raise RuntimeError('Тело после R не совпадает с EXE')
                    g.points(image.entry);g.key('g',100,160);g.key('enter',100,0)
                    pc=g.regs()['pc']
                    if pc!=image.entry:pc=g.advance(60000)
                    result.update(g_pc=pc,g_entry_confirmed=pc==image.entry)
                    if pc!=image.entry:raise RuntimeError('G не достиг точки входа')
                    result['body_sha256_at_g']=sha(g.read(image.load,len(image.data)))
                    if result['body_sha256_at_g']!=result['body_sha256']:raise RuntimeError('Тело изменилось перед входом')
                    g.points()
                    if name=='Budi-std.exe':
                        def snapshot(guest,label,*args,**kwargs):guest.frame(label)
                        budi_dig_start(g,snapshot,'budi_std_brick')
                        result['start_cell']=0x8852;result['moved_cell']=g.regs()['hl'];g.frame('before-dig')
                        action_to(g,'q',0x05fb)
                        result['brick_before']=9;result['brick_after']=g.read(0x8853,1)[0]
                        if result['brick_after']!=0x48:raise RuntimeError('Кирпич не изменился после Q')
                        g.frame('after-dig')
                    else:
                        budi(g)
                    result['gameplay']='verified'
                    g.points();g.key('clear',300,500);g.frame('return');result['return_pc']=g.regs()['pc']
                except Exception as e:
                    result.update(gameplay='unconfirmed',error=str(e),traceback=traceback.format_exc())
                    try:g.call('debug_break');g.frame('failure')
                    except Exception:pass
                # Привязка к результату родственного запуска RUN -MON.
                trials=ROOT/'docs/compat/game-campaign/trials'
                matches=[]
                for f in trials.glob('*/result.json'):
                    t=json.loads(f.read_text(encoding='utf8'))
                    if t.get('file')==result['source'] and t.get('profile')=='MON':
                        matches.append(dict(id=t['id'],evidence=t['evidence'],action_proof=t.get('action_proof')))
                result['mxos_mon']=matches
                if matches:
                    ref=matches[0];trial=trials/ref['id']/'result.json'
                    ref['result_sha256']=pilot.sha(trial)
                    paired=ROOT/'docs/compat/game-campaign/paired'/ref['id']/'result.json'
                    if paired.is_file():
                        pd=json.loads(paired.read_text(encoding='utf8'))
                        ref.update(paired_result_sha256=pilot.sha(paired),paired_status=pd.get('status'),
                                   paired_object_proof=pd.get('object_proof'),
                                   paired_initial_flash_protection=[a.get('initial_flash_protection') for a in pd.get('arms',[])])
                    sys.path.insert(0,str(ROOT/'.local/game-campaign-python'))
                    from PIL import Image as PILImage, ImageChops
                    frames={}
                    reference_dir=trials/ref['id']/ref['evidence']
                    for frame in ('before','moved','after'):
                        left=folder/(frame+'.png');right=reference_dir/(frame+'.png')
                        if not left.is_file() or not right.is_file():continue
                        a=PILImage.open(left).convert('L').point(lambda z:255 if z else 0)
                        b=PILImage.open(right).convert('L').point(lambda z:255 if z else 0)
                        frames[frame]=dict(native_sha256=pilot.sha(left),mxos_sha256=pilot.sha(right),
                                           same_binary_pixels=a.size==b.size and ImageChops.difference(a,b).getbbox() is None)
                    result['comparison']=dict(same_action_proof=result.get('action_proof')==ref.get('action_proof') if result.get('action_proof') and ref.get('action_proof') else None,frames=frames)
                (folder/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
                print(name,result.get('gameplay'),result.get('error',''),flush=True)
    finally:
        (OUT/'index.json').write_text(json.dumps(results,ensure_ascii=False,indent=2)+'\n',encoding='utf8')

if __name__=='__main__':main()
