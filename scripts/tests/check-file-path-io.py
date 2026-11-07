"""Полные пути: предел буфера, вложенный каталог, замена, пустой файл; 8080."""
import json
import runpy
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
h=runpy.run_path(str(ROOT/'scripts/tests/check-file-handle.py'))
run=h['run'];g=run.__globals__;S=h['S'];W=h['W']
# Используем тот же блочный драйвер и геометрию, но чистый том.
m=bytearray(h['default_memory']);m[:8192]=bytes(8192);m[0x100:0x104]=bytes.fromhex('f8 ff ff ff')
__import__('runpy').run_path(str(Path(__file__).with_name('mem-image.py')))['install'](m, S)
m[S['v_directory']:S['v_directory']+2]=W(0)
m[S['v_directories']:S['v_directories']+16]=bytes(16)
m[S['v_memTop']:S['v_memTop']+2]=W(S['DOS_CODE_START']-1)
g['memory']=m
assert run(S['resetSectorCache'])==(0,False)
rows=[]

def call(save,path,count,data=None,buffer=0x2400,error=0):
    m=g['memory'];m[0x2300:0x2380]=path.encode()+bytes(128-len(path))
    m[0x2200:0x2228]=W(0x2300)+W(buffer)+W(count)+bytes(34)
    if data is not None:m[buffer:buffer+len(data)]=data
    result=run(S['bios_fileSavePath' if save else 'bios_fileLoadPath'],hl=0x2200)
    assert result==(error,bool(error)),(path,count,result,error)
    actual=int.from_bytes(g['memory'][0x2206:0x2208],'little')
    rows.append(dict(save=save,path=path,count=count,actual=actual,error=error))
    return actual

data=bytes((n*19+7)&255 for n in range(517))
assert call(True,'B:/ROUND.BIN',len(data),data)==len(data)
assert call(False,'B:\\ROUND.BIN',len(data),bytes([0xCC])*518)==len(data)
assert g['memory'][0x2400:0x2605]==data and g['memory'][0x2605]==0xCC
assert call(False,'B:/ROUND.BIN',516,bytes([0xAA])*518,error=7)==0
assert g['memory'][0x2400:0x2606]==bytes([0xAA])*518
assert call(True,'B:/ROUND.BIN',3,b'XYZ')==3
assert call(False,'B:/ROUND.BIN',8,b'........')==3
assert g['memory'][0x2400:0x2408]==b'XYZ.....'
assert call(True,'B:/EMPTY.BIN',0,buffer=0)==0
assert call(False,'B:/EMPTY.BIN',0,buffer=0)==0
assert call(False,'B:/MISSING.BIN',1,error=2)==0
assert call(False,'B:/ROUND.BIN',8,buffer=0x2201,error=7)==0
assert call(True,'B:/ROUND.BIN',256,buffer=S['DOS_CODE_START']-128,error=7)==0
assert call(False,'B:/ROUND.BIN',3,b'...')==3
assert g['memory'][0x2400:0x2403]==b'XYZ','Отказ не должен удалять прежний файл'

m=g['memory'];m[0x2300:0x2307]=b'B:/SUB\0'
assert run(S['bios_directory'],a=S['DIR_OP_MKDIR'],hl=0x2300)==(0,False)
assert call(True,'B:/SUB/INNER.BIN',len(data),data)==len(data)
assert call(False,'B:\\SUB\\INNER.BIN',len(data),bytes(517))==len(data)
assert g['memory'][0x2400:0x2605]==data
assert g['memory'][S['v_directory']:S['v_directory']+2]==W(0),'Сохранённый CWD должен восстановиться'
call(True,'B:/SUB',0,buffer=0,error=6)

out=ROOT/'docs/banked/syscall-main-001';out.mkdir(parents=True,exist_ok=True)
(out/'path-io.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
print('FILE PATH IO PASS',len(rows))
