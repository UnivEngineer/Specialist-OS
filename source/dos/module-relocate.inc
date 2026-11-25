;----------------------------------------------------------------------------
; MXOS
; Проверить поля расширенного заголовка перед размещением.
; вход: v_imageHeader = 40 прочитанных байт v3. Регистровых параметров нет.
; выход: CF=0 либо CF=1/A=DOS_ERR_BAD_IMAGE. AF/BC/DE/HL разрушены.
;----------------------------------------------------------------------------
relocCheckHeader:
    ld a,(v_imageHeader+IMAGE_HEADER_BYTES)
    cp IMAGE_RELOC_HEADER
    jp nz,imageError
    ld hl,(v_imageHeader+IMAGE_MEMORY_BEGIN)
    MD_GET_DE v_imageHeader+IMAGE_RELOC_RESERVED
    ld a,h
    or l
    or d
    or e
    jp nz,imageError
    ld hl,(v_imageHeader+IMAGE_BSS)
    inc hl
    ld a,h
    or l
    jp z,imageError
    ld hl,(v_imageHeader+IMAGE_RELOC_MODE)
    ld a,h
    cp 16
    jp nc,imageError
    ld a,l
    cp IMAGE_RELOC_HIGH8
    jp c,relocCheckSum
    jp nz,imageError
    ld a,h
    cp 8
    jp c,imageError
relocCheckSum:
    ld hl,(v_imageHeader+IMAGE_HEADER_CHECKSUM)
    push hl
    ld hl,0
    ld (v_imageHeader+IMAGE_HEADER_CHECKSUM),hl
    ld hl,v_imageHeader
    ld de,v_imageHeader+IMAGE_RELOC_HEADER-1
    call calcCS
    pop hl
    ld (v_imageHeader+IMAGE_HEADER_CHECKSUM),hl
    ld d,b
    ld e,c
    call cmp_hl_de
    ret z
    jp imageError

;----------------------------------------------------------------------------
; MXOS
; Выбрать базу полного интервала v3. Для v2 прочитать фиксированные адреса.
; вход: после imageOpenModule, A=target; imagePlaceDos задаёт DOS. Для MAIN
; mdRecord/mdReuseHit уже выбраны по ключу файла. Непрерывный запас minHeap
; участвует в поиске и не входит в сохраняемый резерв.
; выход: CF=0, HL=вход, DE=загрузка, BC=байты тела. v_tmpFileDescr+6/+8 и
; v_imageHeader+IMAGE_MEMORY_BEGIN/END задают фактические адреса. CF=1/A=ошибка
; при нехватке места. Страницы, данные и живые записи модулей сохранены.
;----------------------------------------------------------------------------
imagePlaceDos:
    ld a,(v_imageHeader+IMAGE_OPTIONS)
    and IMAGE_OPTION_REUSABLE
    jp nz,imageError
    ld hl,(v_imageHeader+IMAGE_STACK)
    ld de,DOS_BANK_STACK_TOP-DOS_BANK_STACK_BOTTOM-64
    ex de,hl
    call cmp_hl_de
    ld a,MEM_ERROR_STACK
    jp c,dosSetError
    ld a,MODULE_TARGET_DOS
imagePlace:
    ld c,a
    ld a,(v_imageHeader+4)
    cp IMAGE_RELOC_VERSION
    jp nz,imageResult
    ld a,(v_imageHeader+IMAGE_RELOC_ALIGN)
    ld b,a
    ld hl,1
    or a
    jp z,relocMaskReady
relocMaskLoop:
    add hl,hl
    dec b
    jp nz,relocMaskLoop
relocMaskReady:
    dec hl
    push hl
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    MD_GET_DE v_imageHeader+IMAGE_MIN_HEAP
    add hl,de                    ; imageOpen проверил переполнение.
    ld a,c
    ld b,h
    ld c,l
    pop hl
    cp MODULE_TARGET_DOS
    jp z,relocPickDos
    or a
    jp z,relocPickMain
    ; EXEC целиком свободен до стека, кроме резидентного хвоста выше него.
    ld de,DOS_BANK_STACK_BOTTOM
    ex de,hl
    push de
    ld d,b
    ld e,c
    call sub_hl_de
    pop de
    jp c,relocNoSpace
    ld a,d
    cpl
    and h
    ld h,a
    ld a,e
    cpl
    and l
    ld l,a
    jp relocPlaced
