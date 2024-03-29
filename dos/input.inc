;+---------------------------------------------------------------------------
; MXOS
; ���� ������
;
; �� �����
;  hl - ������ ������
;  de - ����� ������
;
; �� ������
;  bc, de, hl - �����������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

input:      ; ��������� ��������
            push  hl
            push  bc
            push  af

            ; ��������� ��������
            ld    (v_input_start), hl
            ex    de, hl
            ld    (v_input_end), hl

            ; �������� � ����� ������ 0
            ld    h, d
            ld    l, e
            ld  (hl), 0

input_loop: ; ���� ��������
            call  j_getch
            ld    c, a

            ; ������ ��������� �������
            cp    20h
            jp c, input_spec

            ; ������ ������� Back space
            cp    7Fh
            jp z, input_bkspc

            ; ...
            ;nop
            ;nop
            ;nop

            ; ��� ����� ������
            push  hl
            ld    hl, (v_input_end)
            call  cmp_hl_de_2
            pop   hl
            jp nz,  loc_C8DE          
            call  cmp_hl_de_2
            jp z, input_loop
            ld    a, (de)
            dec   de
            ld    (de),a
loc_C8DE:   push  bc
            ld    b, h
            ld    c, l
            inc   bc
            call  memmove_bc_hl
            pop   bc
            ld  (hl), c       ; *hl++     = c;
            call  j_printChar ; �����     �������     �� �����
            inc   hl
            inc   de          
loc_C8EC:   push  hl
            ld    hl, (bios_vars.cursorY)
            ex    (sp), hl
            push  hl
            call  j_printString
            ld    c, ' '
            call  j_printChar
            pop   hl
            ex    (sp), hl
            ld    (bios_vars.cursorY), hl
            pop   hl

            ; �����������
            jp    input_loop

; ---------------------------------------------------------------------------

input_bkspc:      ; ���� ������ � ������ ������ ������ �� �������
            call  cmp_hl_de_2
            jp z, input_loop

            ; �������� ������
            ld    b, h
            ld    c, l
            inc   hl
            call  memmove_bc_hl

            ; ��������� ��������� �������
            dec   hl

            dec   de
            jp    loc_C8EC

; ---------------------------------------------------------------------------

input_spec: ; ������ �����
            cp    8
            jp z, input_left

            ; ������ ������
            cp    18h
            jp z, input_right

            ; ����� �� ����
            cp    0Dh
            jp nz,  input_loop

            ; ����� ����

            ; ��������� � ����� ����� 0D
            ld    (de),a

            ; ��������������� �������� � �������
            pop   af
            pop   bc
            pop   hl
            ret

; ---------------------------------------------------------------------------

input_left: ; ���� ������ � ������ ������ (hl==v_input_start) �� ���������� ������
            ex    de, hl
            push  hl          
            ld    hl, (v_input_start)
            call  cmp_hl_de_2
            pop   hl
            ex    de, hl
            jp z, input_loop

            ; ��������� ��������� �������
            dec   hl

input_lr:   ; ���������� ������ ������/�����
            call  j_printChar

            ; ���������� ����
            jp    input_loop

; ---------------------------------------------------------------------------

input_right:      ; ���� ������ � ����� ����� (hl==de) �� ���������� ������
            call  cmp_hl_de_2
            jp z, input_loop

            ; ����������� ��������� �������
            inc   hl

            ; ����� �����������
            jp    input_lr

