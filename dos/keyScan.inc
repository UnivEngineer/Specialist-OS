;+---------------------------------------------------------------------------
; MXOS
; �������� ��� ������� �������
;
; �� ������
;  a - ���
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

setKeybMode83:
            ld      a, 83h
            ld      (IO_KEYB_MODE),a
            ret

; ---------------------------------------------------------------------------

keyScan2:   ; ����� ������ ����
            call    setKeybMode82

            ; ����� ������� ��������� ��� �������
            ld      a, (IO_KEYB_B)     

            ; ��� ���� �� ������������ � ������������
            or      3

            ; �� ���� ������� �� ������
            cp      0FFh
            ret z 

            ; ��������� ��������
            push    hl
            push    bc
            push    de
            
            ; ���������� 12 �������� ������
            ld      hl, 0FFEh
            ld      de, v_keybTbl + 11
            ld      b, 0FFh
loc_C1DD:   ld      a, h
            ld      (IO_KEYB_C),a
            ld      a, l
            ld      (IO_KEYB_A),a
            rrca
            cpl
            and     4
            ld      c, a
            ld      a, (IO_KEYB_B)
            or      c

            ; ���������� 6 ������ � �������
            ld      c, 6
            rrca
            rrca
loc_C1F2:   rrca
            call    nc,    keybScan3
            dec     c
            jp nz,  loc_C1F2

            ; ����
            dec     de
            add     hl, hl
            inc     hl
            ld      a, h
            add     0F0h
            ld      h, a
            jp c,   loc_C1DD

            ; ����� ��55 �� ���������
            call    setKeybMode82

            ; ���������
            ld      a, b

            ; �������������� ��������
            pop     de
            pop     bc
            pop     hl
            ret

;----------------------------------------------------------------------------

keybScan3:  push    de
            push    af
            ld      a, 6    ; e += (6 - c) * 16
            sub     c
            add     a, a
            add     a, a
            add     a, a
            add     a, a
            add     a, e
            ld      e, a
            ld      a, (de) ; b = *de;
            ld      b, a
            pop     af
            pop     de
            ret

