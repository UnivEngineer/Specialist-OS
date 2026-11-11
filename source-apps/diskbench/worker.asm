; Общий измеритель MAIN/EXEC/DOS: один FILEHANDLE-запрос на весь локальный буфер.
; Вход HL=main-адрес десяти байтов настроек: диск, проверка, проходы, секторы,
; reserved:word, dataStart:word, RAW/FILE mask, scatter. Выход B=FAR-банк, HL=60 байт статистики.
; Первые 48 байт прежние; +48/+52 — RAW WRITE/READ HIGH, +56 — VERIFY HIGH U32.
; CF/A=ошибка DOS. Загрузка/пересылка результатов вне замеров.
    INCLUDE "../../source/include/mxos.inc"
    IFDEF WORKER_MAIN
    ORG 02000h
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ELSE
    ORG DOS_BANK_DRIVER_ADDR
    IFDEF WORKER_DOS
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
    ELSE
IMAGE_FLAGS = IMAGE_FLAG_BANK
    ENDIF
    ENDIF
entry:
    call workerEntry
    push af
    push bc
    push de
    push hl
    call restoreOptions
    pop hl
    pop de
    pop bc
    pop af
    ret
workerEntry:
    ld (copyRequest+1),hl
    call mx_memInfo
    ld (bankId),a
    ld (copyRequest+3),a
    ld hl,copyRequest
    call mx_memcpyFar
    ret c
    ld a,(config)
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    ret c
    ld b,3
    MXOS_CALL MXOS_DOS_RAW_IO
    ret c
    ex de,hl
    ld (capacity),hl
    ld b,0FFh
    MXOS_CALL MXOS_DOS_RAW_IO
    ret c
    ld a,c
    ld (caps),a
    ld hl,stats
    ld b,60
    xor a
clearStats:
    ld (hl),a
    inc hl
    dec b
    jp nz,clearStats
    call setOptions
    ret c
    call pitTimerInit
    call timerRange
    ld a,(config+3)
    cp BUFFER_SECTORS+1
    jp nc,chainInvalid
    ld a,(config+2)
    or a
    jp z,readOnly
    ld hl,descriptor
    MXOS_CALL MXOS_DOS_FILE_LOAD_INFO
    jp nc,exists
    MXOS_CALL MXOS_DOS_GET_ERROR
    cp DOS_ERR_NOT_FOUND
    jp z,fileAbsent
    scf
    ret
fileAbsent:
    ld a,1
    ld (pass),a
nextPass:
    ; Обратная связь вне измеряемого интервала.
    ld hl,txtPass
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    ld a,(pass)
    add a,'0'
    ld c,a
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    ld c,'/'
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    ld a,(config+2)
    add a,'0'
    ld c,a
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    ld a,(config+9)
    or a
    ld hl,txtScattered
    jp z,passAllocationReady
    MXOS_CALL MXOS_BIOS_PRINT_STRING
passAllocationReady:
    ld a,1
    ld (operation),a
    call prepareFileBuffer
    call prepareFile
    jp c,failed
    ld a,(config+8)
    and 2
    jp z,rawFileReady
    call begin
    call bhWriteOpen
    jp c,failed
    call fileTransfer
    jp c,failed
    xor a
    call flush
    jp c,failed
    call bhClose
    jp c,failed
    call timer_sample
    ld de,stats+20
    call addStats
    ld a,(config+1)
    cp 6
    jp nz,fileVerified
    call verifyWrite
    jp c,failed
fileVerified:
    ld a,2
    ld (operation),a
    call prepareFileBuffer
    call begin
    call bhReadOpen
    jp c,failed
    call fileTransfer
    jp c,failed
    call bhClose
    jp c,failed
    call timer_sample
    ld de,stats+30
    call addStats
    call compareFileBuffer
    jp c,failed
rawFileReady:
    ld a,(config+8)
    and 1
    jp z,passDone
    call buildChain
    jp c,failed
    call prepareRaw
    jp c,failed
    ld a,1
    ld (operation),a
    call prepareRawBuffer
    call begin
    call rawSeries
    jp c,failed
    ld a,(caps)
    and DISK_CAP_FLUSH
    ld a,1
    call nz,flush
    jp c,failed
    call timer_sample
    ld de,stats
    call addStats
    ld a,(config+1)
    cp 6
    jp nz,rawVerified
    call prepareRaw
    jp c,failed
    call begin
    call validateRawData
    jp c,failed
    call timer_sample
    ld de,stats+40
    call addElapsed
