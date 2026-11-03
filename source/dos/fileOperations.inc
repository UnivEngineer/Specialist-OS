; Общие файловые операции. Диалоги и выбор памяти принадлежат вызывающему.
; Вызовы не реентерабельны, как остальные файловые функции DOS.

; A=диск 0..7; BC/DE/HL сохранены. CF=ошибка выбора/монтирования.
fileChangeDrive:
            cp      8
            jp nc,  fileOpError
            jp      fileSelectDrive

; HL=буфер минимум на 8 номеров, BC=ёмкость в байтах.
; Выход B=число установленных дисков, CF=ошибка параметров.
; Исходный диск восстановлен. AF/BC/DE/HL разрушены.
fileGetDrives:
            ld      a, b
            or      a
            jp nz,  fgd_SizeReady
            ld      a, c
            cp      8
            jp c,   fileOpError
fgd_SizeReady:
            ld      bc, 8
            call    fileOpRange
            ret c
            ld      a, (v_drive)
            push    af
            ld      c, 0
            ld      b, 0
fgd_Loop:
            push    bc
            push    hl
            ld      a, c
            call    fileSelectDrive
            ld      b, 3
            call    j_diskDriver
            ld      a, d
            or      e
            pop     hl
            pop     bc
            jp z,   fgd_Next
            ld      (hl), c
            inc     hl
            inc     b
fgd_Next:
            inc     c
            ld      a, c
            cp      8
            jp c,   fgd_Loop
            pop     af
            call    fileSelectDrive
            xor     a
            ret

; HL=начало, BC=число байтов; только обычное ОЗУ, без оборота адреса.
; HL/BC/DE сохранены; AF/флаги разрушены, CF=ошибка.
fileOpRange:
            push    hl
            push    de
            add     hl, bc
            jp c,   for_Bad
            ld      de, bios_vars
            call    cmp_hl_de
            jp c,   for_Good
            jp z,   for_Good
for_Bad:
            pop     de
            pop     hl
fileOpError:
            ld      a, FILE_OP_ERROR
            scf
            ret
for_Good:
            pop     de
            pop     hl
            or      a
            ret

; HL=FILE_COPY_REQUEST. Все рабочие поля — в памяти вызывающего.
; buffer/bufferSize явно задают область данных; используется целое число
; секторов по 256 байт. Минимум 256 байт, остаток ёмкости не используется.
; Запрос и буфер не пересекаются, оба расположены ниже bios_vars.
; Исходный диск восстановлен. AF/BC/DE/HL разрушены.
; CF=0,A=0: готово; CF=1,A=1: ошибка; CF=1,A=2: назначение существует.
; При ошибке после создания удаляется незавершённое назначение.
fileCopy:
            ld      bc, FILE_COPY_REQUEST
            call    fileOpRange
            ret c
            ld      (v_copyRequest), hl
            ld      e, FILE_COPY_REQUEST.bufferSize
            call    fc_ReadWord
            ld      a, h
            or      a
            jp z,   fileOpError
            ld      (v_copyCapacity), a
            ld      b, h
            ld      c, l
            ld      e, FILE_COPY_REQUEST.buffer
            call    fc_ReadWord
            call    fileOpRange
            ret c
            ; [буфер,конец) не должен перекрывать сам запрос.
            push    hl
            ld      hl, (v_copyRequest)
            ex      de, hl
            pop     hl
            call    cmp_hl_de
            jp nc,  fc_BufferAfter
            add     hl, bc
            call    cmp_hl_de
            jp c,   fc_RangeReady
            jp z,   fc_RangeReady
            jp      fileOpError
fc_BufferAfter:
            push    hl
            ld      hl, FILE_COPY_REQUEST
            add     hl, de
            ex      de, hl
            pop     hl
            call    cmp_hl_de
            jp c,   fileOpError
fc_RangeReady:
            ld      a, (v_drive)
            push    af
            ld      e, FILE_COPY_REQUEST.sourceDrive
            call    fc_Select
            jp c,   fc_Error
            ld      e, FILE_COPY_REQUEST.source
            call    fc_Address
            call    fileLoadInfo
            jp c,   fc_Error
            ld      e, FILE_COPY_REQUEST.source + FILE_DESCRIPTOR.size + 3
            call    fc_Address
            ld      a, (hl)
            or      a
            jp nz,  fc_Error
            dec     hl
            ld      d, (hl)
            dec     hl
            ld      e, (hl)
            dec     hl
            ld      a, (hl)
            or      a
            jp z,   fc_CountReady
            inc     de
            ld      a, d
            or      e
            jp z,   fc_Error
