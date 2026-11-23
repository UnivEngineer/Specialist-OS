;----------------------------------------------------------------------------
; MXOS
;
; Перенести диапазон памяти, выбрав направление по взаимному положению адресов.
;
; вход:
;   hl = начало источника, de = конец исключительно, bc = адрес назначения. Диапазоны
;   без оборота, de = 0 допускает конец 10000h. Вызов нереентерабелен, прерывания выключены, код и стек вне
;   диапазонов.
;
; выход:
;   hl = исходный конец, bc = конец назначения. SP восстановлен. de, a и флаги
;   изменяются. Пустой диапазон допустим.
;----------------------------------------------------------------------------

moveCore:
            push    de
            ld      a, e
            sub     l
            ld      e, a
            ld      a, d
            sbc     a, h
            ld      d, a
            or      e
            jp z,   moveEmpty
            push    bc
            push    hl
            ex      de, hl
            ld      b, h
            ld      c, l
            pop     hl
            pop     de
            ; BC=число, HL=источник, DE=назначение.
            ld      a, d
            cp      h
            jp c,   moveForward
            jp nz,  moveBackward
            ld      a, e
            cp      l
            jp c,   moveForward
moveBackward:
            add     hl, bc
            ex      de, hl
            add     hl, bc
            push    hl
            ex      de, hl
            ld      a, c
            and     1
            jp z,   moveBackEven
            dec     hl
            dec     de
            ld      a, (hl)
            ld      (de), a
            dec     bc
moveBackEven:
            ld      a, b
            or      c
            call nz, movePairsBack
            pop     bc
            pop     hl
            ret
moveForward:
            ld      a, c
            and     1
            jp z,   moveForwardEven
            ld      a, (hl)
            ld      (de), a
            inc     hl
            inc     de
            dec     bc
moveForwardEven:
            ex      de, hl
            ld      a, b
            or      c
            call nz, movePairsForward
            ld      b, h
            ld      c, l
            pop     hl
            ret
moveEmpty:
            pop     hl
            ret

; Остаток до 16 байт: 54 такта / 2 байта вперёд, 55 назад (8080).
; Полный блок: 16 байт за 287 тактов вперёд, 295 назад;
; ещё 22 такта управления на каждые 256 байт. Настройка считается отдельно.
; Хвост 16..254 байта использует тот же развёрнутый блок, без второго тела.
; moveCore: 295 байт; +40 байт к прежнему циклу, по копии в BIOS и DOS.
; На каждый байт одно чтение и одна запись, без обращения к портам.
; Вход BC=чётное ненулевое число; вперёд HL=назначение, DE=источник,
; назад HL=конец источника, DE=конец назначения. Разрушает AF/BC/DE.
movePairsForward:
            ex      de, hl
            ld      (moveReadSP+1), hl
            ld      hl, 0
            add     hl, sp
            ld      (moveRestoreSP+1), hl
            ex      de, hl
moveReadSP: ld      sp, 0
            ld      a, c
            or      a
            jp z,   moveForwardPageCheck
            and     0Fh
            jp z,   moveForwardTailBlocks
; Сначала остаток 2..14 байт; C сохраняет исходный хвост для деления на 16.
movePairForwardLoop:
            pop     de
            ld      (hl), e
            inc     hl
            ld      (hl), d
            inc     hl
            dec     a
            dec     a
            jp nz,  movePairForwardLoop
moveForwardTailBlocks:
            ld      a, c
            and     0F0h
            jp z,   moveForwardPageCheck
            rrca
            rrca
            rrca
            rrca
            ld      c, a
            ; После хвоста DEC B вернёт исходное число полных страниц.
            ; INC/DEC работают по модулю 256 и при исходном B=FFh.
            inc     b
            jp      moveForwardBlock
moveForwardPageCheck:
            ld      a, b
            or      a
            jp z,   moveRestoreSP
moveForwardPage:
            ld      c, 16
moveForwardBlock:
            DUP 8
            pop     de
            ld      (hl), e
            inc     hl
            ld      (hl), d
            inc     hl
            EDUP
            dec     c
            jp nz,  moveForwardBlock
            dec     b
            jp nz,  moveForwardPage
            jp      moveRestoreSP
movePairsBack:
            ex      de, hl
            ld      (moveWriteSP+1), hl
            ld      hl, 0
            add     hl, sp
            ld      (moveRestoreSP+1), hl
            ex      de, hl
moveWriteSP: ld      sp, 0
            ld      a, c
            or      a
            jp z,   moveBackPageCheck
            and     0Fh
            jp z,   moveBackTailBlocks
movePairBackLoop:
            dec     hl
            ld      d, (hl)
            dec     hl
            ld      e, (hl)
            push    de
            dec     a
            dec     a
            jp nz,  movePairBackLoop
moveBackTailBlocks:
            ld      a, c
            and     0F0h
            jp z,   moveBackPageCheck
            rrca
            rrca
            rrca
            rrca
            ld      c, a
            inc     b
            jp      moveBackBlock
moveBackPageCheck:
            ld      a, b
            or      a
            jp z,   moveRestoreSP
moveBackPage:
            ld      c, 16
moveBackBlock:
            DUP 8
            dec     hl
            ld      d, (hl)
            dec     hl
            ld      e, (hl)
            push    de
            EDUP
            dec     c
            jp nz,  moveBackBlock
            dec     b
            jp nz,  moveBackPage
moveRestoreSP:
            ld      sp, 0
            ret
moveCoreEnd:

;----------------------------------------------------------------------------
; MXOS
;
; Скопировать неперекрывающийся фрагмент в порядке возрастания адресов.
;
; вход:
;   hl = источник, de = назначение, bc = положительная длина в байтах. Требования к
;   стеку и прерываниям как у moveCore.
;
; выход:
;   hl и bc указывают сразу за источником и назначением соответственно. SP
;   восстановлен, af и de изменяются.
;----------------------------------------------------------------------------

moveBytesForward:
            push hl
            add hl,bc
            ex (sp),hl
            jp moveForward