relocNoSpace:
    ld a,MEM_ERROR_NOMEM
    jp dosSetError
relocPickDos:
    call relocDosHole
    jp relocPlacementStatus
relocPickMain:
    ld a,(mdReuseHit)
    or a
    jp z,relocNewMain
    push hl
    push bc
    ld hl,(mdRecord)
    ld de,10
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    add hl,de
    jp c,relocDifferentRange
    ld a,h
    cp b
    jp nz,relocDifferentRange
    ld a,l
    cp c
    jp nz,relocDifferentRange
    pop bc
    pop hl
    push hl
    push bc
    call relocMainProbeReuse
    pop bc
    pop de
    jp nc,relocPlaced
    ex de,hl
    jp relocColdMain
relocDifferentRange:
    pop bc
    pop hl
relocColdMain:
    xor a
    ld (mdReuseHit),a
relocNewMain:
    call relocMainHole
relocPlacementStatus:
    jp c,dosSetError
relocPlaced:
    ld (v_imageHeader+IMAGE_MEMORY_BEGIN),hl
    ex de,hl
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    add hl,de
    ld (v_imageHeader+IMAGE_MEMORY_END),hl
    ld hl,(v_imageHeader+6)
    add hl,de
    ld (v_tmpFileDescr+6),hl
    ld hl,(v_imageHeader+8)
    add hl,de
    ld (v_tmpFileDescr+8),hl
    jp imageResult

;----------------------------------------------------------------------------
; MXOS
; Зарезервировать выбранный участок DOS и высвободить требуемый непрерывный запас.
; вход: HL=база, DE=конец интервала. Для v3 imagePlaceDos уже проверил полную
; сумму с minHeap и границы арены. Вызов для встроенной службы использует
; собственный диапазон и обращается непосредственно к dosArenaImage.
; выход: CF/A как у dosArenaImage, BC/DE/HL сохранены. В таблице владелец
; публикует только интервал тела/данных, без временно проверенного minHeap.
;----------------------------------------------------------------------------
imageClaimDos:
    ld a,(v_imageHeader+4)
    cp IMAGE_RELOC_VERSION
    jp nz,dosArenaImage
    push de
    push hl
    ld hl,(v_imageHeader+IMAGE_MIN_HEAP)
    add hl,de
    ex de,hl
    pop hl
    call dosArenaImage
    pop de
    ret

;----------------------------------------------------------------------------
; MXOS
; Проверить и применить таблицу поправок после успешной проверки суммы тела.
; вход: курсор fatStream сразу за телом, выбранный приёмник локален для DOS
; либо kdFarActive=1 и kdPage задаёт страницу; заголовок уже размещён.
; выход: CF=0, HL=фактический вход; CF=1/A=ошибка FAT либо BAD_IMAGE.
; AF/BC/DE изменены, kdFarActive восстановлен. Тело при ошибке отбрасывают.
; Таблица читается по 128 байт в существующий KD_DATA и не попадает в модуль.
;----------------------------------------------------------------------------
relocReadTable:
    ld a,(kdFarActive)
    push af
    or a
    jp z,relocPatchPage
    ld a,(kdPage)
    cp DOS_IO_PAGE
    ld a,0
    jp z,relocPatchPage
    inc a
relocPatchPage:
    ld (relocRemote),a
    xor a
    ld (kdFarActive),a
    call relocStream
    pop bc
    push af
    ld a,b
    ld (kdFarActive),a
    pop af
    ret

;----------------------------------------------------------------------------
; MXOS
; Перенести таблицу из FAT небольшими порциями и проверить конец цепочки.
; вход: как relocReadTable; kdFarActive=0, relocRemote выбирает путь поправки.
; выход: CF/A и HL как у relocReadTable, BC/DE разрушены. Временные слова
; reloc* принадлежат нерекурсивной фазе чтения образа.
;----------------------------------------------------------------------------
relocStream:
    ld hl,0
    ld (relocSum),hl
    ld (relocNextOffset),hl
    ld hl,(v_imageHeader+IMAGE_RELOC_COUNT)
    add hl,hl
