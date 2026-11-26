; CHKDSK: диагностика текущего тома MXOS, без операций записи.
; Физический вход bios_diskDriver, сектор/кластер 256 байт, одна FAT16-подобная таблица.
    INCLUDE "../../sdk/asm/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    MACRO LOAD_DE address
    push hl
    ld hl,(address)
    ex de,hl
    pop hl
    ENDM
    MACRO STORE_DE address
    push hl
    ex de,hl
    ld (address),hl
    ex de,hl
    pop hl
    ENDM
    ORG 0

;----------------------------------------------------------------------------
; MXOS
;
; Проверить геометрию и файловые цепочки тома из MAIN.
;
; вход:
;   de = адрес строки с буквой диска либо пустой строки для запроса с клавиатуры. Для
;   карты кластеров необходима память до IMAGE_MEM_END.
;
; выход:
;   Результаты выведены на экран. При обычном завершении после нажатия клавиши
;   восстановлены кодировка и SP вызывающего; проверка читает секторы тома.
;----------------------------------------------------------------------------

entry:
    STORE_DE argumentPtr
    di
    ld hl,0
    add hl,sp
    ld (callerSP),hl
    ; Карта кластеров и секторные буферы требуют верхнего прикладного ОЗУ.
    ld a,(bios_upper.koi7)
    ld (callerKoi),a
    xor a
    call bios_setFontEncoding       ; Имена файлов на диске хранятся в KOI-8.
    call bios_getMemTop
    ld de,IMAGE_MEM_END-1
    call bios_cmp_hl_de
    jp nc,checkMemoryReady
    ld hl,txtNoMemory
    call puts
    jp waitKey
checkMemoryReady:
    ld hl,txtTitle
    call puts
    ; MXOS передаёт адрес аргументов в DE, длина в BC не определена.
readArgument:
    ld hl,(argumentPtr)
    call bios_argNext
    jp c,askDrive
    ld a,(de)
    jp chooseDrive
askDrive:
    ld hl,txtDrive
    call puts
    ; Штатный редактор: поле в один символ с отображением и Enter.
    ld hl,driveInput
    ld de,driveInput+1
    call bios_input
    ld a,(driveInput)
chooseDrive:
    and 0DFh
    sub 'A'
    cp 8
    jp nc,askDrive
    ld e,1
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
scan:
    xor a
    ld (phase),a
    ld hl,counts
    ld b,countsEnd-counts
clearCounts:
    ld (hl),a
    inc hl
    dec b
    jp nz,clearCounts
    ld b,3
    call bios_diskDriver
    jp c,ioError
    ld bc,DISK_INFO.blockShift
    add hl,bc
    ld a,(hl)
    cp 8
    jp nz,badGeometry         ; Проверяем до чтения в 256-байтный sectorBuffer.
    ex de,hl
    ld (capacity),hl
    ld de,0
    ld hl,sectorBuffer
    call readSector
    call geometry
    jp c,badGeometry
    ; Собственный односекторный кэш FAT. FFh вне диапазона секторов FAT.
    ld a,0FFh
    ld (fatCachedSector),a
    ld de,0
    call fatNext
    ex de,hl
    ld de,0FFF8h
    call bios_sub_hl_de
    ld a,h
    or l
    call nz,reservedError
    ld de,1
    call fatNext
    ex de,hl
    inc hl
    ld a,h
    or l
    call nz,reservedError
    xor a
    ld hl,usedMap
    ld de,usedMap+1024
clearMap:
    xor a
    ld (hl),a
    inc hl
    call compare
    jp nz,clearMap
    ld hl,names
    ld (namesEnd),hl
    ld hl,txtStructure
    call puts
    xor a
    ld (phase),a
    call directory
    call lostClusters
    ld hl,(errors)
    ld a,h
    or l
    jp nz,structureFailed
    ld hl,txtChecksums
    call puts
    ld a,1
    ld (phase),a
    call directory
    jp finished
structureFailed:
    ld hl,txtSkipCS
    call puts
finished:
    ld hl,txtErrors
    call puts
    ld hl,(errors)
    call printNumber
    ld hl,txtChecked
    call puts
    ld hl,(checked)
    call printNumber
    ld hl,txtSkipped
    call puts
    ld hl,(skipped)
    call printNumber
waitPrompt:
    ld hl,txtDone
    call puts
