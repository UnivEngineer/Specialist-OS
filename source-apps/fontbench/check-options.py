"""8080: разбор параметров FONTBEN и отбор всей матрицы Main/DOS."""
from pathlib import Path
import argparse
import itertools
import json
import re
import subprocess
ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'docs/bench/font4-opt/options')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    sym = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (ROOT/'bin/FONTCOM.sym').read_text(), re.M)}
    cases = [x.split(',') for x in re.findall(r'^    CASE (.+)$', (ROOT/'source-apps/fontbench/matrix-data.inc').read_text(), re.M)]
    word=lambda x:x.to_bytes(2,'little')
    call=lambda name:b'\xcd'+word(sym[name])
    asm=args.out/'argNext.asm'
    asm.write_text('    ORG 0C8B7h\n    INCLUDE "'+(ROOT/'source/common/argNext.inc').as_posix()+'"\n',encoding='utf-8')
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow','--raw='+str(args.out/'argNext.bin'),str(asm)],capture_output=True,check=True)
    base=bytearray(65536)
    data=(ROOT/'bin/FONTCOM.i80').read_bytes();base[:len(data)]=data
    data=(ROOT/'bin/BIOS.sys').read_bytes();base[0xc000:0xc000+len(data)]=data
    arg_code=(args.out/'argNext.bin').read_bytes();base[sym['bios_argNext']:sym['bios_argNext']+len(arg_code)]=arg_code
    checks=0
    def run(text, options=(7,7,3,63,0,0,0), bad=False, null=False, engines=7):
        nonlocal checks
        m=bytearray(base)
        encoded=text.encode()+b'\0';m[0x7c00:0x7c00+len(encoded)]=encoded
        code=bytes.fromhex('31 00 8B 21')+word(0 if null else 0x7c00)+call('benchParseArgs')+bytes.fromhex('F5 E1 22 B0 83')
        code+=call('benchCountSelected')
        for i in range(len(cases)):
            code+=b'\x21'+word(sym['matrixCases']+i*14)+call('benchCaseSelected')+b'\x32'+word(0x7b00+i)
        code+=b'\xc3\x80\x8b';m[0x8000:0x8000+len(code)]=code
        (args.out/'input.bin').write_bytes(m)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8000','-end','8b80','-counter','20000000','-output',str(args.out/'output.bin'),str(args.out/'input.bin')],capture_output=True,check=True,timeout=20)
        d=(args.out/'output.bin').read_bytes()
        assert bool(d[0x83b0]&1)==bad,('parse CF',text,d[0x83b0:0x83b2])
        if not bad:
            assert d[sym['benchOptions']:sym['benchOptions']+7]==bytes(options),('options',text)
            assert d[sym['benchEngine']]==engines,('engines',text)
            selected=[]
            for case in cases:
                kind,flags=int(case[1]),int(case[2])
                font=1 if kind<3 else 2 if kind<5 else 4
                fmt=1 if kind<7 else 2 if kind<9 else 4
                method=4 if flags&16 else 8 if flags&64 else 16 if flags&32 else 32 if kind==1 else 1 if kind in (0,3,5,7,9) else 2
                supported=True
                selected.append(int(bool(options[0]&font and (kind<5 or options[1]&fmt) and options[3]&method and (options[2]!=2 or supported) and engines&(2 if case[0].startswith('kernel') else 4 if case[0].startswith('paged') else 1))))
            assert d[0x7b00:0x7b00+len(cases)]==bytes(selected),('case filter',text)
            count=sum(selected)
            assert d[sym['benchCaseCount']]==count
            bank_count=sum(selected)
            assert d[sym['benchStageTotal']]==count*bool(options[2]&1)+bank_count*bool(options[2]&2),('stage count',text)
        checks+=1
    run('-K',engines=2);run('-E=STANDALONE',engines=1);run('-E=KERNEL',engines=2);run('-K -E=ALL');run('-B=BANK -K',options=(7,7,2,63,0,0,0),engines=2);run('-E=BAD',bad=True)
    run('');run('',null=True);run('   ');run('-? ',options=(7,7,3,63,0,0,1))
    run('-n -Q -f=4 -p=dup -b=main -c=serial',options=(4,4,1,2,1,1,0))
    run('-F=6 -F=8',options=(2,7,3,63,0,0,0))
    for f,p,b,c in itertools.product([('4',4),('6',1),('8',2),('ALL',7)],[('RAW',1),('PACK',2),('DUP',4),('ALL',7)],[('MAIN',1),('DOS',2),('BOTH',3)],[('CHAR',1),('SERIAL',2),('WIN20',4),('WIN256',4),('FAR48',8),('FAR1152',16),('GROUP',32),('ALL',63)]):
        run(f'-F={f[0]} -P={p[0]} -B={b[0]} -C={c[0]}', options=(f[1],p[1],b[1],c[1],0,0,0))
    for text in ['-F=5','-F=4X','-P=PA','-B=','-C=far480','x','-N=0','-qX','--help','-F=4 -bad']:
        run(text,bad=True)
    (args.out/'results.json').write_text(json.dumps(dict(checks=checks,parse='PASS',filters='PASS',stageCount='PASS',emptySelection='PASS'),indent=2)+'\n')
    print('FONTBEN OPTIONS CPU PASS',checks)
if __name__=='__main__':main()
