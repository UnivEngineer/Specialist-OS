; Универсальный вход main: B=группа, C=функция, DE=параметр.
; BDOS использует исторические DE/E. Родные группы BIOS/DOS получают
; DE -> MX_REGS (HL, DE, BC, AF), все слова little-endian, пакет изменяемый.
; Результаты исполнителя записываются в пакет и возвращаются в регистрах.
; На неверной группе/номере: CF=1, A=DOS_ERR_UNSUPPORTED, пакет не трогаем.
; Контекст вызова хранится на стеке; сам диспетчер допускает вложенность.
; Реентерабельность файловой системы этим не обеспечивается.
sysCall:
    ld a,b
    or a
    jp z,sysBdos
    ; Восемь байт пакета не могут пересекать общее окно/регистры.
    ld a,d
    cp 0FFh
    jp nz,sysPacketReady
    ld a,e
    cp 0B9h
    jp nc,sysBadPacket
sysPacketReady:
    call sysBankSelected
    jp nz,sysBankCall
    call sysFindTarget
    ret c
    push de
    ld bc,sysReturned
    push bc
    push hl
    jp sysLoadRegs
sysFindTarget:
    ld a,b
    cp MX_GROUP_BIOS
    jp z,sysBios
    cp MX_GROUP_DOS
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
    ld b,8
sysSaveResult:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,sysSaveResult
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

; Именованный регистровый вызов: CALL mx_callRegs / DW группа*256+номер.
; Сохраняет входные регистры/флаги в локальном пакете; возвращает результаты
; исполнителя. Код после DW исполняется обычным RET, не как инструкция.
sysCallRegs:
    push af
    push bc
    push de
    push hl
    ld hl,8
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ex de,hl
    ld (hl),d
    dec hl
    ld (hl),e
    ld hl,0
    add hl,sp
    ex de,hl
    call sysBankSelected
    jp z,sysRegsNative
    call sysBankCall
    jp sysRegsReturned
sysRegsNative:
    call sysFindTarget
    jp c,sysRegsError
    ld bc,sysRegsReturned
    push bc
    push hl
    jp sysLoadRegs
sysRegsReturned:
    ; Результат уже в регистрах. Локальный пакет больше не нужен:
    ; восемь INC SP не меняют ни один регистр и ни один флаг.
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    ret
sysRegsError:
    ; Ошибка диспетчеризации не меняет пакет: переносим её AF отдельно.
    push af
    pop de
    ld hl,6
    add hl,sp
    ld (hl),e
    inc hl
    ld (hl),d
    pop hl
    pop de
    pop bc
    pop af
    ret

sysCpmEntry:
    ld b,0
    jp sysCall
sysLoadPath:
    MXCALL MX_DOS_FILELOADPATH
    ret
sysSavePath:
    MXCALL MX_DOS_FILESAVEPATH
    ret

    INCLUDE "syscall-tables.inc"
    INCLUDE "syscall-banked.inc"
