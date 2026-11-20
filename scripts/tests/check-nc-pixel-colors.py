"""8080 trace: color selection changes only cell attributes."""
from pathlib import Path
import runpy

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'.local/nc-pixel-colors';OUT.mkdir(exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/tests/benchmark-nc-color-scroll.py'))
n=h['symbols'](ROOT/'bin/NC.sym');b=h['symbols'](ROOT/'bin/BIOS.sym')
W=lambda x:int(x).to_bytes(2,'little')

def run(m,entry,trace=False):
    return h['execute'](m,entry,OUT,trace=trace)

cases=0
for font,width,columns,text_width in [('Cpm4',4,3,48),('Compact',6,2,72),('Fast8',8,1,176)]:
    m=bytearray(65536)
    for at,path in [(0,'NC.i80'),(0xc000,'BIOS.sys'),(0xf000,'IO.main.bin'),(0xd800,f'fonts/{font}.i80')]:
        data=(ROOT/'bin'/path).read_bytes();m[at:at+len(data)]=data
    m[b['bios_vars']:b['bios_vars']+b['BIOS_VARIABLES']]=m[b['initVars']:b['initVarsEnd']]
    m[b['bios_upper.escMode']]=255
    m=run(m,n['FONT_ENGINE_INIT'])[0];m=run(m,n['ncLayoutInit'])[0]
    for panel in (0,1):
        for column in range(columns):
            q=bytearray(m);q[n['state.activePanel']]=panel
            q[n['state.panelA_filesCnt']+panel]=190
            q[n['state.panelA_firstFile']+panel]=0
            q[n['state.panelA_curFile']+panel]=column*n['P_FILE_LIST_H']+7
            x=8+panel*192+column*q[n['ncColumnStep']]*2
            y=n['SCROLL_TOP']+70
            normal=[n['COLOR_DIRECTORY'],n['COLOR_EXECUTABLE'],n['COLOR_ARCHIVE']][column]
            color_at=n['vars.rowColors']+panel*3*n['P_FILE_LIST_H']+column*n['P_FILE_LIST_H']+7
            q[color_at]=normal
            attrs=bytearray([normal])*12288
            for frame_col,pattern in [(0,0x50),(23,0x0a),(24,0x50),(47,0x0a)]:
                for py in range(230):
                    q[0x9000+frame_col*256+py]=pattern
                    attrs[frame_col*256+py]=n['COLOR_BORDER']
            for col in range(text_width//8):
                for py in range(y+1,y+9):q[0x9000+(x//8+col)*256+py]=0x5a
            original=bytes(q[0x9000:0xc000])
            wanted=bytearray(attrs)
            for col in range(x//8,(x+text_width)//8):
                for py in range(y,y+10):wanted[col*256+py]=n['COLOR_CURSOR']
            q[n['ncCurrentColor']]=q[n['IO_COLOR']]=n['COLOR_CURSOR']
            selected,_,trace,initial=run(q,n['drawCursor'],True)
            stats=h['replay_trace'](trace,initial,attrs,selected,wanted)
            assert stats['wrong_visible_writes']==0
            assert selected[0x9000:0xc000]==original,(font,panel,column,'selection changed bitmap')
            selected[n['ncCurrentColor']]=selected[n['IO_COLOR']]=n['COLOR_FILE']
            hidden,_,trace,initial=run(selected,n['drawCursor'],True)
            h['replay_trace'](trace,initial,wanted,hidden,attrs)
            assert hidden[0x9000:0xc000]==original,(font,panel,column,'deselection changed bitmap')
            cases+=1
    print(font,'selection palette and unchanged frame/bitmap PASS',flush=True)
print('Pixel selection color traces PASS:',cases*2,'traces; no color-register reads')
