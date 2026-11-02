; Общая реализация B=5 для flash-драйверов.
; Вход BC=адрес драйвера, DE=сектор, HL=данные (ровно 256 байт).
; Выход CF=ошибка записи/чтения/сравнения; BC/DE/HL сохранены, AF разрушен.
; Буфер чтения временно занимает 256 байт стека, постоянного буфера нет.
; Вызывающая сторона обеспечивает 320 свободных байт стека. Вызов не реентерабелен,
; как и переключаемый аппаратный ABI. B=2 синхронизирует нужный чип FlashFst.
diskWriteChecked:
            push    bc
            push    de
            push    hl
            ld      h, b
            ld      l, c
            ld      (dwcCall + 1), hl
            pop     hl
            push    hl
            ld      b, 1
            call    dwcCall
            jp c,   dwcExit

            ld      hl, -256
            add     hl, sp
            ld      sp, hl
            ld      b, 2
            call    dwcCall
            jp c,   dwcFailed

            ; Над временным буфером лежит исходный HL.
            ld      de, 256
            add     hl, de
            ld      e, (hl)
            inc     hl
            ld      d, (hl)
            ld      hl, 0
            add     hl, sp
            ld      b, 0
; 11 байт, 49 тактов на совпавший байт: два чтения ОЗУ, без доступа к flash.
dwcCompare:
            ld      a, (de)
            cp      (hl)
            jp nz,  dwcFailed
            inc     de
            inc     hl
            dec     b
            jp nz,  dwcCompare
            or      a
            jp      dwcFree
dwcFailed:
            ld      a, 1
            ld      (bios_vars.diskWriteErr), a
            scf
dwcFree:
            ; Освобождаем буфер, сохраняя CF через младший бит слова AF.
            push    af
            pop     bc
            ld      hl, 256
            add     hl, sp
            ld      sp, hl
            ld      a, c
            rra
dwcExit:
            pop     hl
            pop     de
            pop     bc
            ret
dwcCall:
            jp      0