waitKey:
    call bios_getch
    ld c,0Ah
    call bios_printChar
    ld a,(callerKoi)
    call bios_setFontEncoding
    ld hl,(callerSP)
    ld sp,hl
    or a
    ret
badGeometry:
    ld a,3
    ld (result),a
    ld hl,txtFormat
    call puts
    jp waitPrompt
ioError:
    ld a,4
    ld (result),a
    ld hl,txtIO
    call puts
    jp waitPrompt

; DE=физический сектор, HL=буфер; BC/DE/HL сохраняются драйвером.

;----------------------------------------------------------------------------
; MXOS
;
; Прочитать сектор проверяемого диска через физический драйвер.
;
; вход:
;   de = номер сектора, hl = доступный для записи секторный буфер. Выбран диск,
;   геометрия которого используется проверкой.
;
; выход:
;   При успехе буфер заполнен и bc восстановлен. При ошибке ввода-вывода управление
;   передаётся общей диагностике ioError.
;----------------------------------------------------------------------------

readSector:
    push bc
    ld b,2
    or a                  ; Исторические драйверы сохраняют входной CF.
    call bios_diskDriver
    pop bc
    jp c,ioError
    ret
; HL=беззнаковое число; печать штатным сервисом DE/B.
printNumber:
    ex de,hl
    ld b,0
    jp bios_printDecWord
puts:
    jp bios_printString
compare:
    jp bios_cmp_hl_de

; Проверяем границу поддерживаемого формата до чтения его таблиц.
; CF=1: геометрия вне текущего формата; AF/BC/DE/HL разрушаются.
geometry:
    ld hl,(sectorBuffer+11)
    ld de,256
    call compare
    jp nz,invalid
    ld a,(sectorBuffer+13)
    cp 1
    jp nz,invalid
    ld a,(sectorBuffer+16)
    cp 1
    jp nz,invalid
    ld hl,sectorBuffer+3
    ld de,txtFat
    ld b,8
checkType:
    ld a,(de)
    cp (hl)
    jp nz,invalid
    inc hl
    inc de
    dec b
    jp nz,checkType
    ld hl,(sectorBuffer+19)
    ld de,8193
    call compare
    jp nc,invalid
    ld de,16
    call compare
    jp c,invalid
    ld (total),hl
    LOAD_DE capacity
    call compare
    jp c,totalOK
    jp nz,invalid
totalOK:
    ld hl,(sectorBuffer+14)
    ld a,h
    or l
    jp z,invalid
    ld (fatStart),hl
    ld hl,(sectorBuffer+22)
    ld a,h
    or a
    jp nz,invalid
    ld a,l
    or a
    jp z,invalid
    cp 65
    jp nc,invalid
    ld (fatSectors),a
    LOAD_DE fatStart
    add hl,de
    jp c,invalid
    ld (dirStart),hl
    ld hl,(sectorBuffer+17)
    ld a,h
    cp 1
    jp c,rootOK
    jp nz,invalid
    ld a,l
    or a
    jp nz,invalid
rootOK:
    ld (rootEntries),hl
    ld a,h
    or l
    jp z,invalid
    ld de,7
    add hl,de
    ld b,3
rootDivide:
    or a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    dec b
    jp nz,rootDivide
    ld (dirSectors),hl
    LOAD_DE dirStart
    add hl,de
    jp c,invalid
    ld (dataStart),hl
    ex de,hl
    ld hl,(total)
    call bios_sub_hl_de
    jp c,invalid
    ld a,h
    or l
    jp z,invalid
    inc hl
    ld (lastCluster),hl
    ; FAT должна вмещать lastCluster+1 ячеек.
    ld a,(fatSectors)
    ld d,a
    ld e,0
    ex de,hl
    call checkMath.div_hl_2
    ex de,hl
    ld hl,(lastCluster)
    call compare
    jp nc,invalid
    or a
    ret
invalid:
    scf
    ret

; Два прохода по корню. В phase=0 проверяются записи/цепочки,
; в phase=1 только контрольные суммы уже проверенных цепочек.
directory:
    xor a
    ld (directoryDotMask),a
    ld hl,directoryQueue
    ld (queueRead),hl
    ld (queueWrite),hl
    ld hl,0
    ld (currentDirectory),hl
    ld (currentParent),hl
    ld hl,names
    ld (namesEnd),hl
    ld hl,(rootEntries)
    ld (dirEntriesLeft),hl
    ld hl,(dirStart)
    ld (dirSector),hl
    ld hl,(dirSectors)
    ld (dirLeft),hl