fc_CountReady:
            ex      de, hl
            ld      e, FILE_COPY_REQUEST.blocksLeft
            call    fc_WriteWord
            ld      e, FILE_COPY_REQUEST.source + FILE_DESCRIPTOR.firstCluster
            call    fc_ReadWord
            ld      e, FILE_COPY_REQUEST.sourceCluster
            call    fc_WriteWord
            ; Копируем только метаданные, имя назначения оставляем прежним.
            ld      e, FILE_COPY_REQUEST.destination + FILE_DESCRIPTOR.attrib
            call    fc_Address
            ld      b, h
            ld      c, l
            ld      e, FILE_COPY_REQUEST.source + FILE_DESCRIPTOR.attrib
            call    fc_Address
            push    hl
            ld      de, FILE_DESCR_SIZE - FILE_DESCRIPTOR.attrib
            add     hl, de
            ex      de, hl
            pop     hl
            call    memcpy_bc_hl
            ld      e, FILE_COPY_REQUEST.destinationDrive
            call    fc_Select
            jp c,   fc_Error
            ld      e, FILE_COPY_REQUEST.destination
            call    fc_Address
            call    fileLoadInfo
            jp nc,  fc_Exists
            ; Даже то же имя/диск попадает в проверку существования выше.
            xor     a
            ld      (bios_vars.diskWriteErr), a
            ld      e, FILE_COPY_REQUEST.destination + FILE_DESCRIPTOR.firstCluster
            call    fc_Address
            ld      (hl), a
            inc     hl
            ld      (hl), a
            ld      e, FILE_COPY_REQUEST.blocksLeft
            call    fc_ReadWord
            push    hl
            ld      e, FILE_COPY_REQUEST.destination
            call    fc_Address
            pop     de
            call    fileCreatePrepare
            jp nc,  fc_Prepared
            ld      e, FILE_COPY_REQUEST.destination + FILE_DESCRIPTOR.firstCluster
            call    fc_ReadWord
            ld      a, h
            or      l
            jp z,   fc_Error
            jp      fc_Cleanup
fc_Prepared:
            ld      e, FILE_COPY_REQUEST.destination + FILE_DESCRIPTOR.firstCluster
            call    fc_ReadWord
            ld      e, FILE_COPY_REQUEST.destinationCluster
            call    fc_WriteWord
fc_Loop:
            ld      e, FILE_COPY_REQUEST.blocksLeft
            call    fc_ReadWord
            ld      a, h
            or      l
            jp z,   fc_Success
            ld      a, (v_copyCapacity)
            ld      c, a
            ld      a, h
            or      a
            jp nz,  fc_BatchReady
            ld      a, l
            cp      c
            jp nc,  fc_BatchReady
            ld      c, a
fc_BatchReady:
            push    bc
            ld      e, FILE_COPY_REQUEST.sourceDrive
            call    fc_Select
            pop     bc
            jp c,   fc_Cleanup
            ld      e, FILE_COPY_REQUEST.sourceCluster
            ld      b, 2
            call    fc_Transfer
            jp c,   fc_Cleanup
            push    bc
            ld      e, FILE_COPY_REQUEST.destinationDrive
            call    fc_Select
            pop     bc
            jp c,   fc_Cleanup
            ld      e, FILE_COPY_REQUEST.destinationCluster
            ld      b, 1
            call    fc_Transfer
            jp c,   fc_Cleanup
            ld      e, FILE_COPY_REQUEST.blocksLeft
            call    fc_ReadWord
            ld      d, 0
            ld      e, c
            call    sub_hl_de
            ld      e, FILE_COPY_REQUEST.blocksLeft
            call    fc_WriteWord
            jp      fc_Loop
fc_Cleanup:
            ld      e, FILE_COPY_REQUEST.destinationDrive
            call    fc_Select
            jp c,   fc_Error
            ld      e, FILE_COPY_REQUEST.destination
            call    fc_Address
            ; Незавершённая копия могла унаследовать R/S источника.
            xor     a
            ld      c, 1
            call    fileGetSetAttr
            call    fileDelete
fc_Error:
            ld      a, FILE_OP_ERROR
            scf
            jp      fc_Return
fc_Exists:
            ld      a, FILE_OP_EXISTS
            scf
            jp      fc_Return
fc_Success:
            xor     a
fc_Return:
            pop     bc              ; исходный AF, номер диска в B
            push    af
            ld      a, b
            call    fileSelectDrive
            pop     af
            ret

; E=поле кластера, B=режим, C=число секторов. BC сохранён.
fc_Transfer:
            push    de
            call    fc_ReadWord
            push    hl
            ld      e, FILE_COPY_REQUEST.buffer
            call    fc_ReadWord
            pop     de
            call    fileTransferClusters
            pop     hl              ; L=смещение поля
            ret c
            ld      a, l
            ex      de, hl
            ld      e, a
            call    fc_WriteWord
            or      a
            ret
fc_Select:
            call    fc_Address
            ld      a, (hl)
            call    fileChangeDrive
            ret c
            ld      a, (v_diskInfo + DISK_INFO.isValid)
            cp      DISK_VALID
            ret z
            scf
            ret
; E=смещение поля запроса. AF/DE/HL разрушены, BC сохранён.
; 8080: Address 7 байт/43 такта; ReadWord 8 байт/94 такта;
; WriteWord 9 байт/110 тактов, без внешнего CALL. Во всех путях
; только обычное ОЗУ; стоимость операций с томом зависит от драйвера.
fc_Address:
            ld      hl, (v_copyRequest)
            ld      d, 0
            add     hl, de
            ret
fc_ReadWord:
            call    fc_Address
            ld      a, (hl)
            inc     hl
            ld      h, (hl)
            ld      l, a
            ret
fc_WriteWord:
            push    hl
            call    fc_Address
            pop     de
            ld      (hl), e
            inc     hl
            ld      (hl), d
            ret

v_copyRequest:  DW 0
v_copyCapacity: DB 0
