;----------------------------------------------------------------------------
; Переход в RAMFOS и возврат к загрузке MXOS из системного ПЗУ.
; Вход: HL = адрес программы, DE = аргументы. Регистры и флаги разрушаются.
; Стек RAMFOS находится ниже D300h и не пересекается с системной программой.
;----------------------------------------------------------------------------
compatStart:
    di
    ld sp, STACK_TOP
    push hl
    push de
    call biosInit
    ld hl, RAM_TOP
    call setMemTop
    pop de
    pop hl
    ld bc, compatReturn
    push bc
    jp (hl)

; После замены ядра адресов MXOS больше нет. Возврат выполняет загрузчик ПЗУ.
; Только шесть байтов в общем окне остаются видимыми при переключении банка.
compatReturn:
    di
    ld hl, compatRomStub
    ld de, 0FFC0h
    ld b, compatRomStubEnd-compatRomStub
compatReturnCopy:
    ld a, (hl)
    ld (de), a
    inc hl
    inc de
    dec b
    jp nz, compatReturnCopy
    jp 0FFC0h
compatRomStub:
    ld (IO_PAGE_ROM), a
    jp 0
compatRomStubEnd:

; Первая стадия: собственная файловая система RAMFOS не получает доступ к FAT.
; Диалоги: Z=1 означает отмену, CF=0. Файловые операции: NZ=1 и CF=1.
; Значения A=1/C=FFh также дают определённый результат запрещённого чтения.
compatReadFailure:
    ld c, 0FFh
compatFailure:
    ld a, 1
    or a
    scf
    ret
compatCancel:
    xor a
    ret
