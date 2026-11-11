; Универсальный вход main: B=группа, C=функция, DE=параметр.
; Группа BDOS пока не поддерживается. Родные группы BIOS/DOS получают
; DE -> MX_REGS (HL, DE, BC, AF), все слова little-endian, пакет изменяемый.
; Результаты исполнителя записываются в пакет и возвращаются в регистрах.
; На неверной группе/номере: CF=1, A=DOS_ERR_UNSUPPORTED, пакет не трогаем.
; Контекст вызова хранится на стеке; сам диспетчер допускает вложенность.
; Реентерабельность файловой системы этим не обеспечивается.
sysCall:
    ld a,b
    or a
    jp z,sysUnsupported
    ; Восемь байт пакета не могут пересекать общее окно/регистры.
    ld a,d
    cp 0FFh
    jp nz,sysPacketReady
    ld a,e
    cp 0B9h
    jp nc,sysBadPacket
sysPacketReady:
    ; Для BIOS банкового маршрута нет; не вызываем отдельную проверку.
    ld a,b
    cp MXOS_GROUP_DOS
    jp nz,sysLocalCall
    ld a,(v_dosBanked)
    or a
    jp nz,sysBankCall
sysLocalCall:
    call sysFindTarget
    ret c
    push de
    ld bc,sysReturned
    push bc
    push hl
    jp sysLoadRegs
sysFindTarget:
    ld a,b
    cp MXOS_GROUP_BIOS
    jp z,sysBios
    cp MXOS_GROUP_DOS
    jp nz,sysUnsupported
    ld a,(v_dosModuleReady)
    or a
    jp z,sysUnsupported
    ld hl,sysDOSTable
    ld a,sysDOSCount
    jp sysSelect
sysBios:
    ld hl,sysBIOSTable
    ld a,sysBIOSCount
sysSelect:
    cp c
    jp z,sysUnsupported
    jp c,sysUnsupported
    ld b,0
    add hl,bc
    add hl,bc
    ld c,(hl)
    inc hl
    ld h,(hl)
    ld l,c
    or a
    ret
sysLoadRegs:
    ; На стеке: адрес исполнителя, продолжение, адрес пакета.
    ld hl,6
    add hl,de
    ld c,(hl)
    inc hl
    ld b,(hl)
    push bc
    dec hl
    dec hl
    ld b,(hl)
    dec hl
    ld c,(hl)
    dec hl
    ld d,(hl)
    dec hl
    ld e,(hl)
    dec hl
    ld a,(hl)
    dec hl
    ld l,(hl)
    ld h,a
    pop af
    ret
sysReturned:
    push af
    push bc
    push de
    push hl
    ld hl,8
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,0
    add hl,sp
    ; Фиксированные восемь байт: нет счётчика и перехода на каждый байт.
    REPT 7
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    ENDR
    ld a,(hl)
    ld (de),a
    pop hl
    pop de
    pop bc
    pop af
    inc sp
    inc sp
    ret
sysUnsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret
sysBadPacket:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret

    INCLUDE "syscall-banked.inc"
