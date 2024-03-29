;----------------------------------------------------------------------------
; MXOS NC.COM
; ����� ������ �� ������ hl ������ b ��� �� �����������.
; ������� ������ �������� �������, ����� �� ������� ����� ���
; ������������������� ��� ����������� �����.
;
; ����:
;   hl = ����� ������
;   b  = ����. ����� ������
;   c  = ����������
;
; �����:
;   hl = ����� ����� ������ + 1
;
; 2022-01-31 SpaceEngineer
;----------------------------------------------------------------------------

printString2:
    push    af
    push    de

printString2loop:
    ld      a, (hl)
    cp      c
    jp c,   printString2exit

    push    bc
    push    hl

    call    bios_printCharReal

    pop     hl
    pop     bc

    inc     hl
    dec     b
    jp nz,  printString2loop

printString2exit:
    pop     de
    pop     af
    ret
