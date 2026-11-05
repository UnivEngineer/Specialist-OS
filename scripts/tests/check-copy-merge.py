"""Расширение подкаталогов и COPY Yes/No на настоящем коде 8080."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2]
h=runpy.run_path(str(R/'scripts/tests/check-cli.py'))
g=h['run'].__globals__
exec(r'''
command('CD /BIN')
command('MD SRC');command('MD DST')
parent=cwd()
command('CD SRC');source=cwd()
for i in range(21): create(word('E%02d'%i),b'')
create(word('ONE'),b'NEW DATA')
command('CD ../DST');destination=cwd()
create(word('ONE'),b'OLD')
command('CD ..')
req=0x5500
hook=S['FILE_COPY_REQUEST.onExists']
def request(src,dst,srcdir,dstdir,answer=None):
 m[req:req+S['FILE_COPY_REQUEST']]=bytes(S['FILE_COPY_REQUEST'])
 for field,value in [('buffer',0x5700),('bufferSize',256),('sourceDirectory',srcdir),('destinationDirectory',dstdir)]:
  m[req+S['FILE_COPY_REQUEST.'+field]:req+S['FILE_COPY_REQUEST.'+field]+2]=W(value)
 for field,name in [('source',src),('destination',dst)]:
  at=req+S['FILE_COPY_REQUEST.'+field];m[at:at+11]=word(name)
 if answer is not None:
  code=bytes.fromhex('3A 50 5E 3C 32 50 5E')+bytes([0x3E,answer,0xB7,0xC9])
  m[0x5800:0x5800+len(code)]=code;m[0x5E50]=0
  m[req+hook:req+hook+2]=W(0x5800)
 return run(S['bios_cli'],S['CLI_OP_COPY'],req)
def content(directory,name):
 m[S['v_directory']:S['v_directory']+2]=W(directory)
 m[0x5400:0x540B]=word(name)
 assert not run(S['fileFindAny'],hl=0x5400)[1]
 cl=int.from_bytes(m[S['v_tmpFileDescr']+26:S['v_tmpFileDescr']+28],'little')
 size=int.from_bytes(m[S['v_tmpFileDescr']+28:S['v_tmpFileDescr']+32],'little')
 return bytes(m[data_start+(cl-2)*sector*spc:data_start+(cl-2)*sector*spc+size])
# Имя существующего каталога не заменяется обычным файлом.
assert request('ONE','DST',source,parent,1)==(1,True)
assert m[S['v_dosError']]==S['DOS_ERR_ACCESS']
m[S['v_directory']:S['v_directory']+2]=W(parent)
m[0x5400:0x5420]=word('DST')+bytes(21)
assert run(S['bios_fileCreate'],hl=0x5400,de=0x5700)[1]
assert m[S['v_dosError']]==S['DOS_ERR_ACCESS']
assert request('ONE','ONE',source,destination)==(2,True)
assert request('ONE','ONE',source,destination,0)==(0,False)
assert content(destination,'ONE')==b'OLD'
assert content(source,'ONE')==b'NEW DATA'
assert request('ONE','ONE',source,destination,1)==(0,False)
got=content(destination,'ONE');assert got==b'NEW DATA',('overwrite payload',got)
got=request('ONE','ONE',source,source,1);assert got==(1,True),('self-copy must not truncate',got,source,destination)
assert content(source,'ONE')==b'NEW DATA'
# Объединение двух уже существующих каталогов: No/Yes на совпадениях.
assert request('SRC','DST',parent,parent,0)==(0,False)
assert request('SRC','DST',parent,parent,1)==(0,False)
assert request('SRC','DST',parent,parent,2)==(0,False)
assert m[0x5E50]==1,('Yes to all asked repeatedly',m[0x5E50])
assert request('ONE','ONE',source,destination)==(2,True),'Yes to all leaked into next operation'
media_before=bytes(m[:32*sector])
assert request('SRC','DST',parent,parent,3)==(0,False)
assert m[0x5E50]==1,('No to all asked repeatedly',m[0x5E50])
assert bytes(m[:32*sector])==media_before,'No to all modified disk'
assert request('ONE','ONE',source,destination)==(2,True),'No to all leaked into next operation'
# No to all не запрещает создание отсутствующего файла.
m[S['v_directory']:S['v_directory']+2]=W(destination)
m[0x5400:0x540B]=word('E20')
assert not run(S['fileDelete'],hl=0x5400)[1]
assert request('SRC','DST',parent,parent,3)==(0,False)
assert m[0x5E50]==1
m[S['v_directory']:S['v_directory']+2]=W(destination)
assert not run(S['fileFindAny'],hl=0x5400)[1],'No to all skipped a new file'
# Подтверждение не снимает защиту R; No допускает пропуск защищённого файла.
m[0x5400:0x540B]=word('ONE')
assert not run(S['fileGetSetAttr'],hl=0x5400,bc=0x0101)[1]
assert request('ONE','ONE',source,destination,0)==(0,False)
assert request('ONE','ONE',source,destination,1)==(1,True)
assert m[S['v_dosError']]==S['DOS_ERR_ACCESS']
assert content(destination,'ONE')==b'NEW DATA'
assert not run(S['fileGetSetAttr'],hl=0x5400,bc=1)[1]

for directory in [source,destination]:
 m[S['v_directory']:S['v_directory']+2]=W(directory)
 for i in range(21):
  m[0x5400:0x540B]=word('E%02d'%i)
  assert not run(S['fileFindAny'],hl=0x5400)[1],(directory,i)
# Случай нескольких кластеров содержит EOC в последнем и сохраняет . / .. .
for directory in [source,destination]:
 count=0;cl=directory
 while cl<0xFFF8:
  count+=1;assert count<20
  cl=int.from_bytes(m[sector+cl*2:sector+cl*2+2],'little')
 assert count>1,count
# Текстовый CLI передаёт обработчик совпадений из DE.
line=b'COPY /BIN/SRC/ONE /BIN/DST/ONE\0';m[0x5000:0x5000+len(line)]=line
assert run(S['bios_cli'],S['CLI_OP_COMMAND'],0x5000,0x5800)==(0,False)
# Проверка алгоритма межтомового MOVE с пропуском: не удалять файл/предков.
assert request('SRC','DST',parent,parent,0)==(0,False)
assert run(M['fileCopyForMove'],hl=req)==(0,False)
assert content(source,'ONE')==b'NEW DATA'
m[S['v_directory']:S['v_directory']+2]=W(parent)
m[0x5400:0x540B]=word('SRC')
assert not run(S['fileFindAny'],hl=0x5400)[1]
# Фиксированный корень: DIR_FULL остаётся доступен без загружаемого модуля.
command('CD /')
for i in range(sector//32):
 m[0x5400:0x5420]=word('ROOT%02d'%i)+bytes(21)
 got=run(S['bios_fileCreate'],hl=0x5400,de=0x5700)
 if got[1]:break
else:raise AssertionError('root unexpectedly expanded')
assert m[S['v_dosError']]==S['DOS_ERR_DIR_FULL']
m[S['v_dosModuleReady']]=0
assert run(S['bios_fileCreate'],hl=0x5400,de=0x5700)[1]
assert m[S['v_dosError']]==S['DOS_ERR_DIR_FULL']
# Полный том: расширение не изменяет старую цепочку и возвращает FULL.
m[S['v_dosModuleReady']]=1
m[S['v_directory']:S['v_directory']+2]=W(source)
slots=sector*spc//32
for i in range((-24)%slots):create(word('FILL%02d'%i),b'')
assert not run(S['fatFinishWrite'])[1]
# Отказ между выделением нового кластера и публикацией ссылки: откат FAT.
start=M['directoryExtend'];end=M['directoryRootFull']
call=bytes([0xCD])+W(S['fatFinishWrite']);at=m.index(call,start,end)
original=bytes(m[at:at+3]);m[0x5900:0x5905]=bytes([0x3E,S['DOS_ERR_IO'],0xC3])+W(S['dosSetError'])
m[at:at+3]=bytes([0xCD])+W(0x5900)
fat_before_growth=bytes(m[sector:root])
m[0x5400:0x5420]=word('IO_FAIL')+bytes(21)
assert run(S['bios_fileCreate'],hl=0x5400,de=0x5700)[1]
assert m[S['v_dosError']]==S['DOS_ERR_IO']
assert bytes(m[sector:root])==fat_before_growth,'unlinked directory cluster leaked'
m[at:at+3]=original
for cl in range(2,2+29//spc):
 if m[sector+cl*2:sector+cl*2+2]==b'\0\0':
  assert not run(S['fatWriteCluster'],hl=cl,de=0xFFFF)[1]
assert not run(S['fatFinishWrite'])[1]
fat_full=bytes(m[sector:root])
m[0x5400:0x5420]=word('NO_ROOM')+bytes(21)
assert run(S['bios_fileCreate'],hl=0x5400,de=0x5700)[1]
assert m[S['v_dosError']]==S['DOS_ERR_FULL'],m[S['v_dosError']]
assert bytes(m[sector:root])==fat_full
print('COPY MERGE PASS; directory growth; overwrite Yes/No; identity guard; root/full-volume limits; sector',sector)
''',g)
