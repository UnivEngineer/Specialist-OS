; Общая реализация B=5 для flash-драйверов.
; Вход BC=адрес драйвера, DE=сектор, HL=данные (ровно 256 байт).
; Выход CF=ошибка записи/чтения/сравнения; BC/DE/HL сохранены, AF разрушен.
; DWC_READ_BUFFER: временный буфер текущей страницы, не пересекающий источник.
; Без него локальный DOS/BANK использует 256 байт стека (нужно 320 свободных).
; DWC_DOS_BUFFER: MEM выбирает буфер DOS по bankPage; BANK оставляет стек.
; BIOS использует общий временный буфер MAIN. Вызов не реентерабелен,
; как и переключаемый аппаратный ABI. B=2 синхронизирует нужный чип FlashFst.
    IFDEF DWC_DOS_BUFFER
    ASSERT DOS_BANK_PAGE == 1
    ENDIF
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

    IFDEF DWC_READ_BUFFER
            ld      hl, DWC_READ_BUFFER
    ELSE
    IFDEF DWC_DOS_BUFFER
            ld      (dwcSource + 1), hl
            ld      a, (bankPage)
            dec     a              ; DOS_BANK_PAGE=1; DEC не меняет CF
            ld      hl, DWC_DOS_BUFFER
            jp z,   dwcBufferReady
    ENDIF
            ld      hl, -256
            add     hl, sp
            ld      sp, hl
    IFDEF DWC_DOS_BUFFER
dwcBufferReady:
            ld      (dwcBuffer + 1), hl
    ENDIF
    ENDIF
            ld      b, 2
            call    dwcCall
            jp c,   dwcFailed

    IFDEF DWC_READ_BUFFER
            ; Вершина стека содержит исходный HL; прочитанные данные — в scratch.
            pop     de
            push    de
            ld      hl, DWC_READ_BUFFER
    ELSE
    IFDEF DWC_DOS_BUFFER
dwcSource:
            ld      de, 0
dwcBuffer:
            ld      hl, 0
    ELSE
            ; Над временным буфером лежит исходный HL.
            ld      de, 256
            add     hl, de
            ld      e, (hl)
            inc     hl
            ld      d, (hl)
            ld      hl, 0
            add     hl, sp
    ENDIF
    ENDIF
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
    IFNDEF DWC_NO_ERROR_FLAG
            ld      (bios_upper.diskWriteErr), a
    ENDIF
            scf
dwcFree:
    IFNDEF DWC_READ_BUFFER
    IFDEF DWC_DOS_BUFFER
            ld      a, (bankPage)
            dec     a              ; Сохраняем CF результата сравнения/чтения.
            jp z,   dwcExit
    ENDIF
            ; Освобождаем буфер, сохраняя CF через младший бит слова AF.
            push    af
            pop     bc
            ld      hl, 256
            add     hl, sp
            ld      sp, hl
            ld      a, c
            rra
    ENDIF
dwcExit:
            pop     hl
            pop     de
            pop     bc
            ret
dwcCall:
            jp      0