nextDirSector:
    LOAD_DE dirSector
    ld hl,dirBuffer
    call readSector
    ld hl,dirBuffer
    ld (entryPtr),hl
    ld a,8
    ld (entriesLeft),a
nextEntry:
    ld hl,(dirEntriesLeft)
    ld a,h
    or l
    jp z,directoryComplete
    dec hl
    ld (dirEntriesLeft),hl
    ld hl,(entryPtr)
    ld a,(hl)
    cp 0FFh
    jp z,skipEntry
    cp 0E5h
    jp z,skipEntry
    or a
    jp z,directoryComplete
    ld de,entryCopy
    ld b,32
copyEntry:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,copyEntry
    ld a,(entryCopy)
    cp 5
    jp nz,entryFirstReady
    ld a,0E5h
    ld (entryCopy),a
entryFirstReady:
    ld a,(entryCopy+11)
    cp 00Fh
    jp z,skipEntry
    and 018h
    jp z,regularFile
    cp 8
    jp z,skipEntry
    cp 010h
    jp z,subdirectoryEntry
    jp badEntry
regularFile:
    ld a,(phase)
    or a
    jp nz,checksumFile
    call duplicateName
    call checkChain
    jp skipEntry
badEntry:
    call structureError
    ld hl,txtEntry
    call puts
    call printRecord
skipEntry:
    ld hl,(entryPtr)
    ld de,32
    add hl,de
    ld (entryPtr),hl
    ld a,(entriesLeft)
    dec a
    ld (entriesLeft),a
    jp nz,nextEntry
    ld hl,(currentDirectory)
    ld a,h
    or l
    jp nz,subdirectoryNextCluster
    ld hl,(dirSector)
    inc hl
    ld (dirSector),hl
    ld hl,(dirLeft)
    dec hl
    ld (dirLeft),hl
    ld a,h
    or l
    jp nz,nextDirSector
    jp directoryComplete
subdirectoryNextCluster:
    LOAD_DE scanCluster
    call fatNext
    ld a,d
    cp 0FFh
    jp nz,subdirectoryFollow
    ld a,e
    cp 0F8h
    jp nc,directoryComplete
subdirectoryFollow:
    call validCluster
    jp c,directoryComplete
    STORE_DE scanCluster
    call directorySectorFromCluster
    jp nextDirSector
; Очередь вместо рекурсии: глубина каталогов не расходует системный стек.
directoryComplete:
    ld hl,(currentDirectory)
    ld a,h
    or l
    jp z,directoryQueueNext
    ld a,(directoryDotMask)
    cp 3
    jp z,directoryQueueNext
    call structureError
    ld hl,txtDirectoryDots
    call puts
directoryQueueNext:
    ld hl,(queueRead)
    LOAD_DE queueWrite
    call compare
    ret z
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    STORE_DE currentDirectory
    STORE_DE scanCluster
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    STORE_DE currentParent
    ld (queueRead),hl
    LOAD_DE scanCluster
    call directorySectorFromCluster
    ld hl,0FFFFh
    ld (dirEntriesLeft),hl
    xor a
    ld (directoryDotMask),a
    ld hl,names
    ld (namesEnd),hl
    jp nextDirSector
; DE=кластер, один логический сектор на кластер в поддерживаемой геометрии.
directorySectorFromCluster:
    ld h,d
    ld l,e
    dec hl
    dec hl
    LOAD_DE dataStart
    add hl,de
    ld (dirSector),hl
    ret
subdirectoryEntry:
    ld hl,(entryCopy+20)
    ld a,h
    or l
    jp nz,badEntry
    ld hl,(entryCopy+28)
    LOAD_DE entryCopy+30
    ld a,h
    or l
    or d
    or e
    jp nz,badEntry
    ld a,(entryCopy)
    cp '.'
    jp z,directoryDotEntry
    ld a,(phase)
    or a
    jp nz,directoryEnqueue
    call duplicateName
    LOAD_DE entryCopy+26
directoryCheckChain:
    call validCluster
    jp c,directoryChainBad
    call markCluster
    jp nz,directoryChainBad
    call fatNext
    ld a,d
    cp 0FFh
    jp nz,directoryCheckChain
    ld a,e
    cp 0F8h
    jp c,directoryChainBad
