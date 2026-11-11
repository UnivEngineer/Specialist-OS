; Только публичные API. Новые системные движки добавляются в эту группу.
kernelCoreStart:
kernelChar:
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    ret
kernelReal:
    push af
    push bc
    push de
    push hl
    ld a,c
    MXOS_CALL MXOS_BIOS_PRINT_CHAR_REAL
    pop hl
    pop de
    pop bc
    pop af
    ret
; Эти адреса также служат тегами CASE; общий цикл делает один MXOS_CALL на поток.
kernelString:
    jp bios_printString
kernelRealString:
    jp bios_printStringReal
kernelCoreEnd:
; HL=описатель CASE. A=1 для API, A=0 для standalone, ZF; BC/DE/HL сохранены.
benchIsKernel:
    push hl
    push de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld a,l
    sub kernelCoreStart & 0FFh
    ld a,h
    sbc a,kernelCoreStart / 256
    jp c,kernelNo
    ld a,l
    sub kernelCoreEnd & 0FFh
    ld a,h
    sbc a,kernelCoreEnd / 256
    jp nc,kernelNo
    ld a,1
    jp kernelClassDone
kernelNo:
    xor a
kernelClassDone:
    pop de
    pop hl
    or a
    ret
kernelPrepare:
    ret
kernelRealBatch:
    push hl
    push bc
    ld b,0
    add hl,bc
    ld a,(hl)
    push af
    ld (hl),0
    pop af
    pop bc
    pop hl
    push af
    push hl
    MXOS_CALL MXOS_BIOS_PRINT_STRING_REAL_MAIN
    pop hl
    push hl
    ld b,0
    add hl,bc
    pop de
    pop af
    ld (hl),a
    ex de,hl
    ret
labelApiChar: DB 'API BIOS chr:',0
apiCharMain: DD 0
apiCharBank: DD 0
labelApiReal: DB 'API real chr:',0
apiRealMain: DD 0
apiRealBank: DD 0
labelApiString: DB 'API Win20 :  ',0
apiStringMain: DD 0
apiStringBank: DD 0
labelApiFar48: DB 'API Far48:   ',0
apiFar48Main: DD 0
apiFar48Bank: DD 0
labelApiBulk: DB 'API Far1152: ',0
apiBulkMain: DD 0
apiBulkBank: DD 0
