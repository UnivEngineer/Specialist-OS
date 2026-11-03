; Полный цикл: DOS WRITE -> DOS READ -> Raw WRITE -> Raw READ -> удаление.
; Данные и размещение общие. Печать и разбор цепочки вне замеров.
; Сравнение входит в WRITE только в режиме полной проверки файла.
; Все процедуры ниже разрушают AF/BC/DE/HL, если не указано обратное.
file_write_case:
    ld hl,(v_sectorSize)
    dec h
    ld a,h
    or l
    jp nz,error
    ld a,(v_spc)
    dec a
    jp nz,error
    ld hl,(v_capacity)
    ld de,8193
    call bios_cmp_hl_de
    jp nc,error
    ld hl,tmpDescriptor
    call bios_fileLoadInfo
    jp nc,file_exists
    ld a,(v_budget)
    ld l,a
    ld h,0
    ld a,(v_fileSectors)
    ld e,a
    ld d,0
    call bios_mul_hl_de
    ld (v_operations),hl
    xor a
    ld (v_bufferMode),a
    ld hl,statAll
    ld b,40
    call clear_bytes
    ld hl,resultsHigh
    ld b,48                    ; HIGH-статистика и два счётчика VERIFY.
    call clear_bytes
    inc a
    ld (v_pass),a
file_buffer:
file_pass:
    call print_progress
    xor a
    call fill_payload
    ld a,1
    ld (v_fileOwned),a
    call measure_begin
    ld hl,tmpDescriptor
    ld de,sectorBuffer
    call bios_fileCreate
    call c,measure_end
    jp c,error
    call maybe_verify_file
    call measure_end
    jp c,error
    ld de,statFileWrite
    call add_stats
    ld hl,v_verifyTicks
    ld de,statFileVerify
    ld b,4
    call add_bytes
    ; Меняем буфер ДО замера: чтение обязано действительно вернуть данные.
    ld a,0FFh
    call fill_payload
    ; Тот же файл читается до удаления. Поиск имени и проход FAT входят.
    call measure_begin
    ld hl,tmpDescriptor
    ld de,verifyBuffer
    call bios_fileLoad2
    call measure_end
    jp c,error
    ld de,statFileRead
    call add_stats
    call file_compare
    jp c,error
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    jp nz,file_read_high_done
    ld b,0
    call read_high
    jp c,error
    ld de,resultsHigh+30
    call add_stats
file_read_high_done:
    call raw_build_chain
    jp c,error
    ld b,1
    call raw_transfer
    jp c,error
    ld de,statRawWrite
    call add_stats
    ld hl,v_verifyTicks
    ld de,statRawVerify
    ld b,4
    call add_bytes
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    jp nz,raw_write_high_done
    call write_high
    jp c,error
    ld de,resultsHigh
    call add_stats
raw_write_high_done:
    ld a,0FFh
    call fill_payload
    ld b,2
    call raw_transfer
    jp c,error
    ld de,statRawRead
    call add_stats
    call file_compare
    jp c,error
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    jp nz,raw_read_high_done
    ld b,1
    call read_high
    jp c,error
    ld de,resultsHigh+10
    call add_stats
raw_read_high_done:
    call file_cleanup
    jp c,error
    ld hl,txtPassDone
    call puts
    ld hl,v_pass
    inc (hl)
    ld a,(v_budget)
    cp (hl)
    jp nc,file_pass
file_results:
    ld hl,txtRawAccess
    call puts
    ld hl,resultsLow
    call print_group
    ld hl,txtFileAccess
    call puts
    ld hl,resultsLow+20
    call print_group
    jp done
file_exists:
    ld hl,txtFileExists
    jp show_error

; Счётчик печатается до начала прохода, чтобы не менять измеряемое время.
print_progress:
    ld hl,txtPass
    call puts
    ld a,(v_pass)
    add a,'0'
    ld c,a
    call bios_printChar
    ld c,'/'
    call bios_printChar
    ld a,(v_budget)
    add a,'0'
    ld c,a
    call bios_printChar
    ret

; A=0 — шаблон записи, A=FFh — противоположные байты перед чтением.
; AF/BC/HL разрушены. Вызов всегда вне измеряемого интервала.
fill_payload:
    ld c,a
    ld hl,sectorBuffer
file_fill:
    ld a,(v_pass)
    xor h
    xor l
    xor c
    ld (hl),a
    inc hl
    ld a,(v_bufferEndHi)
    cp h
    jp nz,file_fill
    ret

; CF=ошибка. Сверяем прочитанный файл с вычисляемым шаблоном;
; второй полный буфер не требуется, поэтому доступны файлы до 24 КиБ.
file_compare:
    ld hl,sectorBuffer
file_compare_loop:
    ld a,(v_pass)
    xor h
    xor l
    cp (hl)
    scf
    ret nz
    inc hl
    ld a,(v_bufferEndHi)
    cp h
    jp nz,file_compare_loop
    or a
    ret

