; Прямое верхнее чтение: файл передаётся порциями до 3 КиБ в 8200h.
; Обе области видны CPU одновременно; копирования в нижний буфер нет.
; B=0 DOS / B=1 Raw. CF=ошибка. Все регистры разрушены.
read_high:
    ld a,2
    ld (v_streamOp),a
    ld a,b
    ld (v_streamKind),a
    xor a
    ld (v_streamVerify),a
    ld a,b
    or a
    call nz,raw_warmup_file
    ret c
    call measure_begin
    call stream_read
    jp measure_end

; Прямая Raw-запись из верхнего окна. Подготовка каждой порции вне WRITE.
; Нижний оригинал сохраняется для проверки. AF/BC/DE/HL разрушены, CF=ошибка.
write_high:
    ld a,1
    ld (v_streamKind),a
    ld (v_streamOp),a
    xor a
    ld (v_streamVerify),a
    call raw_warmup_file
    ret c
    call measure_begin
    call stream_read
    call nc,maybe_verify_raw
    jp measure_end

; Полное создание файла через DOS с источником в верхнем окне.
; Удаление предыдущего тестового файла и подготовка порций исключены;
; выделение FAT/каталога, FileStream и flush входят в WRITE HIGH.
file_write_high:
    call file_cleanup
    ret c
    call measure_begin
    ld a,1
    ld (v_fileOwned),a
    ld hl,tmpDescriptor
    MXCALL MX_DOS_FILECREATEPREPARE
    call c,measure_end
    ret c
    xor a
    ld (v_streamKind),a
    ld (v_streamVerify),a
    ld a,1
    ld (v_streamOp),a
    call stream_read
    call nc,maybe_verify_file
    jp measure_end

; Физический драйвер DOS работает в другом банке: перехват C863h
; видит только Raw. Для DOS оба режима проверки читают готовый файл целиком.
maybe_verify_file:
    ld a,(v_writeMode)
    cp 5
    jp z,verify_file_data
    cp 6
    jp z,verify_file_data
    or a
    ret
maybe_verify_raw:
    ld a,(v_writeMode)
    cp 6
    jp z,verify_raw_data
    or a
    ret
verify_file_data:
    xor a
    jp verify_stream
verify_raw_data:
    ld a,1
; A=0 DOS / 1 Raw. Весь файл уже записан и завершён штатным flush.
; Сравниваем считанное с оригинальным нижним буфером. Проверка включена
; в WRITE; отдельно сохраняем её время. Poison верхнего окна исключается.
verify_stream:
    ld (v_streamKind),a
    ld a,2
    ld (v_streamOp),a
    call timer_sample
    ld a,(v_streamKind)
    or a
    jp z,verify_warm_ready
    ; Flush мог передать устройство обратно DOS. Подготовка повторного
    ; RAW-чтения не является проверкой данных и в VERIFY не входит.
    call raw_warmup_file
    ret c
    call timer_read
    ld (v_last),hl
verify_warm_ready:
    ld hl,v_elapsed
    ld de,v_verifyStart
    ld b,4
    call copy_bytes
    ld a,1
    ld (v_streamVerify),a
    call stream_read
    push af
    call timer_sample
    ld hl,v_elapsed
    ld de,v_verifyTicks
    ld b,4
    call copy_bytes
    ld hl,v_verifyTicks
    ld de,v_verifyStart
    call subtract_u32
    pop af
    ret

; Чтение и проверка общие для обычного HIGH и полной проверки файла.
; При v_streamVerify=0 сравнение исключено из READ, при 1 входит в WRITE.
stream_read:
    ld hl,sectorBuffer
    ld (v_streamOriginal),hl
    ld hl,rawSectors
    ld (v_streamList),hl
    ld a,(v_fileSectors)
    ld (v_streamLeft),a
    ld a,(v_streamKind)
    or a
    jp nz,stream_next
    ld hl,tmpDescriptor
    call bios_fileLoadInfo
    ret c
    ld hl,(tmpDescriptor+FILE_DESCRIPTOR.firstCluster)
    ld (v_streamCluster),hl
    ld hl,0
    ld (v_streamOffset),hl
stream_next:
    ld a,(v_streamLeft)
    cp HIGH_SECTORS+1
    jp c,stream_size
    ld a,HIGH_SECTORS
