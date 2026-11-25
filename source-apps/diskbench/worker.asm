; Общий измеритель MAIN/EXEC/DOS: один FILEHANDLE-запрос на весь локальный буфер.
; Вход HL=main-адрес одиннадцати байтов настроек: диск, проверка, проходы, страницы по 256 байт,
; секторов/кластер, страниц/сектор, dataStart:word, RAW/FILE mask, scatter, смещение буфера 0/1. Выход B=FAR-банк, HL=88 байт статистики.
; Первые 48 байт прежние; +48/+52 — RAW WRITE/READ HIGH, +56 — VERIFY HIGH U32.
; +60/+70 — FILE HIGH WRITE/READ (по 10 байт), +84 — VERIFY HIGH U32.
; CF/A=ошибка DOS. Загрузка/пересылка результатов вне замеров.
    INCLUDE "../../source/include/mxos.inc"
    INCLUDE "layout.inc"
    IFDEF WORKER_MAIN
    ORG BENCH_MAIN_WORK_ADDR
IMAGE_FLAGS = IMAGE_FLAG_MAIN
IMAGE_MEMORY_POLICY = IMAGE_POLICY_MANAGED
IMAGE_API           = 2
    ELSE
    ORG DOS_BANK_DRIVER_ADDR
    IFDEF WORKER_DOS
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
    ELSE
IMAGE_FLAGS = IMAGE_FLAG_BANK
    ENDIF
    ENDIF

;----------------------------------------------------------------------------
; MXOS
;
; Выполнить измерение дискового обмена в выбранной странице и восстановить опции.
;
; вход:
;   hl = адрес 11 байт настроек в MAIN: диск, проверка, число проходов, страницы по 256 байт,
;   секторы/кластер, страницы/сектор, первое LBA данных, маска RAW/FILE, размещение и смещение буфера 0 или 1.
;   Для MAIN контроллер владеет буфером с BENCH_MAIN_BUFFER_ADDR и областью
;   BENCH_MAIN_STATS_ADDR на весь вызов; длина буфера включает смещение.
;
; выход:
;   cf = 0: b = FAR-страница, hl = локальный адрес 88 байт статистики. При отказе cf =
;   1, a = код. restoreOptions выполняется на обоих путях и сохраняет регистры
;   результата.
;----------------------------------------------------------------------------

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
    ld a,(config+10)
    cp 2
    jp nc,chainInvalid
    ld hl,buffer
    ld l,a
    ld (dataBuffer),hl
    ld (bufferAddress),hl
    ld a,(config)
    MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
    ret c
    ld b,3
    MXOS_CALL_REGS MXOS_DOS_RAW_IO
    ret c
    ex de,hl
    ld (capacity),hl
    ld b,0FFh
    MXOS_CALL_REGS MXOS_DOS_RAW_IO
    ret c
    ld a,c
    ld (caps),a
    ld a,b
    ld (technology),a
    ld a,(config+5)
    ld (sectorPages),a
    ld hl,stats
    ld b,BENCH_MAIN_STATS_SIZE
    xor a
clearStats:
    ld (hl),a
    inc hl
    dec b
    jp nz,clearStats
    call setOptions
    ret c
    call pitTimerInit
    ld a,DOS_ERR_UNSUPPORTED
    ret c
    call timerRange
    ld a,(config+3)
    cp BUFFER_SECTORS+1
    jp nc,chainInvalid
    ld a,(config+2)
    or a
    jp z,readOnly
    ld hl,descriptor
    MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO
    jp nc,exists
    MXOS_CALL_REGS MXOS_DOS_GET_ERROR
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
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING
    ld a,(pass)
    add a,'0'
    ld c,a
    call mx_printChar
    ld c,'/'
    call mx_printChar
    ld a,(config+2)
    add a,'0'
    ld c,a
    call mx_printChar
    ld a,(config+9)
    or a
    ld hl,txtScattered
    jp z,passAllocationReady
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING
passAllocationReady:
    ld a,1
    ld (operation),a
    call prepareFileBuffer
    call prepareFile
    jp c,failed
    ld a,(config+8)
    and 2
    jp z,rawFileReady
    call filePass
    jp c,failed
    IFDEF WORKER_MAIN
    call fileHigh
    jp c,failed
    ENDIF
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
    call workerMapped
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
    MXOS_CALL_REGS MXOS_DOS_FILE_DELETE
    ret c
    xor a
    ld (owned),a
    ret

