;+---------------------------------------------------------------------------
; MXOS
; �������������� ��������
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; ��������� hl � de:
; hl = hl - de
;
; de, bc - �����������
;
; SpaceEngineer
;----------------------------------------------------------------------------

sub_hl_de:
        ld    a, l
        sub   e
        ld    l, a
        ld    a, h
        sbc   d
        ld    h, a
        ret

;----------------------------------------------------------------------------
; ������� hl �� 2:
; hl = hl / 2
;
; de, bc - �����������
;
; SpaceEngineer
;----------------------------------------------------------------------------

div_hl_2:
        ld  a, h
        or  a    ; ����� ����� c
        rra 
        ld  h, a
        ld  a, l
        rra
        ld  l, a
        ret

;----------------------------------------------------------------------------
; ��������� hl �� de:
; hl = hl * de
;
;
; �� �����
;  hl, de - ���������
;
; �� ������
;  hl - ������������
;
;  bc - �����������
;
; ����� �� ������� c8080 VWarlock'a
; https://github.com/VWarlock/C8080
;----------------------------------------------------------------------------

mul_hl_de:
        push    bc
        ld      b, h
        ld      c, l
        ld      hl, 0
        ld      a, 17
_mul1:  dec     a
        jp      z, pop_bc_ret
        add     hl, hl
        ex      hl, de
        jp      nc, _mul2
        add     hl, hl
        inc     h
        jp      _mul3
_mul2:  add     hl, hl
_mul3:  ex      hl, de
        jp      nc, _mul1
        add     hl, bc
        jp      nc, _mul1
        inc     d
        jp      _mul1

pop_bc_ret:
        pop     bc
        ret

;----------------------------------------------------------------------------
; ������� hl �� de:
; hl = hl / de
; de = hl % de
;
; �� �����
;  hl - �������
;  de - ��������
;
; �� ������
;  hl - �������
;  de - �������
;
;  bc - �����������
;
; ����� �� ������� c8080 VWarlock'a
; https://github.com/VWarlock/C8080
;----------------------------------------------------------------------------

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
