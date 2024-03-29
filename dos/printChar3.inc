;+---------------------------------------------------------------------------
; MXOS
; ��������� ��������� ����� ��� ������ �������
;
; �� �����
;  a - ������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

printChar3: ; ��� 7
            sub   7
            jp z, beep
            ; ��� 8
            dec   a
            jp z, printChar_c08
            ; ��� 10
            sub   2
            jp z, printChar_c0A
            ; ��� 12
            sub   2
            jp z, printChar_c0C
            ; ��� 13
            dec   a
            jp z, printChar_c0D
            ; ��� 24
            sub   11
            jp z, printChar_c18
            ; ��� 25
            dec   a
            jp z, printChar_c19
            ; ��� 26
            dec   a
            jp z, printChar_c1A
            ; �����������, ���� ��� 1Fh (31)
            sub   5
            ret nz
            ; ���  1Fh (31) ������� �����
            call  clearScreen

            ; ����������� � printChar_c0C

; ---------------------------------------------------------------------------
; ��������� ���� 0Ch. ������ � ����� ������� ���� ������

printChar_c0C:
            ; bios_vars.cursorY = 8
            ld    l, 8

            ; ����������� � printChar_c0D

; ---------------------------------------------------------------------------
; ��������� ���� 0Dh. ������ � ������ ������

printChar_c0D:
            ; bios_vars.cursorX = 0
            xor   a
saveVCursorHA:
            ld    h, a
saveVCursor:
            ld    (bios_vars.cursorY), hl
            ret

; ---------------------------------------------------------------------------
; ��������� ���� 8h. �����

printChar_c08:    ; cursorX -= 3
            ld    a, h
            sub   3

            ; ���� ��� ��������, ��������� ��������� ������� � �����
            jp nc,  saveVCursorHA

            ; cursorX = 189
            ld    h, 189
            ld    (bios_vars.cursorY), hl

            ; ����������� � printChar_c19

; ---------------------------------------------------------------------------
; ��������� ���� 19h. �����

printChar_c19:    ; ���� cursorY < 18, �� �����. ����� cursorY -= 10
            ld    a, l
            sub   0Ah
            ret c 
            cp    8
            ret c 

saveVCursorLA:    ; ��������� ��������� ������� � �����
            ld    l, a
            jp    saveVCursor

; ---------------------------------------------------------------------------
; ��������� ���� 18h. ������

printChar_c18:    ; ���� cursorX+3 >= 192, �� ������� �� printChar_c0A
            ; ����� cursorX += 3
            ld    a, h
            add   a,3
            cp    0C0h
            jp c, saveVCursorHA
            jp    printChar_c0A

