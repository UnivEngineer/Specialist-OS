; Прямой обмен полным физическим блоком в странице клиента.
; HL=NEAR-буфер, DE=блок, B=1/2. BC/DE/HL сохранены, CF/A=ошибка.
; Слот H: лениво заполняется общей реализацией драйвера; FAT остаётся в DOS.
klBlock:
    push bc
    push de
    push hl
    ld (klRegs),hl
    ex de,hl
    ld (klRegs+2),hl
    ld a,(v_drive)
    ld c,a
    ld h,b
    ld l,c
    ld (klRegs+4),hl
    cp 1
    jp nz,klNotRam
    ld a,(klRegs+2)
    cp 0FFh
    jp z,klBad
    ld a,(bankPageCount)
    dec a
    ld hl,klRegs+3
    cp (hl)
    jp c,klBad
    jp z,klBad
klNotRam:
    ld a,c
    cp 7
    jp nz,klInvoke
    call klInstall
    jp c,klDone
klInvoke:
    xor a
    ld hl,bankCallback
    ld bc,LOCAL_BLOCK_ENTRY
    ld de,klRegs
    call bankCallPacket
klDone:
    pop hl
    pop de
    pop bc
    ret
klBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    jp klDone

; Разрешены только штатные A/B/H, main и 256-байтные блоки.
; Выход Z=прямой маршрут доступен. AF разрушен.
klEligible:
    ld a,(kdPage)
    or a
    ret nz
    ld a,(v_diskInfo.blockShift)
    cp 8
    ret nz
    ld a,(v_drive)
    cp 1
    ret z
    or a
    ret z
    cp 7
    ret nz
    ld a,(DOS_DRIVER_REGISTRY+7*DRIVER_RECORD_SIZE+1)
    or a
    jp nz,klEligibleH
    inc a
    ret
klEligibleH:
    xor a
    ret

klInstall:
    ; Незавершённая запись DOS должна завершиться до работы другой копии.
    ld a,(kernelHDriverKind)
    cp 2
    jp nz,klReady
    ld a,(klFastOwner)
    or a
    jp nz,klReady
    ld b,4
    call LOCAL_DRIVER_ADDR
    ret c
    ld a,1
    ld (klFastOwner),a
klReady:
    ld a,(kernelHDriverKind)
    ld hl,klInstalled
    cp (hl)
    ret z
    ld hl,(registryImageSize)
    ld b,h
    ld c,l
    ld hl,DOS_DRIVER_IMAGE
klCopyImage:
    ld de,LOCAL_DRIVER_ADDR
    call kdStore
    ret c
    ld hl,kernelHDriverKind
    ld de,LOCAL_DRIVER_KIND
    ld bc,1
    call kdStore
    ret c
    ld a,(kernelHDriverKind)
    ld (klInstalled),a
    or a
    ret
klInstalled: DB 0FFh
klRegs: DS MX_REGS,0

; Копии FLASHFST владеют конвейером по очереди. Ожидание — только при
; реальном переходе аппаратного обмена между DOS и main, не между блоками.
; Здесь только секторный обмен/flush; B=3/FFh обслуживает реестр DOS.
fastDriver:
    ld a,(kernelHDriverKind)
    cp 2
    jp nz,LOCAL_DRIVER_ADDR
    ld a,(klFastOwner)
    or a
    jp z,LOCAL_DRIVER_ADDR
    push bc
    push de
    push hl
    xor a
    ld bc,LOCAL_BLOCK_ENTRY
    ld hl,bankCallback
    ld de,klFlushRegs
    call bankCallPacket
    pop hl
    pop de
    pop bc
    ret c
    xor a
    ld (klFastOwner),a
    jp LOCAL_DRIVER_ADDR
klFastOwner: DB 0
klFlushRegs: DW 0,0,0407h,0

; Прямая серия RAW не требует перехода main -> DOS на каждом блоке.
; Пакет результата уже сохранён в kdRegs; AF/BC/DE/HL разрушены.
klGrant:
    ld a,(kdRegs+6)
    and 1
    ret nz
    ld a,(v_drive)
    push af
    call registryRecord
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (klLease+1),hl
    pop af
    inc a
    ld (klLease),a
    ld hl,klLease
    ld de,MAIN_RAW_LEASE
    ld bc,3
    jp kdStore
klLease: DS 3,0