stream_size:
    ld (v_streamChunk),a
    add a,highBuffer / 256
    ld (v_streamEndHi),a
    ; Исключаем искусственное заполнение противоположными данными.
    call timer_sample
    ld hl,highBuffer
    LOAD_DE v_streamOriginal
stream_poison:
    ld a,(v_streamOp)
    dec a
    ld a,(de)
    jp z,stream_copy_byte
    cpl
stream_copy_byte:
    ld (hl),a
    inc de
    inc hl
    ld a,(v_streamEndHi)
    cp h
    jp nz,stream_poison
    ld a,(v_streamOp)
    dec a
    jp nz,stream_source_ready
    STORE_DE v_streamOriginal
stream_source_ready:
    call timer_read
    ld (v_last),hl
    ld a,(v_streamKind)
    or a
    jp nz,stream_raw
    ld de,v_streamCluster
    ld hl,highBuffer
    ld a,(v_streamChunk)
    ld b,a
    ld c,0
    ld a,(v_streamOp)
    MXCALL MX_DOS_FILESTREAM
    ret c
    jp stream_compare
stream_raw:
    ld a,(v_streamChunk)
    ld (v_streamCounter),a
    ld hl,highBuffer
stream_raw_sector:
    push hl
    ld hl,(v_streamList)
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld (v_streamList),hl
    pop hl
    ld a,(v_streamOp)
    ld b,a
    call bios_diskDriver
    ret c
    inc h
    ld a,(v_streamCounter)
    dec a
    ld (v_streamCounter),a
    jp nz,stream_raw_sector
stream_compare:
    ld a,(v_streamOp)
    dec a
    jp z,stream_advance
    ld a,(v_streamVerify)
    or a
    call z,timer_sample
    ld hl,highBuffer
    LOAD_DE v_streamOriginal
stream_compare_byte:
    ld a,(de)
    cp (hl)
    scf
    ret nz
    inc hl
    inc de
    ld a,(v_streamEndHi)
    cp h
    jp nz,stream_compare_byte
    STORE_DE v_streamOriginal
    ld a,(v_streamVerify)
    or a
    jp nz,stream_advance
    call timer_read
    ld (v_last),hl
stream_advance:
    ld a,(v_streamChunk)
    ld b,a
    ld a,(v_streamLeft)
    sub b
    ld (v_streamLeft),a
    jp nz,stream_next
    ld a,(v_streamOp)
    dec a
    jp nz,stream_success
    ld a,(v_streamKind)
    or a
    jp nz,$+9
    MXCALL MX_DOS_FLUSHVOLUME
    ret
    ld a,(v_caps)
    and DISK_CAP_FLUSH
    jp z,stream_success
    ld b,4
    jp bios_diskDriver
stream_success:
    or a
    ret

print_verify:
    ld hl,txtVerifyTime
    call puts
    ld hl,(v_outputBase)
    ld de,resultsLow+20
    call bios_cmp_hl_de
    ld hl,statRawVerify
    jp c,verify_print_value
    ld hl,statFileVerify
verify_print_value:
    call print_time_ms
    ld hl,txtVerifyIncluded
    call puts
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    ret nz
    ld hl,txtVerifyHigh
    call puts
    ld hl,(v_outputBase)
    ld de,resultsLow+20
    call bios_cmp_hl_de
    ld hl,statRawVerifyHigh
    jp c,verify_print_high
    ld hl,statFileVerifyHigh
verify_print_high:
    call print_time_ms
    ld hl,txtVerifyIncluded
    jp puts

txtVerifyTime:     DB "VERIFY: ",009h,009h,0
txtVerifyHigh:     DB "VERIFY HIGH:",009h,0
txtVerifyIncluded: DB " ms (included in WRITE)",00Ah,0
v_verifyTicks:     DD 0
v_verifyStart:     DD 0
v_streamKind:      DB 0
v_streamOp:        DB 2
v_streamVerify:    DB 0
v_streamOriginal:  DW 0
v_streamList:      DW 0
v_streamCluster:   DW 0
v_streamOffset:    DW 0
v_streamLeft:      DB 0
v_streamChunk:     DB 0
v_streamEndHi:     DB 0
v_streamCounter:   DB 0
