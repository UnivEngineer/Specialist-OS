; Настройка записи и стратегия размещения, без изменения формата FAT.
; OPTIONS не требует открытого файла или смонтированного диска.
handleOptions:
            ld a,FH_ORIGIN
            call handleField
            ld a,(hl)
            cp 2
            jp nc,handleArgument
            or a
            jp z,handleOptionsGet
            ld a,FH_COUNT
            call handleLoadWord
            ld a,l
            and 0FEh
            or h
            jp nz,handleArgument
            ld a,(v_fileOptions)
            ld b,a
            ld a,l
            ld (v_fileOptions),a
            ld a,b
            jp handleOptionsReturn
handleOptionsGet:
            ld a,(v_fileOptions)
handleOptionsReturn:
            push af
            ld a,FH_COUNT
            call handleField
            pop af
            ld (hl),a
            inc hl
            ld (hl),0
            jp handleSuccess

; HL=FH_MODE. Проверяем возможности до CREATE/TRUNCATE.
handleCheckMode:
            ld a,(hl)
            and FILE_MODE_WRITE|FILE_MODE_VERIFY
            ret z
            push af
            ld b,0FFh
            call j_diskDriver
            pop de
            ret c
            ld a,c
            and DISK_CAP_WRITE
            ld a,DOS_ERR_WRITE_PROTECTED
            jp z,dosSetError
            ld a,d
            and FILE_MODE_VERIFY
            ret z
            ld a,c
            and DISK_CAP_VERIFY
            ret nz
            ld a,DOS_ERR_UNSUPPORTED
            jp dosSetError

; Снимок настройки на весь запрос WRITE. Чтение никогда не включает B=5.
handleWritePolicy:
            xor a
            ld (v_handleChecked),a
            ld a,(v_handleOp)
            xor FILE_OP_WRITE
            ret nz
            ld a,FH_MODE
            call handleField
            ld a,(hl)
            and FILE_MODE_VERIFY
            ld b,a
            ld a,(v_fileOptions)
            and DOS_IO_VERIFY_FILES
            or b
            ret z
            ld b,0FFh
            call j_diskDriver
            ret c
            ld a,c
            and DISK_CAP_VERIFY
            ld (v_handleChecked),a
            ret

; LFSR x^16+x^14+x^13+x^11+1. Остаток даёт позицию по всему тому;
; общий поиск выбирает первый свободный кластер от неё, с возвратом к 2.
handleAllocateScatter:
            ld a,FH_ALLOC_SEED
            call handleLoadWord
            ld a,h
            or a
            rra
            ld h,a
            ld a,l
            rra
            ld l,a
            jp nc,handleSeedReady
            ld a,h
            xor 0B4h
            ld h,a
handleSeedReady:
            push hl
            ld a,FH_ALLOC_SEED
            call handleField
            pop de
            ld (hl),e
            inc hl
            ld (hl),d
            ex de,hl
            push hl
            ld hl,(v_diskInfo.dataClusters)
            ex de,hl
            pop hl
            call div_hl_de
            inc de
            inc de
            call fatFindClusterAt
            jp handleAllocateFound
v_fileOptions: DB 0
v_handleChecked: DB 0