rawVerified:
    ld a,2
    ld (operation),a
    call prepareRawBuffer
    call begin
    call rawSeries
    jp c,failed
    call timer_sample
    ld de,stats+10
    call addStats
    call validateRawData
    jp c,failed
    IFDEF WORKER_MAIN
    ld a,(config)
    or a
    call z,rawHigh
    jp c,failed
    ENDIF
passDone:
    call cleanup
    ret c
    ld hl,pass
    inc (hl)
    ld a,(config+2)
    cp (hl)
    jp nc,nextPass
result:
    ld a,(bankId)
    ld b,a
    ld hl,stats
    xor a
    ret
exists:
    ld a,DOS_ERR_EXISTS
    scf
    ret
failed:
    push af
    call cleanup
    pop af
    ret
cleanup:
    call bhClose
    ret c
    ld a,(owned)
    or a
    ret z
    ld hl,descriptor
    MXOS_CALL MXOS_DOS_FILE_DELETE
    ret c
    xor a
    ld (owned),a
    ret

; Параметры/цепочка определены до начала обмена. CF=ошибка, регистры разрушены.
begin:
    xor a
    ld (flushTicks),a
    ld (flushTicks+1),a
    ld (flushTicks+2),a
    ld (flushTicks+3),a
    ld (flushCount),a
    ld (flushCount+1),a
    jp reset_timing

; RAW использует один стабильный секторный буфер. Подготовка и обычная
; валидация отделены от непрерывной измеряемой передачи всего списка LBA.
prepareRawBuffer:
    ld hl,(bufferAddress)
    ld a,(operation)
    cp 1
    ld c,0
    jp z,rawFill
    dec c
rawFill:
    ld a,(pass)
    xor l
    xor c
    ld (hl),a
    inc l
    jp nz,rawFill
    ret
rawSeries:
    ld a,(operation)
    ld b,a
    dec a
    jp nz,rawSelect
    ld a,(config+1)
    cp 5
    jp nz,rawSelect
    ld b,5
rawSelect:
    ld hl,(rawCall+1)
rawSelected:
    ld (rawActive+1),hl
    ld a,b
    ld (rawOperation),a
    ld hl,sectors
    ld (list),hl
    ld a,(config+3)
    ld (index),a
rawBlock:
    ld hl,(list)
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld (list),hl
    ld hl,(bufferAddress)
    ld a,(rawOperation)
    ld b,a
    ld a,(config)
    ld c,a
    call rawActive
    ret c
    ld hl,index
    dec (hl)
    jp nz,rawBlock
    xor a
    ret
rawActive:
    jp 0
; Для обычного READ это отдельная нетактуемая проверка всех секторов;
; для WRITE+VERIFY — измеряемый полный проход чтения и сравнения.
validateRawData:
    ld a,(readUnknown)
    or a
    ret nz
    ld hl,sectors
    ld (list),hl
    ld a,(config+3)
    ld (index),a
validateRawNext:
    ld hl,(list)
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld (list),hl
    ld hl,(bufferAddress)
    ld b,2
    ld a,(config)
    ld c,a
    call rawCall
    ret c
    ld hl,(bufferAddress)
validateRawByte:
    ld a,(pass)
    xor l
    cp (hl)
    jp nz,mismatch
    inc l
    jp nz,validateRawByte
    ld hl,index
    dec (hl)
    jp nz,validateRawNext
    xor a
    ret
mismatch:
    ld a,DOS_ERR_IO
    scf
    ret

; Создание и выделение всей цепочки вне WRITE. SEEK за EOF и запись
; последнего байта используют штатный FILEHANDLE без передачи всего файла.
; Содержимое промежутка не читается до полной измеряемой перезаписи.
prepareFile:
    call bhCreate
    ret c
    ld a,1
    ld (owned),a
    ld a,(config+3)
    ld h,a
    ld l,0
    dec hl
    ld (bhHandle+FH_SEEK),hl
    xor a
    ld (bhHandle+FH_SEEK+2),a
    ld (bhHandle+FH_SEEK+3),a
    ld (bhHandle+FH_ORIGIN),a
    ld a,FILE_OP_SEEK
    call bhCall
    ret c
    ld bc,1
    ld hl,buffer
    ld a,1
    call bhTransfer
    ret c
    jp bhClose

