"""CPU 8080: выделение не меняется при вводе, инверсия сохраняется при скролле."""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[2]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ticks',type=Path,default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    p.add_argument('--report-directory',type=Path,default=ROOT/'docs/nc/cursor-006/cpu')
    p.add_argument('--variant',default='NC')
    args=p.parse_args();out=args.report_directory.resolve();out.mkdir(parents=True,exist_ok=True)
    def syms(name):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin'/name).read_text(),re.M)}
    ms=syms('BIOS.sym')
    ns,ds=syms(args.variant+'.sym'),syms('DOS.sym');dos=(ROOT/'bin/BIOS.sys').read_bytes();nc=(ROOT/'bin'/(args.variant+'.i80')).read_bytes()
    count=ns['FILE_LIST_SIZE'];height=count//2*10;top=ns['P_FILE_LIST_Y']-8;inversions=0 if 'scrollBitmapPanel' in ns else 2
    def run(m,entry,bc=0,event=False):
        code=bytes.fromhex('31 00 8F 01')+bc.to_bytes(2,'little')+b'\xCD'+entry.to_bytes(2,'little')+bytes.fromhex('C3 00 63');m[0x6200:0x6200+len(code)]=code
        (out/'in.bin').write_bytes(m)
        r=subprocess.run([str(args.ticks),'-m8080','-pc','6200','-end','6300','-counter','3000000','-output',str(out/'out.bin'),str(out/'in.bin')],check=True,capture_output=True,text=True,timeout=15)
        b=(out/'out.bin').read_bytes();assert int.from_bytes(b[65542:65544],'little')==0x6300,r.stdout
        return bytearray(b[:65536])
    def memory(panel=0,first=0,selected=35):
        m=bytearray(65536);m[0xC000:0xC000+len(dos)]=dos;io=(ROOT/'bin/IO.sys').read_bytes();m[0xF000:0xF000+len(io)]=io;m[:len(nc)]=nc
        module=(ROOT/'bin/DOS.native.sys').read_bytes();m[ds['DOS_CODE_START']:ds['DOS_CODE_START']+len(module)]=module;m[ds['v_dosModuleReady']]=1
        __import__('runpy').run_path(str(Path(__file__).with_name('mem-image.py')))['install'](m, ds)
        font=(ROOT/'assets/system-rom/FONTKOI.FNT').read_bytes();m[ns['FONT_ADDR']:ns['FONT_ADDR']+len(font)]=font;m[0x8FE7:0x8FE9]=(ns['FONT_ADDR']//8).to_bytes(2,'little')
        m[ns['state']+1]=panel;m[ns['state.panelA_filesCnt']+panel]=140;m[ns['state']+8+panel]=first;m[ns['state']+10+panel]=selected
        m[ns['vars.cacheValidA']+panel]=1;m[ns['vars.cacheDriveA']+panel]=m[ns['state']+4+panel]
        base=ns['FILE_CACHE_B'] if panel else ns['FILE_CACHE_A']
        for i in range(140):m[base+i*16:base+(i+1)*16]=f'FILE{i:04}TXT'.encode()+bytes(5)
        m[base+140*16]=255;m[ns['vars.fileListPtr']:ns['vars.fileListPtr']+2]=(base+first*16).to_bytes(2,'little')
        m[ds['bios_fileGetSetDrive']]=0xC9
        # Кэш изолирован от диска; контекст каталога уже задан фикстурой.
        for entry in ['bios_fileSelectDrive', 'bios_directory']:
            m[ds[entry]:ds[entry]+2]=bytes.fromhex('AF C9')
        # MXOS_CALL обходит исторические JMP; изолируем тот же файловый слой
        # в его локальных целях диспетчера, не подменяя графику BIOS.
        for entry in ['fileSelectDrive','directoryService','fileGetSetDrive']:
            m[ds[entry]:ds[entry]+2]=bytes.fromhex('AF C9')
        # Считаем вызовы инверсии отдельно от окончательного вида пикселей.
        entry=ns['uiInverseRect']; original=bytes(m[entry:entry+3])
        trace=bytes.fromhex('3A 10 65 3C 32 10 65')+original+b'\xC3'+(entry+3).to_bytes(2,'little');m[0x6600:0x6600+len(trace)]=trace;m[entry:entry+3]=bytes.fromhex('C3 00 66')
        m=run(m,ns['printFilesUpDown'],bc=ns['FILE_LIST_SIZE']<<8);m=run(m,ns['drawCursor']);m=run(m,ns['clearCmdLine']);m[0x6510]=0
        return m
    def key(m,code,ctrl=False):
        # Второй getch завершает одну итерацию штатного цикла NC.
        stub=bytes.fromhex('3A 00 65 3C 32 00 65 3D C2 00 63 3E')+bytes([code,0x37 if ctrl else 0xB7,0xC9]);m[0x6400:0x6400+len(stub)]=stub;m[0x6500]=0;m[ds['bios_getch']:ds['bios_getch']+3]=bytes.fromhex('C3 00 64')
        return run(m,ns['mainLoop_cursorReady'],event=True)
    def file_pixels(m,panel):
        start=24*panel
        return b''.join(m[(x+start)*256+top:(x+start)*256+top+height] for x in list(range(0x91,0x9B))+list(range(0x9D,0xA7)))
    rows=[]
    for code,ctrl,name in [(ord('a'),False,'text'),(0x7F,False,'backspace-empty'),(0x1B,False,'escape'),(0x0D,True,'ctrl-enter'),(0x8A,False,'F9'),(0x0B,False,'ignored'),(ord('b'),True,'ctrl-text')]:
        m=memory();before=file_pixels(m,0);b=key(m,code,ctrl)
        assert file_pixels(b,0)==before,name
        assert b[0x6510]==0,(name,b[0x6510])
        rows.append(dict(case=name,inverse_calls=0,panel_pixels_unchanged=True))
    for panel in [0,1]:
        for code,first,selected,new_first,new_selected in [(0x1A,0,count-1,1,count),(0x19,1,1,0,0)]:
            m=memory(panel,first,selected);b=key(m,code)
            assert b[ns['state']+8+panel]==new_first and b[ns['state']+10+panel]==new_selected,(panel,code,list(b[ns['state']:ns['state']+12]),new_first,new_selected)
            assert b[0x6510]==inversions and b[ns['bios_vars.inverse']]==0,(panel,code,b[0x6510])
            actual=file_pixels(b,panel)
            # Полная перерисовка + битовая инверсия служит независимым эталоном.
            for x in list(range(0x91,0x9B))+list(range(0x9D,0xA7)):
                at=(x+24*panel)*256+top;b[at:at+height]=bytes(height)
            b=run(b,ns['printFilesUpDown'],bc=ns['FILE_LIST_SIZE']<<8);b=run(b,ns['drawCursor'])
            assert file_pixels(b,panel)==actual,(panel,code)
            rows.append(dict(case='down' if code==0x1A else 'up',panel=panel,inverse_calls_during_step=inversions,equals_inverse_reference=True))
    (out/'result.json').write_text(json.dumps(rows,indent=2)+'\n');print('CURSOR PASS',len(rows))


if __name__=='__main__':main()
