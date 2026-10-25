;----------------------------------------------------------------------------
; Запуск программы с отдельным ядром RAMFOS из FAT-диска MXOS.
; Вход: DE = аргументы команды RAMRUN.COM <файл> [аргументы].
; При ошибке до замены ядра выводится сообщение и управление возвращается DOS.
; После начала загрузки ядра ошибки возвращают машину к загрузчику ПЗУ.
; Регистры и флаги разрушаются. Переходник временно занимает 8C00h-8DFFh.
;----------------------------------------------------------------------------
    INCLUDE "../include/mxos.inc"
    INCLUDE "../include/ramfos.inc"

    ORG 0F800h
start:
    ex de, hl
skipSpaces:
    ld a, (hl)
    cp ' '
    jp nz, haveArgument
    inc hl
    jp skipSpaces
haveArgument:
    cp ' '+1
    ld de, txtUsage
    jp c, printError

    ld de, appDescriptor
    call bios_fileNamePrepare
    ld (argumentPtr), hl
    ld e, 0
    call bios_fileGetSetDrive
    ld (appDrive), a
    ld hl, appDescriptor
    call bios_fileLoadInfo
    ld de, txtNotFound
    jp c, printError

    ; Ненулевой 16-битный размер, без переполнения конечного адреса.
    ld hl, (appDescriptor+FILE_DESCRIPTOR.size+2)
    ld a, h
    or l
    jp nz, invalidRange
    ld hl, (appDescriptor+FILE_DESCRIPTOR.size)
    ; В текущем формате MXOS хранится последний индекс байта, то есть N-1.
    ; После приведения FAT16 к стандарту эта поправка должна быть удалена.
    inc hl
    ld (appSize), hl
    ld a, h
    or l
    jp z, invalidRange
    ld hl, (appDescriptor+FILE_DESCRIPTOR.loadAddress)
    ld (appEntry), hl
    ex de, hl
    ld hl, (appSize)
    add hl, de
    jp c, invalidRange
    ld (appEnd), hl

    ; Низкое ОЗУ: [0000h,8C00h). Промежуток ядра: [D300h,F800h).
    ld hl, (appEntry)
    ld de, RAMFOS_CORE_ADDR+RAMFOS_CORE_SIZE
    call bios_cmp_hl_de
    jp nc, highProgram
    ld hl, (appEnd)
    ld de, RAMFOS_BRIDGE
    call bios_cmp_hl_de
    jp c, lowProgram
    jp nz, invalidRange
lowProgram:
    ld hl, (appEntry)
    jp rangeValid
highProgram:
    ld hl, (appEnd)
    ld de, RAMFOS_HIGH_ADDR
    call bios_cmp_hl_de
    jp c, highProgramValid
    jp nz, invalidRange
highProgramValid:
    ld hl, 0                  ; верхнюю программу пока размещаем с нуля
rangeValid:
    ld (appBuffer), hl

    ; Проверить ядро до загрузки программы и до разрушения шрифта/драйверов.
    call prepareKernel
    call bios_fileLoadInfo
    ld de, txtKernel
    jp c, printError
    ld hl, (kernelDescriptor+FILE_DESCRIPTOR.size+2)
    ld a, h
    or l
    jp nz, invalidKernel
    ld hl, (kernelDescriptor+FILE_DESCRIPTOR.size)
    ld de, RAMFOS_IMAGE_SIZE-1
    call bios_cmp_hl_de
    jp nz, invalidKernel

    ; Исходный накопитель программы восстановлен явно: ядро всегда с A:.
    ld a, (appDrive)
    ld e, 1
    call bios_fileGetSetDrive
    ld hl, (appBuffer)
    ex de, hl
    ld hl, appDescriptor
    call bios_fileLoad2
    ld de, txtReadError
    jp c, printError

    ; Аргументы копируются до того, как ядро затрёт командную строку MXOS.
    ld hl, (argumentPtr)
    ld de, RAMFOS_ARGS
    ld b, 127
copyArguments:
    ld a, (hl)
    cp 20h
    jp c, endArguments
    ld (de), a
    inc hl
    inc de
    dec b
    jp nz, copyArguments
