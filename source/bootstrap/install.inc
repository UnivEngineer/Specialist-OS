;----------------------------------------------------------------------------
; MXOS
; Установить сегмент до появления MEM в целевой странице.
; вход: A — физический банк, DE — источник, HL — назначение, BC — байты.
; На MX источник в MAIN. На MMX нижние 32 КиБ видны из выбранной страницы
; ROM (операнд installRomPage), верхние — из MAIN. Диапазоны не пересекают
; границу отображения, назначение ниже общего окна. Прерывания выключены.
; выход: записаны ровно BC байт, выбран MAIN, SP восстановлен. Нулевая
; длина допустима. AF/BC/DE/HL и содержимое общего окна не сохраняются.
;----------------------------------------------------------------------------
INSTALL_PACKET_BYTES = 16
install:
    ld (installBank+1),a
    ld a,c
    and INSTALL_PACKET_BYTES-1
    jp z,installBlocks
    ; Восстановить короткую процедуру записи после пакетного режима.
    push bc
    push de
    push hl
    ld hl,bootByteWriteImage
    ld de,bootByteEnd
    ld bc,BOOT_BYTE_WRITE
    call moveCore
    pop hl
    pop de
    pop bc
    push bc
    ld a,c
    and INSTALL_PACKET_BYTES-1
    ld b,a
installByte:
    IFNDEF ORIGINAL_MX
    ld a,(installRomPage+1)
    ld (0FFFEh),a
    ENDIF
    ld a,(de)
    ld c,a
    ld a,(installBank+1)
    call BOOT_BYTE_WRITE
    inc de
    inc hl
    dec b
    jp nz,installByte
    pop bc
    ld a,c
    and 256-INSTALL_PACKET_BYTES
    ld c,a
installBlocks:
    ld a,b
    or c
    ret z
    push hl
    push de
    push bc
    ld hl,packetWindow
    ld de,packetWindowEnd
    ld bc,BOOT_WINDOW
    call moveCore
    pop bc
    pop de
    pop hl
    ; Идём от конца к началу. После восьми PUSH значение SP уже равно
    ; исключающему концу назначения для следующего пакета.
    ld a,l
    ld (installLastLow+1),a
    ld a,h
    ld (installLastHigh+1),a
    ld a,e
    sub l
    ld e,a
    ld a,d
    sbc a,h
    ld d,a
    push hl
    ld hl,-INSTALL_PACKET_BYTES
    add hl,de
    ld (installSourceDelta+1),hl
    ld hl,2
    add hl,sp
    ld (installRestoreSP+1),hl
    pop hl
    add hl,bc
installPacket:
    ld (installDestSP+1),hl
installSourceDelta:
    ld de,0
    add hl,de
    ld sp,hl
    IFNDEF ORIGINAL_MX
installRomPage:
    ld a,0
    ld (0FFFEh),a
    ENDIF
    ; Пять младших слов становятся операндами LD HL,nn в общем окне.
    ; Три старших доходят до банка в BC, DE, HL.
    DUP 5, wordIndex
    pop hl
    ld (BOOT_WINDOW+packetWords-packetWindow+4*(4-wordIndex)+1),hl
    EDUP
    pop bc
    pop de
    pop hl
installDestSP:
    ld sp,0
installBank:
    ld a,0
    jp BOOT_WINDOW
installNext:
    ld hl,0
    add hl,sp
    ld a,l
installLastLow:
    cp 0
    jp nz,installPacket
    ld a,h
installLastHigh:
    cp 0
    jp nz,installPacket
installRestoreSP:
    ld sp,0
    ret

; Цикл на 16 байт: MMX — 464 такта, при проверке H — 486, MX — 444/466.
; Все 32 байта аппаратного окна исполняются на месте. Обратный порядок
; PUSH сохраняет порядок байтов, включая произвольно выровненные слова.
packetWindow:
    ld (0FFFDh),a
    push hl
    push de
    push bc
packetWords:
    DUP 5
    ld hl,0
    push hl
    EDUP
    ld (0FFFCh),a
    jp installNext
packetWindowEnd:
    ASSERT packetWindowEnd-packetWindow == 32
