; Локальная арифметика Intel 8080: подключать в области кода, не в цепочке исполнения.
; Перед INCLUDE определить только нужные MATH_SUB / MATH_DIV2 / MATH_MUL / MATH_DIV.
; При необходимости изолировать имена через MODULE. Нет BIOS/MXOS_CALL и скрытых буферов.
; SUB: HL-=DE; DIV2: HL>>=1; MUL: HL=младшие 16 бит HL*DE (DE разрушен).
; DIV: HL=частное, DE=остаток. При DE=0 возвращает HL=0, DE=исходное HL.
; Все процедуры сохраняют BC, SUB/DIV2 сохраняют DE; AF разрушен.

    IFDEF MATH_SUB
sub_hl_de:
        ld    a, l
        sub   e
        ld    l, a
        ld    a, h
        sbc   d
        ld    h, a
        ret
    ENDIF

    IFDEF MATH_DIV2
div_hl_2:
        ld  a, h
        or  a    ; сброс флага c
        rra 
        ld  h, a
        ld  a, l
        rra
        ld  l, a
        ret
    ENDIF

    IFDEF MATH_MUL
mul_hl_de:
        push bc
        ld b,h
        ld c,l
        ld hl,0
        ld a,16
_mul1:
        ; Для результата modulo 65536 достаточно сдвига суммы и множителя.
        ; Не переносим переполнение суммы в DE: оно не влияет на младшее слово.
        add hl,hl
        ex de,hl
        add hl,hl
        ex de,hl
        jp nc,_mul2
        add hl,bc
_mul2:
        dec a
        jp nz,_mul1
        pop bc
        ret
    ENDIF

    IFDEF MATH_DIV
div_hl_de:
        push    bc
        ex      hl, de
        call    _div0
        pop     bc
        ret

_div0:  ld      a, h
        or      l
        ret     z
        ld      bc, 0
        push    bc
_div1:  ld      a, e
        sub     L
        ld      a, d
        sbc     h
        jp      c, _div2
        push    hl
        add     hl, hl
        jp      nc, _div1
_div2:  ld      hl, 0
_div3:  pop     bc
        ld      a, b
        or      c
        ret     z
        add     hl, hl
        push    de
        ld      a, e
        sub     c
        ld      e, a
        ld      a, d
        sbc     b
        ld      d, a
        jp      c, _div4
        inc     hl
        pop     bc
        jp      _div3
_div4:  pop     de
        jp      _div3
    ENDIF


; Не оставлять настройки следующему включению в парной сборке BIOS/DOS.
    IFDEF MATH_SUB
    UNDEFINE MATH_SUB
    ENDIF
    IFDEF MATH_DIV2
    UNDEFINE MATH_DIV2
    ENDIF
    IFDEF MATH_MUL
    UNDEFINE MATH_MUL
    ENDIF
    IFDEF MATH_DIV
    UNDEFINE MATH_DIV
    ENDIF
