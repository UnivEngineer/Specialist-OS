"""Measure eight-row builds with a fixed encoding against the resident slot."""
import argparse
import json
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('checkout', type=Path)
root = parser.parse_args().checkout.resolve()
here = Path(__file__).resolve().parent
src = root/'source/font'
fonts = root/'bin/fonts'


def read(name):
    return (src/name).read_text(encoding='utf-8')


def write(name, text):
    (src/name).write_text(text,encoding='utf-8')


def assemble(name, entry, defines):
    listing = fonts/f'{name}.lst'
    output = fonts/f'{name}.i80'
    command = [str(root/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',
               f'--lst={listing}',f'--raw={output}',*defines,str(src/entry)]
    result = subprocess.run(command,cwd=src,capture_output=True,text=True)
    pattern = r'\b([0-9A-F]{4})\s+ASSERT \$ <= (?:FONT_ADDR|K7_TABLES)'
    code_end = int(re.search(pattern,listing.read_text(encoding='utf-8'))[1],16)
    return dict(assembled=result.returncode==0,code_end=code_end,code=code_end-0xD800,
                fits_font_at_E400=code_end<=0xE400,spare_before_font=0xE400-code_end)


install = read('install.inc')
begin = install.index('; Переставляем')
end = install.index('; The module owns')
install = install[:begin]+'setEncoding: ret\n'+install[end:]
install = install.replace('    call setEncoding\n','').replace('    ld (encodingOld+1),a\n','')
write('fixed8-install.inc',install)

normal = read('ENGINE.asm')
normal = normal.replace('"packet6.inc"','"fixed8-packet.inc"').replace('"install.inc"','"fixed8-install.inc"')
write('FIXED8KOI8.asm',normal)
packet = read('packet6.inc')
write('fixed8-packet.inc',packet.replace('"packed-columns.inc"','"fixed8-columns.inc"'))
old = read('packed-columns.inc')
prototype = (here.parent/'fontben-6x8-snake-2026-11-21/rotate-columns.inc').read_text(encoding='utf-8')
write('fixed8-columns.inc',prototype+old[old.index('groupNormalDown:'):])

data = {
    'fixed_koi8': assemble('FixedKOI8Unroll','FIXED8KOI8.asm',['-DROTATE_COLUMN_ROWS=8']),
    'fixed_koi7': assemble('FixedKOI7Unroll','KOI7.asm',['-DK7_MODE=0']),
}
print(data)
(here/'unroll.json').write_text(json.dumps(data,indent=2)+'\n')
