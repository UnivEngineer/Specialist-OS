"""8080 retained line snapshot, navigation, replacement and F8 relocation."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2]
g=runpy.run_path(str(R/'scripts/tests/check-editor-layout.py'))
fixture,run,document,word,number=(g[k] for k in ('fixture','run','document','word','number'))
S=g['S'];T=g['T']

def edited(text=b'ABC\rSECOND\rTHIRD\r',row=0,column=1,font='Rotate'):
    m=fixture(text,font)
    for _ in range(row):m=run(m,'keyHandler_Down')
    word(m,'v_column',column)
    return run(m,'keyHandler_Space',bc=ord('X'))

for font in ['Rotate','Fast8','Cpm4']:
    original=b'ABC\rSECOND\rTHIRD\r'
    m=edited(original,font=font);changed=document(m)
    # Moving away and back, even through document/page jumps, keeps the
    # original snapshot instead of taking a new one from the edited line.
    for key in ['keyHandler_Down','keyHandler_Down','keyHandler_Up','keyHandler_DocumentEnd','keyHandler_DocumentHome','editorPageDown','editorPageUp']:
        m=run(m,key)
        assert m[S['v_lineDirty']] and number(m,'v_undoLine')==T
        assert document(m)==changed
    m=run(m,'keyHandler_Down')
    m=run(m,'editorUndoLine');m=run(m,'editorPositionCursor')
    assert document(m)==original
    assert number(m,'v_currentLine')==T and number(m,'v_lineNumber')==1
    assert number(m,'v_column')==1 and m[S['mon2_vars.cursorY']]==S['EDITOR_FIRST_Y']
    assert not m[S['v_lineDirty']]

    # A saved line still on screen retains the viewport and the correct Y.
    m=edited(original,row=1,column=2,font=font)
    m=run(m,'keyHandler_Up');m=run(m,'editorUndoLine')
    assert document(m)==original and number(m,'v_currentLine')==T+4
    assert number(m,'v_lineNumber')==2 and number(m,'v_column')==2
    assert number(m,'v_topLine')==T and m[S['mon2_vars.cursorY']]==S['EDITOR_FIRST_Y']+10

    # A genuine edit in another line replaces the snapshot. F8 must retain
    # the first line's changes and undo only the newly edited line.
    m=edited(original,font=font);first=document(m)
    m=run(m,'keyHandler_Down');word(m,'v_column',2)
    m=run(m,'keyHandler_Tab');assert number(m,'v_undoLine')==T+5
    m=run(m,'keyHandler_Up');m=run(m,'editorUndoLine')
    assert document(m)==first and number(m,'v_currentLine')==T+5
    assert number(m,'v_lineNumber')==2 and number(m,'v_column')==2

    # Returning to the same line and typing again keeps its first snapshot.
    m=edited(original,font=font);m=run(m,'keyHandler_Down');m=run(m,'keyHandler_Up')
    m=run(m,'keyHandler_Space',bc=ord('Y'));m=run(m,'editorUndoLine')
    assert document(m)==original

    # Structural changes invalidate the snapshot, including Backspace at
    # the start of a different line and Enter on another line.
    for key in ['keyHandler_Enter','keyHandler_Backspace']:
        m=edited(original,font=font);m=run(m,'keyHandler_Down');m=run(m,'keyHandler_Home')
        m=run(m,key);structural=document(m)
        assert not m[S['v_lineDirty']]
        m=run(m,'editorUndoLine');assert document(m)==structural

# Offscreen return in both directions must repaint the whole changed page.
for original,row in [(b'\rNEXT',0),(b'FIRST\r',1)]:
    m=edited(original,row=row,column=0)
    m=run(m,'keyHandler_DocumentHome' if row else 'keyHandler_DocumentEnd')
    m=run(m,'editorUndoLine')
    assert document(m)==original and number(m,'v_column')==0
    assert number(m,'v_lineNumber')==row+1

original=b'ROW\r'*70
for row,key in [(0,'keyHandler_DocumentEnd'),(60,'keyHandler_DocumentHome')]:
    m=edited(original,row=row);m=run(m,key)
    m=run(m,'editorUndoLine');m=run(m,'editorPositionCursor')
    assert document(m)==original and number(m,'v_currentLine')==T+row*4
    assert number(m,'v_lineNumber')==row+1 and number(m,'v_column')==1
    assert number(m,'v_topLine')==T+row*4
    full=run(m,'redrawPreservingCursor')
    assert m[0x9000:0xc000]==full[0x9000:0xc000]

# Search and opening/cancelling a filename prompt aren't edits.
original=b'A'*300+b'\rNEXT\r'
m=fixture(original);word(m,'v_column',200);m=run(m,'editorPositionCursor')
left=number(m,'v_leftColumn')
m=run(m,'keyHandler_Space',bc=ord('X'));m=run(m,'keyHandler_Down')
m=run(m,'keyHandler_Home');m=run(m,'editorPositionCursor')
m=run(m,'editorUndoLine')
assert document(m)==original and number(m,'v_column')==200
assert number(m,'v_leftColumn')==left
full=run(m,'redrawPreservingCursor')
assert m[0x9000:0xc000]==full[0x9000:0xc000]

m=edited();m=run(m,'prepareFilePrompt')
assert m[S['v_lineDirty']]
m[S['v_searchText']:S['v_searchText']+6]=b'THIRD\0'
m=run(m,'editorFindNext');assert m[S['v_lineDirty']]
m=run(m,'editorUndoLine');assert document(m)==b'ABC\rSECOND\rTHIRD\r'

# An OVR undo after saving must mark the restored text dirty even though
# its length did not change and resizeTextRange therefore did nothing.
m=fixture(b'ABC\r');m=run(m,'editorToggleInsert');word(m,'v_column',1)
m=run(m,'keyHandler_Space',bc=ord('X'));m[S['v_documentDirty']]=0
m=run(m,'keyHandler_Down');m=run(m,'editorUndoLine')
assert document(m)==b'ABC\r' and m[S['v_documentDirty']]
print('EDITOR LINE UNDO PASS',g['run'].__globals__['count'])
