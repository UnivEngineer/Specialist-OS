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
    ld de,ncOverwriteName
    call compactName
    ld hl,ncOverwriteModal
    ld a,2
    ld bc,UI_FOCUS_GRID*256
    call uiModalFocusInit
    ld hl,ncOverwriteModal
    call ncModalOpen
    ld hl,ncOverwriteTitle
    call uiModalTitle
    ld hl,ncOverwriteName
    call uiModalBodyText
ncOverwriteKey:
    call uiModalReadChoice
    jp nc,ncOverwriteDone
    cp 1Bh
    jp z,ncOverwriteNoKey
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
    ld e,a
    ld d,0
    ld hl,ncOverwriteAnswers
    add hl,de
    ld a,(hl)
    or a
    jp ncDialogClose
ncOverwriteModal:
    UI_MODAL_BEGIN 12,2,2
    UI_MODAL_BUTTON ncOverwriteYes,0,0,0
    UI_MODAL_BUTTON ncOverwriteYesAll,0,1,0
    UI_MODAL_BUTTON ncOverwriteNo,1,0,0
    UI_MODAL_BUTTON ncOverwriteNoAll,1,1,0
    UI_MODAL_END
ncOverwriteAnswers:     DB 1,2,0,3
ncOverwriteTitle:       DB 'Overwrite file?',0
ncOverwriteYes:         DB 'Yes [Y]',0
ncOverwriteYesAll:      DB 'All [A]',0
ncOverwriteNo:          DB 'No [N]',0
ncOverwriteNoAll:       DB 'Skip all [S]',0
ncOverwriteName:        DS 16,0