filePass:
    call begin
    call bhWriteOpen
    ret c
    call fileTransfer
    ret c
    xor a
    call flush
    ret c
    call bhClose
    ret c
    call timer_sample
    ld de,0
    call fileAddStats
    ld a,(config+1)
    cp 6
    jp nz,fileVerified
    call verifyWrite
    ret c
fileVerified:
fileReadPass:
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
    ld de,10
    call fileAddStats
    call compareFileBuffer
    ret c
    ret
fileAddStats:
    ld hl,(fileStats)
    add hl,de
    ex de,hl
    jp addStats

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
    ld a,(sectorPages)
    ld d,a
    ld hl,(bufferAddress)
    ld a,(operation)
    cp 1
    ld a,0
    jp z,rawFillPattern
    dec a
rawFillPattern:
    ld b,a
    ld a,(pass)
    xor b
    ld b,a
    ld c,0
rawFill:
    ld a,b
    xor l
    ld (hl),a
    inc hl
    dec c
    jp nz,rawFill
    dec d
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
    ld hl,(rawJump+1)
rawSelected:
    ld (rawActive+1),hl
    ld a,b
    ld (rawOperation),a
    ld hl,sectors
    ld (list),hl
    call rawCount
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
    ld a,(rawCall+1)
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
    call rawCount
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
    ld a,(sectorPages)
    ld d,a
    ld hl,(bufferAddress)
    ld c,0
validateRawByte:
    ld a,(pass)
    xor l
    cp (hl)
    jp nz,mismatch
    inc hl
    dec c
    jp nz,validateRawByte
    dec d
    jp nz,validateRawByte
validateRawSectorDone:
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
    ld hl,(dataBuffer)
    ld a,1
    call bhTransfer
    ret c
    jp bhClose

; В FILE весь буфер передаётся одним запросом без обращений к таймеру.
fileTransfer:
    ld a,(config+3)
    ld b,a
    ld c,0
    ld hl,(dataBuffer)
    ld a,(operation)
    jp bhTransfer
prepareFileBuffer:
    ld hl,(dataBuffer)
    xor a
    ld (index),a
bulkFillPage:
    ld a,(operation)
    cp 1
    ld a,0
    jp z,bulkFillPattern
    dec a
bulkFillPattern:
    ld b,a
    ld a,(pass)
    xor b
    ld b,a
    ld a,(index)
    xor b
    ld b,a
    ld c,0
bulkFill:
    ld a,b
    xor l
    ld (hl),a
    inc hl
    dec c
    jp nz,bulkFill
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
    ld hl,(dataBuffer)
    xor a
    ld (index),a
bulkComparePage:
    ld a,(pass)
    ld b,a
    ld a,(index)
    xor b
    ld b,a
    ld c,0
bulkCompare:
    ld a,b
    xor l
    cp (hl)
    jp nz,mismatch
    inc hl
    dec c
    jp nz,bulkCompare
    ld a,(index)
    inc a
    ld (index),a
    ld b,a
    ld a,(config+3)
    cp b
    jp nz,bulkComparePage
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

; Выход CF/A — статус, rawCall настроен для B=1/2/4/5. BC/DE/HL рабочие.
; Блоки 512 байт идут через RAW_IO. Аренда локального входа для блоков
; 256 байт действует до следующего вызова DOS. Список LBA уже подготовлен.
prepareRaw:
    ld a,(sectorPages)
    dec a
    ld hl,rawViaDos
    ld c,0
    jp nz,prepareRawReady
    ld b,RAW_OP_PREPARE
    MXOS_CALL_REGS MXOS_DOS_RAW_IO
    ret c
prepareRawReady:
    ld (rawJump+1),hl
    ld a,c
    ld (rawCall+1),a
    xor a
    ret
