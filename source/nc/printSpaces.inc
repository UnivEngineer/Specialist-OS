; B пробелов (0..63) одним строковым вызовом; HL/DE сохранены.
printSpaces:
        ld a,b
        or a
        ret z
        push hl
        ld hl,ncEmptyText
        call ncPrintField
        pop hl
        ld b,0
        ret
ncEmptyText: DB 0
