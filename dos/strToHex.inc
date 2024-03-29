;+---------------------------------------------------------------------------
; MXOS
; ������������� ������ � 16-������ �����
; ������������ �� RAMFOS
;
; �� �����
;  de - ������
;
; �� ������
;  bc       - �����������
;  de       - ��������� ������ �� ������� ��� ������ ������
;  jz       - ������
;  jnz, jc  - ����� ������
;  jnz, jnc - ������ ��� �������
;  hl       - �����
;
; 2013-11-01 ����������������� vinxru
;----------------------------------------------------------------------------

strToHex:
            ld  hl, 0       

strToHex_loop:    ; ������ �������
            ld    a, (de)
            inc   de

            ; ���� ����� ������, ������� � ������� NZ, c
            cp    0Dh
            jp z, ora_a_stc_ret

            ; ���� ������ ��� �������, ������� � ������� NZ, NC
            cp    ','
            jp z, ora_a_ret         
            cp    ' '
            jp z, ora_a_ret

            ; ���� ������������ ������, ������� � ������� �,Z
            sub   '0'
            jp M, xra_a_stc_ret           

            ; ���� ��� �����
            cp    10
            jp M, strToHex_0

            ; ���� ������������ ������, ������� � ������� �,Z
            cp    11h
            jp M, xra_a_stc_ret
            cp    17h
            jp p, xra_a_stc_ret

            ; ��� ����� A..F
            sub   'A'-'0'-10

strToHex_0: 
            ; hl *= 16
            add   hl, hl
            add   hl, hl
            add   hl, hl
            add   hl, hl

            ; ���� ������������, ������� c ������� �,Z
            jp c, xra_a_stc_ret
            
            ; hl += bc
            push bc
            ld   b, 0
            ld   c, a
            add  hl, bc
            pop  bc

            jp    strToHex_loop

xra_a_stc_ret:    ; ����� ����� �������
            ld  c, '?'
            call      printChar
            xor a

ora_a_stc_ret:
            or  a
            scf
            ret

ora_a_ret:
            or  a
            ret