rawViaDos:
    MXOS_CALL_REGS MXOS_DOS_RAW_IO
    ret
rawCall:
    ld c,0
rawJump:
    jp 0

; Выход A = число физических секторов испытания, BC/DE/HL сохранены.
rawCount:
    ld a,(sectorPages)
    dec a
    ld a,(config+3)
    ret z
    rrca
    ret

    IFDEF WORKER_MAIN
; Z=1 для системного ROM/flash, CF=0. A разрушен, BC/DE/HL сохранены.
workerMapped:
    ld a,(technology)
    xor DISK_TECH_MX_SYSTEM_FLASH
    ret z
    xor DISK_TECH_MX_SYSTEM_FLASH ^ DISK_TECH_MX_SYSTEM_ROM
    ret

BENCH_HIGH_SCREEN = 09000h
    ASSERT BENCH_HIGH_SCREEN+8193 <= 0C000h

;----------------------------------------------------------------------------
; MXOS
;
; Измерить полный файл через экран для системного ROM/flash.
;
; вход:
;   config задаёт режим, размеры и смещение. LOW-буфер свободен после FILE.
;
; выход:
;   Статистика FILE HIGH дополняется. Экран и рабочие указатели возвращаются
;   в исходное состояние. CF/A — статус обмена, регистры разрушаются.
;----------------------------------------------------------------------------
fileHigh:
    call workerMapped
    ret nz
    ld a,(config+3)
    cp 33
    ret nc
    ld hl,txtScreenBuffer
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING
    ld a,(config+3)
    ld h,a
    ld a,(config+10)
    ld l,a
    call saveHighScreen
    ret c
    ld (dataBuffer),hl
    ld hl,stats+60
    ld (fileStats),hl
    MXOS_CALL_REGS MXOS_DOS_GET_ERROR ; Восстановить COMMAND вне таймера.
    ld a,(config+2)
    or a
    jp z,fileHighRead
    ld a,1
    ld (operation),a
    call prepareFileBuffer
    call filePass
    jp fileHighReturn
fileHighRead:
    call fileReadPass
fileHighReturn:
    call restoreHighScreen
    ld hl,(bufferAddress)
    ld (dataBuffer),hl
    ld hl,stats+20
    ld (fileStats),hl
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Измерить RAW HIGH через экран, сохранив его в начале LOW-буфера.
;
; вход:
;   Выбран системный ROM/flash с физическим блоком 256 байт. Список секторов
;   готов. config+2 задаёт число проходов записи, config+10 — смещение 0/1.
;
; выход:
;   Статистика RAW HIGH дополняется. Экран и bufferAddress восстановлены.
;   CF/A — статус обмена или восстановления, регистры разрушаются.
;----------------------------------------------------------------------------
rawHigh:
    ld hl,257
    call saveHighScreen
    ret c
    ld (bufferAddress),hl
    ld a,(config+2)
    or a
    jp z,rawHighReadStart
    call rawHighWrite
    jp c,rawHighReturn
rawHighReadStart:
    call rawHighRead
rawHighReturn:
    call restoreHighScreen
    ld hl,(dataBuffer)
    ld (bufferAddress),hl
    ret
rawHighWrite:
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
    jp z,rawHighVerify
    or a
    ret
rawHighVerify:
    call begin
    call validateRawData
    ret c
    call timer_sample
    ld de,stats+56
    jp addElapsed
rawHighRead:
    ld a,2
    ld (operation),a
    call prepareRawBuffer
    call begin
    call rawSeries
    ret c
    call timer_sample
    ld de,stats+52
    call addElapsed
    jp validateRawData

