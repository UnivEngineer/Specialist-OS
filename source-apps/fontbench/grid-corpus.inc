; 924 bytes is divisible by 42 data bytes and by 44 with CRLF.
; Even with CR/LF drawn as glyphs and X=6 px, 44 cells of width 8 fit.
GRID_ROWS       = 21
GRID_PHASE_ROWS = 22
GRID_GLYPHS     = 42
GRID_STRIDE     = GRID_GLYPHS+2
GRID_TEXT_BYTES = GRID_ROWS*GRID_STRIDE
GRID_UNROLL     = 12
    ASSERT GRID_PHASE_ROWS*GRID_GLYPHS == GRID_TEXT_BYTES
    ASSERT GRID_TEXT_BYTES % GRID_UNROLL == 0