directoryEnqueue:
    ld hl,(queueWrite)
    ld de,directoryQueueEnd-4
    call compare
    jp c,directoryQueueRoom
    jp nz,badEntry
directoryQueueRoom:
    LOAD_DE entryCopy+26
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    LOAD_DE currentDirectory
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (queueWrite),hl
    jp skipEntry
directoryChainBad:
    call chainError
    jp skipEntry
directoryDotEntry:
    ld hl,(currentDirectory)
    ld a,h
    or l
    jp z,badEntry
    ld a,(entryCopy+1)
    cp ' '
    ld c,1
    jp z,directoryDotCheck
    cp '.'
    jp nz,badEntry
    ld a,(entryCopy+2)
    cp ' '
    jp nz,badEntry
    ld hl,(currentParent)
    ld c,2
directoryDotCheck:
    ld a,(directoryDotMask)
    and c
    jp nz,badEntry
    ld a,(directoryDotMask)
    or c
    ld (directoryDotMask),a
    LOAD_DE entryCopy+26
    call compare
    jp nz,badEntry
    jp skipEntry

; Все имена в корне: до 256*11 байт, временная память приложения.
duplicateName:
    ld hl,names
dupNext:
    LOAD_DE namesEnd
    call compare
    jp z,appendName
    ld de,entryCopy
    push hl
    ld c,11
    call bios_stricmp
    pop hl
    call z,duplicateError
    ld de,11
    add hl,de
    jp dupNext
appendName:
    push hl
    ld de,11
    add hl,de
    ld de,directoryQueue
    call compare
    pop hl
    jp c,appendNameRoom
    jp z,appendNameRoom
    call structureError
    ld hl,txtDirectoryLimit
    call puts
    ret
appendNameRoom:
    ld de,entryCopy
    ld b,11
appendByte:
    ld a,(de)
    ld (hl),a
    inc hl
    inc de
    dec b
    jp nz,appendByte
    ld (namesEnd),hl
    ret

; DE=кластер -> DE=FAT[кластер], HL/AF разрушаются.
fatNext:
    ld h,d
    ld l,e
    add hl,hl
    ld a,(fatCachedSector)
    cp h
    jp z,fatNextCached
    ld a,h
    ld (fatCachedSector),a
    push hl
    ld e,h
    ld d,0
    ld hl,(fatStart)
    add hl,de
    ex de,hl
    ld hl,fatBuffer
    call readSector
    pop hl
fatNextCached:
    ld h,fatBuffer >> 8
    ld e,(hl)
    inc hl
    ld d,(hl)
    ret
; DE=кластер. CF=1 вне области данных; HL/AF разрушаются.

;----------------------------------------------------------------------------
; MXOS
;
; Проверить, находится ли кластер в области данных проверяемого тома.
;
; вход:
;   de = проверяемый номер, lastCluster = последняя допустимая запись.
;
; выход:
;   cf = 0 для диапазона 2..lastCluster, cf = 1 вне его. de сохранён; hl и af рабочие.
;----------------------------------------------------------------------------

validCluster:
    ld hl,2
    call compare
    jp z,validTop
    jp nc,invalid
validTop:
    ld hl,(lastCluster)
    call compare
    ret
; DE=кластер. Z=уже посещён; AF/BC/HL разрушаются, DE сохранён.
markCluster:
    ld a,e
    and 7
    ld b,a
    ld a,1
    inc b
bitShift:
    dec b
    jp z,bitReady
    add a,a
    jp bitShift
bitReady:
    ld c,a
    ld h,d
    ld l,e
    ld b,3
mapDivide:
    or a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    dec b
    jp nz,mapDivide
    ld a,h
    add a,usedMap >> 8
    ld h,a
    ld a,(hl)
    and c
    push af
    ld a,(hl)
    or c
    ld (hl),a
    pop af
    ret
checkChain:
    ; Точный 16-битный размер N; старшее слово пока не поддерживается сканером.
    ld hl,(entryCopy+30)
    ld a,h
    or l
    jp nz,chainError
    ld hl,(entryCopy+20)
    ld a,h
    or l
    jp nz,chainError
    ld hl,(entryCopy+28)
    ld a,h
    or l
    jp nz,chainNonempty
    LOAD_DE entryCopy+26
    ld a,d
    or e
    ret z
    jp chainError
