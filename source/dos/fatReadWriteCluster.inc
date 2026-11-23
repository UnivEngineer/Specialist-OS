;----------------------------------------------------------------------------
; MXOS
;
; Чтение ячейки FAT текущего тома.
;
; вход:
;   de = номер ячейки FAT.
;
; выход:
;   cf = 0: de = содержимое ячейки, включая служебные значения FAT.
;   cf = 1: a = код ошибки кэша, значение de непригодно как ссылка.
;   hl, bc сохраняются. a при успехе и остальные флаги меняются.
;
; СИ аналог:
;   de = fat[de];
;
; 2022-02-04 SpaceEngineer
;----------------------------------------------------------------------------

fatReadCluster:
    IFDEF BANKED_DOS_BUILD
            ld a,(v_fat12)
            or a
            jp nz,f12Read
    ENDIF
            push    hl

            ; вычисляем адрес ячейки, куда указывает de
            xor     a ; флаг - сектор fat не будет модифицироваться
            call    getFatCellAddress

            jp c,   fatReadExit

            ; читаем в de номер кластера из fat по адресу hl
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

            ; восстанавливаем регистры и выходим
fatReadExit:
            pop     hl
            ret

;----------------------------------------------------------------------------
; MXOS
;
; Запись ссылки в цепочку FAT. Сектор помечается для отложенной записи.
;
; вход:
;   hl = номер изменяемой ячейки FAT.
;   de = новое 16-битное значение ячейки.
;
; выход:
;   cf = 0: hl = записанное значение, пригодное для следующего шага цепочки.
;   cf = 1: a = код ошибки кэша, ячейка не записана.
;   bc, de сохраняются. a при успехе и остальные флаги меняются.
;
; СИ аналог:
;   fat[hl] = de;
;   hl = de;
;
; 2022-02-04 SpaceEngineer
;----------------------------------------------------------------------------

fatWriteCluster:
    IFDEF BANKED_DOS_BUILD
            ld a,(v_fat12)
            or a
            jp nz,f12Write
    ENDIF
            ; вычисляем адрес ячейки, куда указывает hl
            push    de
            ex      hl, de
            ld      a, 1 ; флаг - сектор fat будет модифицироваться
            call    getFatCellAddress
            pop     de
            ret c

            ; записываем номер кластера из de в fat по адресу hl
            ld      (hl), e
            inc     hl
            ld      (hl), d

            ; hl = de
            ld      h, d
            ld      l, e
            ret

;----------------------------------------------------------------------------
; MXOS
;
; Освобождение ячейки FAT с возвратом прежней ссылки цепочки.
;
; вход:
;   de = номер освобождаемой ячейки FAT.
;
; выход:
;   cf = 0: de = прежнее значение ячейки. В кэше ячейка обнулена.
;   cf = 1: a = код ошибки кэша, de = исходный номер, ячейка не изменена.
;   hl, bc сохраняются. a при успехе и остальные флаги меняются.
;
; СИ аналог:
;   next = fat[de];
;   fat[de] = 0;
;   de = next;
;
; 2022-02-04 SpaceEngineer
;----------------------------------------------------------------------------

fatFreeCluster:
    IFDEF BANKED_DOS_BUILD
            ld a,(v_fat12)
            or a
            jp nz,f12Free
    ENDIF
            push    hl

            ; вычисляем адрес ячейки с номером из de
            push    de
            ld      a, 1 ; флаг - сектор fat будет модифицироваться
            call    getFatCellAddress
            pop     de

            jp c,   fatFreeExit

            ; читаем номер кластера из fat, и сразу записываем туда нули
            ld      e, (hl)
            ld      (hl), 0
            inc     hl
            ld      d, (hl)
            ld      (hl), 0

            ; восстанавливаем регистры и выходим
fatFreeExit:
            pop     hl
            ret



;----------------------------------------------------------------------------
; MXOS
;
; Проверка номера кластера данных текущего тома.
;
; вход:
;   de = проверяемый номер. v_diskInfo.dataClusters задан при монтировании.
;
; выход:
;   cf = 0: номер лежит в диапазоне 2..dataClusters+1 и ниже 0FFF0h.
;   cf = 1: a и v_dosError = DOS_ERR_CORRUPT.
;   bc, de, hl сохраняются. Прочие флаги и a при успехе не определены.
;----------------------------------------------------------------------------

fatCheckCluster:
            ld a,d
            cp 0FFh
            jp nz,fatCheckData
            ld a,e
            cp 0F0h
            jp nc,dosCorrupt
fatCheckData:
            push hl
            ld hl,2
            call cmp_hl_de
            jp c,fatCheckUpper
            jp nz,fatCheckBad
fatCheckUpper:
            ld hl,(v_diskInfo.dataClusters)
            inc hl
            call cmp_hl_de
            jp c,fatCheckBad
            pop hl
            or a
            ret
fatCheckBad:
            pop hl
            jp dosCorrupt

    IFDEF BANKED_DOS_BUILD
    INCLUDE "fat12.inc"
    ENDIF