; В FILE весь буфер передаётся одним запросом без обращений к таймеру.
fileTransfer:
    ld a,(config+3)
    ld b,a
    ld c,0
    ld hl,buffer
    ld a,(operation)
    jp bhTransfer
prepareFileBuffer:
    ld hl,buffer
    xor a
    ld (index),a
bulkFillPage:
    ld a,(operation)
    cp 1
    ld c,0
    jp z,bulkFill
    dec c
bulkFill:
    ld a,(pass)
    ld b,a
    ld a,(index)
    xor b
    xor l
    xor c
    ld (hl),a
    inc l
    jp nz,bulkFill
    inc h
    ld a,(index)
    inc a
    ld (index),a
    ld b,a
    ld a,(config+3)
    cp b
    jp nz,bulkFillPage
    ret
compareFileBuffer:
    ld a,(readUnknown)
    or a
    ret nz
    ld hl,buffer
    xor a
    ld (index),a
bulkCompare:
    ld a,(pass)
    ld b,a
    ld a,(index)
    xor b
    xor l
    cp (hl)
    jp nz,mismatch
    inc l
    jp nz,bulkCompare
    inc h
    ld a,(index)
    inc a
    ld (index),a
    ld b,a
    ld a,(config+3)
    cp b
    jp nz,bulkCompare
    xor a
    ret

; A=0 DOS flush, A=1 физический flush. Он также входит в WRITE.
flush:
    ld b,a
    ld a,(caps)
    and DISK_CAP_FLUSH
    ld a,b
    jp nz,flushMeasured
    or a
    jp z,bhFlush
    ret
flushMeasured:
    push af
    call timer_read
    ld (flushStart),hl
    pop af
    or a
    jp nz,physicalFlush
    call bhFlush
    jp flushDone
physicalFlush:
    ld b,4
    ld a,(config)
    ld c,a
    call rawCall
flushDone:
    push af
    call timer_read
    ex de,hl
    ld hl,(flushStart)
    ld a,l
    sub e
    ld e,a
    ld a,h
    sbc a,d
    ld d,a
    ld hl,flushTicks
    call add_u16
    ld hl,(flushCount)
    inc hl
    ld (flushCount),hl
    pop af
    ret

; Только подготовка пересекает DOS. Список LBA уже находится здесь.
; Нельзя вызывать DOS между prepareRaw и последним локальным flush.
prepareRaw:
    ld b,RAW_OP_PREPARE
    MXOS_CALL MXOS_DOS_RAW_IO
    ret c
    ld (rawCall+1),hl
    xor a
    ret
rawCall:
    jp 0

    IFDEF WORKER_MAIN
; A: верхний буфер одновременно виден с ROM. Подготовка/сравнение
; исключены так же, как для нижнего буфера; число секторов одинаковое.
rawHigh:
    ld hl,08200h
    ld (bufferAddress),hl
    ld a,1
    ld (operation),a
    call prepareRawBuffer
    call begin
    call rawSeries
    ret c
    ld a,(caps)
    and DISK_CAP_FLUSH
    ld a,1
    call nz,flush
    ret c
    call timer_sample
    ld de,stats+48
    call addElapsed
    ld a,(config+1)
    cp 6
    jp nz,readHigh
    call begin
    call validateRawData
    ret c
    call timer_sample
    ld de,stats+56
    call addElapsed
    jp readHigh
readHigh:
    ld hl,08200h
    ld (bufferAddress),hl
    ld a,2
    ld (operation),a
    call prepareRawBuffer
    call begin
    call rawSeries
    ret c
    call timer_sample
    ld de,stats+52
    call addElapsed
    call validateRawData
    push af
    ld hl,buffer
    ld (bufferAddress),hl
    pop af
    ret
    ENDIF

; DE=WRITE, HL=VERIFY статистика. Проверяется готовый файл после flush.
verifyWrite:
    ld a,2
    ld (operation),a
    call prepareFileBuffer
    call begin
    call bhReadOpen
    ret c
    call fileTransfer
    ret c
    call bhClose
    ret c
    call compareFileBuffer
    ret c
    call timer_sample
    ld de,stats+44
    jp addElapsed

; DE=10 байт: elapsed U32, flush U32, flush count U16.
addStats:
    ld hl,flushTicks
    call scaleTime
    call addElapsed
    ld hl,flushTicks
    ld b,6
    jp addBytes
addElapsed:
    ld hl,v_elapsed
    call scaleTime
    ld b,4
addBytes:
    or a
