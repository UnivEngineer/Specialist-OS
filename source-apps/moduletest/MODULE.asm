; Один тестовый модуль для main, EXEC-банка и арены DOS. Вход — RET ABI.
    INCLUDE "../../source/include/mxos.inc"
    IF MAIN_MODULE == 2
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
    ORG DOS_BANK_DRIVER_ADDR
    ELSE
    IF MAIN_MODULE == 1
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 06000h
    ELSE
    ORG 00100h
    ENDIF
    ENDIF
entry:
    or a
    jp z,registers
    dec a
    jp z,console
    dec a
    jp z,allocate
    dec a
    jp z,activeUnload
    dec a
    jp z,rejectedBios
    dec a
    jp z,child
    dec a
    jp z,shared
    dec a
    jp z,info
    dec a
    jp z,keepChild
    dec a
    jp z,retain
    dec a
    jp z,driver
    dec a
    jp z,readSector
    dec a
    jp z,readFile
keepChild:
    ld hl,childPath
    ld a,MODULE_TARGET_BANK
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    ret
retain:
    ld de,04444h
    ld a,RESIDENT_RETAIN
    MXOS_CALL MXOS_DOS_RESIDENT
    ret
driver:
    ld hl,driverPath
    MXOS_CALL MXOS_DOS_FILE_EXEC_NATIVE
    ret
readSector:
    ; Только штатный RAW: диск выбирает инициатор, буфер локален DOS-DLL.
    ld b,2
    ld de,0
    ld hl,sectorBuffer
    MXOS_CALL MXOS_DOS_RAW_IO
    ret c
    ld a,(sectorBuffer+11)
    ld l,a
    ld a,(sectorBuffer+12)
    ld h,a
    ret
readFile:
    ld hl,rootPath
    ld a,DIR_OP_CHDIR
    MXOS_CALL MXOS_DOS_DIRECTORY
    ret c
    ld hl,readHandle
    ld a,FILE_OP_OPEN
    MXOS_CALL MXOS_DOS_FILEHANDLE
    ret c
    ld hl,readHandle
    ld a,FILE_OP_READ
    MXOS_CALL MXOS_DOS_FILEHANDLE
    push af
    ld hl,(readHandle+FH_COUNT)
    push hl
    ld hl,readHandle
    ld a,FILE_OP_CLOSE
    MXOS_CALL MXOS_DOS_FILEHANDLE
    pop hl
    pop af
    ret
info:
    call mx_memInfo
    ret
registers:
    inc hl
    inc de
    inc bc
    xor a
    scf
    ret
console:
    ld hl,message
    MXOS_CALL MXOS_BIOS_PRINT_STRING_BANK
    ld e,0
    MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
    ret
allocate:
    ld b,MEM_ALLOC_ANY
    ld hl,128
    ld a,MEM_OP_ALLOC
    MXOS_CALL MXOS_DOS_MEMORY
    ret
activeUnload:
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    ret
rejectedBios:
    MXOS_CALL 001FFh
    ret
child:
    ld hl,childPath
    ld a,MODULE_TARGET_BANK
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    ret c
    push hl
    ld de,childRegs
    MXOS_CALL MXOS_DOS_MODULE_CALL
    pop hl
    ret c
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    ld hl,01234h
    ret
shared:
    ld a,MEM_OP_PUT16
    call mx_memory
    ret
message: DB 'Module BIOS/DOS call',13,10,0
childPath: DB 'B:',05Ch,'MRET.DLL',0
    DS MAX_PATH-($-childPath),0
childRegs: DW 1,2,3,0
driverPath: DB 'B:',05Ch,'DOSAPP.EXE',0
    DS MAX_PATH-($-driverPath),0
sectorBuffer: DS 512,0
rootPath: DB 'A:',05Ch,0
    DS MAX_PATH-($-rootPath),0
readHandle:
    DB 'README  TXT'
    DS FH_DRIVE-($-readHandle),0
    DB 0,FILE_MODE_READ
    DS FH_BUFFER-($-readHandle),0
    DW sectorBuffer,37
    DS FILE_HANDLE-($-readHandle),0
