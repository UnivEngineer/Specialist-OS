"""Проверка R и G оригинального Monitor-2 с входом через RkTapeInHook Emu80."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import traceback
import uuid
import importlib.util

ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'scripts/games'),str(ROOT/'scripts/emu80')]
spec=importlib.util.spec_from_file_location('monitor_pilot',ROOT/'scripts/games/check-monitor-pilot.py')
pilot=importlib.util.module_from_spec(spec);spec.loader.exec_module(pilot)
from mcp_pool import lease
from campaign_play import budi

OUT=ROOT/'docs/compat/game-campaign/native-monitor'
GAME=ROOT/'assets/games/families/budi/originals/Budi2-std.rks'

def main():
    target=OUT/('tape-'+uuid.uuid4().hex[:12]);target.mkdir(parents=True)
    result={'game':str(GAME.relative_to(ROOT)),'source_sha256':pilot.sha(GAME),'rom_sha256':pilot.sha(pilot.SOURCE/'spec_m2.rom'),
            'method':'Оригинальный Monitor-2: F1, Enter, R, Enter; штатный RkTapeInHook читает предварительно назначенный RKS; G, Enter.',
            'caveat':'Перехват байтов ленты, а не проверка физической формы сигнала.', 'frames':{},'events':[]}
    try:
        with lease(size=16,port=19792,artifact_root=OUT/'tape-slots') as slot:
            runtime=slot.runtime/'spec';shutil.copytree(pilot.SOURCE,runtime,dirs_exist_ok=True)
            shutil.copyfile(GAME,runtime/'input.rks')
            conf=runtime/'spec.conf'
            conf.write_text(conf.read_text(encoding='utf-8-sig')+'\ntapeInFile.fileName = "spec/input.rks"\nkbdLayout.layout = smart\nbeepSoundSource.muted = yes\ntapeSoundSource.muted = yes\n',encoding='utf8')
            (runtime/'spec.opt').write_text('beepSoundSource.muted = yes\ntapeSoundSource.muted = yes\nkbdLayout.layout = smart\n',encoding='utf8')
            g=pilot.Native(slot,target,result)
            g.call('debug_break');g.call('bp_clear');g.call('kbd_reset')
            st=g.call('switch_platform',platform='spec')
            if st.get('current')!='spec' or not slot.verify_owner():raise RuntimeError('Платформа или владелец не подтверждены')
            g.call('debug_break');g.call('emu_speed',factor=32);g.call('emu_reset');g.advance(1500);g.frame('boot')
            g.key('f1');g.key('enter');g.advance(250);g.frame('monitor')
            g.key('r');g.key('enter');g.advance(60000);g.advance(60000);g.frame('read')
            data=GAME.read_bytes();start=int.from_bytes(data[:2],'little');end=int.from_bytes(data[2:4],'little')
            loaded=g.read(start,end-start+1)
            result.update(start=start,end=end,loaded_sha256=hashlib.sha256(loaded).hexdigest(),body_matches=loaded==data[4:5+end-start],
                          read_pc=g.regs()['pc'])
            if not result['body_matches']:raise RuntimeError('После команды R тело RKS не совпало с исходным')
            g.key('g');g.key('enter');g.advance(400);budi(g)
            result['gameplay']='verified';g.points();g.key('clear',300,500);g.frame('return')
            result['return_pc']=g.regs()['pc']
    except Exception as e:
        result.update(gameplay='unconfirmed',error=str(e),traceback=traceback.format_exc())
    (target/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    (OUT/'tape-index.json').write_text(json.dumps({'evidence':target.name,**result},ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    print(result.get('gameplay'),result.get('error',''),flush=True)

if __name__=='__main__':main()
