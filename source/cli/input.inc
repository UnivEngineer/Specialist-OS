; Консоль использует общий редактор; Escape очищает текущую строку.
cliInput:
    xor a
    ld (cliHistoryBrowse),a
    ld hl,CMD_LINE
    ld de,CMD_LINE
    ld bc,0
    jp lineEdit
    DEFINE LINEEDIT_HISTORY
    INCLUDE "../include/lineedit.inc"