; Замер одного файлового вызова либо физического пакета. Код обёртки
; дополняет U32 между обращениями; каждый промежуток короче оборота ВИ53.
measure_begin:
    xor a
    ld hl,v_verifyTicks
    ld b,4
    call clear_bytes
    ld hl,v_flushTicks
    ld b,6
measure_clear:
    ld (hl),a
    inc hl
    dec b
    jp nz,measure_clear
    ld hl,(bios_diskDriver+1)
    ld (file_original+1),hl
    ld hl,file_driver
    ld (bios_diskDriver+1),hl
    jp reset_timing
clear_bytes:
    ld (hl),a
    inc hl
    dec b
    jp nz,clear_bytes
    ret

; CF исходного вызова и BC/DE/HL сохраняются.
measure_end:
    call timer_sample
file_restore_driver:
    push hl
    ld hl,(file_original+1)
    ld (bios_diskDriver+1),hl
    pop hl
    ret

; DE=приёмник записи статистики (тики U32, flush U32, число flush U16).
add_stats:
    ld hl,v_elapsed
    ld b,4
    call add_bytes
    ld hl,v_flushTicks
    ld b,4
    call add_bytes
    ld b,2
add_bytes:
    or a
add_bytes_loop:
    ld a,(de)
    adc a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,add_bytes_loop
    ret

; HL=начало статистики группы в resultsLow. Печать READ, WRITE, FLUSH.
; Для A: сначала LOW/HIGH одной операции, затем следующая операция.
; Вход: HL=resultsLow или resultsLow+20. Все регистры разрушены.
print_group:
    ld (v_outputBase),hl
    ld a,1                    ; READ
    ld (v_outputMode),a
    call print_group_operation
    xor a                     ; WRITE
    ld (v_outputMode),a
    call print_group_operation
    ld a,(v_writeMode)
    cp 6
    call z,print_verify
    ld a,(v_caps)
    and DISK_CAP_FLUSH
    ret z
    ld a,2                    ; FLUSH
    ld (v_outputMode),a
print_group_operation:
    ld hl,(v_outputBase)
    ld a,(v_outputMode)
    cp 1
    jp nz,group_low
    ld de,10
    add hl,de
group_low:
    push hl
    call print_single
    pop hl
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    ret nz
    ld a,(v_outputMode)
    cp 1
    jp z,group_high
    or a
    ret nz
    ; Верхний источник измеряется отдельно только для Raw WRITE.
    ld a,h
    cp resultsLow / 256
    ret nz
    push hl
    ld de,resultsLow
    call bios_cmp_hl_de
    pop hl
    ret nz
group_high:
    ld de,resultsHigh-resultsLow
    add hl,de
    ; Продолжаем с той же операцией для верхнего буфера.
    jp print_single

; HL=одна запись статистики; подпись определяется операцией и буфером.
print_single:
    ld a,(v_outputMode)
    cp 1
    jp z,single_read
    ld de,txtWriteRow
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    jp nz,print_stat
    push hl
    ld de,resultsHigh
    call bios_cmp_hl_de
    pop hl
    ld de,txtWriteLow
    jp c,print_stat
    ld de,txtWriteHigh
    jp print_stat
single_read:
    ld de,txtRead
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    jp nz,print_stat
    push hl
    ld de,resultsHigh
    call bios_cmp_hl_de
    pop hl
    ld de,txtReadLow
    jp c,print_stat
    ld de,txtReadHigh
    jp print_stat

; HL=статистика, DE=подпись. Счётчики копируем для общего форматтера скорости.
print_stat:
    push de
    ld de,v_elapsed
    ld b,4
    call copy_bytes
    ld de,v_flushTicks
    ld b,6
    call copy_bytes
    pop hl
    ld a,(v_outputMode)
    cp 2
    jp z,print_flush
    call puts
    jp print_result_value
print_flush:
    ld hl,txtFlush
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    jp nz,print_flush_label
    ld hl,txtFlushMapped
print_flush_label:
    call puts
    ld hl,v_flushTicks
    call ticks_to_ms
    call print_number
    ld hl,txtRate
    call puts
    ld hl,(v_flushCount)
    ex de,hl
    ld b,0
    call bios_printDecWord
    ld hl,txtTimes
    jp puts
copy_bytes:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,copy_bytes
    ret

; Список физических секторов строим по цепочке DOS до замера.
; Текущие тома <= 8192 секторов, кластер = сектор = 256 байт.
raw_build_chain:
    ld hl,(tmpDescriptor+FILE_DESCRIPTOR.firstCluster)
    ld (v_rawCluster),hl
    ld hl,rawSectors
    ld (v_rawPtr),hl
    ld a,(v_fileSectors)
    ld (v_rawLeft),a
raw_chain:
    ld hl,(v_rawCluster)
    dec hl
    dec hl
    LOAD_DE v_dataStart
    add hl,de
    ex de,hl
    ld hl,(v_rawPtr)
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (v_rawPtr),hl
    ld a,(v_rawLeft)
    dec a
    ld (v_rawLeft),a
    ret z
    ld hl,(v_rawCluster)
    add hl,hl
    ld a,l
    ld (v_rawOffset),a
    ld e,h
    ld d,0
    ld hl,(v_reserved)
    add hl,de
    ex de,hl
    ld hl,fatBuffer
    ld b,2
    call bios_diskDriver
    ret c
    ld a,(v_rawOffset)
    ld l,a
    ld h,fatBuffer / 256
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (v_rawCluster),hl
    jp raw_chain

