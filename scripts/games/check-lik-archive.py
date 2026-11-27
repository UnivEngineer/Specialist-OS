"""ЛИК: подлинные архивные носители, штатный I= и авторский Monitor *BAI*."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import sys
import traceback
import zipfile

ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'scripts/games'),str(ROOT/'scripts/emu80')]
spec=importlib.util.spec_from_file_location('monitor_pilot',ROOT/'scripts/games/check-monitor-pilot.py')
pilot=importlib.util.module_from_spec(spec);spec.loader.exec_module(pilot)
from mcp_pool import lease

OUT=ROOT/'docs/compat/game-campaign/native-lik'
SOURCE=ROOT/'.local/emu80-pool-pristine/dist/spec'
ARCHIVE=ROOT/'docs/compat/game-campaign/fixtures/native/lik/lik_spc1.zip'
MON=ROOT/'docs/compat/game-campaign/fixtures/native/lik/mon-bai2.rks'
STAR={'football.rks.bin','hockey.rks.bin','karate.rks.bin','nupogodi.rks.bin','tetris-d.rks.bin',
      'zoo-lik-game.rks.bin','zoo-lik-editor.rks.bin','zoo-lik-levels.rks.bin'}
ZIP_NAMES={'zoo-lik-game.rks.bin':'ZOO.RKS','zoo-lik-editor.rks.bin':'ZOO-ED.RKS','zoo-lik-levels.rks.bin':'ZOO-LEV.RKS'}

def sha(data):return hashlib.sha256(data).hexdigest()
def save(path,obj):
    tmp=path.with_suffix('.tmp');tmp.write_text(json.dumps(obj,ensure_ascii=False,indent=2)+'\n',encoding='utf8');tmp.replace(path)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--limit',type=int);parser.add_argument('--force',action='store_true')
    parser.add_argument('--source',type=Path,default=SOURCE)
    args=parser.parse_args();source=args.source.resolve()
    OUT.mkdir(parents=True,exist_ok=True);index=OUT/'index.json'
    if index.exists():report=json.loads(index.read_text(encoding='utf8'))
    else:report=dict(cases={})
    mon=MON.read_bytes();sync=mon.index(bytes([0xe6]),3);ms=int.from_bytes(mon[sync+1:sync+3],'little');me=int.from_bytes(mon[sync+3:sync+5],'little');mbody=mon[sync+5:sync+6+me-ms]
    if sha(mon)!='ffb6b593de8488d6eace834193552b8b626db5fa48d54c41c87d723ea7cbf10b' or ms!=0x8100:raise ValueError('Неизвестный авторский монитор')
    report.update(platform='spec.lik',rom_sha256=pilot.sha(source/'lik.rom'),config_sha256=pilot.sha(source/'lik.conf'),
                  monitor88_com_sha256=pilot.sha(ROOT/'assets/programs/MON-88.COM'),
                  author_monitor_sha256=sha(mon),archive_sha256=pilot.sha(ARCHIVE),
                  attribution='ROM ЛИК из upstream Emu80; отдельно MONITOR *BAI* из авторского архива, точный SHA указан.',
                  method='Исходный архивный файл целиком назначен RkTapeInHook; LIK I= вводится с клавиатуры. Авторский монитор загружается штатным SpecFileLoader без autorun, запускается J=8100.',
                  physical_tape='Перехват байтов не проверяет физические сигналы.')
    files=sorted(list((ROOT/'assets/games/unknown/games_lik_rks').glob('*.rks.bin'))+
                 list((ROOT/'assets/games/families/zoo/originals').glob('Zoo-lik-*.rks.bin')))
    pending=[p for p in files if args.force or sha(p.read_bytes()) not in report['cases']]
    if args.limit is not None:pending=pending[:args.limit]
    print('LIK pending',len(pending),flush=True)
    if not pending:save(index,report);return
    with zipfile.ZipFile(ARCHIVE) as z:
        archived={Path(n).name.upper():z.read(n) for n in z.namelist() if n.upper().startswith('GAMES/') and n.upper().endswith('.RKS')}
    with lease(size=16,port=19794,artifact_root=OUT/'slots') as slot:
        directory=slot.runtime/'spec';shutil.copytree(source,directory,dirs_exist_ok=True)
        conf=directory/'lik.conf'
        conf.write_text(conf.read_text(encoding='utf-8-sig')+'\ntapeInFile.fileName = "spec/input.rks"\nkbdLayout.layout = smart\nbeepSoundSource.muted = yes\ntapeSoundSource.muted = yes\n',encoding='utf8')
        for f in pending:
            raw=f.read_bytes();h=sha(raw);folder=OUT/h;folder.mkdir(exist_ok=True)
            if (folder/'result.json').exists():
                prior=folder/'previous';prior.mkdir(exist_ok=True)
                for artifact in folder.glob('*.png'):shutil.copyfile(artifact,prior/artifact.name)
                shutil.copyfile(folder/'result.json',prior/'result.json')
            name=f.name.lower();zipname=ZIP_NAMES.get(name,f.name[:-4].upper())
            c=dict(file=f.relative_to(ROOT).as_posix(),sha256=h,bytes=len(raw),archive_member='GAMES/'+zipname,
                   archive_sha256_matches=zipname in archived and sha(archived[zipname])==h,
                   needs_author_monitor=name in STAR,frames={},events=[],classification='unconfirmed',
                   lik_prefix_hex=raw[:24].hex(),sync_at_278=raw[278:279].hex())
            report['cases'][h]=c;g=pilot.Native(slot,folder,c)
            try:
                if not c['archive_sha256_matches']:raise ValueError('Файл не совпадает с членом авторского архива')
                if not raw.startswith(bytes.fromhex('708f828f')) or raw[278]!=0xe6:raise ValueError('Неизвестный формат LIK')
                start=int.from_bytes(raw[279:281],'little');end=int.from_bytes(raw[281:283],'little');body=raw[283:]
                if end-start+1!=len(body):raise ValueError('Диапазон и длина тела LIK расходятся')
                c.update(lik_checksum_declared=int.from_bytes(raw[21:23],'little'),
                         lik_checksum_computed=(sum(body)*257)&65535,
                         lik_checksum_matches=int.from_bytes(raw[21:23],'little')==((sum(body)*257)&65535),
                         lik_checksum_method='(sum(body) * 0x101) & 0xffff')
                c.update(body_start=start,body_end=end,body_sha256=sha(body),body_length=len(body),
                         first_block_start=int.from_bytes(raw[:2],'little'),first_block_end=int.from_bytes(raw[2:4],'little'))
                g.call('debug_break');g.call('bp_clear');g.call('kbd_reset')
                if g.call('sys_info')['platform']=='spec.lik':g.call('switch_platform',platform='specialist-os')
                (directory/'input.rks').write_bytes(raw)
                state=g.call('switch_platform',platform='spec.lik')
                if state.get('current')!='spec.lik' or not slot.verify_owner():raise RuntimeError('Платформа или владелец не подтверждены')
                g.call('debug_break');g.call('emu_speed',factor=32);g.call('emu_reset');g.advance(1200);g.frame('boot')
                if c['needs_author_monitor']:
                    g.call('emu_load',file=MON.as_posix(),autorun=False);g.call('debug_break')
                    c['author_monitor_loaded']=g.read(ms,len(mbody))==mbody
                    if not c['author_monitor_loaded']:raise RuntimeError('Тело монитора BAI не совпадает')
                def equals():
                    g.call('kbd_send',keys=['shift','minus'],action='tap',duration_ms=120);g.advance(150)
                    c['events'].append(dict(keys=['shift','minus'],action='tap'))
                g.key('i',100,150);equals();g.frame('i-command');g.key('enter',100,100)
                g.advance(60000);g.advance(60000);g.frame('after-i')
                c['pc_after_i']=g.regs()['pc']
                if end>=start and end<0x9000:
                    actual=g.read(start,len(body));c.update(memory_after_i_sha256=sha(actual),body_loaded_after_i=actual==body)
                if not c.get('body_loaded_after_i'):raise RuntimeError('После I= полное тело LIK не совпало')
                if c['needs_author_monitor']:
                    c['author_monitor_intact_after_i']=g.read(ms,len(mbody))==mbody
                    if not c['author_monitor_intact_after_i']:raise RuntimeError('Игра перекрыла авторский монитор')
                    g.points(ms);g.key('j',100,150);equals()
                    for key in '8100':g.key(key,100,150)
                    g.frame('author-j-command');g.key('enter',100,0)
                    pc=g.regs()['pc']
                    if pc!=ms:pc=g.advance(60000)
                    c.update(author_entry_pc=pc,author_entry_confirmed=pc==ms)
                    if pc!=ms:raise RuntimeError('Не достигнут вход монитора BAI')
                    g.points();g.advance(2500);g.frame('author-monitor')
                if name=='zoo-lik-levels.rks.bin':
                    c['classification']='data_loaded_no_execution'
                else:
                    g.points(start)
                    if c['needs_author_monitor']:
                        g.key('g',100,150)
                        # G без аргумента запускает 0000h; ненулевой адрес задаётся явно.
                        if start:
                            for key in format(start,'04x'):g.key(key,100,150)
                    else:
                        g.key('j',100,150);equals()
                        for key in format(start,'04x'):g.key(key,100,150)
                    g.frame('game-command');g.key('enter',100,0)
                    pc=g.regs()['pc']
                    if pc!=start:pc=g.advance(60000)
                    c.update(entry_pc=pc,entry_confirmed=pc==start)
                    g.frame('entry')
                    if pc!=start:raise RuntimeError('Не подтверждён вход игры')
                    actual=g.read(start,len(body));c.update(body_at_entry_sha256=sha(actual),body_matches_at_entry=actual==body)
                    if actual!=body:raise RuntimeError('Тело изменилось до входа')
                    g.points();g.advance(2500);g.frame('game')
                    c['classification']='archive_body_and_entry_verified'
            except Exception as e:
                c.update(error=str(e),traceback=traceback.format_exc())
                try:g.call('debug_break');g.frame('failure')
                except Exception:pass
            finally:
                save(folder/'result.json',c);save(index,report)
            print(f.name,c['classification'],c.get('error',''),flush=True)
    print('LIK indexed',len(report['cases']),flush=True)

if __name__=='__main__':main()