;----------------------------------------------------------------------------
; MXOS
;
; Сохранить часть экрана в начале LOW-буфера, затем выбрать чёрный цвет.
;
; вход:
;   hl = длина в байтах с учётом выравнивания, 1..8193. Начало LOW-буфера
;   свободно на эту длину, config+10 задаёт смещение 0/1.
;
; выход:
;   cf = 0: hl = адрес обмена в экране. cf = 1: a = ошибка MEM, цвет не меняется.
;   Остальные регистры разрушаются.
;----------------------------------------------------------------------------
saveHighScreen:
    ld (copyRequest+6),hl
    ld hl,BENCH_HIGH_SCREEN
    ld de,buffer
    call copyHighScreen
    ret c
    xor a
    ld (IO_COLOR),a
    ld hl,BENCH_HIGH_SCREEN
    ld a,(config+10)
    ld l,a
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Вернуть сохранённую часть экрана и цвет консоли.
;
; вход:
;   LOW-буфер содержит снимок, copyRequest+6 — длина снимка, AF — статус I/O.
;
; выход:
;   CF/A возвращают исходный статус при успешном восстановлении, иначе ошибку MEM.
;   Остальные регистры разрушаются. Поля copyRequest подготовлены для запуска.
;----------------------------------------------------------------------------
restoreHighScreen:
    push af
    ld a,COLOR_BIOS
    ld (IO_COLOR),a
    ld hl,buffer
    ld de,BENCH_HIGH_SCREEN
    call copyHighScreen
    ld hl,config
    ld (copyRequest+4),hl
    ld hl,11
    ld (copyRequest+6),hl
    pop bc
    ret c
    push bc
    pop af
    ret

; hl = источник, de = приёмник в MAIN. Длина уже задана, CF/A — статус MEM.
copyHighScreen:
    ld (copyRequest+1),hl
    ex de,hl
    ld (copyRequest+4),hl
    ld hl,copyRequest
    jp mx_memcpyFar
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
    ld hl,(fileStats)
    ld de,24
    add hl,de
    ex de,hl
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

; Собрать список физических секторов собственного DISKBEN.TMP до замера.
; Вход: config+4 = секторов/кластер, config+5 = страниц по 256 байт/сектор.
; Выход: sectors содержит только LBA данных файла. CF/A — ошибка DOS
; или геометрии. Регистры рабочие. При ошибке cleanup закрывает объект.
; READ одного байта материализует курсор DOS. Упакованную FAT читает ядро.
buildChain:
    call bhReadOpen
    ret c
    ld hl,sectors
    ld (list),hl
    xor a
    ld (index),a
chainNext:
    ld a,(index)
    ld h,a
    ld l,0
    ld (bhHandle+FH_SEEK),hl
    ld a,FILE_OP_SEEK
    call bhCall
    ret c
    ld hl,(bufferAddress)
    ld bc,1
    ld a,2
    call bhTransfer
    ret c
    ld hl,(bhHandle+FH_CURSOR)
    ld de,0FFFEh
    add hl,de
    jp nc,chainInvalid            ; Кластеры 0 и 1 не содержат данных.
    ld a,(config+4)
chainScale:
    rrca
    jp c,chainBase
    add hl,hl
    jp nc,chainScale
    jp chainInvalid
chainBase:
    ld a,(bhHandle+FH_CURSOR+3)   ; Страницы с начала кластера после READ 1.
    ld e,a
    ld a,(sectorPages)
    dec a
    ld a,e
    jp z,chainInside
    or a
    rra
chainInside:
    ld e,a
    ld d,0
    add hl,de
    jp c,chainInvalid
    ex de,hl
    ld hl,(config+6)
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
    ld a,(sectorPages)
    add a,(hl)
    ld (hl),a
    ld a,(config+3)
    cp (hl)
    jp nz,chainNext
    jp bhClose
chainInvalid:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret


; Только чтение: до 8 КиБ, число физических секторов зависит от носителя.
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
    ld a,(config+8)
    and 1
    jp z,readOnlyNext
    call rawCount
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
    call workerMapped
    call z,rawHigh
    ret c
    ENDIF
readOnlyFile:
    ld a,(config+8)
    and 2
    jp z,readOnlyDone
    ld hl,descriptor
    MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO
    jp c,readOnlyDone
    call fileReadPass
    ret c
    IFDEF WORKER_MAIN
    call fileHigh
    ret c
    ENDIF
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
    ld a,(technology)
    cp DISK_TECH_RAM
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
    MXOS_CALL_REGS MXOS_DOS_FILEHANDLE
    ret
