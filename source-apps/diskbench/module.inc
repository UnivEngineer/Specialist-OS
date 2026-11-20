; MAIN/EXEC/DOS используют один измеритель с полным локальным буфером файла.
; Настройки копируются из main до замера; 88 байт результата — до unload.
run_bank_module:
    ld a,(argBank)
    cp 2
    jp z,moduleLimitDos
    cp 4
    jp nz,moduleSizeReady
moduleLimitDos:
    ld a,(v_fileSectors)
    cp BENCH_DOS_BUFFER_KIB*4+1
    jp c,moduleSizeReady
    ld a,BENCH_DOS_BUFFER_KIB*4
    ld (v_fileSectors),a
    ld a,BENCH_DOS_BUFFER_KIB
    ld (v_fileKiB),a
    ld hl,txtDosLimit
    call puts
moduleSizeReady:
    ld hl,(v_sectorSize)
    dec h
    ld a,h
    or l
    jp nz,bank_module_error
    ld a,(v_spc)
    dec a
    jp nz,bank_module_error
moduleGeometryReady:
    xor a
    ld (v_layout),a
    call print_summary
    ld hl,batchStats
    ld (batchNext),hl
    ld a,(argBank)
    cp 4
    ld a,0
    jp nz,moduleNotBatch
    inc a
    ld (batchEnabled),a
    ld a,3
    ld (argBank),a
moduleNotBatch:
    ld a,(argAlign)
    ld (workerConfig+10),a
    ld a,(argMode)
    ld (workerConfig+8),a
    ld a,(argFrag)
    inc a
    jp z,moduleFragmentDefault
    dec a
moduleFragmentDefault:
    and 1
    ld (workerConfig+9),a
    ld a,(v_drive)
    ld (workerConfig),a
    ld a,(v_writeMode)
    ld (workerConfig+1),a
    ld a,(v_budget)
    ld (workerConfig+2),a
    ld e,a
    ld d,0
    ld a,(v_fileSectors)
    ld (workerConfig+3),a
    ld l,a
    ld h,0
    call bench_mul_hl_de
    ld (v_operations),hl
    ld a,(v_budget)
    or a
    jp nz,moduleOperationsReady
    ld a,(v_fileSectors)
    ld l,a
    ld h,0
    ld de,3
    call bench_mul_hl_de
    ld (v_operations),hl
moduleOperationsReady:
    ld hl,(v_reserved)
    ld (workerConfig+4),hl
    ld hl,(v_dataStart)
    ld (workerConfig+6),hl
    ld a,(argBank)
    cp 3
    jp nz,moduleSelect
    ld a,MEM_OP_ARENA
    ld b,0
    ld hl,02100h
    ld de,LEGACY_APP_MEMORY_END-02100h
    MXOS_CALL MXOS_DOS_MEMORY
    jp c,bank_module_error
moduleSelect:
    ld a,(argBank)
    cp 2
    ld de,dosModuleName
    ld a,MODULE_TARGET_DOS
    jp z,moduleLoad
    ld a,(argBank)
    cp 3
    ld de,mainModuleName
    ld a,MODULE_TARGET_MAIN
    jp z,moduleLoad
    ld de,bankModuleName
    ld a,MODULE_TARGET_BANK
moduleLoad:
    push af
    ld hl,moduleFileName
moduleCopyName:
    ld a,(de)
    ld (hl),a
    inc de
    inc hl
    or a
    jp nz,moduleCopyName
    ld hl,modulePath
    push hl
    ld hl,workerConfig
    ld (bankModuleRegs),hl
    ld hl,resultsLow
    ld (workerResultCopy+4),hl
    ld hl,40
    ld (workerResultCopy+6),hl
    pop hl
    pop af
    MXOS_CALL MXOS_DOS_MODULE_LOAD
    jp c,bank_module_error
    ld (bankModuleHandle),hl
    ld de,bankModuleRegs
    MXOS_CALL MXOS_DOS_MODULE_CALL
    jp c,moduleUnload
    ld a,(bankModuleRegs+6)
    rra
    ld a,(bankModuleRegs+7) ; Код ошибки исполнителя; LD сохраняет CF.
    jp c,moduleUnload
    ld a,(bankModuleRegs+5)
    ld (workerResultCopy),a
    ld hl,(bankModuleRegs)
    ld (workerResultCopy+1),hl
    ld hl,workerResultCopy
    call mx_memcpyFar
    jp c,moduleUnload
    ld hl,(bankModuleRegs)
    ld de,40
    add hl,de
    ld (workerResultCopy+1),hl
    ld hl,statRawVerify
    ld (workerResultCopy+4),hl
    ; +48/+52 — RAW HIGH WRITE/READ; +56 — полная проверка HIGH.
    ld hl,48
    ld (workerResultCopy+6),hl
    ld hl,workerResultCopy
    call mx_memcpyFar
