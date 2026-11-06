;+---------------------------------------------------------------------------
; MXOS
; Загрузить файл по указанному адресу
;
; На входе
;  hl - имя
;  de - адрес загрузки
;
; На выходе
;  bc, de, hl - сохраняются
;  v_foundDescrPtr - адрес копии дескриптора найденного файла
;  cf - ошибка
;
; 2013-12-12 Дизассемблировано vinxru
; 2022-02-11 Переработано SpaceEngineer
;----------------------------------------------------------------------------

fatFileLoad2:
            call fileFind
            ret c
fatFileLoadFound:
            push hl
            push bc
            push de
            ex de,hl
            ld (v_input_start),hl
            ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.size+2)
            ld a,h
            or l
            jp nz,fileLoadTooLarge
            ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.size)
            ld b,h
            ld c,l
    IFDEF BANKED_DOS_BUILD
            ; Внешний диапазон уже проверен шлюзом в странице клиента.
            ld a,(kdFarActive)
            or a
            jp nz,fileLoadRangeReady
    ENDIF
            ; Даже исторический load-at не должен затереть выполняемый DOS.
            ld hl,(v_input_start)
            ld de,DOS_MODULE_END
            call cmp_hl_de
            jp nc,fileLoadRangeReady
            add hl,bc
            jp c,fileLoadTooLarge
            ld de,DOS_CODE_START
            call cmp_hl_de
            jp c,fileLoadRangeReady
            jp nz,fileLoadTooLarge
fileLoadRangeReady:
            ld hl,0
            ld (v_streamOffset),hl
            ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstCluster)
            ex de,hl
            ld hl,(v_input_start)
            ld a,2
            call fatStream
            call nc,fatStreamFinish
            jp c,popDBH_ret
            pop de
            pop bc
            pop hl
            jp fileLoadExit
fileLoadTooLarge:
            call blockArgument
            jp popDBH_ret

; ---------------------------------------------------------------------------
fatFileLoadInfo:
            ; Ищем файл с именем в hl
            call    fileFind
            ret c 

; ---------------------------------------------------------------------------

fileLoadExit:
            ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; de = указатель на первое поле после расширения
            ld      bc, FILE_DESCRIPTOR.attrib
            add     hl, bc
            ex      de, hl

            ; Информация о файле, исключая имя
            ld      hl, (v_foundDescrPtr)
            add     hl, bc

            ; Копируем дескриптор из hl в de, исключая имя и расширение
            ld      b, FILE_DESCR_SIZE - FILE_NAME_LENGTH - 3
fileLoadCDLoop:
            ld      a, (hl)
            ld      (de), a
            inc     hl
            inc     de
            dec     b
            jp nz,  fileLoadCDLoop

            ; Выход CF=0
            or      a
            jp      popDBH_ret


fatFileLoad:
            jp imageLoad