; Дискеты и разбросанные цепочки медленнее: период 33,55 с вместо 8,39 с.
; В отчёт по-прежнему возвращаются эквивалентные тики по 128 мкс.
timerRange:
    ld a,(sectorPages)
    cp 2
    jp z,timerLongRange
    ld a,(config+9)
    or a
    ret z
timerLongRange:
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
elapsedShift:   DB 0
oldOptions:     DB 0
optionsSaved:   DB 0
txtScattered:   DB "  Allocation: random",0

copyRequest: DB 0
    DW 0
    DB 0
    DW config,11
config:             DS 11,0
bankId:             DB 0
caps:               DB 0
technology:         DB 0
capacity:           DW 0
owned:              DB 0
pass:               DB 0
operation:          DB 0
raw:                DB 0
verify:             DB 0
rawOperation:       DB 0
readUnknown:        DB 0
readOnlyLeft:       DB 0
index:              DB 0
sectorPages:        DB 1
list:               DW 0
dataBuffer:         DW buffer
bufferAddress:      DW buffer
writeStat:          DW 0
verifyStat:         DW 0
flushStart:         DW 0
flushTicks:         DD 0
flushCount:         DW 0
v_elapsed:          DD 0
v_setup:            DD 0
v_last:             DW 0
fileStats:          DW stats+20
    IFNDEF WORKER_MAIN
stats:              DS BENCH_MAIN_STATS_SIZE,0
    ENDIF
descriptor:         DB "DISKBEN TMP",0
    DS 20,0
    ASSERT $-descriptor == FILE_DESCRIPTOR
    IFNDEF WORKER_MAIN
sectors: DS 24*4*2,0
    ENDIF
    IFDEF WORKER_MAIN
txtPass:            DB 10,"MAIN: Pass ",0
txtScreenBuffer:    DB 10,"HIGH USES SCREEN BUFFER",10,0
    ELSE
    IFDEF WORKER_DOS
txtPass: DB 10,"DOS:  Pass ",0
    ELSE
txtPass: DB 10,"BANK: Pass ",0
    ENDIF
    ENDIF
    ALIGN 256
; RAW использует один сектор в начале рабочего буфера.
workerCodeEnd:
    IFDEF WORKER_MAIN
BUFFER_SECTORS = 24*4
    ASSERT $ <= 02B00h
    ; Точный диапазон буфера регистрирует контроллер до вызова DLL.
buffer = BENCH_MAIN_BUFFER_ADDR
    ; После FILE список LBA использует тот же буфер за сектором RAW.
sectors = buffer+514
    ASSERT buffer+257 <= sectors
    ASSERT sectors+2*BUFFER_SECTORS <= buffer+1024
    ELSE
    IFDEF WORKER_DOS
BUFFER_SECTORS = BENCH_DOS_BUFFER_KIB*4
buffer = $
IMAGE_BSS_SIZE = BUFFER_SECTORS*256+1
IMAGE_MEM_END = buffer+IMAGE_BSS_SIZE
    ELSE
BUFFER_SECTORS = 24*4
; EXEC-банк целиком принадлежит DLL; нижняя область свободна от её тела.
buffer = 02000h
    ASSERT buffer+BUFFER_SECTORS*256+1 <= DOS_BANK_DRIVER_ADDR
    ENDIF
    ENDIF
    IFDEF WORKER_MAIN
stats = BENCH_MAIN_STATS_ADDR
workerEnd = buffer+BUFFER_SECTORS*256+1
IMAGE_BSS_SIZE = 0
IMAGE_MEM_END = workerCodeEnd
    ELSE
    IFDEF WORKER_DOS
workerEnd = IMAGE_MEM_END
    ELSE
workerEnd = $
    ENDIF
    ENDIF
    IFDEF WORKER_MAIN
    ASSERT workerEnd <= LEGACY_APP_MEMORY_END
    ELSE
    IFDEF WORKER_DOS
    ASSERT workerEnd <= DOS_BANK_MODULE_END
    ELSE
    ASSERT workerEnd <= DOS_BANK_STACK_BOTTOM
    ENDIF
    ENDIF