relocChunkStart:
    ld a,h
    or l
    jp z,relocTableDone
    ld bc,128
    ld a,h
    or a
    jp nz,relocChunkReady
    ld a,l
    cp c
    jp nc,relocChunkReady
    ld c,l
relocChunkReady:
    ld d,b
    ld e,c
    call sub_hl_de
    ld (relocLeft),hl
    MD_PUT_BC relocChunk
    ld hl,(v_tmpFileDescr+26)
    ex de,hl
    ld hl,KD_DATA
    ld a,2
    call fatStream
    ret c
    push hl
    call imageSaveCursor
    pop de
    dec de
    ld hl,(relocSum)
    ld b,h
    ld c,l
    ld hl,KD_DATA
    call calcCS_loop
    push af
    ld hl,(relocLeft)
    ld a,h
    or l
    jp z,relocFinalSum
    pop af
    ld a,(de)
    adc a,b                      ; Не последний байт всего файла.
    ld b,a
    jp relocSumReady
relocFinalSum:
    pop af
relocSumReady:
    MD_PUT_BC relocSum
    MD_GET_BC relocChunk
    ld hl,KD_DATA
relocEntryLoop:
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push hl
    push bc
    call relocPatch
    pop bc
    pop hl
    ret c
    dec c
    dec c
    jp nz,relocEntryLoop
    ld hl,(relocLeft)
    jp relocChunkStart
relocTableDone:
    ld hl,(v_tmpFileDescr+26)
    ex de,hl
    call fatStreamFinish
    ret c
    ld hl,(relocSum)
    MD_GET_DE v_imageHeader+IMAGE_RELOC_CHECKSUM
    call cmp_hl_de
    jp nz,imageError
    ld hl,(v_tmpFileDescr+8)
    or a
    ret

;----------------------------------------------------------------------------
; MXOS
; Проверить положение операнда и прибавить базу к WORD16 или её страницу к HIGH8.
; вход: DE=смещение от начала тела, relocNextOffset=конец предыдущего операнда.
; выход: CF=0 при успехе, CF=1/A=BAD_IMAGE при ошибке; регистры рабочие.
; Обмен между страницами охватывает только байты проверенного операнда.
;----------------------------------------------------------------------------
relocPatch:
    ld hl,(relocNextOffset)
    ex de,hl
    call cmp_hl_de
    jp c,imageError
    push hl
    ld a,(v_imageHeader+IMAGE_RELOC_MODE)
    xor 1
    inc a
    ld c,a                      ; WORD16 — 2, HIGH8 — 1.
    ld b,0
    add hl,bc
    jp c,relocBadOperand
    ld (relocNextOffset),hl
    MD_GET_DE v_tmpFileDescr+10
    ex de,hl
    call cmp_hl_de
    jp c,relocBadOperand
    pop hl
    MD_GET_DE v_tmpFileDescr+6
    add hl,de
    ld a,(relocRemote)
    or a
    jp z,relocAdd
    push hl
    push bc
    ld de,relocWord
    call kdFetch
    pop bc
    pop de
    jp c,imageError
    push de
    ld hl,relocWord
    call relocAdd
    pop de
    ld hl,relocWord
    jp kdStore
relocBadOperand:
    pop hl
    jp imageError

;----------------------------------------------------------------------------
; MXOS
; Прибавить выбранную базу к проверенному операнду по правилам 8080.
; вход: HL=операнд в текущей странице, C=1 (HIGH8) либо 2 (WORD16).
; выход: CF=0, BC сохранён, AF/DE/HL изменены; арифметика по модулю разрядности.
;----------------------------------------------------------------------------
relocAdd:
    MD_GET_DE v_imageHeader+IMAGE_MEMORY_BEGIN
    ld a,c
    cp 1
    jp z,relocHighOnly
    ld a,(hl)
    add a,e
    ld (hl),a
    inc hl
relocHighOnly:
    ld a,(hl)
    adc a,d
    ld (hl),a
    or a
    ret
