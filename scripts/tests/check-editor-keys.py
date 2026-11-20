"""8080: Shift toolbars, line marking, modal search and aligned menu."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2]
g=runpy.run_path(str(R/'scripts/tests/check-editor-layout.py'))
fixture,run,document,word,number=(g[k] for k in ('fixture','run','document','word','number'))
S=g['S'];T=g['T'];W=g['W']

def key(m,code,ctrl=False,end=0x8c40):
    m=bytearray(m);at=S['bios_getch']
    m[at:at+4]=bytes([0x3e,code,0x37 if ctrl else 0xb7,0xc9])
    return run(m,'keyInput',end=end)

expected=[
    ['editorToggleInsert','editorHelp','editorFind','editorToggleMark','editorCopyMarked','editorMoveMarked','editorFindNext','editorDeleteMarked'],
    ['editorCycleTab','editorToggleEncoding','editorRequestNew','editorRequestLoad','editorWriteDocument','editorAppendDocument','editorUnusedFunction','editorUnusedFunction'],
]
for bank,actions in enumerate(expected):
    for code,handler in enumerate(actions):
        m=fixture(b'ABC\rNEXT');m[0xffe1]=0 if bank else 2
        key(m,code,end=S[handler])
for code,handler in [(0x19,'editorPageUp'),(0x1a,'editorPageDown'),(8,'movePreviousWord'),(0x18,'moveNextWord'),(12,'keyHandler_DocumentHome'),(10,'keyHandler_DocumentEnd'),(ord('z'),'editorUndoLine'),(0xfa,'editorUndoLine')]:
    key(fixture(b'ABC\rNEXT'),code,True,end=S[handler])
for shift in (0,2):
    for code,handler in [(0x8a,'hexToggle'),(0x8b,'keyHandler_CTP'),(0x1f,'keyHandler_CTP')]:
        m=fixture(b'ABC\r');m[0xffe1]=shift
        key(m,code,end=S[handler])
m=fixture(b'ABC\r');m=key(m,27);assert document(m)==b'ABC\r'
# Document input changes only the shape used during its key wait.
for mode,shape in [(0xff,0x21),(0,0x28)]:
    for initial in (0x37,0xa1,0):
        m=fixture(b'ABC\r')
        cfg=S['bios_upper.cursorCfg']
        m[cfg]=initial
        m[S['v_insertMode']]=mode
        entered=run(m,'editorGetch',end=S['bios_getch'])
        assert entered[cfg]==(shape if initial==0x37 else initial)
        at=S['bios_getch']
        m[at:at+4]=b'\x3e\x1b\xb7\xc9'
        returned=run(m,'editorGetch')
        assert returned[cfg]==initial

assert not any(k.startswith(('editorEsc','keyHandler_Esc','KeyHendler')) for k in S)

for font in ['Rotate','Fast8','Cpm4']:
    m=fixture(b'ABC\rNEXT',font);m=run(m,'drawStatusBar')
    # Status fields start on the VRAM byte containing the nominal column.
    glyph=m[S['v_charStep']]*2
    for start,width in [(4,m[S['editorStatusNumBytes']]),
                        (m[S['editorStatusColPos']]+4,m[S['editorStatusNumBytes']]),
                        (m[S['editorStatusTabPos']]+4,m[S['editorStatusTabBytes']])]:
        assert start*glyph//8+width<=48
    assert m[S['editorStatusModePos']]+6<m[S['editorStatusLangPos']]
    assert m[S['editorStatusLangPos']]+3==m[S['v_columns']]
    for shift in (0,2):
        m[0xffe1]=shift
        m=run(m,'editorUpdateToolbarKeys')
        assert m[S['editorShiftState']]==(shift==0)
    m=run(m,'editorToggleInsert');assert not m[S['v_insertMode']]
    assert b'Ins\0' in m[S['editorToolbarMain']:S['editorToolbarFiles']]
    # Four-digit values leave the same black margin on both ends.
    m=fixture(b'X'*9998,font)
    word(m,'v_lineNumber',9999)
    word(m,'v_column',9998)
    m=run(m,'drawStatusBar')
    glyph=m[S['v_charStep']]*2
    width=m[S['editorStatusNumBytes']]*8
    margin=(width-4*glyph)//2
    assert margin>=4 and m[S['editorStatusTabBytes']]==3*glyph//8
    for col in (4,m[S['editorStatusColPos']]+4):
        x=col*glyph//8*8
        for y in range(1,9):
            pixels=[bool(m[0x9000+(pos//8)*256+y] & (0x80>>(pos%8)))
                    for pos in range(x,x+width)]
            assert not any(pixels[:margin]+pixels[-margin:]),(font,col,y)
        assert any(m[0x9000+(pos//8)*256+y] & (0x80>>(pos%8))
                   for y in range(1,9) for pos in range(x+margin,x+width-margin))
    # KOI8 alphabet is the language lock; Shift changes case only.
    for lock,shift in [(0,False),(0,True),(1,False),(1,True),(0,False)]:
        m[S['bios_vars.keyLocks']]=0xba|lock;m[0xffe1]=0 if shift else 2
        word(m,'bios_vars.cursorY',0x3015);word(m,'bios_vars.inverse',65535)
        m=run(m,'editorKeyboardIdle')
        assert m[S['editorLastLanguage']]==lock
        assert number(m,'bios_vars.cursorY')==0x3015 and number(m,'bios_vars.inverse')==65535
    # The buffered viewer expands tabs and pads the row used by RawLim.
    m=fixture(b'AB\tCD\r',font)
    data=b'F2\tOpen help\r\n'
    m[S['inputBuf']:S['inputBuf']+len(data)]=data
    word(m,'uiHelpNext',S['inputBuf'])
    m[S['uiHelpRemaining']]=len(data)
    m=run(m,'uiHelpLine')
    assert m[S['uiHelpRow']:S['uiHelpRow']+28]==b'F2           Open help      '
    m=run(m,'editorCycleTab');assert m[S['v_tabWidth']]==2 and document(m)==b'AB\tCD\r'

# This notice is synchronous and non-interactive, even when it is drawn.
for size in (0,4096,4097,23039):
    m=fixture(b'ABC\r')
    word(m,'fileDescSize',size)
    m=run(m,'editorShowFileProgress',hl=S['editorBusyLoading'])
    assert m[S['editorProgressVisible']]==(size>4096)
    m=run(m,'editorFinishFileProgress')
    assert not m[S['editorProgressVisible']] and document(m)==b'ABC\r'

original=b'AA\rBB\rCC\rDD\r'
for op,want in [('editorCopyMarked',b'AA\rBB\rAA\rBB\rCC\rDD\r'),('editorMoveMarked',b'AA\rBB\rCC\rDD\r'),('editorDeleteMarked',b'CC\rDD\r')]:
    m=fixture(original);m=run(m,'editorToggleMark')
    assert m[S['editorMarkActive']] and not number(m,'v_selectionEnd')
    m=run(m,'keyHandler_Down');m=run(m,'keyHandler_Down');m=run(m,'editorUpdateMark')
    assert number(m,'v_selectionBegin')==T and number(m,'v_selectionEnd')==T+6
    m=run(m,'editorToggleMark');assert not m[S['editorMarkActive']]
    m=run(m,op);assert document(m)==want,(op,document(m))
m=fixture(original)
for _ in range(2):m=run(m,'keyHandler_Down')
m=run(m,'editorToggleMark')
for _ in range(2):m=run(m,'keyHandler_Up');m=run(m,'editorUpdateMark')
assert number(m,'v_selectionBegin')==T and number(m,'v_selectionEnd')==T+6
m=run(m,'editorToggleMark');m=run(m,'editorToggleMark')
assert not number(m,'v_selectionBegin') and not m[S['editorMarkActive']]
m=fixture(b'ABC\r');word(m,'v_column',1);m=run(m,'editorDeleteMarked');assert document(m)==b'AC\r'

# Overlapping matches, current line independent of viewport, wrap and a gap.
m=fixture(b'ABA ABABA\rABA');m[S['v_searchText']:S['v_searchText']+4]=b'ABA\0'
m=run(m,'gapMove',hl=T+5)
for target in [4,6,10,0,4]:
    m=run(m,'editorFindNext')
    assert number(m,'v_currentLine')+number(m,'v_column')==T+target
    assert document(m)==b'ABA ABABA\rABA'
# Search itself is an SDK modal, cancelled without changing text.
m=fixture(b'ABC\rNEXT');m=run(m,'drawStatusBar')
m=run(m,'editorFind',end=S['lineEditLoop'])
m[S['bios_getch']:S['bios_getch']+4]=b'\x3e\x1b\xb7\xc9'
m=run(m,'editorFind');assert document(m)==b'ABC\rNEXT'
m=fixture(b'ABC\r');m[S['v_searchText']:S['v_searchText']+4]=b'ZZZ\0'
info=run(m,'editorFindNext',end=S['bios_getch'])
assert info[S['IO_COLOR']]==7 and info[S['bios_upper.cursorCfg']]==0
info=run(m,'editorFindNext',end=S['editorInfoWait'])
assert number(info,'uiModalCurrent')==S['editorInfoDescriptor']
assert info[S['editorInfoDescriptor']+18]==0

# New is guarded by the existing Save/Discard/Cancel dialog.
m=fixture(b'ABC\r');m[S['editorShiftState']]=1
m=run(m,'editorRequestNew',end=S['clearAndRedrawDocument'])
assert document(m)==b'' and m[S['editorShiftState']]==1 and not m[S['v_documentDirty']]
m=fixture(b'ABC\r');m[S['v_documentDirty']]=1
m=run(m,'editorRequestNew',end=S['editorSaveWait'])
assert document(m)==b'ABC\r' and number(m,'editorPendingAction')==S['editorNewDocument']
assert number(m,'uiModalCurrent')==S['editorSaveDescriptor']
assert m[S['editorSaveDescriptor']+18]==0
# Save changes hides the caret during getch and restores the configuration.
cfg=S['bios_upper.cursorCfg']
m[cfg]=0x37
hidden=run(m,'editorSaveWait',end=S['bios_getch'])
assert hidden[cfg]==0
m[S['bios_getch']:S['bios_getch']+4]=b'\x3e\x1b\xb7\xc9'
m=run(m,'editorSaveWait')
assert m[cfg]==0x37 and document(m)==b'ABC\r'
m=run(m,'editorDiscardFile',end=S['clearAndRedrawDocument']);assert document(m)==b''
print('EDITOR KEYS/MODALS PASS',g['run'].__globals__['count'])
