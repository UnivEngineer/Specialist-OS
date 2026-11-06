; Подтверждение замены файла. Вход: v_copyRequest, назначение уже найдено.
; Выход A=0 пропустить / A=1 заменить, CF=отказ. Все регистры разрушены.
; Диалоги находятся в вызывающей программе, повторный вход в DOS запрещён.
copyConfirm:
    ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
    and 010h
    jp nz,copyConfirmAccess
    ld e,FILE_COPY_REQUEST.sourceDrive
    call fc_Address
    ld a,(hl)
    inc hl
    cp (hl)
    jp nz,copyConfirmHook
    ld e,FILE_COPY_REQUEST.sourceDirectory
    call fc_ReadWord
    push hl
    ld e,FILE_COPY_REQUEST.destinationDirectory
    call fc_ReadWord
    pop de
    call cmp_hl_de
    jp nz,copyConfirmHook
    ld e,FILE_COPY_REQUEST.source
    call fc_Address
    push hl
    ld e,FILE_COPY_REQUEST.destination
    call fc_Address
    ex de,hl
    pop hl
    ld c,11
    call stricmp
    jp z,copyConfirmAccess
copyConfirmHook:
    ld a,(copyAnswerAll)
    or a
    jp z,copyConfirmAsk
    cp 1
    ld a,0
    ret nz
    inc a
    ret
copyConfirmAsk:
    ld e,FILE_COPY_REQUEST.onExists
    call fc_ReadWord
    ld a,h
    or l
    jp z,copyConfirmDenied
    IFNDEF BANKED_DOS_BUILD
    ld bc,1
    call fileOpRange
    ret c
    ELSE
    ; В DOS-банке адрес уже локальный: внешний callback проверяет шлюз,
    ; внутренний обработчик вправе находиться в коде самого ядра.
    ENDIF
    ex de,hl
    ld hl,(v_copyRequest)
    call copyConfirmJump
    jp c,copyConfirmAccess
    cp 2
    jp c,copyConfirmAnswer
    cp 4
    jp nc,copyConfirmDenied
    sub 1
    ld (copyAnswerAll),a
    xor 3
    dec a
copyConfirmAnswer:
    or a
    ret
copyConfirmDenied:
    ld a,DOS_ERR_EXISTS
    jp dosSetError
copyConfirmAccess:
    ld a,DOS_ERR_ACCESS
    jp dosSetError
copyConfirmJump:
    push de
    ret
copySkipped: DB 0

copyAnswerAll: DB 0
