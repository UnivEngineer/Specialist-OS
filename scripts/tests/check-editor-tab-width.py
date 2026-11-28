"""8080 variable tab stops, menu fields, input idle and incremental drawing."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2]
g=runpy.run_path(str(R/'scripts/tests/check-editor-layout.py'))
fixture,run,document,word,number=(g[k] for k in ('fixture','run','document','word','number'))
S=g['S'];T=g['T'];W=g['W']
wide=lambda m,k:number(m,k)+(m[S[k+'Hi']]<<16)

for font in ['Rotate','Fast8','Cpm4']:
    for tab in [2,4,6,8]:
        raw=b'A'*255+b'\tB\tC'+b'xy\t'*40
        payload=raw+b'\rNEXT\r'
        m=fixture(payload,font);m[S['v_tabWidth']]=tab
        m=run(m,'gapMove',hl=T+257)
        for col in [0,1,254,255,256,257,258,300,len(raw)]:
            word(m,'v_column',col);m=run(m,'editorPositionCursor')
            assert wide(m,'v_visualColumn')==len(raw[:col].expandtabs(tab)),(font,tab,col)
        for left in [0,8,248,256,264,280]:
            word(m,'v_leftColumn',left);m[S['v_leftColumnHi']]=0
            m=run(m,'drawTextLine',hl=T)
            expected=raw.expandtabs(tab)[left:left+m[S['v_columns']]]
            assert m[S['editorRowBuffer']:S['editorRowBuffer']+len(expected)]==expected,(font,tab,left)
        # Goal mapping must choose the legal byte boundary before a Tab.
        m=fixture(b'1234567\r\t\t\tQ\r1234567',font);m[S['v_tabWidth']]=tab
        word(m,'v_column',5);m=run(m,'editorPositionCursor')
        m=run(m,'keyHandler_Down');m=run(m,'editorPositionCursor')
        goal=5;want=min(goal//tab,3)
        assert number(m,'v_column')==want,(font,tab,'vertical')
        m=run(m,'keyHandler_Down');assert number(m,'v_column')==5

        # All normal edits must match a full redraw while using no clear.
        for key,col in [('keyHandler_Space',11),('keyHandler_Del',11),('keyHandler_Backspace',12),('keyHandler_Tab',11)]:
            original=b'AB\tCDEFGHIJ\tTAIL\rNEXT\r'
            m=fixture(original,font);m[S['v_tabWidth']]=tab
            m=run(m,'redrawPreservingCursor');word(m,'v_column',col)
            m=run(m,'editorPositionCursor')
            m=run(m,key,bc=ord('X'))
            full=run(m,'redrawPreservingCursor')
            assert m[0x9000:0xc000]==full[0x9000:0xc000],(font,tab,key)

    m=fixture(b'AB\tLONG\rNEXT',font)
    m=run(m,'drawStatusBar')
    cols=m[S['v_columns']]
    assert m[S['editorStatusModePos']]+6<m[S['editorStatusLangPos']]
    assert m[S['editorStatusLangPos']]+3==cols
    assert m[S['editorStatusColPos']]+8<m[S['editorStatusTabPos']]
    original=document(m)
    m=run(m,'keyHandler_Space',bc=ord('X'))
    for tab in [2,4,6,8]:
        m=run(m,'editorAdvanceTabWidth');m=run(m,'updateStatusBar')
        assert m[S['v_tabWidth']]==tab and m[S['editorLastTab']]==tab
        assert m[S['v_lineDirty']] and document(m)==b'X'+original
    m=run(m,'editorUndoLine');assert document(m)==original

    # The idle callback refreshes language only, preserving BIOS text state.
    m=run(m,'drawStatusBar');word(m,'mon2_vars.cursorY',0x1815)
    word(m,'mon2_vars.inverse',0xffff)
    before=bytes(m[0x9000:0xc000]);m[S['mon2_vars.keyLocks']]|=1
    m=run(m,'editorKeyboardIdle')
    assert m[S['editorLastLanguage']]==1
    assert number(m,'mon2_vars.cursorY')==0x1815 and number(m,'mon2_vars.inverse')==65535
    start=m[S['editorStatusLangPos']]*m[S['v_charStep']]*2//8
    for x in range(48):
        for y in range(256):
            if x>=start and 1<=y<=8:continue
            assert m[0x9000+x*256+y]==before[x*256+y]

    # Observe the actual raw-print range. Prefix and unrelated tail pixels
    # must survive; calling the SDK fill rectangle marks a failure.
    m=fixture(b'0123456789ABCDEFGHIJ\r',font)
    m=run(m,'redrawPreservingCursor');word(m,'v_column',11)
    m=run(m,'editorPositionCursor')
    prefix=0x9000+S['EDITOR_TEXT_TOP']+3
    tail=0x9000+45*256+S['EDITOR_TEXT_TOP']+3
    m[prefix]^=0x55;m[tail]^=0x33;markers=(m[prefix],m[tail])
    public=S['FONT_ENGINE_LIM'];target=int.from_bytes(m[public+1:public+3],'little')
    code=b'\xf5\x3a'+W(S['mon2_vars.cursorX'])+b'\x32\x80\x8d\x79\x32\x81\x8d\x22\x82\x8d\xf1\xc3'+W(target)
    m[0x8d00:0x8d00+len(code)]=code;m[public+1:public+3]=W(0x8d00)
    at=S['uiFillRectBytes'];m[at:at+6]=b'\x3e\x01\x32\x84\x8d\xc9'
    m=run(m,'keyHandler_Space',bc=ord('X'))
    assert not m[0x8d84] and (m[prefix],m[tail])==markers
    assert m[0x8d80]==8*m[S['v_charStep']] and m[0x8d81]==16
    assert int.from_bytes(m[0x8d82:0x8d84],'little')==S['editorRowBuffer']+8

    # Removing an entire 8-cell Tab shifts the suffix by eight screen cells.
    # The cached old length, not the new byte length, determines its erasure.
    for prefix in [b'',b'ABCDEFGH',b'A'*(cols-8)]:
        for key,col in [('keyHandler_Del',len(prefix)),('keyHandler_Backspace',len(prefix)+1)]:
            original=prefix+b'\tTAIL\rNEXT'
            m=fixture(original,font);m[S['v_tabWidth']]=8
            m=run(m,'redrawPreservingCursor');word(m,'v_column',col)
            m=run(m,'editorPositionCursor');m=run(m,key)
            assert document(m)==prefix+b'TAIL\rNEXT'
            full=run(m,'redrawPreservingCursor')
            assert m[0x9000:0xc000]==full[0x9000:0xc000],(font,key,'8-cell tail')

    # C0/DEL are blank cells. KOI8 graphics and letters in 80h..FFh remain
    # printable, and none of the original document bytes are rewritten.
    controls=bytes(c for c in range(1,32) if c not in [9,10,13])+b'\x7f'
    raw=b'A'+controls+b'\tZ\x80\x9f\xa0\xff'
    m=fixture(raw+b'\r',font)
    m=run(m,'drawTextLine',hl=T)
    expected=(b'A'+b' '*len(controls)+b'\tZ\x80\x9f\xa0\xff').expandtabs(8)
    assert m[S['editorRowBuffer']:S['editorRowBuffer']+len(expected)]==expected
    assert document(m)==raw+b'\r'

# Width 6 remains correct beyond the low 16 display bits as well.
m=fixture(b'\t'*12000+b'Z');m[S['v_tabWidth']]=6
word(m,'v_column',12000);m=run(m,'editorPositionCursor')
assert wide(m,'v_visualColumn')==72000
print('EDITOR TAB WIDTH/REDRAW PASS',g['run'].__globals__['count'])
