"""Keyboard scenarios hosted by check-nc-info.py in its owned test emulator."""
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[2]

def symbols(name):
    return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
            (ROOT/'bin'/name).read_text('utf-8'),re.M)}

def shell(m,n,command,type_char,wait_nc,out):
    cli=symbols('CLI.sym');bio=symbols('BIOS.sym')
    def ready():
        m.until(cli['cliPrompt'],120,page=0)
        assert m.read(0,32)==(ROOT/'bin/CLI.i80').read_bytes()[:32]
        m.call('bp_clear');m.call('emu_run_for',ms=500)
    def line(value):
        for char in value:type_char(char)
        m.stop_at(cli['cliPrompt']);m.tap('enter');ready()
    # Writes affect only the owning harness's disposable system flash.
    command('COPY C:/DOS/NC_MN.COM B:/NC_MN.COM')
    command('ATTRIB -RS C:/DOS/NC_MN.COM');command('DEL C:/DOS/NC_MN.COM')
    m.stop_at(cli['cliPrompt']);m.tap('f10');ready()
    line('NC_MN.COM');line('EXIT')
    print('Missing NC: failed command and EXIT return to CLI PASS',flush=True)
    line('ATTRIB -RS C:/DOS/CLI.COM');line('DEL C:/DOS/CLI.COM')
    for char in 'EXIT':type_char(char)
    m.stop_at(bio['bios_input']);m.tap('enter');m.until(bio['bios_input'],120,page=0)
    m.screenshot(out/'both-shells-missing.png')
    m.call('bp_clear');m.call('emu_run_for',ms=500)
    for char in 'A:/MISSING.COM':type_char(char)
    m.stop_at(bio['biosRecoveryStart']);m.tap('enter');m.until(bio['biosRecoveryStart'],120,page=0)
    m.stop_at(bio['bios_input']);m.call('debug_step',mode='into');m.until(bio['bios_input'],120,page=0)
    m.call('bp_clear');m.call('emu_run_for',ms=500)
    for char in 'B:/NC_MN.COM':type_char(char)
    m.stop_at(n['ncReadKey']);m.tap('enter');wait_nc()
    m.screenshot(out/'shell-recovered-from-B.png')
    print('BIOS shell-path prompt, invalid retry and B:/NC_MN.COM launch PASS',flush=True)
    command('COPY B:/NC_MN.COM C:/DOS/NC_MN.COM')
    m.stop_at(bio['biosBankShell']);m.tap('f10');m.until(bio['biosBankShell'],120,page=0)
    m.stop_at(n['ncReadKey']);m.call('debug_step',mode='into');wait_nc()
    print('Missing CLI: F10 falls back to NC PASS',flush=True)

def directory(m,n,command,type_char,wait_nc,out):
    command('MKDIR B:/EMPTY')
    for value,error in [('DIR B:/EMPTY',False),('DIR B:/ABSENT.TXT',False),
                        ('DIR B:/ABSENT/',True)]:
        for char in value:type_char(char)
        m.stop_at(n['ncCommandResult']);m.tap('enter')
        cpu=m.until(n['ncCommandResult'],120,page=0)
        assert cpu['flags']['C']==error,(value,cpu)
        if error:
            assert cpu['registers']['af']['dec']>>8==n['DOS_ERR_NOT_FOUND']
            m.stop_at(n['copyFileAnyKey']);m.call('debug_step',mode='into')
            m.until(n['copyFileAnyKey'],120,page=0)
        else:
            m.call('bp_clear');m.call('emu_run_for',ms=500)
        m.screenshot(out/('dir-invalid-path.png' if error else 'dir-no-matches.png'))
        m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('enter');wait_nc()
        print('DIR status and UI PASS',value,'error' if error else 'success',flush=True)

def inputs(m,n,command,type_char,wait_nc,out):
    def cancel():
        m.call('bp_clear');m.call('emu_run_for',ms=300);m.tap('esc');wait_nc()
    def field(key,expected,font,shift=False):
        def check_padding():
            baseline,half_x=m.read(n['lineEditOrigin'],2)
            height=m.read(n['ncGlyphHeight'],1)[0]
            for y in (baseline-height,baseline+1):
                assert all(m.read(0x9000+(x//8)*256+y,1)==b'\xff'
                           for x in range(half_x*2,half_x*2+240,8)),(font,key,y)
        if shift:
            m.call('kbd_send',keys=['shift'],action='press')
            m.call('emu_run_for',ms=600)
        m.stop_at(n['lineEditLoop']);m.tap(key);m.until(n['lineEditLoop'],120,page=0)
        if shift:m.call('kbd_send',keys=['shift'],action='release')
        assert int.from_bytes(m.read(n['lineEditBuffer'],2),'little')==n['vars.input']
        actual=m.read(n['vars.input'],128).split(b'\0')[0]
        assert actual.upper()==expected.upper(),(font,key,expected,actual)
        assert m.read(n['lineEditLength'],1)==bytes([len(expected)])
        assert m.read(n['lineEditPosition'],1)==bytes([len(expected)])
        assert m.read(n['lineEditWidth'],1)[0]==240//m.read(n['ncFontWidth'],1)[0]
        check_padding()
        m.screenshot(out/(font+'-initial-'+('rename' if shift else key)+'.png'))
        m.call('bp_clear');m.call('emu_run_for',ms=300)
        if expected:
            m.tap('backspace')
            assert m.read(n['vars.input'],128).split(b'\0')[0].upper()==expected[:-1].upper()
            type_char(chr(expected[-1]))
            assert m.read(n['vars.input'],128).split(b'\0')[0].upper()==expected.upper()
        else:
            type_char('q');m.tap('backspace')
            assert m.read(n['vars.input'],1)==b'\0'
        check_padding()
        cancel()
    command('A:')
    for font,path in [('Cpm4','4x8/Packed'),('Rotate','6x8/Rotate'),('Fast8','8x8/Rotate')]:
        command('FONT '+path)
        m.tap('home');m.call('emu_run_for',ms=500)
        panel=m.read(n['state.activePanel'],1)[0]
        other=m.read(n['vars.pathB'] if panel==0 else n['vars.pathA'],128).split(b'\0')[0]
        slot=m.read(n['vars.cacheSlotA']+panel,1)[0]
        base=int.from_bytes(m.read(n['ncCatalogBases']+slot*2,2),'little')
        index=m.read(n['FILE_INDEX_A']+panel*256,1)[0]
        record=m.read(base+index*16,11)
        name=record[:8].rstrip()+((b'.'+record[8:].rstrip()) if record[8:].strip() else b'')
        expected=other.rstrip(b'\\')+b'\\'+name
        field('f5',expected,font)
        field('f6',expected,font)
        field('f6',name,font,shift=True)
        field('f7',b'',font)
        print(font,'F5/F6 destination, rename, empty F7 and editing PASS',flush=True)