chainNonempty:
    ld a,l
    ld l,h
    ld h,0
    or a
    jp z,chainCountReady
    inc hl
chainCountReady:
    ld (chainLeft),hl
    LOAD_DE entryCopy+26
chainLoop:
    call validCluster
    jp c,chainError
    call markCluster
    jp nz,chainError
    ; markCluster возвращает Z для новой ячейки (биты до записи были нулём).
    STORE_DE cluster
    call fatNext
    ld hl,(chainLeft)
    dec hl
    ld (chainLeft),hl
    ld a,h
    or l
    jp z,chainLast
    ld hl,(cluster)
    call compare
    jp z,chainError
    ld a,d
    cp 0FFh
    jp z,chainError
    jp chainLoop
chainLast:
    ld a,d
    cp 0FFh
    jp nz,chainError
    ld a,e
    cp 0F8h
    ret nc
chainError:
    call structureError
    ld hl,txtChain
    call puts
    call printRecord
    ret
duplicateError:
    push bc
    push de
    push hl
    call structureError
    ld hl,txtDuplicate
    call puts
    call printRecord
    pop hl
    pop de
    pop bc
    ret
lostClusters:
    ld de,2
lostLoop:
    push de
    call fatNext
    ld a,d
    or e
    jp z,lostNext
    ld a,d
    cp 0FFh
    jp nz,lostAllocated
    ld a,e
    cp 0F7h
    jp z,lostNext
lostAllocated:
    pop de
    push de
    call markCluster
    call z,lostError
lostNext:
    pop de
    inc de
    ld hl,(lastCluster)
    call compare
    jp nc,lostLoop
    ret
lostError:
    call structureError
    push hl
    ld hl,txtLost
    call puts
    ex de,hl
    call bios_printHexWord
    ex de,hl
    pop hl
    ret
reservedError:
    call structureError
    ld hl,txtReserved
    jp puts

;----------------------------------------------------------------------------
; MXOS
;
; Учесть структурную ошибку текущей фазы проверки диска.
;
; вход:
;   phase = индекс фазы, errors = 16-битный счётчик ошибок.
;
; выход:
;   result = phase + 1, errors увеличен на единицу. Регистры и флаги сохранены.
;----------------------------------------------------------------------------

structureError:
    push af
    ld a,(phase)
    inc a
    ld (result),a
    pop af
    push hl
    ld hl,(errors)
    inc hl
    ld (errors),hl
    pop hl
    ret

; Чтение потока по уже проверенной цепочке. Выход A=байт, остальные регистры
; сохраняются. Ошибка чтения немедленно прерывает утилиту, диск не изменяется.

;----------------------------------------------------------------------------
; MXOS
;
; Взять следующий байт файла из предварительно проверенной цепочки.
;
; вход:
;   streamPtr и состояние потока указывают на следующий байт. При переходе на другой
;   сектор требуется доступ к выбранному диску.
;
; выход:
;   a = прочитанный байт, позиция потока продвинута. hl, de и bc сохранены. Ошибка
;   чтения передаётся общей диагностике и прерывает проверку.
;----------------------------------------------------------------------------

streamByte:
    push hl
    push de
    push bc
    ld hl,(streamPtr)
    ld a,h
    cp (fileBuffer+256) >> 8
    jp nz,streamLoaded
    ld hl,(streamCluster)
    dec hl
    dec hl
    LOAD_DE dataStart
    add hl,de
    ex de,hl
    ld hl,fileBuffer
    call readSector
    LOAD_DE streamCluster
    call fatNext
    STORE_DE streamCluster
    ld hl,fileBuffer
streamLoaded:
    ld a,(hl)
    inc hl
    ld (streamPtr),hl
    pop bc
    pop de
    pop hl
    ret
checksumFile:
    LOAD_DE entryCopy+26
    STORE_DE streamCluster
    ld hl,fileBuffer+256
    ld (streamPtr),hl
    ; Read the common 16-byte prefix, then the rest of a v2 header if present.
    ld hl,(entryCopy+28)
    ld de,16
    call compare
    jp c,tryRKS
    ld hl,header
    ld b,16
headerRead:
    call streamByte
    ld (hl),a
    inc hl
    dec b
    jp nz,headerRead
    ld hl,header
    ld de,mxMagic
    ld b,4
