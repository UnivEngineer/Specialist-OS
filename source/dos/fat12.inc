; Вход HL = число кластеров данных. Выход CF=1 при слишком короткой FAT.
; При успехе обновлены fatNumCells и dataClusters. AF/BC/DE/HL рабочие.
; Проверяет ёмкость с учётом двух служебных ячеек и секторов по 512 байт.
f12Capacity:
    inc hl
    inc hl
    push hl
    ld hl,(v_diskInfo+DISK_INFO.fatNumCells)  ; Количество 16-битных ячеек.
    ld de,3
    call div_hl_de
    add hl,hl
    add hl,hl
    jp c,f12CapacityMax
    add hl,de
    jp nc,f12CapacityReady
f12CapacityMax:
    ld hl,0FFFFh
f12CapacityReady:
    ld (v_diskInfo+DISK_INFO.fatNumCells),hl
    pop de
    call cmp_hl_de
    ret c
    ; FF0h..FF6h зарезервированы, даже если BPB формально допускает их.
    ld hl,0FF0h
    call cmp_hl_de
    jp nc,f12CapacityDone
    dec hl
    dec hl
    ld (v_diskInfo+DISK_INFO.dataClusters),hl
f12CapacityDone:
    or a
    ret

; Прочитать упакованную ячейку. Вход DE = номер, выход DE = нормализованная ссылка.
; BC/HL сохранены. CF/A передают статус кэша.
f12Read:
    push hl
    push bc
    ld b,0
    call f12Cell
    pop bc
    pop hl
    ret

; Записать ссылку DE в ячейку HL. Выход HL = DE, BC/DE сохранены, CF/A — статус.
f12Write:
    push bc
    push de
    ex de,hl
    ld (f12New),hl
    ld b,1
    call f12Cell
    pop de
    pop bc
    ld h,d
    ld l,e
    ret

; Освободить ячейку DE. Выход DE = прежняя ссылка, при отказе DE сохраняется.
; BC/HL сохранены, CF/A — статус.
f12Free:
    push hl
    push bc
    push de
    ld hl,0
    ld (f12New),hl
    ld b,2
    call f12Cell
    jp nc,f12FreeDone
    pop de
    jp f12FreeReturn
f12FreeDone:
    pop hl
f12FreeReturn:
    pop bc
    pop hl
    ret

; Режим в B и номер ячейки в DE. Общий кэш зеркалирует изменённые
; сектора первой таблицы. Сначала получаем оба байта, затем меняем их.
f12Cell:
    ld a,b
    ld (f12Mode),a
    ld a,e
    and 1
    ld (f12Odd),a
    ld h,d
    ld l,e
    call div_hl_2
    add hl,de
    ld a,9
    call splitSectorOffset
    push de
    ex de,hl
    ld hl,(v_diskInfo+DISK_INFO.resSectors)
    add hl,de
    ex de,hl
    xor a
    call getSectorAddressInCache
    pop bc                    ; Смещение внутри сектора.
    ret c
    add hl,bc
    ld (f12First),hl
    push hl
    ld hl,(v_cacheLastSlot)
    ld (f12Slot),hl
    pop hl
    inc hl
    inc bc
    ld a,b
    cp 2
    jp nz,f12PairReady

    ; Первая строка выровнена на 512 байт. CLOCK пропускает её при
    ; поиске жертвы для второй строки. Снимаем закрепление при любом CF.
    push de
    ld hl,(f12Slot)
    ld de,FAT_CACHE_DATA.reference
    add hl,de
    ld (hl),080h
    ex (sp),hl
    ex de,hl
    inc de
    xor a
    call getSectorAddressInCache
    ex (sp),hl
    ld (hl),1
    pop hl
    ret c
f12PairReady:
    ld (f12Second),hl
    ld d,(hl)
    ld hl,(f12First)
    ld e,(hl)
    call f12Decode
    ld a,(f12Mode)
    or a
    ret z

    push de
    ld b,1
    ld hl,(v_cacheLastSlot)
    call cacheTouchSlot
    ld hl,(f12Slot)
    call cacheTouchSlot
    ld hl,(f12New)
    ex de,hl
    ld a,(f12Odd)
    or a
    jp nz,f12StoreOdd
    ld hl,(f12First)
    ld (hl),e
    ld hl,(f12Second)
    ld a,(hl)
    xor d
    and 0Fh
    xor (hl)
    ld (hl),a
    jp f12StoreDone
f12StoreOdd:
    ld hl,(f12First)
    ld a,e
    call f12SwapNibbles
    ld e,a
    xor (hl)
    and 0F0h
    xor (hl)
    ld (hl),a
    ld hl,(f12Second)
    ld a,d
    call f12SwapNibbles
    and 0F0h
    ld d,a
    ld a,e
    and 0Fh
    or d
    ld (hl),a
f12StoreDone:
    pop de
    or a
    ret

; Привести специальные значения FFxh к FFFxh, общим для всего ядра.
f12Decode:
    ld a,(f12Odd)
    or a
    jp z,f12DecodeEven
    ld a,e
    call f12SwapNibbles
    and 0Fh
    ld e,a
    ld a,d
    call f12SwapNibbles
    ld d,a
    and 0F0h
    or e
    ld e,a
f12DecodeEven:
    ld a,d
    and 0Fh
    ld d,a
    cp 0Fh
    ret nz
    ld a,e
    cp 0F0h
    ret c
    ld d,0FFh
    ret

f12SwapNibbles:
    rrca
    rrca
    rrca
    rrca
    ret

; Подсчитать свободные кластеры текущего тома. Выход DE = количество при CF=0.
; При CF=1 A содержит ошибку кэша. HL/BC сохраняются.
f12Count:
    push hl
    push bc
    ld hl,(v_diskInfo+DISK_INFO.dataClusters)
    ld b,h
    ld c,l
    ld hl,0
    ld de,2
f12CountLoop:
    ld a,b
    or c
    jp z,f12CountDone
    push de
    call f12Read
    jp c,f12CountError
    ld a,d
    or e
    pop de
    jp nz,f12CountNext
    inc hl
f12CountNext:
    inc de
    dec bc
    jp f12CountLoop
f12CountDone:
    ex de,hl
    jp f12CountReturn
f12CountError:
    pop de
f12CountReturn:
    pop bc
    pop hl
    ret