endArguments:
    xor a
    ld (de), a
    ld hl, (appEntry)
    ld (RAMFOS_CONTEXT), hl
    ld hl, (appSize)
    ld (RAMFOS_CONTEXT+2), hl

    call prepareKernel
    ld de, RAMFOS_TEMP
    call bios_fileLoad2
    jp c, coldReturn
    ; Защита от файла правильной длины, но с посторонним содержимым.
    ld a, (RAMFOS_TEMP+RAMFOS_CORE_SIZE)
    cp 0C3h
    jp nz, coldReturn
    ld a, (RAMFOS_TEMP+RAMFOS_CORE_SIZE+RAMFOS_START-RAMFOS_HIGH_ADDR)
    cp 0C3h
    jp nz, coldReturn

    ld hl, bridgeSource
    ld de, RAMFOS_BRIDGE
    ld bc, bridgeEnd-bridge
    call copyBytes
    jp RAMFOS_BRIDGE

; Входа нет. HL = дескриптор RAMFOS.SYS, активен A:. Регистры разрушаются.
prepareKernel:
    ld hl, kernelPath
    ld de, kernelDescriptor
    call bios_fileNamePrepare
    ld hl, kernelDescriptor
    ret

invalidRange:
    ld de, txtRange
    jp printError
invalidKernel:
    ld de, txtKernel
printError:
    ex de, hl
    jp bios_printString

; Нужен холодный старт: временный буфер уже мог затереть шрифт и драйверы.
coldReturn:
    di
    ld hl, romStub
    ld de, bios_RAMDRead
    ld bc, romStubEnd-romStub
    call copyBytes
    jp bios_RAMDRead
romStub:
    ld (IO_PAGE_ROM), a
    jp 0
romStubEnd:

; BC > 0, HL = источник, DE = назначение. Разрушаются AF/BC/DE/HL.
; 11 байт, 48 тактов на байт плюс RET (10), одно чтение и одна запись ОЗУ.
copyBytes:
    ld a, (hl)
    ld (de), a
    inc hl
    inc de
    dec bc
    ld a, b
    or c
    jp nz, copyBytes
    ret

; Копия выполняется в нижнем ОЗУ. Ядро затирает и загрузчик, и стек MXOS.
bridgeSource:
    DISP RAMFOS_BRIDGE
bridge:
    di
    ld sp, RAMFOS_BRIDGE_SP
    ld hl, RAMFOS_TEMP
    ld de, RAMFOS_CORE_ADDR
    ld bc, RAMFOS_CORE_SIZE
    call bridgeCopy
    ld hl, RAMFOS_TEMP+RAMFOS_CORE_SIZE
    ld de, RAMFOS_HIGH_ADDR
    ld bc, RAMFOS_HIGH_SIZE
    call bridgeCopy
    ld hl, (RAMFOS_CONTEXT)
    ex de, hl
    ld a, d
    cp 0D3h
    jp c, bridgeStart
    ld hl, (RAMFOS_CONTEXT+2)
    ld b, h
    ld c, l
    ld hl, 0
    call bridgeCopy
bridgeStart:
    ld hl, (RAMFOS_CONTEXT)
    ld de, RAMFOS_ARGS
    jp RAMFOS_START
bridgeCopy:
    ld a, (hl)
    ld (de), a
    inc hl
    inc de
    dec bc
    ld a, b
    or c
    jp nz, bridgeCopy
    ret
bridgeEnd:
    ENT
    ASSERT bridgeEnd <= RAMFOS_ARGS

kernelPath:       DB "A:RAMFOS.SYS",0
txtUsage:         DB 0Ah,"USAGE: RAMRUN.COM <FILE> [ARGS]",0
txtNotFound:      DB 0Ah,"FILE NOT FOUND",0
txtKernel:        DB 0Ah,"BAD OR MISSING A:RAMFOS.SYS",0
txtReadError:     DB 0Ah,"FILE READ ERROR",0
txtRange:         DB 0Ah,"BAD LOAD ADDRESS OR SIZE",0
appDrive:         DB 0
appEntry:         DW 0
appEnd:           DW 0
appSize:          DW 0
appBuffer:        DW 0
argumentPtr:      DW 0
appDescriptor:    BLOCK FILE_DESCR_SIZE,0
kernelDescriptor: BLOCK FILE_DESCR_SIZE,0
    ASSERT $ <= FAT_CACHE_ADDR
    END
