; Исполняемый модуль: профиль, стек и возврат устанавливает загрузчик DOS.
    INCLUDE "../../source/include/mxos.inc"
; Адрес не совпадает с управляющим циклом main, что упрощает отладку.
    ORG 04000h
entry:
    jp nativeEntry
moduleEntry:
    IF FONT_TEST
    cp 080h
    jp nc,gridBank
    ld (action),a
    ld a,c
    ld (kind),a
    ld a,b
    ld (flags),a
    ld (bulkRequest+4),hl
    ld (lineRequest+4),hl
    call moduleTimerStart
    ld a,(flags)
    and 32
    jp z,bankSelectCall
    ld hl,bulkRequest
    call mx_memcpyFar
    ret c
    ld hl,(bulkRequest+4)
    MXOS_CALL MXOS_BIOS_PRINT_STRING_MAIN
    jp bankFinish
bankSelectCall:
    ld a,(kind)
    or a
    jp z,bankCharacters
    cp 3
    jp z,bankCharacters
    cp 5
    jp c,bankString
    and 1
    jp nz,bankCharacters
bankString:
    ld a,(flags)
    and 64
    jp nz,bankLines
    ; Один штатный вызов на весь поток, включая CRLF.
    ld hl,02000h
    MXOS_CALL MXOS_BIOS_PRINT_STRING_BANK
    jp bankFinish
bankCharacters:
    ld hl,02000h
bankCharacter:
    ld c,(hl)
    ld a,c
    or a
    jp z,bankFinish
    push hl
    cp 020h
    jp c,bankCharacterNormal
    ld a,(action)
    cp 4
    jp nz,bankCharacterNormal
    ld a,c
    MXOS_CALL MXOS_BIOS_PRINT_CHAR_REAL
    jp bankCharacterDone
bankCharacterNormal:
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
bankCharacterDone:
    pop hl
    inc hl
    jp bankCharacter
bankLines:
    ld hl,02000h
    ld (lineRequest+1),hl
    ld a,24
    ld (rowsLeft),a
bankLine:
    ld hl,lineRequest
    call mx_memcpyFar
    ret c
    ; 48 печатных + CRLF, локальный нуль не входит в memcpy_far.
    ld hl,(lineRequest+4)
    ld de,50
    add hl,de
    ld (hl),0
    ld hl,(lineRequest+4)
    MXOS_CALL MXOS_BIOS_PRINT_STRING_MAIN
    ld hl,(lineRequest+1)
    ld de,50
    add hl,de
    ld (lineRequest+1),hl
    ld hl,rowsLeft
    dec (hl)
    jp nz,bankLine
bankFinish:
    call moduleTimerSample
    jp moduleTimerResult
kind: DB 0
flags: DB 0
action: DB 0
rowsLeft: DB 0
bulkRequest: DB 0
    DW 02000h
    DB 0
    DW 0,1201
lineRequest: DB 0
    DW 02000h
    DB 0
    DW 0,50
    ELSE
    ld a,c
    ld (mode),a
    ex de,hl
    ld (rowsLeft),hl
    call moduleTimerStart
    ret c
row:
    ld hl,text
    ld a,(mode)
    or a
    jp nz,string
glyph:
    ld a,(hl)
    or a
    jp z,endRow
    ld c,a
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    inc hl
    jp glyph
string:
    MXOS_CALL MXOS_BIOS_PRINT_STRING_BANK
endRow:
    call moduleTimerSample
    ret c
    ld hl,(rowsLeft)
    dec hl
    ld (rowsLeft),hl
    ld a,h
    or l
    jp nz,row
    jp moduleTimerResult
mode: DB 0
rowsLeft: DW 0
text: DB '0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghij',13,10,0
    ENDIF
    IF FONT_TEST
    INCLUDE "../fontbench/grid-bank.inc"
    ENDIF
    INCLUDE "native-entry.inc"
    INCLUDE "module-timing.inc"
    ASSERT $ < 05000h
