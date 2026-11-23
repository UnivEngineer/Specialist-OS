"""Build MAIN and banked editor profiles from the same sources."""
from pathlib import Path
import subprocess,runpy,argparse,sys,re
from tempfile import TemporaryDirectory
R=Path(__file__).absolute().parents[2]
cpu=runpy.run_path(str(R/'scripts/images/cpu.py'))
exe=runpy.run_path(str(R/'scripts/images/executable.py'))
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--profile',choices=['main','bank','all'],default='all')
p.add_argument('--hex-pseudo',action='store_true',help='Render HEX tables with ten-pixel pseudographics')
a=p.parse_args()
hex_defines = ['-DEDIT_HEX_PSEUDO=1'] if a.hex_pseudo else []

def assemble(stem, defines):
    subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',*defines,
        f'--raw={stem.with_suffix(".i80")}',f'--sym={stem.with_suffix(".sym")}',
        f'--lst={stem.with_suffix(".lst")}', 'EDIT.asm'],cwd=R/'source/EDIT',check=True)
    return stem.with_suffix('.sym').read_text()

def write_overlay(name, origin, body):
    stem=R/'bin'/name
    stem.with_suffix('.i80').write_bytes(body)
    stem.with_suffix('.cpu').write_text(cpu['format_cpu'](origin,flags=2),encoding='ascii')
    stem.with_suffix('.dll').write_bytes(exe['encode'](exe['Image'](body,origin,origin,2),'dll'))

def assemble_bank(stem, origin):
    defines=['-DEDIT_BANKED=1', *hex_defines]
    with TemporaryDirectory(dir=stem.parent) as temp:
        linked_stem=Path(temp)/stem.name
        listing=assemble(linked_stem,defines)
        offsets={key:int(value,16) for key,value in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',listing,re.M)}
        binary=linked_stem.with_suffix('.i80')
        linked=binary.read_bytes()
        hex_at=offsets['hexPacked']-origin
        text_at=offsets['textPacked']-origin
        hex_body, text_body=linked[hex_at:text_at],linked[text_at:]
        assert len(hex_body)==offsets['hexOverlayBytes']
        assert len(text_body)==offsets['textOverlayBytes']
        listing=assemble(linked_stem,defines+[f'-DEDIT_HEX_CS={exe["checksum"](hex_body)}',
                                             f'-DEDIT_TEXT_CS={exe["checksum"](text_body)}'])
        linked=binary.read_bytes()
        assert linked[hex_at:]==hex_body+text_body
        binary.write_bytes(linked[:hex_at])
        for extension in ('.i80','.sym','.lst'):
            linked_stem.with_suffix(extension).replace(stem.with_suffix(extension))
        write_overlay('EDITHEX',offsets['EDIT_HEX_OVERLAY'],hex_body)
        write_overlay('EDITTEXT',offsets['EDIT_TEXT_OVERLAY'],text_body)
        return listing

for name,origin,flags,profile in [('EDIT',0,4,'main'),('EDITCORE',0x100,2,'bank')]:
    if a.profile not in ('all',profile):continue
    stem=R/'bin'/name
    listing=assemble_bank(stem,origin) if profile=='bank' else assemble(stem,hex_defines)
    fields=cpu['fields_from_symbols'](listing)
    (R/'bin'/(name+'.cpu')).write_text(cpu['format_cpu'](origin,flags=flags,fields=fields),encoding='ascii')
    descriptor=R/'bin'/(name+'.cpu')
    kind='dll' if name == 'EDITCORE' else 'com'
    image=exe['decode'](descriptor.read_bytes(),'cpu',path=descriptor)
    filename='EDIT_MN.com' if profile=='main' else 'EDITCORE.dll'
    (R/'bin'/filename).write_bytes(exe['encode'](image,kind))

if a.profile in ('bank','all'):
    subprocess.run([sys.executable,str(R/'scripts/build/build-driver.py'),
        str(R/'source/edit/EDIT_DOS.asm')],check=True)
    descriptor=R/'bin/EDIT_DOS.cpu'
    image=exe['decode'](descriptor.read_bytes(),'cpu',path=descriptor)
    (R/'bin/EDIT.com').write_bytes(exe['encode'](image,'com'))
