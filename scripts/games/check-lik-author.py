"""Монитор автора LIK: загрузка штатным SpecFileLoader, вход J=8100 и архивные игры."""
import hashlib,importlib.util,json,shutil,sys,traceback
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'scripts/games'),str(ROOT/'scripts/emu80')]
spec=importlib.util.spec_from_file_location('pilot',ROOT/'scripts/games/check-monitor-pilot.py')
pilot=importlib.util.module_from_spec(spec);spec.loader.exec_module(pilot)
from mcp_pool import lease
OUT=ROOT/'docs/compat/game-campaign/native-lik'
SOURCE=ROOT/'.local/emu80-pool-pristine/dist/spec'
MON=ROOT/'docs/compat/game-campaign/fixtures/native/lik/mon-bai2.rks'

def main():
    OUT.mkdir(parents=True,exist_ok=True);folder=OUT/'author-monitor';folder.mkdir(exist_ok=True)
    data=MON.read_bytes();sync=data.index(bytes([0xe6]),3);start=int.from_bytes(data[sync+1:sync+3],'little');end=int.from_bytes(data[sync+3:sync+5],'little');body=data[sync+5:sync+6+end-start]
    out=dict(source=MON.as_posix(),sha256=hashlib.sha256(data).hexdigest(),start=start,end=end,body_sha256=hashlib.sha256(body).hexdigest(),frames={},events={})
    try:
        with lease(size=16,port=19794,artifact_root=OUT/'author-slots') as slot:
            directory=slot.runtime/'spec';shutil.copytree(SOURCE,directory,dirs_exist_ok=True)
            conf=directory/'lik.conf';conf.write_text(conf.read_text(encoding='utf-8-sig')+'\ntapeInFile.fileName = "spec/input.rks"\nkbdLayout.layout = smart\nbeepSoundSource.muted = yes\ntapeSoundSource.muted = yes\n',encoding='utf8')
            shutil.copyfile(ROOT/'assets/games/unknown/games_lik_rks/football.rks.bin',directory/'input.rks')
            out['events']=[];g=pilot.Native(slot,folder,out)
            g.call('debug_break');g.call('bp_clear');g.call('kbd_reset');state=g.call('switch_platform',platform='spec.lik')
            if state.get('current')!='spec.lik':raise RuntimeError('Платформа LIK не включена')
            g.call('debug_break');g.call('emu_speed',factor=32);g.call('emu_reset');g.advance(1200);g.frame('boot')
            out['load']=g.call('emu_load',file=MON.as_posix(),autorun=False)
            g.call('debug_break');out['body_matches']=g.read(start,len(body))==body;g.frame('loaded')
            if not out['body_matches']:raise RuntimeError('Загруженный монитор расходится с источником')
            # Загрузить неизменённый архивный LIK-контейнер штатной I= родного монитора.
            game=ROOT/'assets/games/unknown/games_lik_rks/football.rks.bin';raw=game.read_bytes()
            game_start=int.from_bytes(raw[279:281],'little');game_end=int.from_bytes(raw[281:283],'little');game_body=raw[283:]
            out.update(game_sha256=hashlib.sha256(raw).hexdigest(),game_start=game_start,game_end=game_end)
            g.key('i',100,150);g.call('kbd_send',keys=['shift','minus'],action='tap',duration_ms=120);g.advance(150);g.frame('game-read-command')
            g.key('enter',100,100);g.advance(60000);g.advance(60000);g.frame('game-read')
            out['game_body_matches']=g.read(game_start,len(game_body))==game_body
            out['monitor_still_matches']=g.read(start,len(body))==body
            if not out['game_body_matches'] or not out['monitor_still_matches']:raise RuntimeError('После I= тело игры или автора не совпало')
            g.points(0x8100)
            g.key('j',100,150);g.call('kbd_send',keys=['shift','minus'],action='tap',duration_ms=120);g.advance(150)
            for key in '8100':g.key(key,100,150)
            g.frame('command');g.key('enter',100,0);pc=g.regs()['pc']
            if pc!=0x8100:pc=g.advance(60000)
            out['entry_pc']=pc;out['entry_confirmed']=pc==0x8100;g.frame('entry')
            if pc==0x8100:
                g.points();g.advance(2500);g.frame('author-boot')
                g.key('g',100,300);g.frame('author-g');g.key('enter',100,300);g.advance(5000);g.frame('author-g-enter')
                out['game_pc_after_g']=g.regs()['pc']
    except Exception as e:out.update(error=str(e),traceback=traceback.format_exc())
    (folder/'result.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    print('entry',out.get('entry_confirmed'),out.get('error',''),flush=True)

if __name__=='__main__':main()