addByte:
    ld a,(de)
    adc a,(hl)
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,addByte
    ret

; FAT16, физический/логический сектор 256, один сектор на кластер.
; Список строится вне замера; FAT и данные используют разные буферы.
buildChain:
    ld hl,0FFFFh
    ld (chainFatSector),hl
    ld hl,(descriptor+FILE_DESCRIPTOR.firstCluster)
    ld (cluster),hl
    ld hl,sectors
    ld (list),hl
    xor a
    ld (index),a
chainNext:
    ld hl,(cluster)
    dec hl
    dec hl
    push hl
    ld hl,(config+6)
    ex de,hl
    pop hl
    add hl,de
    jp c,chainInvalid
    ex de,hl
    call validateSector
    ret c
    ld hl,(list)
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (list),hl
    ld hl,index
    inc (hl)
    ld a,(config+3)
    cp (hl)
    ret z
    ld hl,(cluster)
    add hl,hl
    ld a,l
    ld (offset),a
    ld e,h
    ld d,0
    ld hl,(config+4)
    add hl,de
    ex de,hl
    ld hl,(chainFatSector)
    ld a,h
    cp d
    jp nz,chainFatRead
    ld a,l
    cp e
    jp z,chainFatReady
chainFatRead:
    ld hl,fatBuffer
    ld b,2
    MXOS_CALL MXOS_DOS_RAW_IO
    ret c
    ex de,hl
    ld (chainFatSector),hl
chainFatReady:
    ld a,(offset)
    ld l,a
    ld h,fatBuffer/256
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (cluster),hl
    jp chainNext
chainInvalid:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret

; Только RAW READ: фиксированные 32 сектора, без создания файла.
readOnly:
    ld a,3
    ld (readOnlyLeft),a
    ld a,1
    ld (readUnknown),a
    ld (raw),a
    ld a,(config+3)
    cp 33
    jp c,readOnlySize
    ld a,32
readOnlySize:
    ld (config+3),a
    push hl
    ld hl,(config+6)
    ex de,hl
    pop hl
    ld hl,sectors
    ld b,a
readOnlyList:
    call validateSector
    ret c
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    inc de
    dec b
    jp nz,readOnlyList
readOnlyNext:
    ld a,(config+8)
    and 1
    jp z,readOnlyFile
    ld a,1
    ld (raw),a
    call prepareRaw
    ret c
    ld a,2
    ld (operation),a
    call prepareRawBuffer
    call begin
    call rawSeries
    ret c
    call timer_sample
    ld de,stats+10
    call addStats
    IFDEF WORKER_MAIN
    ld a,(config)
    or a
    call z,readHigh
    ret c
    ENDIF
readOnlyFile:
    ld a,(config+8)
    and 2
    jp z,readOnlyDone
    ld hl,descriptor
    MXOS_CALL MXOS_DOS_FILE_LOAD_INFO
    jp c,readOnlyDone
    ld a,2
    ld (operation),a
    call prepareFileBuffer
    call begin
    call bhReadOpen
    ret c
    call fileTransfer
    ret c
    call bhClose
    ret c
    call timer_sample
    ld de,stats+30
    call addStats
readOnlyDone:
    ld hl,readOnlyLeft
    dec (hl)
    jp nz,readOnlyNext
    jp result

; Проверки списка вне измерений: локальный аппаратный вход не проверяет
; границы. Нельзя превращать повреждённую FAT в запись за концом чипа.
validateSector:
    push hl
    ld hl,(capacity)
    ld a,d
    cp h
    jp c,sectorInside
    jp nz,sectorOutside
    ld a,e
    cp l
    jp nc,sectorOutside
sectorInside:
    ld a,(config)
    dec a
    jp nz,sectorValid
    ld a,e
    cp 0FFh
    jp z,sectorOutside
sectorValid:
    pop hl
    xor a
    ret
sectorOutside:
    pop hl
    ld a,DOS_ERR_ARGUMENT
    scf
    ret

; Отсчёты не пересекают банк: иначе стоимость BIOS-маршрута искажает
; сравнение MAIN/DOS/BANK несколько раз на каждом секторе.
    INCLUDE "../../source/include/timing-pit.inc"
timer_read = pitTimerRead
    INCLUDE "../../source/include/timing-counter.inc"
    DEFINE BH_DESCRIPTOR descriptor
    DEFINE BH_SCATTER config+9
    DEFINE BH_DRIVE config
    INCLUDE "file-handle.inc"

