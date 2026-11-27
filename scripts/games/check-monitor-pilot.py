"""Пилот двух игр в исходном Monitor-2 платформы Specialist из Emu80 dist."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
import traceback
import uuid

ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'scripts/games'),str(ROOT/'scripts/emu80')]
from campaign_guest import Bench, Observation
from campaign_play import budi, ochko
from mcp_pool import lease

SOURCE=ROOT/'docs/compat/game-campaign/fixtures/native/dist/spec'
OUT=ROOT/'docs/compat/game-campaign/native-monitor'
GAMES=[('budi2',ROOT/'assets/games/families/budi/originals/Budi2-std.rks',budi),
       ('ochko',ROOT/'assets/games/std/Ochko.rks',ochko)]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

class Native(Bench):
    def __init__(self,slot,folder,result):
        super().__init__(slot.url)
        self.slot=slot
        self.out=folder
        self.result=result
        self.return_points={}
        self.keys=result.setdefault('events',[])
    def regs(self):
        return {k:v['dec'] for k,v in self.call('cpu_state')['registers'].items()}
    def points(self,*pcs):
        self.call('bp_clear')
        for pc in set(pcs):
            self.call('bp_set',addr=pc,type='exec')
    def advance(self,ms):
        return self.call('emu_run_for',ms=ms)['registers']['pc']['dec']
    def seek(self,pc,ms=12000,check=None):
        self.points(pc)
        for _ in range(40):
            actual=self.advance(ms)
            if actual!=pc:
                return False
            if check is None or check():
                return True
            self.call('debug_step',mode='into')
        return False
    def frame(self,name):
        p=self.out/(name+'.png')
        self.screenshot(p)
        self.result.setdefault('frames',{})[name]={'file':p.name,'sha256':sha(p)}
    def key(self,key,duration=100,settle=120):
        before=self.regs()['pc']
        self.call('kbd_send',keys=[key],action='tap',duration_ms=duration)
        if settle:self.advance(settle)
        self.keys.append(dict(key=key,action='tap',duration_ms=duration,pc=before))
        return self.regs()['pc']
    def watch_exit(self,pc):
        return False

def main():
    global SOURCE
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',type=int,default=19794)
    parser.add_argument('--source',type=Path,default=SOURCE)
    parser.add_argument('--out',type=Path,default=OUT)
    args=parser.parse_args()
    SOURCE=args.source.resolve()
    args.out.mkdir(parents=True,exist_ok=True)
    attempt=args.out/('attempt-'+uuid.uuid4().hex[:12]);attempt.mkdir()
    report={'platform':'spec','loader':'emu_load: штатный SpecFileLoader из upstream Emu80','attempt':attempt.name,
            'rom_sha256':sha(SOURCE/'spec_m2.rom'),'config_sha256':sha(SOURCE/'spec.conf'),
            'help_sha256':sha(SOURCE/'spec.html'),'games':[]}
    try:
        with lease(size=16,port=args.port,artifact_root=args.out/'slots') as slot:
            shutil.copytree(SOURCE,slot.runtime/'spec',dirs_exist_ok=True)
            conf=slot.runtime/'spec/spec.conf'
            conf.write_text(conf.read_text(encoding='utf-8-sig')+'\nkbdLayout.layout = smart\nbeepSoundSource.muted = yes\ntapeSoundSource.muted = yes\n',encoding='utf8')
            (slot.runtime/'spec/spec.opt').write_text('beepSoundSource.muted = yes\ntapeSoundSource.muted = yes\nkbdLayout.layout = smart\n',encoding='utf8')
            machine=Native(slot,attempt,report)
            report.update(pid=slot.pid,hwnd=slot.hwnd,emulator_sha256=sha(slot.runtime/'Emu80qt-mcp.exe'))
            for label,path,play in GAMES:
                folder=attempt/label;folder.mkdir()
                result={'game':label,'file':str(path.relative_to(ROOT)),'source_sha256':sha(path),'frames':{},'events':[]}
                report['games'].append(result)
                g=Native(slot,folder,result)
                try:
                    g.call('debug_break');g.call('bp_clear');g.call('kbd_reset')
                    if g.call('sys_info')['platform']=='spec':
                        g.call('switch_platform',platform='specialist-os')
                    state=g.call('switch_platform',platform='spec')
                    if state.get('current')!='spec' or not slot.verify_owner():raise RuntimeError('Платформа или владелец не подтверждены')
                    g.call('debug_break');g.call('emu_speed',factor=32);g.call('emu_reset');g.advance(1500)
                    g.frame('boot')
                    g.key('f1');g.key('enter');g.advance(500);g.frame('monitor')
                    # Штатный загрузчик Emu80 является отдельным способом загрузки:
                    # он копирует RKS в адресное пространство и устанавливает PC сам.
                    result['load']=g.call('emu_load',file=path.as_posix(),autorun=True,break_after_load=True)
                    g.call('debug_break');g.call('bp_clear')
                    play(g)
                    result['gameplay']='verified'
                    # Захват MCP доступен только источнику MX PIT; фиксируем ответ для spec.
                    g.points();g.call('emu_speed',factor=1)
                    try:
                        g.call('mx_audio',action='start')
                        g.advance(500)
                        sound=g.call('mx_audio',action='stop')
                        result['mx_audio_probe']={k:v for k,v in sound.items() if k!='pcm_base64'}
                    except Exception as error:
                        result['mx_audio_probe']={'error':str(error)}
                    finally:
                        g.call('emu_speed',factor=32)
                    g.points();g.key('clear',300,500);g.frame('exit-key')
                    result['exit_pc']=g.regs()['pc']
                    g.call('emu_reset');g.advance(1500);g.frame('reset')
                    result['return']=('Clear показал приглашение Monitor; Reset возвращает PROGRAM ?.' if label=='budi2' else 'Clear оставил экран «ЕЩЁ?»; Reset возвращает PROGRAM ?.')
                except Exception as ex:
                    result.update(gameplay='unconfirmed',error=str(ex),traceback=traceback.format_exc())
                    try:g.call('debug_break');g.frame('failure')
                    except Exception:pass
                (folder/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
                print(label,result.get('gameplay'),result.get('error',''),flush=True)
    finally:
        # Сравниваем именно снимки текущих контрольных проб; оттенок платформ различается.
        sys.path.insert(0,str(ROOT/'.local/game-campaign-python'))
        from PIL import Image, ImageChops
        trials=ROOT/'docs/compat/game-campaign/trials'
        ids={'budi2':'rks-449588e8175e752819e24a37d1f9184bbb412e3f359b57d14378d7e6685425fc-MON',
             'ochko':'rks-17af65cbcf376a848517244d5cad9ce5cc68687e172a38d6b6228af647cb395d-MON'}
        for g in report['games']:
            folder=trials/ids[g['game']]
            if not (folder/'result.json').is_file():continue
            reference=json.loads((folder/'result.json').read_text(encoding='utf8'))
            frames={}
            names=('before','moved','after') if g['game']=='budi2' else ('title','before','dealt')
            for name in names:
                a=attempt/g['game']/(name+'.png');b=folder/reference['evidence']/(name+'.png')
                if not a.is_file() or not b.is_file():continue
                left=Image.open(a).convert('L').point(lambda x:255 if x else 0)
                right=Image.open(b).convert('L').point(lambda x:255 if x else 0)
                frames[name]=dict(native_sha256=sha(a),mxos_sha256=sha(b),
                                  same_binary_pixels=left.size==right.size and ImageChops.difference(left,right).getbbox() is None)
            g['comparison_mxos_mon']=dict(trial_id=ids[g['game']],evidence=reference['evidence'],
                action_proof=reference.get('action_proof'),same_action_proof=g.get('action_proof')==reference.get('action_proof'),
                frames=frames,palette='STD #FFFFFF; MXOS MON #C0C0C0, чёрный фон')
        report['audio_limit']='MCP mx_audio снимает только SpecMxPit8253SoundSource; spec использует GeneralSoundSource, запись звука STD не выполнялась.'
        (args.out/'index.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf8')

if __name__=='__main__':main()
