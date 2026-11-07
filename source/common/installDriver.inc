;+---------------------------------------------------------------------------
; MXOS
; Установить драйвер устройства
;
; На входе
;  a - диск
;  hl - адрес драйвера
;
; На выходе
;  hl - адрес структуры DISK_INFO
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

installDriver:
            push bc
            ld c,8
            call installDriverCommon
            pop bc
            ret
installDriverCommon:
            and 7
            push hl
            push de
            ld e,a
            ld d,0
            ld hl,v_blockShifts
            add hl,de
            ld (hl),c
            pop de
            pop hl
            ; Сохранение регистров
            push de
            push hl

            ; Максимум дисков
            and 7

            ; Вычисление адреса в таблице дисков
            add   a, a
            ld    l, a
            ld    h, 0
            ld    de, v_drives
            add   hl, de

            ; Установка драйвера
            pop   de
            ld  (hl), e
            inc   hl
            ld  (hl), d

            ; Восстановление регистров
            ex  de, hl
            pop de
            rrca

            ; Замена активного драйвера вступает в силу сразу, даже если
            ; следующий выбор диска совпадёт с уже выбранным.
            push af
            push hl
            ld   hl, v_drive
            cp   (hl)
            pop  hl
            jp nz, installDriverInactive
    IFNDEF BANKED_DOS_BUILD
            push af
            ld a,(v_dosBanked)
            or a
            jp nz,installDriverBanked
            ld (j_diskDriver+1),hl
installDriverBanked:
            pop af
    ELSE
            ld (j_diskDriver+1),hl
    ENDIF
installDriverInactive:
            pop af

            ; Передача адреса структуры v_diskInfo
            ld  hl, v_diskInfo
            or a

            ; Выход
            ret


v_blockShifts: DB 8,8,8,8,8,8,8,8

; A=диск 0..7, HL=драйвер, C=log2 блока. HL=DISK_INFO, CF=ошибка.
; BC/DE сохранены; AF разрушается. Перед заменой драйвера нужен flush.
installBlockDriver:
            cp 8
            jp nc,installBlockError
            push af
            ld a,c
            cp 7
            jp c,installBlockBad
            cp 10
            jp nc,installBlockBad
            pop af
            jp installDriverCommon
installBlockBad:
            pop af
installBlockError:
            ld a,DOS_ERR_ARGUMENT
            scf
            ret
