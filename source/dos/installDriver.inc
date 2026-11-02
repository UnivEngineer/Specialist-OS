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

installDriver:    ; Сохранение регистров
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
            ld   (j_diskDriver+1), hl
installDriverInactive:
            pop af

            ; Передача адреса структуры v_diskInfo
            ld  hl, v_diskInfo

            ; Выход
            ret

