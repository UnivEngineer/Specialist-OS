; Следующее слово командной строки (не более 255 байт в слове).
; Вход: HL=курсор в строке; пробел — разделитель, байт <20h — конец.
; Выход: CF=0 — DE=начало, B=длина, HL=разделитель после слова.
;        CF=1 — конец строки, B=0, DE=HL=терминатор.
; C сохраняется; AF/B/DE/HL разрушаются. Строка не изменяется.
; Нет буферов и переменных. Кавычки и экранирование не интерпретируются.
argNext:
    ld b,0
argSkipSpaces:
    ld a,(hl)
    cp ' '
    jp nz,argStart
    inc hl
    jp argSkipSpaces
argStart:
    ld d,h
    ld e,l
    cp ' '+1
    ret c
argScanWord:
    inc b
    inc hl
    ld a,(hl)
    cp ' '+1
    jp nc,argScanWord
    or a
    ret
