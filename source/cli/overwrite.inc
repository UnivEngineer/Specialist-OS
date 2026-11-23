;----------------------------------------------------------------------------
; MXOS
;
; Запросить у пользователя решение при существующем назначении COPY.
;
; вход:
;   a = FAR-код банка DOS, hl = адрес FILE_DESCRIPTOR назначения в этом банке. Консоль
;   доступна в MAIN.
;
; выход:
;   При успешном чтении описателя a = 0 для No, 1 для Yes, 2 для Yes all, 3 для Skip
;   all, cf = 0. Ошибка межбанкового чтения передаётся вызывающему. bc, de и hl
;   рабочие.
;----------------------------------------------------------------------------

cliOverwritePrompt:
    MODULE localConfirm
    DEFINE COPY_CONFIRM_BODY cliOverwriteBody
    INCLUDE "../common/copyConfirmLocal.inc"
    UNDEFINE COPY_CONFIRM_BODY
    ENDMODULE
cliOverwriteBody:
    push hl
    ld hl,cliOverwriteText
    call bios_printString
    pop hl
    ld b,8
    call cliOverwritePart
    ld a,(hl)
    cp ' '
    jp z,cliOverwriteSuffix
    push hl
    ld c,'.'
    call bios_printChar
    pop hl
    ld b,3
    call cliOverwritePart
cliOverwriteSuffix:
    ld hl,cliOverwriteKeys
    call bios_printString
cliOverwriteKey:
    call bios_getch
    cp 0Dh
    jp z,cliOverwriteNo
    cp 01Bh
    jp z,cliOverwriteNo
    and 0DFh
    cp 'N'
    jp z,cliOverwriteNo
    cp 'A'
    jp z,cliOverwriteAll
    cp 'S'
    jp z,cliOverwriteSkipAll
    cp 'Y'
    jp nz,cliOverwriteKey
    ld a,1
    jp cliOverwriteReturn
cliOverwriteAll:
    ld a,2
    jp cliOverwriteReturn
cliOverwriteSkipAll:
    ld a,3
    jp cliOverwriteReturn
cliOverwriteNo:
    xor a
cliOverwriteReturn:
    push af
    ld c,0Ah
    call bios_printChar
    pop af
    or a
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Напечатать поле короткого имени, пропуская пробелы.
;
; вход:
;   hl = начало поля, b = ненулевое число байтов.
;
; выход:
;   hl указывает сразу за полем, b = 0. Непробельные символы выведены через BIOS.
;----------------------------------------------------------------------------

cliOverwritePart:
    ld a,(hl)
    cp ' '
    jp z,cliOverwritePartNext
    push hl
    push bc
    ld c,a
    call bios_printChar
    pop bc
    pop hl
cliOverwritePartNext:
    inc hl
    dec b
    jp nz,cliOverwritePart
    ret
cliOverwriteText: DB 'OVERWRITE ',0
cliOverwriteKeys: DB '? YES/NO/ALL/SKIP ALL [N] ',0
