; Модальный выбор замены: A=FAR-банк DOS, HL=FILE_DESCRIPTOR назначения, A=0/1/2/3, CF=0.
; 0=No, 1=Yes, 2=Yes to all, 3=No to all. Escape равен No.
; Стрелки/Tab меняют выбор; Y/N/A/S выбирают ответ напрямую.
; Все регистры разрушены. Дисковые службы из обработчика не вызываются.
ncOverwritePrompt:
    MODULE localConfirm
    DEFINE COPY_CONFIRM_BODY ncOverwriteBody
    INCLUDE "../common/copyConfirmLocal.inc"
    UNDEFINE COPY_CONFIRM_BODY
    ENDMODULE
ncOverwriteBody:
    push hl
    ld hl,ncOverwriteWindow
    call ncDialogOpen
    pop hl
    ld de,ncOverwriteName
    call compactName
    COLOR COLOR_DIALOG
    ld hl,ncOverwriteWindow
    call draw
    ld hl,ncOverwriteTitle
    ld e,103
    call ncOverwriteCentered
    ld hl,ncOverwriteName
    ld e,120
    call ncOverwriteCentered
    ld hl,ncOverwriteButtons
    ld b,4
ncOverwriteDraw:
    push bc
    push hl
    ld de,UI_BUTTON.selected
    add hl,de
    ld (hl),0
    pop hl
    push hl
    call ncDrawButton
    pop hl
    ld de,UI_BUTTON
    add hl,de
    pop bc
    dec b
    jp nz,ncOverwriteDraw
    ld a,2
    ld (ncOverwriteChoice),a
    call ncOverwriteInvert
ncOverwriteKey:
    call ncModalGetch
    cp 01Bh
    jp z,ncOverwriteNoKey
    cp 0Dh
    jp z,ncOverwriteDone
    cp 8
    jp z,ncOverwriteLeft
    cp 018h
    jp z,ncOverwriteRight
    cp 9
    jp z,ncOverwriteNext
    cp 019h
    jp z,ncOverwriteVertical
    cp 01Ah
    jp z,ncOverwriteVertical
    and 0DFh
    cp 'Y'
    jp z,ncOverwriteYesKey
    cp 'N'
    jp z,ncOverwriteNoKey
    cp 'A'
    jp z,ncOverwriteYesAllKey
    cp 'S'
    jp z,ncOverwriteNoAllKey
    jp ncOverwriteKey
ncOverwriteLeft:
ncOverwriteRight:
    ld b,1
    jp ncOverwriteChange
ncOverwriteVertical:
    ld b,2
ncOverwriteChange:
    push bc
    call ncOverwriteInvert
    pop bc
    ld a,(ncOverwriteChoice)
    xor b
    ld (ncOverwriteChoice),a
    call ncOverwriteInvert
    jp ncOverwriteKey
ncOverwriteNext:
    call ncOverwriteInvert
    ld a,(ncOverwriteChoice)
    inc a
    and 3
    ld (ncOverwriteChoice),a
    call ncOverwriteInvert
    jp ncOverwriteKey
ncOverwriteCentered:
    ld b,32
    ld c,124
    ld a,UI_ALIGN_CENTER
    jp ncPrintAligned
ncOverwriteYesKey:
    ld a,1
    or a
    jp ncDialogClose
ncOverwriteYesAllKey:
    ld a,2
    or a
    jp ncDialogClose
ncOverwriteNoAllKey:
    ld a,3
    or a
    jp ncDialogClose
ncOverwriteNoKey:
    xor a
    jp ncDialogClose
ncOverwriteDone:
    ld a,(ncOverwriteChoice)
    ld e,a
    ld d,0
    ld hl,ncOverwriteAnswers
    add hl,de
    ld a,(hl)
    or a
    jp ncDialogClose
ncOverwriteInvert:
    ld a,(ncOverwriteChoice)
    ld l,a
    ld h,0
    add hl,hl
    ld d,h
    ld e,l
    add hl,hl
    add hl,hl
    add hl,de
    ld de,ncOverwriteButtons
    add hl,de
    push hl
    ld de,UI_BUTTON.selected
    add hl,de
    ld a,(hl)
    xor 1
    pop hl
    jp uiSelectButton
ncOverwriteButtons:
    UI_BUTTON ncOverwriteYes,32,60,128,14,6,0
    UI_BUTTON ncOverwriteYesAll,96,60,128,14,6,0
    UI_BUTTON ncOverwriteNo,32,60,144,14,6,0
    UI_BUTTON ncOverwriteNoAll,96,60,144,14,6,0
ncOverwriteLabels:
    DW ncOverwriteYes,ncOverwriteYesAll,ncOverwriteNo,ncOverwriteNoAll
ncOverwriteAnswers: DB 1,2,0,3
ncOverwriteWindow:
    G_MODAL_WINDOW 80h,56,88,264,76
    G_LINE 80h,60,108,256
    G_LINE 80h,60,124,256
    DB 0
ncOverwriteTitle: DB 'Overwrite file?',0
ncOverwriteYes: DB 'Yes [Y]',0
ncOverwriteYesAll: DB 'Yes to all [A]',0
ncOverwriteNo: DB 'No [N]',0
ncOverwriteNoAll: DB 'No to all [S]',0
ncOverwriteChoice: DB 2
ncOverwriteButtonIndex: DB 0
ncOverwriteName: DS 16,0
