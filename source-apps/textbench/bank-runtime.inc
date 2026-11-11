; Нативный запуск BANK COM через FILE_EXEC_NATIVE; окно и стек обслуживает ОС.
bank_open:
bank_close:
    xor a
    ret
bankInvoke:
    ld hl,bankRegs
    ld de,bankArgument
    ld b,4
bankCommandDigit:
    ld a,h
    rrca
    rrca
    rrca
    rrca
    and 00Fh
    cp 10
    jp c,bankCommandNumber
    add a,7
bankCommandNumber:
    add a,'0'
    ld (de),a
    inc de
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    dec b
    jp nz,bankCommandDigit
    ld hl,bankPath
    MXOS_CALL MXOS_DOS_FILE_EXEC_NATIVE
    ret c
    ld a,(bankRegs+6)
    rra
    ret
    IFDEF FONT_CORPUS_TEST
    INCLUDE "../fontbench/module-runtime.inc"
    ELSE
; Один вызов на весь прогон, счётчик ведётся внутри модуля.
bank_run_native:
    ld a,(bankTextMode)
    ld (bankRegs+4),a
    ld hl,32
    ld (bankRegs+2),hl
    xor a
    ld (bankRegs+7),a
    call bankInvoke
    ret c
    call bankGetTime
    ret
bankTextMode: DB 0
    ENDIF
bankGetTime:
    ld hl,(bankRegs)
    ld (v_elapsed),hl
    ld hl,(bankRegs+2)
    ld (v_elapsed+2),hl
    ret
bench_copy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,bench_copy
    ret
bankHandle: DW 0
bankRegs: DW 0,0,0,0
    IFDEF FONT_CORPUS_TEST
    DW 0,bankImage
    ELSE
    DW 0,0
    ENDIF
bankPageCode: DB 0,0
bankPath:
    IFDEF FONT_CORPUS_TEST
    DB 'A:',05Ch,'BENCH',05Ch,'FONTBEN',05Ch,'FONTBANK.COM '
    ELSE
    DB 'A:',05Ch,'BENCH',05Ch,'FONTBEN',05Ch,'TEXTBANK.COM '
    ENDIF
bankArgument: DB '0000',0
    DS MAX_PATH-($-bankPath),0
bankErrorText: DB 'Cannot load/call benchmark module',13,10,0
