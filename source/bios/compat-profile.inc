;----------------------------------------------------------------------------
; MXOS
;
; Установить таблицу совместимости MAIN или запросить текущий профиль.
;
; вход:
;   A = BIOS_PROFILE_* или BIOS_PROFILE_QUERY. MAIN доступна в текущем профиле.
;
; выход:
;   CF=0, A = прежний профиль. CF=1, A = DOS_ERR_ARGUMENT при неверном коде.
;   BC/DE/HL сохранены. Остальные флаги неопределены.
;----------------------------------------------------------------------------

biosSetProfile:
    cp BIOS_PROFILE_QUERY
    jp z,biosQueryProfile
    cp BIOS_PROFILE_RAMFOS
    jp c,biosBadProfile
    cp BIOS_PROFILE_MON2+1
    jp nc,biosBadProfile
    push bc
    push de
    push hl
    ld b,a
    ld a,(biosProfile)
    push af
    ld a,b
    ld (biosProfile),a
    ld de,biosCompatVectors
    dec a
    jp z,biosUseRamfos
    ld hl,mon2Vectors
    ld b,(mon2VectorsEnd-mon2Vectors)/2
    call biosCompatBuild
    ld a,0C9h
    ld (0C821h),a
    ld (0C82Dh),a
    call biosCompatFillTail
    ld hl,biosStdPpiTargets
    ld a,low ioStd.PPI_KEY_MODE
    jp biosBindPpi
biosUseRamfos:
    ld hl,biosRamfosVectors
    ld b,(biosRamfosVectorsEnd-biosRamfosVectors)/2
    call biosCompatBuild
    ld hl,biosCompatVectors
    ld de,0F800h
    ld bc,36
    call moveBytesForward
    ld hl,biosCompatExit
    ld (0F801h),hl
    ld hl,biosRamfosHigh
    ld b,(biosRamfosHighEnd-biosRamfosHigh)/2
    call biosCompatBuild
    call ramfosCursorOut
    ld hl,biosMxPpiTargets
    ld a,low IO_KEYB_MODE
biosBindPpi:
    ld (setKeybMode82+3),a
    ld (setKeybMode91+3),a
    dec a
    dec a
    ld (biosKeyRows+1),a
    ld de,biosPpiVectors
    ld b,(biosPpiVectorsEnd-biosPpiVectors)/3
    call biosCompatBuild
biosProfileDone:
    pop af
    pop hl
    pop de
    pop bc
    or a
    ret
biosQueryProfile:
    ld a,(biosProfile)
    or a
    ret
biosBadProfile:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret

; DE указывает на свободный хвост профиля C800h.
biosCompatFillTail:
    ld a,0C9h
    ld (de),a
    inc de
    ld a,e
    cp low biosCompatVectorsEnd
    jp nz,biosCompatFillTail
    ret

; HL = список адресов, B = число переходов, DE = начало записи.
; На выходе DE/HL указывают за таблицы; AF/B изменены.
biosCompatBuild:
    ld a,0C3h
    ld (de),a
    inc de
    ld a,(hl)
    inc hl
    ld (de),a
    inc de
    ld a,(hl)
    inc hl
    ld (de),a
    inc de
    dec b
    jp nz,biosCompatBuild
    ret

biosProfile: DB BIOS_PROFILE_DEFAULT

; В C821h/C82Dh сохраняются исторические RET. Их операнды не исполняются.
mon2Vectors:
    DW reboot3, getch, t_tapeRead, printChar, t_tapeWrite, input
    DW keyScan2, printHexByte, printStringZ, keyScan2, getCursorPos, 0
    DW t_tapeLoad, t_tapeSave, calcCS, 0, setMemTop, getMemTop
mon2VectorsEnd:
    ASSERT (mon2VectorsEnd-mon2Vectors)*3/2 == 036h

biosPpiVectors:
biosBeep:       jp beep
keyScan2:       jp ioKeyScan2
biosKeyCtrl:    jp ioKeyReadCtrl
biosPpiVectorsEnd:

biosKeyRows:
    ld a,(IO_KEYB_B)
    ret
biosMxPpiTargets:
    DW beep, ioKeyScan2, ioKeyReadCtrl
biosStdPpiTargets:
    DW ioStd.beep, ioStd.ioKeyScan2, ioStd.ioKeyReadCtrl