magicLoop:
    ld a,(de)
    cp (hl)
    jp nz,resetRKS
    inc hl
    inc de
    dec b
    jp nz,magicLoop
    ld a,(hl)
    cp 2
    jp z,checksumMxV2
    dec a
    jp nz,resetRKS
    inc hl
    ld a,(hl)
    and 0FEh
    jp nz,sumBad
    ld de,16
    jp checksumMx
checksumMxV2:
    ld hl,(entryCopy+28)
    ld de,IMAGE_HEADER_SIZE
    call compare
    jp c,sumBad
    ld hl,header+16
    ld b,IMAGE_HEADER_SIZE-16
checksumMxReadTail:
    call streamByte
    ld (hl),a
    inc hl
    dec b
    jp nz,checksumMxReadTail
    ld a,(header+IMAGE_HEADER_BYTES)
    cp IMAGE_HEADER_SIZE
    jp nz,sumBad
    ld de,IMAGE_HEADER_SIZE
checksumMx:
    ld hl,(header+10)
    ld a,h
    or l
    jp z,sumBad
    ld (payloadLeft),hl
    push de
    add hl,de
    jp nc,checksumMxLength
    pop de
    jp sumBad
checksumMxLength:
    LOAD_DE entryCopy+28
    call compare
    pop de
    jp nz,sumBad
    ld hl,header-3
    add hl,de
    push hl
    ex de,hl
    ld hl,header
    call bios_calcCS
    pop hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld d,b
    ld e,c
    call compare
    jp nz,sumBad
    ld hl,(header+12)
    ld (expected),hl
    jp sumPayload
resetRKS:
    LOAD_DE entryCopy+26
    STORE_DE streamCluster
    ld hl,fileBuffer+256
    ld (streamPtr),hl
tryRKS:
    ld hl,entryCopy+8
    ld de,rksExt
    ld b,3
rksExtension:
    ld a,(hl)
    and 0DFh
    ld c,a
    ld a,(de)
    cp c
    jp nz,noSum
    inc de
    inc hl
    dec b
    jp nz,rksExtension
    ; Старые образы содержат сырой RKS payload: распознаём контейнер только
    ; при согласованной длине. Без контейнера не выдаём ложного подтверждения CS.
    ld hl,(entryCopy+28)
    ld de,6
    call compare
    jp c,noSum
    call streamWord
    push hl
    call streamWord
    pop de
    call bios_sub_hl_de
    jp c,noSum
    inc hl
    ld a,h
    or l
    jp z,noSum
    ld (payloadLeft),hl
    ld de,6
    add hl,de
    jp c,noSum
    LOAD_DE entryCopy+28
    call compare
    jp nz,noSum
    ld a,1
    ld (rksMode),a
    jp sumPayloadStart
sumPayload:
    xor a
    ld (rksMode),a
sumPayloadStart:
    ; BIOS CS применяется к порциям с нулевым последним байтом, кроме
    ; последней порции: B учитывает каждый байт, кроме последнего в файле.
    ld hl,0
    ld (runningSum),hl
sumChunk:
    ld hl,sumBuffer
    ld b,0
sumRead:
    call streamByte
    ld (hl),a
    inc hl
    inc b
    push hl
    ld hl,(payloadLeft)
    dec hl
    ld (payloadLeft),hl
    ld a,h
    or l
    pop hl
    jp z,sumLast
    ld a,b
    or a
    jp nz,sumRead
    ld (hl),0
    ex de,hl
    jp sumCalc
sumLast:
    dec hl
    ex de,hl
sumCalc:
    ld a,(de)
    ld (sumLastByte),a
    ld hl,sumBuffer
    call bios_calcCS
    ld hl,(runningSum)
    ld a,(sumLastByte)
    ld e,a
    ld a,c
    sub e
    add a,l
    ld a,h
    adc a,b
    ld h,a
    ld a,l
    add a,c
    ld l,a
    ld (runningSum),hl
    ld hl,(payloadLeft)
    ld a,h
    or l
    jp nz,sumChunk
    ld a,(rksMode)
    or a
    jp z,sumCompare
    call streamWord
    ld (expected),hl
sumCompare:
    ld hl,(expected)
    LOAD_DE runningSum
    call compare
    jp nz,sumBad
    ld hl,(checked)
    inc hl
    ld (checked),hl
    ld hl,txtOK
    jp sumPrint
noSum:
    ld hl,(skipped)
    inc hl
    ld (skipped),hl
    ld hl,txtNoSum
    jp sumPrint
