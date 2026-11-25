; MXOS. Прямой перенос в текущем полном профиле, развёртка по 32 байта.
; Вход: HL = источник, DE = назначение, BC = длина. Допускается перекрытие вниз.
; Выход: HL и BC = исключающие концы источника и назначения, SP восстановлен.
    ASSERT $ == MEM_LOCAL_FORWARD
moveBytesForward:
        push    hl
        add     hl, bc
        ex      (sp), hl
        call    bankNearForward
        pop     hl
        ret

compact223Page:
        ld      c, 8
        jp      compact223Block
compact223Block:
        DUP 16
        pop     de
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     hl
        EDUP
compact223BlockEnd:
        dec     c
        jp nz,  compact223Block
        dec     b
        jp nz,  compact223Page
compact223RestoreSP:
        ld      sp, 0
        ld      b, h
        ld      c, l
        ret
    ASSERT compact223BlockEnd == MEM_FORWARD_BLOCK_END
    ASSERT compact223RestoreSP == MEM_FORWARD_RESTORE