; B=1 запись / B=2 чтение. Один общий цикл, один flush после пакета записи.
raw_transfer:
    ld a,b
    ld (v_rawOp),a
    ld hl,sectorBuffer
    dec a
    jp z,raw_buffer_ready
    ld hl,verifyBuffer
raw_buffer_ready:
    ld (v_rawBuffer),hl
    ld hl,rawSectors
    ld (v_rawPtr),hl
    ld a,(v_fileSectors)
    ld (v_rawLeft),a
    call measure_begin
raw_io:
    ld hl,(v_rawPtr)
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld (v_rawPtr),hl
    ld hl,(v_rawBuffer)
    ld a,(v_rawOp)
    ld b,a
    call bios_diskDriver
    jp c,measure_end
    ld hl,(v_rawBuffer)
    inc h
    ld (v_rawBuffer),hl
    ld hl,v_rawLeft
    dec (hl)
    jp nz,raw_io
    ld a,(v_rawOp)
    dec a
    jp nz,raw_success
    ld a,(v_caps)
    and DISK_CAP_FLUSH
    jp z,raw_success
    ld b,4
    call bios_diskDriver
    jp raw_success_check
raw_success:
    or a
raw_success_check:
    call c,measure_end
    ret c
    ld a,(v_rawOp)
    dec a
    jp nz,measure_end
    call maybe_verify_raw
    jp measure_end

; Обёртка замеров и посекторного B=5. Верхние чтения идут напрямую
; в 8200h через stream_read, без обратного копирования в нижнюю память.
file_driver:
    call timer_sample
    push af
    ld a,b
    ld (v_driverOp),a
    cp 1
    jp nz,file_driver_ready
    ld a,(v_writeMode)
    cp 5
    jp nz,file_driver_ready
    ld b,5
file_driver_ready:
    pop af
    push af
    push hl
    call timer_read
    ld (v_flushStart),hl
    pop hl
    pop af
file_original:
    call 0
    push af
    push hl
    push de
    push bc
    ld a,(v_driverOp)
    cp 4
    jp nz,driver_copy_back
    call timer_read
    ex de,hl
    ld hl,(v_flushStart)
    ld a,l
    sub e
    ld e,a
    ld a,h
    sbc a,d
    ld d,a
    ld hl,v_flushTicks
    call add_u16
    ld hl,(v_flushCount)
    inc hl
    ld (v_flushCount),hl
driver_copy_back:
file_driver_done:
    pop bc
    pop de
    pop hl
driver_restore_mode:
    ld a,(v_driverOp)
    cp 1
    jp nz,file_driver_return
    ld b,1
file_driver_return:
    pop af
    ret

; Удаляем только собственный файл. CF=ошибка, AF/HL разрушены.
file_cleanup:
    ld a,(v_fileOwned)
    or a
    ret z
    ld hl,tmpDescriptor
    call bios_fileDelete
    ret c
    xor a
    ld (v_fileOwned),a
    ret

txtFileExists:    DB 00Ah,"DISKBEN.TMP exists; rename/delete.",0
txtFlush:         DB "FLUSH:",009h,0
txtFlushMapped:   DB "FLUSH:     ",009h,0
txtTimes:         DB " times",00Ah,0
txtFileAccess:    DB 00Ah,"File access: writing and reading file with DOS",00Ah,0
txtRawAccess:     DB 00Ah,"Raw access: direct writing/reading of sectors",00Ah,0
txtPass:          DB 00Ah,"Pass ",0
txtPassDone:      DB " done",0
v_outputBase:     DW 0
v_outputMode:     DB 0
v_fileOwned:      DB 0
v_bufferMode:     DB 0
v_pass:           DB 0
v_driverOp:       DB 0
v_flushStart:     DW 0
v_flushTicks:     DD 0
v_flushCount:     DW 0
statAll:
resultsLow:
statRawWrite:     BLOCK 10,0
statRawRead:      BLOCK 10,0
statFileWrite:    BLOCK 10,0
statFileRead:     BLOCK 10,0
resultsHigh:     BLOCK 40,0
statRawVerify:   DD 0
statFileVerify:  DD 0
tmpDescriptor:    DB "DISKBEN TMP",0
                  BLOCK 6,0
                  DW 0
                  BLOCK 8,0
                  DD DEFAULT_FILE_KIB*1024
    ASSERT $-tmpDescriptor == FILE_DESCRIPTOR
v_rawOp:          DB 0
v_rawCluster:     DW 0
v_rawPtr:         DW 0
v_rawBuffer:      DW 0
v_rawLeft:        DB 0
v_rawOffset:      DB 0
rawSectors:       BLOCK MAX_FILE_SECTORS*2,0

    INCLUDE "readback.inc"
