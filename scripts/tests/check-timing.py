"""Общий пересчёт ВИ53/десятичный вывод в двух приложениях: Intel 8080."""
import argparse,json,os,random,re,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--ticks',type=Path,default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
a=p.parse_args();out=ROOT/'docs/timing/shared-001';out.mkdir(parents=True,exist_ok=True)
def symbols(path):return {k:int(v,16) for k,v in re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)',path.read_text(),re.M)}
report={}
llm80 = Path(os.environ.get('LLM80_ROOT', ROOT.parent / 'LLM80'))
for name, folder, source in [('DISKBEN', ROOT/'source-apps/diskbench', 'DISKBEN.asm'), ('LLM80', llm80/'src', 'LLM80.asm')]:
 if not (folder/source).is_file():continue
 if name == 'LLM80':
    # Внешний снимок SDK включал карту MX из мастера и давал локальному чтению имя pitTimerRead.
    # Связываем старый исходник с актуальными листами для проверки арифметики на обоих CLK.
    client = out/'llm80-sdk.asm'
    client.write_text('    INCLUDE "mxos.inc"\n    INCLUDE "hardware/spec_mx.inc"\n'
                      'pitTimerRead = io_pitRead\n'
                      f'    INCLUDE "{(folder/source).as_posix()}"\n', encoding='utf-8')
    source = client.as_posix()
 for hz in [2000000,2500000]:
    stem=out/f'{name}-{hz}'
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080',f'--inc={ROOT / "sdk/asm"}',f'--inc={ROOT / "sdk/asm/lib"}','-Wno-rdlow',f'-DPIT_HZ={hz}',f'--sym={stem}.sym',f'--raw={stem}.bin',source],cwd=folder,check=True)
    s=symbols(stem.with_suffix('.sym'));code=stem.with_suffix('.bin').read_bytes()
    base=bytearray(65536)
    origin = s['loader'] if name == 'LLM80' else 0
    base[origin:origin + len(code)] = code
    def run(label,value,conversion=True):
        mem=bytearray(base)
        if conversion:
            struct.pack_into('<I',mem,0x8000,value)
            wrapper=b'\x31\xF0\xBF\x21\x00\x80\xCD'+struct.pack('<H',s[label])+b'\xC3\xF0\xB8'
        else:
            struct.pack_into('<I',mem,s['v_number'],value)
            # Печатный callback заменяем RET, проверяется строка из десяти цифр.
            mem[s['timing_print']]=0xC9
            if 'timing_printChar' in s:mem[s['timing_printChar']]=0xC9
            wrapper=b'\x31\xF0\xBF\xCD'+struct.pack('<H',s[label])+b'\xC3\xF0\xB8'
        mem[0xB800:0xB800+len(wrapper)]=wrapper
        inp=out/'input.bin';result=out/'output.bin';inp.write_bytes(mem)
        subprocess.run([str(a.ticks),'-m8080','-pc','b800','-end','b8f0','-counter','10000000','-output',str(result),str(inp)],check=True,capture_output=True,timeout=15)
        data=result.read_bytes();assert int.from_bytes(data[65542:65544],'little')==0xB8F0
        return data
    maximum=0xFFFFFFFF>>(4 if hz==2000000 else 6)
    values=[0,1,124,125,624,625,7812,28244,65535,65536,81289,0xFFFFFF,maximum]
    rng=random.Random(8080);values +=[rng.randrange(maximum+1) for _ in range(25)]
    for value in values:
        data=run('ticks_to_ms',value)
        actual=int.from_bytes(data[s['v_number']:s['v_number']+4],'little')
        assert actual==value*256000//hz,(name,hz,value,actual)
    for value in [0,1,999,65535,65536,1000000000,0xFFFFFFFF]:
        data=run('print_number',value,False)
        assert data[s['DECIMAL']:s['DECIMAL']+11]==f'{value:010d}'.encode()+b'\0',(name,hz,value)
    report[f'{name}-{hz}']={'conversions':len(values),'decimalValues':7,'maxTicks':maximum}
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