sumBad:
    call structureError
    ld hl,txtBadSum
sumPrint:
    call puts
    call printRecord
    jp skipEntry
printRecord:
    ld hl,entryCopy
    ld b,8
    call printName
    ld a,(hl)
    cp ' '
    ret z
    ld c,'.'
    call bios_printChar
    ld b,3
printName:
    ld a,(hl)
    cp ' '
    jp z,printNameNext
    ld c,a
    call bios_printChar
printNameNext:
    inc hl
    dec b
    jp nz,printName
    ret
streamWord:
    call streamByte
    ld l,a
    call streamByte
    ld h,a
    ret

mxMagic:        DB 0CDh,020h,"MX"
rksExt:         DB "RKS"
txtFat:         DB "MXOS256 "
txtTitle:       DB "CHKDSK - READ ONLY",0
txtDone:        DB 10,"PRESS ANY KEY TO EXIT",0
txtDrive:       DB 10,"CHKDSK DRIVE [A-H], ENTER: ",0
txtStructure:   DB 10,"STRUCTURE...",0
txtReserved:    DB 10,"INVALID RESERVED FAT CELL",0
txtEntry:       DB 10,"INVALID DIRECTORY ENTRY: ",0
txtSkipCS:      DB 10,"STRUCTURE ERRORS: CHECKSUMS SKIPPED",0
txtChecksums:   DB 10,"FILE CHECKSUMS...",0
txtErrors:      DB 10,"ERRORS: ",0
txtChecked:     DB "  CS OK: ",0
txtSkipped:     DB "  NO CS: ",0
txtChain:       DB 10,"CHAIN/SIZE/CYCLE/CROSS-LINK: ",0
txtDuplicate:   DB 10,"DUPLICATE NAME: ",0
txtLost:        DB 10,"LOST CLUSTER: ",0
txtOK:          DB 10,"CS OK: ",0
txtNoSum:       DB 10,"NO STORED CHECKSUM: ",0
txtBadSum:      DB 10,"BAD CHECKSUM/HEADER: ",0
txtFormat:      DB 10,"UNSUPPORTED/INVALID MXOS BPB",0
txtIO:          DB 10,"DISK READ ERROR",0
counts:
errors:         DW 0
checked:        DW 0
skipped:        DW 0
result:         DB 0
countsEnd:
capacity:           DW 0
total:              DW 0
fatStart:           DW 0
fatSectors:         DB 0
fatCachedSector:    DB 0
dirStart:           DW 0
dirSectors:         DW 0
rootEntries:        DW 0
dataStart:          DW 0
lastCluster:        DW 0
dirSector:          DW 0
dirLeft:            DW 0
dirEntriesLeft:     DW 0
entryPtr:           DW 0
entriesLeft:        DB 0
phase:              DB 0
namesEnd:           DW 0
cluster:            DW 0
chainLeft:          DW 0
streamCluster:      DW 0
streamPtr:          DW 0
payloadLeft:        DW 0
expected:           DW 0
runningSum:         DW 0
rksMode:            DB 0
sumLastByte:        DB 0
driveInput:         DB 0,0
directoryDotMask:   DB 0
txtDirectoryDots:   DB 10,"MISSING . OR .. DIRECTORY ENTRY",0
currentDirectory:   DW 0
currentParent:      DW 0
scanCluster:        DW 0
queueRead:          DW 0
queueWrite:         DW 0
txtDirectoryLimit:  DB 10,"DIRECTORY SCAN LIMIT",0
argumentPtr:        DW 0
callerSP:           DW 0
callerKoi:          DB 0
txtNoMemory:        DB "NOT ENOUGH MEMORY",0

    MODULE checkMath
    DEFINE MATH_DIV2
    INCLUDE "../../sdk/asm/lib/math.inc"
    ENDMODULE

codeEnd:
names             = (codeEnd+255) & 0FF00h
directoryQueue    = names+1000h
directoryQueueEnd = directoryQueue+1000h
fatBuffer         = directoryQueueEnd
usedMap           = fatBuffer+256
sectorBuffer      = usedMap+1024
dirBuffer         = sectorBuffer+256
entryCopy         = dirBuffer+256
fileBuffer        = entryCopy+256
sumBuffer         = fileBuffer+256
header            = sumBuffer+512
IMAGE_MEM_END     = header+256
    END
