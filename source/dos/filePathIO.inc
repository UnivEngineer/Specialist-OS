; Загрузка/сохранение целого файла по пути. HL -> FILE_PATH_IO в main RAM.
; count: вместимость при чтении, точная длина при записи; actual=0 при ошибке.
; descriptor — рабочие 32 байта вызывающего, не пересекаются с буфером/путём.
; Сохранение заменяет существующий обычный файл; R/O и каталоги защищены.
; Диск пути становится текущим; сохранённые CWD дисков не изменяются.
; CF/A — код DOS, AF/BC/DE/HL разрушены. Нереентерабельно.
filePathLoad:
    xor a
    jp filePathIO
filePathSave:
    ld a,1
filePathIO:
    ld (pathIoWrite),a
    ld (pathIoRequest),hl
    ld bc,FILE_PATH_IO
    call streamBufferRange
    jp c,dosGetError
    inc hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld (hl),0
    inc hl
    ld (hl),0
    ex de,hl
    ld (pathIoBuffer),hl
    push hl
    ld h,b
    ld l,c
    ld (pathIoCount),hl
    pop hl
    call streamBufferRange
    jp c,dosGetError
    ; Пустой буфер не пересекает запрос независимо от адреса.
    ld a,b
    or c
    jp z,pathIoRangeReady
    add hl,bc
    ex de,hl
    ld hl,(pathIoRequest)
    ex de,hl
    call cmp_hl_de
    jp c,pathIoRangeReady
    jp z,pathIoRangeReady
    ld hl,FILE_PATH_IO
    add hl,de
    ex de,hl
    ld hl,(pathIoBuffer)
    call cmp_hl_de
    jp c,blockArgument
pathIoRangeReady:
    ld hl,(pathIoRequest)
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de
    ld de,7
    add hl,de
    push hl
    ex de,hl
    ld b,FILE_DESCRIPTOR
    xor a
    call memset_de_a_b
    pop de
    pop hl
    call fileNamePrepare
    jp c,pathIoExit
    ; Здесь используются общие FAT-примитивы, как в FileHandle.
    ld hl,(v_volumeTable)
    ld de,fatVolumeDriver+VOLUME_DRIVER_HEADER
    call cmp_hl_de
    jp nz,pathIoUnsupported
    ld hl,(pathIoRequest)
    ld de,8
    add hl,de
    ld a,(pathIoWrite)
    or a
    jp nz,pathIoSave
    call fileFind
    jp c,pathIoExit
    ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,pathIoArgument
    ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.size)
    ex de,hl
    ld hl,(pathIoCount)
    ex de,hl
    call cmp_hl_de
    jp c,pathIoLoadReady
    jp nz,pathIoArgument
pathIoLoadReady:
    ld (pathIoCount),hl
    ld hl,(pathIoRequest)
    ld de,8
    add hl,de
    ex de,hl
    ld hl,(pathIoBuffer)
    ex de,hl
    ; fileFind уже выполнен, повторный поиск по каталогу не нужен.
    call fatFileLoadFound
    jp pathIoComplete
pathIoSave:
    push hl
    ld de,FILE_DESCRIPTOR.size
    add hl,de
    ex de,hl
    ld hl,(pathIoCount)
    ex de,hl
    ld (hl),e
    inc hl
    ld (hl),d
    pop hl
    ex de,hl
    ld hl,(pathIoBuffer)
    ex de,hl
    call fileCreate
pathIoComplete:
    jp c,pathIoExit
    ld hl,(pathIoRequest)
    ld de,6
    add hl,de
    ex de,hl
    ld hl,(pathIoCount)
    ex de,hl
    ld (hl),e
    inc hl
    ld (hl),d
    xor a
pathIoExit:
    ; Старые внутренние функции могут возвращать служебное A вместо кода DOS.
    call c,dosGetError
    push af
    call selectDirectory
    pop af
    ret
pathIoArgument:
    call blockArgument
    jp pathIoExit
pathIoUnsupported:
    call dosUnsupported
    jp pathIoExit
pathIoRequest: DW 0
pathIoBuffer: DW 0
pathIoCount: DW 0
pathIoWrite: DB 0
