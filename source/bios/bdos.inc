; Первый слой BDOS CP/M 2.2: консоль, диск и DMA. FCB будет подключён
; отдельным адаптером; неизвестный номер возвращает 0 по контракту CP/M.
; C=номер, DE/E=параметр; A=L, B=H при возврате. AF/BC/DE/HL разрушены.
sysBdos:
    ld a,c
    cp 1
    jp z,sysBdosInput
    cp 2
    jp z,sysBdosOutput
    cp 6
    jp z,sysBdosDirect
    cp 9
    jp z,sysBdosString
    cp 11
    jp z,sysBdosStatus
    cp 12
    jp z,sysBdosVersion
    cp 14
    jp z,sysBdosDrive
    cp 25
    jp z,sysBdosCurrentDrive
    cp 26
    jp z,sysBdosDma
    ; До адаптера FCB стандартные файловые номера отказывают явно.
    cp 15
    jp c,sysBdosZero
    cp 24
    jp c,sysBdosFail
    jp sysBdosZero
sysBdosInput:
    call getch
    push af
    ld c,a
    call printChar
    pop af
    jp sysBdosByte
sysBdosOutput:
    ld c,e
    call printChar
    jp sysBdosZero
sysBdosDirect:
    ld a,e
    inc a
    jp nz,sysBdosOutput
    call j_keyScan
    cp 0FFh
    jp nz,sysBdosByte
    jp sysBdosZero
sysBdosString:
    ld a,(de)
    cp '$'
    jp z,sysBdosZero
    push de
    ld c,a
    call printChar
    pop de
    inc de
    jp sysBdosString
sysBdosStatus:
    call j_keyScan
    inc a
    jp z,sysBdosZero
sysBdosFail:
    ld a,0FFh
    jp sysBdosByte
sysBdosVersion:
    ld a,022h
    jp sysBdosByte
sysBdosDrive:
    ld a,(v_dosModuleReady)
    or a
    jp z,sysBdosFail
    ld a,e
    MXCALL MX_DOS_FILESELECTDRIVE
    jp c,sysBdosFail
    jp sysBdosZero
sysBdosCurrentDrive:
    ld e,0
    MXCALL MX_DOS_FILEGETSETDRIVE
    jp c,sysBdosFail
    jp sysBdosByte
sysBdosDma:
    ex de,hl
    ld (sysDma),hl
sysBdosZero:
    xor a
sysBdosByte:
    ld l,a
    ld h,0
    ld b,h
    ret
sysDma: DW 0080h