moduleUnload:
    push af
    ld hl,(bankModuleHandle)
    MXOS_CALL MXOS_DOS_MODULE_UNLOAD
    pop bc
    jp c,bank_module_error
    ld a,c
    rra
    ld a,b                ; Сохранённый A до MODULE_UNLOAD.
    jp c,bank_module_error
    ld a,(batchEnabled)
    or a
    jp nz,batchCollect
    ld a,(v_budget)
    or a
    jp z,moduleReadOnly
    ld a,(v_writeMode)
    ld (moduleCheckMode),a
    ld a,1
    ld (v_writeMode),a
    ld a,(argMode)
    and 1
    jp z,moduleFilePrint
    call moduleRawLayout
    ld hl,txtRawAccess
    call puts
    ld hl,resultsLow
    call print_group
    ld a,(moduleCheckMode)
    cp 6
    call z,printRawVerified
moduleFilePrint:
    ld a,(argMode)
    and 2
    jp z,modulePrintDone
    call moduleFileLayout
    ld hl,txtFileAccess
    call puts
    ld hl,resultsLow+20
    call print_group
    ld a,(moduleCheckMode)
    ld (v_writeMode),a
    cp 6
    call z,printFileVerified
modulePrintDone:
    ld a,(moduleCheckMode)
    ld (v_writeMode),a
    jp done
moduleReadOnly:
    ld a,(v_fileSectors)
    ld l,a
    ld h,0
    ld de,3
    call bench_mul_hl_de
    ld (v_operations),hl
    ld a,(argMode)
    and 1
    jp z,moduleReadOnlyFile
    call moduleRawLayout
    ld hl,txtRawAccess
    call puts
    ld a,(v_layout)
    or a
    jp z,moduleReadOnlyLow
    ld hl,resultsHigh+10
    ld de,txtReadHigh
    call print_stat
moduleReadOnlyLow:
    ld hl,resultsLow+10
    ld de,txtRead
    ld a,(v_layout)
    or a
    jp z,moduleReadOnlyLabel
    ld de,txtReadLow
moduleReadOnlyLabel:
    call print_stat
moduleReadOnlyFile:
    ld a,(argMode)
    and 2
    jp z,done
    call moduleFileLayout
    ld hl,txtFileAccess
    call puts
    ld hl,resultsHigh+30
    ld de,txtReadHigh
    ld a,(v_layout)
    or a
    call nz,print_stat
    ld hl,resultsLow+30
    ld de,txtRead
    ld a,(v_layout)
    or a
    jp z,moduleFileReadLabel
    ld de,txtReadLow
moduleFileReadLabel:
    call print_stat
    jp done
moduleFileLayout:
    xor a
    ld (v_layout),a
    ld hl,statFileHigh+10
    ld a,(hl)
    inc hl
    or (hl)
    inc hl
    or (hl)
    inc hl
    or (hl)
    ret z
    ld a,LAYOUT_MAPPED
    ld (v_layout),a
    ld hl,statFileHigh
    ld de,resultsHigh+20
    ld b,20
    jp copy_bytes
printFileVerified:
    ld a,(v_layout)
    or a
    jp z,printWriteVerified
    ld hl,statFileHighVerify
    ld (verifySource+1),hl
    ld hl,statFileHigh
    call makeVerified
    ld de,txtRawHighVerified
    call printVerifiedLabel
    ld hl,statFileVerify
    ld (verifySource+1),hl
    ld hl,statFileWrite
    call makeVerified
    ld de,txtRawLowVerified
    jp printVerifiedLabel
; Дополнительные U32 компактного измерителя не совпадают с форматом
; legacy resultsHigh. Собираем его только для прямого MAIN A:.
moduleRawLayout:
    ld a,(argBank)
    cp 3
    ret nz
    ld a,(v_drive)
    or a
    ret nz
    inc a
    ld (v_layout),a
    ld hl,statRawVerifyHigh
    ld de,resultsHigh
    ld b,4
    call copy_bytes
    ld hl,statFileVerifyHigh
    ld de,resultsHigh+10
    ld b,4
    call copy_bytes
    ld hl,statRawHighPostVerify
    ld de,statRawVerifyHigh
    ld b,4
    jp copy_bytes
bank_module_error:
    push af
    ld hl,txtBankModuleError
    call puts
    pop af
    ld e,a
    ld d,0
    ld b,0
    call bench_printDecWord
    jp done
bankModuleHandle: DW 0
bankModuleRegs: DW workerConfig,0,0,0
workerConfig: DS 11,0
workerResultCopy: DB 0
    DW 0
    DB 0
    DW resultsLow,40
    INCLUDE "../../source/include/app-drive.inc"
; Share one path between workers; capture its drive at controller entry.
modulePath: DB 'A:',05Ch,'BENCH',05Ch,'DiskBen',05Ch
moduleFileName: DS 13,0
bankModuleName: DB 'DiskBank.dll',0
dosModuleName: DB 'DiskDos.dll',0
mainModuleName: DB 'DiskMain.dll',0
; MODULE_LOAD только читает ASCIIZ; 128 доступных байт обеспечены последующим кодом.
    ASSERT modulePath+MAX_PATH <= sectorBuffer
txtBankModuleError: DB 10,'Disk module error ',0

    INCLUDE "batch.inc"
