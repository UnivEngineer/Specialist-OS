"""Уведомления DOSMOD и маска панелей NC: настоящий код 8080."""
from pathlib import Path
import runpy,re,subprocess,struct
R=Path(__file__).resolve().parents[2]
h=runpy.run_path(str(R/'scripts/tests/check-cli.py'));g=h['run'].__globals__
exec(r'''
asm=O/'observer.asm';asm.write_text(""" ORG 05800h
 push af
 ld hl,(05E22h)
 pop af
 ld (hl),a
 inc hl
 ld (hl),e
 inc hl
 ld (hl),d
 inc hl
 ld (hl),c
 inc hl
 ld (05E22h),hl
 ret
""",encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'observer.bin'),str(asm)],check=True,capture_output=True)
m[0x5800:0x5800+len((O/'observer.bin').read_bytes())]=(O/'observer.bin').read_bytes()
assert run(S['bios_cli'],S['CLI_OP_OBSERVER'],0x5800)==(0,False)
def reset():m[0x5E22:0x5E24]=W(0x5D00)
def events():
 end=int.from_bytes(m[0x5E22:0x5E24],'little');return [tuple(m[i:i+4]) for i in range(0x5D00,end,4)]
command('CD /BIN');parent=cwd();reset()
command('MD AAA');assert events()==[(0,parent&255,parent>>8,0)],events()
command('CD AAA');child=cwd();command('CD /');reset()
command('COPY /BIN/GAME.EXE /BIN/AAA/FILE');assert events()==[(0,child&255,child>>8,0)],events()
reset();command('REN /BIN/AAA/FILE /BIN/AAA/F2');assert events()==[(0,child&255,child>>8,0)],events()
reset();command('MOVE /BIN/AAA/F2 /BIN/MOVED')
assert set(events())=={(0,parent&255,parent>>8,0),(0,child&255,child>>8,0)},events()
reset();command('DEL /BIN/MOVED');assert events()==[(0,parent&255,parent>>8,0)],events()
reset();command('COPY /BIN/GAME.EXE /BIN/GAME.EXE',S['DOS_ERR_ACCESS']);assert events()==[],events()
reset();command('REN /BIN/AAA /BIN/BBB');assert events()==[(0,parent&255,parent>>8,1)],events()
assert run(S['bios_cli'],S['CLI_OP_OBSERVER'],0)==(0,False)
reset();command('RD /BIN/BBB');assert events()==[]
print('DOS CHANGE OBSERVER PASS')
# Чистый обработчик NC не обращается к DOS: отдельная память без диска.
n={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text(),re.M)}
m=bytearray(65536);nc=(R/'bin/NC.i80').read_bytes();m[:len(nc)]=nc
for drives,dirs,event,scope,want in [((0,7),(0,0),(1,0),0,0),((0,7),(0,0),(0,0),0,1),((0,7),(0,0),(7,0),0,2),((1,1),(10,20),(1,10),0,1),((1,1),(10,20),(1,20),0,2),((1,1),(10,10),(1,10),0,3),((1,1),(10,20),(1,30),0,0),((1,1),(10,20),(1,30),1,3)]:
 for side in [0,1]:
  m[n['state.panelA_drive']+side]=drives[side]
  m[n['state.panelA_dirLow']+side]=dirs[side]&255
  m[n['state.panelA_dirHigh']+side]=dirs[side]>>8
 m[n['ncDirtyMask']]=0
 run(n['ncDiskChanged'],event[0],de=event[1],bc=scope)
 assert m[n['ncDirtyMask']]==want,(drives,dirs,event,scope,m[n['ncDirtyMask']],want)
print('NC DIRTY MASK PASS 8')
''',g)
