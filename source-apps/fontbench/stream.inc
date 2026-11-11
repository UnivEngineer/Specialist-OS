; Общий ASCIIZ-фронтенд standalone. Подмена действует только внутри этапа.
; MXOS_CALL использует sysBIOSTable, банковый адаптер — также старые JMP-входы.
streamInstall:
    xor a
    ld (streamReal),a
    ld hl,(output+1)
    ld de,kernelReal
    call bios_cmp_hl_de
    jp nz,streamSaveEntries
    ld a,1
    ld (streamReal),a
streamSaveEntries:
    ld hl,(bios_printChar+1)
    ld (streamOriginalChar+1),hl
    ld hl,(bios_printString+1)
    ld (streamOriginalString+1),hl
    ld hl,(matrixCase)
    call benchIsKernel
    jp z,streamInstallChar
    ld hl,(output+1)
    ld de,kernelRealString
    call bios_cmp_hl_de
    ret nz
    ld hl,kernelRealBatch
    ld (serialOutput+1),hl
    jp streamInstallString
streamInstallChar:
    ld hl,streamChar
    ld (bios_printChar+1),hl
    ld (BASE_BIOS_TABLE+2*(MXOS_BIOS_PRINT_CHAR-0100h)),hl
streamInstallString:
    ld hl,streamString
    ld (bios_printString+1),hl
    ld (BASE_BIOS_TABLE+2*(MXOS_BIOS_PRINT_STRING_BANK-0100h)),hl
    ld (BASE_BIOS_TABLE+2*(MXOS_BIOS_PRINT_STRING_MAIN-0100h)),hl
    ret
streamRestore:
    ld hl,(streamOriginalChar+1)
    ld (bios_printChar+1),hl
    ld (BASE_BIOS_TABLE+2*(MXOS_BIOS_PRINT_CHAR-0100h)),hl
    ld hl,(streamOriginalString+1)
    ld (bios_printString+1),hl
    ld (BASE_BIOS_TABLE+2*(MXOS_BIOS_PRINT_STRING_BANK-0100h)),hl
    ld (BASE_BIOS_TABLE+2*(MXOS_BIOS_PRINT_STRING_MAIN-0100h)),hl
    ret
streamOriginalChar:
    jp bios_printChar
streamOriginalString:
    jp bios_printString
; Управляющие коды всегда обрабатывает исходный BIOS.
streamChar:
    push af
    ld a,c
    cp 020h
    jp c,streamControlChar
    pop af
output:
    jp monitor.scalar
streamControlChar:
    pop af
    jp streamOriginalChar
; AF/BC/DE сохранены, HL возвращается на нуле, как у bios_printString.
streamString:
    push af
    push bc
    push de
streamNext:
    ld a,(hl)
    or a
    jp z,streamDone
    cp 020h
    jp c,streamControl
    ld a,(matrixKind)
    or a
    jp z,streamOne
    cp 3
    jp z,streamOne
    cp 5
    jp c,streamBatch
    and 1
    jp nz,streamOne
streamBatch:
    ld a,(matrixKind)
    cp 3
    jp nc,streamScan
    ; 6x8: голова до границы байта; затем только полные четвёрки.
    ld a,(bios_vars.cursorX)
    and 3
    jp nz,streamTail
streamScan:
    push hl
    ld c,0
streamScanByte:
    ld a,(hl)
    cp 020h
    jp c,streamScanned
    inc hl
    inc c
    ld a,c
    cp 48
    jp nz,streamScanByte
streamScanned:
    pop hl
    ld a,(matrixKind)
    cp 3
    jp nc,streamCountReady
    ld a,c
    and 0FCh
    ld c,a
    jp z,streamTail
streamCountReady:
    push hl
    push bc
serialOutput:
quadOutput:
    call serialBIOS
    pop bc
    pop hl
    ld b,0
    add hl,bc
    jp streamNext
streamTail:
    ld c,(hl)
    push hl
    ld a,(matrixKind)
    cp 3
    jp nc,streamTailEight
    call streamOriginalChar
    jp streamAdvance
streamTailEight:
    call aligned8Scalar
    jp streamAdvance
streamControl:
    ld c,a
    push hl
    call streamOriginalChar
    jp streamAdvance
streamOne:
    ld c,(hl)
    push hl
    call streamChar
streamAdvance:
    pop hl
    inc hl
    jp streamNext
streamDone:
    pop de
    pop bc
    pop af
    ret
; 1152 печатных байта плюс CRLF после каждой строки и общий нуль.
streamPrepare:
    ld hl,textCorpus
    ld de,bankImage
    ld b,FONT_ROWS
streamPrepareRow:
    ld c,FONT_COLUMNS
streamPrepareByte:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec c
    jp nz,streamPrepareByte
    ld a,13
    ld (de),a
    inc de
    ld a,10
    ld (de),a
    inc de
    dec b
    jp nz,streamPrepareRow
    xor a
    ld (de),a
    ld hl,7
    ld (bios_vars.cursorY),hl
    ret

streamReal: DB 0
