"""8080 word navigation crosses CR, empty lines, tabs and viewport edges."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2]
g=runpy.run_path(str(R/'scripts/tests/check-editor-layout.py'))
fixture,run,document,number,word=(g[k] for k in ('fixture','run','document','number','word'))
S=g['S'];T=g['T']
payload=b'one two\r\t \r\r  three four\r\tfive\r'
for font in ['Rotate','Fast8','Cpm4']:
    m=fixture(payload,font);m=run(m,'gapMove',hl=T+7)
    for line,col in [(1,4),(4,2),(4,8),(5,1),(6,0),(6,0)]:
        m=run(m,'moveNextWord');m=run(m,'editorPositionCursor')
        assert (number(m,'v_lineNumber'),number(m,'v_column'))==(line,col),(font,'right',line,col)
        assert document(m)==payload
    for line,col in [(5,1),(4,8),(4,2),(1,4),(1,0),(1,0)]:
        m=run(m,'movePreviousWord');m=run(m,'editorPositionCursor')
        assert (number(m,'v_lineNumber'),number(m,'v_column'))==(line,col),(font,'left',line,col)
        assert document(m)==payload
    # A single-line snapshot must survive even navigation that scrolls.
    m=fixture(b'word\r'*40,font)
    m=run(m,'keyHandler_Space',bc=ord('X'));changed=document(m)
    m=run(m,'keyHandler_Home')
    for line in range(2,34):
        m=run(m,'moveNextWord')
        assert number(m,'v_lineNumber')==line and number(m,'v_column')==0
    assert m[S['mon2_vars.cursorY']]==S['EDITOR_LAST_Y'] and m[S['v_lineDirty']]
    assert document(m)==changed
    for line in range(32,0,-1):
        m=run(m,'movePreviousWord')
        assert number(m,'v_lineNumber')==line and number(m,'v_column')==0
    assert m[S['mon2_vars.cursorY']]==S['EDITOR_FIRST_Y'] and document(m)==changed
print('EDITOR WORD LINES PASS',g['run'].__globals__['count'])