; Настройка FILEHANDLE действует только на время DLL и восстанавливается
; при любом штатном/ошибочном выходе; данные берутся из локального объекта.
setOptions:
    ld a,(config+1)
    cp 5
    ld hl,0
    jp nz,optionsPlain
    inc hl
optionsPlain:
    call exchangeOptions
    ret c
    ld a,(bhHandle+FH_COUNT)
    ld (oldOptions),a
    ld a,1
    ld (optionsSaved),a
    or a
    ret
restoreOptions:
    ld a,(optionsSaved)
    or a
    ret z
    ld a,(oldOptions)
    ld l,a
    ld h,0
exchangeOptions:
    ld (bhHandle+FH_COUNT),hl
    ld a,1
    ld (bhHandle+FH_ORIGIN),a
    ld a,FILE_OP_OPTIONS
    ld hl,bhHandle
    MXOS_CALL MXOS_DOS_FILEHANDLE
    ret
; Разбросанные цепочки медленнее: период 33,55 с вместо 8,39 с.
; В отчёт по-прежнему возвращаются эквивалентные тики по 128 мкс.
timerRange:
    ld a,(config+9)
    or a
    ret z
    ld a,2
    ld (elapsedShift),a
    ld hl,IO_TIMER_MODE
    ld (hl),074h
    dec hl
    dec hl
    ld (hl),0
    ld (hl),4
    ret
; HL=U32; сохранить DE (назначение накопления), BC/AF разрушены.
scaleTime:
    ld a,(elapsedShift)
    or a
    ret z
    ld c,a
scaleTimeBit:
    push hl
    ld b,4
    or a
scaleTimeByte:
    ld a,(hl)
    rla
    ld (hl),a
    inc hl
    dec b
    jp nz,scaleTimeByte
    pop hl
    dec c
    jp nz,scaleTimeBit
    ret
elapsedShift: DB 0
oldOptions: DB 0
optionsSaved: DB 0
txtScattered: DB "  Allocation: random",0

copyRequest: DB 0
    DW 0
    DB 0
    DW config,10
config: DS 10,0
bankId: DB 0
caps: DB 0
capacity: DW 0
owned: DB 0
pass: DB 0
operation: DB 0
raw: DB 0
verify: DB 0
rawOperation: DB 0
readUnknown: DB 0
readOnlyLeft: DB 0
index: DB 0
offset: DB 0
cluster: DW 0
chainFatSector: DW 0FFFFh
list: DW 0
bufferAddress: DW buffer
writeStat: DW 0
verifyStat: DW 0
flushStart: DW 0
flushTicks: DD 0
flushCount: DW 0
v_elapsed: DD 0
v_setup: DD 0
v_last: DW 0
stats: DS 60,0
descriptor: DB "DISKBEN TMP",0
    DS 20,0
    ASSERT $-descriptor == FILE_DESCRIPTOR
sectors: DS 24*4*2,0
    IFDEF WORKER_MAIN
txtPass: DB 10,"MAIN: Pass ",0
    ELSE
    IFDEF WORKER_DOS
txtPass: DB 10,"DOS:  Pass ",0
    ELSE
txtPass: DB 10,"BANK: Pass ",0
    ENDIF
    ENDIF
    ALIGN 256
fatBuffer: DS 256,0
workerCodeEnd:
    IFDEF WORKER_MAIN
BUFFER_SECTORS = 24*4
    ASSERT $ <= 02C00h
    DS 02C00h-$,0
buffer: DS BUFFER_SECTORS*256,0
    ELSE
    IFDEF WORKER_DOS
BUFFER_SECTORS = BENCH_DOS_BUFFER_KIB*4
buffer: DS BUFFER_SECTORS*256,0
    ELSE
BUFFER_SECTORS = 24*4
; EXEC-банк целиком принадлежит DLL; нижняя область свободна от её тела.
buffer = 02000h
    ASSERT buffer+BUFFER_SECTORS*256 <= DOS_BANK_DRIVER_ADDR
    ENDIF
    ENDIF
workerEnd:
    IFDEF WORKER_MAIN
    ASSERT workerEnd <= LEGACY_APP_MEMORY_END
    ELSE
    IFDEF WORKER_DOS
    ASSERT workerEnd <= DOS_BANK_MODULE_END
    ELSE
    ASSERT workerEnd <= DOS_BANK_DRIVER_END
    ENDIF
    ENDIF
