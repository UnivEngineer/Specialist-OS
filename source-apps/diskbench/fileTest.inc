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
    call bench_cmp_hl_de
    jp nc,error
    ld hl,tmpDescriptor
    MXOS_CALL MXOS_DOS_FILE_LOAD_INFO
    jp nc,file_exists
    ld a,(v_budget)
    ld l,a
    ld h,0
    ld a,(v_fileSectors)
    ld e,a
    ld d,0
    call bench_mul_hl_de
    ld (v_operations),hl
    xor a
    ld (v_bufferMode),a
    ld hl,statAll
    ld b,40
    call clear_bytes
    ld hl,resultsHigh
    ld b,56                    ; HIGH-статистика и четыре счётчика VERIFY.
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
    call bhCreate
    call nc,file_write_low
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
    call bhReadOpen
    call nc,file_read_low
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
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    jp nz,file_write_high_done
    call file_write_high
    jp c,error
    ld de,resultsHigh+20
    call add_stats
    ld hl,v_verifyTicks
    ld de,statFileVerifyHigh
    ld b,4
    call add_bytes
file_write_high_done:
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
    ld hl,v_verifyTicks
    ld de,statRawVerifyHigh
    ld b,4
    call add_bytes
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
    call bench_printChar
    ld c,'/'
    call bench_printChar
    ld a,(v_budget)
    add a,'0'
    ld c,a
    call bench_printChar
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
; Для A: сначала HIGH/LOW одной операции, затем следующая операция.
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
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    jp nz,print_single
    ld a,(v_outputMode)
    cp 2                       ; FLUSH остаётся одной строкой.
    jp z,print_single
    push hl
    ld de,resultsHigh-resultsLow
    add hl,de
    call print_single
    pop hl
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
    call bench_cmp_hl_de
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
    call bench_cmp_hl_de
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
    call print_time_ms
    ld hl,txtRate
    call puts
    ld hl,(v_flushCount)
    ex de,hl
    ld b,0
    call bench_printDecWord
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
    jp c,raw_chain_invalid
    ex de,hl
    ld hl,(v_capacity)
    call bench_cmp_hl_de
    jp c,raw_chain_invalid
    jp z,raw_chain_invalid
    ld a,(v_drive)
    cp 1
    jp nz,raw_chain_valid
    ld a,e
    cp 0FFh
    jp z,raw_chain_invalid
raw_chain_valid:
    ld hl,(v_rawPtr)
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (v_rawPtr),hl
    ld a,(v_rawLeft)
    dec a
    ld (v_rawLeft),a
    or a
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
    call raw_driver
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

raw_chain_invalid:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret

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
    call raw_warmup_file
    ret c
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
    call raw_local
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
    ld a,1
    call bench_flush
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

; PREPARE завершает предыдущую запись и возвращает локальные входы.
; Внутри измеряемой серии обращений к DOS больше нет.
; BC/DE/HL сохраняются; CF/A=статус подготовки.
raw_warmup:
    push bc
    push de
    push hl
    ld b,RAW_OP_PREPARE
    MXOS_CALL MXOS_DOS_RAW_IO
    jp c,raw_prepare_done
    ld (raw_local_call+1),hl
raw_prepare_done:
    pop hl
    pop de
    pop bc
    ret
; B=1/2/4/5, DE=LBA, HL=локальный буфер. BC/DE/HL сохраняются.
; Список LBA/буферы подготовлены до серии; права PREPARE действуют до DOS.
raw_local:
    push bc
    ld a,b
    cp 1
    jp nz,raw_local_ready
    ld a,(v_writeMode)
    cp 5
    jp nz,raw_local_ready
    ld b,5
raw_local_ready:
    call raw_local_dispatch
    pop bc
    ret
raw_local_dispatch:
    ld a,(v_drive)
    ld c,a
raw_local_call:
    jp 0
raw_warmup_file:
    push de
    LOAD_DE rawSectors
    call raw_warmup
    pop de
    ret

; Общий RAW ABI: нет перехвата или изменения таблиц BIOS/DOS.
; Вход B=операция, DE=блок, HL=main-буфер. Для I/O BC/DE/HL сохранены;
; B=3/FFh возвращают геометрию и метаданные в регистрах по общему ABI.
; B=5 применяется только для RAW, файловая проверка выполняется целиком.
raw_driver:
    ld a,b
    cp 1
    jp z,raw_write
    MXOS_CALL MXOS_DOS_RAW_IO
    ret
raw_write:
    push bc
    ld a,(v_writeMode)
    cp 5
    jp nz,raw_dispatch
    ld b,5
raw_dispatch:
    MXOS_CALL MXOS_DOS_RAW_IO
    pop bc
    ret

; Явный барьер отделён от передачи данных. CF/BC/DE/HL сохраняются.
; Время входит в WRITE и отдельно показывается в FLUSH.
; A=0 — DOS (включая кэш/FAT/каталог), A=1 — физический RAW.
bench_flush:
    push bc
    push de
    push hl
    push af
    call timer_sample
    call timer_read
    ld (v_flushStart),hl
    pop af
    or a
    jp nz,bench_flush_raw
    call bhFlush
    jp bench_flush_done
bench_flush_raw:
    ld b,4
    call raw_local
bench_flush_done:
    push af
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
    call timer_sample
    pop af
    pop hl
    pop de
    pop bc
    ret

; Запись LOW через открытый FILEHANDLE: цепочку расширяет сама DOS.
; Затем явный файловый flush. CF=ошибка, все регистры разрушены.
file_write_low:
    ld hl,sectorBuffer
    ld a,(v_fileSectors)
    ld b,a
    ld c,0
    ld a,1
    call bhTransfer
    ret c
    xor a
    jp bench_flush

; Удаляем только собственный файл. CF=ошибка, AF/HL разрушены.
file_cleanup:
    call bhClose
    ret c
    ld a,(v_fileOwned)
    or a
    ret z
    ld hl,tmpDescriptor
    MXOS_CALL MXOS_DOS_FILE_DELETE
    ret c
    xor a
    ld (v_fileOwned),a
    ret

file_read_low:
    ld hl,verifyBuffer
    ld a,(v_fileSectors)
    ld b,a
    ld c,0
    ld a,2
    call bhTransfer
    ret c
    jp bhClose

    DEFINE BH_DESCRIPTOR tmpDescriptor
    DEFINE BH_DRIVE v_drive
    INCLUDE "file-handle.inc"

txtFileExists:    DB 00Ah,"DISKBEN.TMP exists; rename/delete.",0
txtFlush:         DB "FLUSH:  ",009h,009h,0
txtFlushMapped:   DB "FLUSH:  ",009h,009h,0
txtTimes:         DB " times",00Ah,0
txtFileAccess:    DB 00Ah,"File access: reading and writing file with DOS",00Ah,0
txtRawAccess:     DB 00Ah,"Raw access: direct reading and writing of sectors",00Ah,0
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
statRawVerifyHigh: DD 0
statFileVerifyHigh: DD 0
statRawHighPostVerify: DD 0
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
; Временный список LBA помещается между FAT-буфером и верхним окном.


    INCLUDE "readback.inc"
