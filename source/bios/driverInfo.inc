; B=0FFh. Снимает сохранённый AF встроенного драйвера.
; Выход HL=0 (описания находятся в DOS); B=технология, C=возможности; CF=0. AF разрушен.
diskDriverInfo:
            pop     af
            ld      a, (v_drive)
            or      a
            ld      bc, DISK_TECH_ROM * 256 + DISK_CAP_READ
            ld      hl, 0
            ret z
            ld      bc, DISK_TECH_RAM * 256 + DISK_CAP_READ + DISK_CAP_WRITE
            ld      hl, 0
            or      a
            ret

; Пустой драйвер не возвращает структуру DISK_INFO вместо текста.
diskDriverNoInfo:
            pop     af
            ld      hl, 0
            scf
            ret
