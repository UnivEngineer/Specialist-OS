;+---------------------------------------------------------------------------
; MXOS
; FLASH.cOM - ������� ����-����� ��55
;
; ������� ����� ��� ������� (����� ��������� � �������� �):
; 1 - �������� ������ (256 ����, ����� ������� � �������� D, ����� ������ � hl);
; 2 - ������� ������  (256 ����, ����� ������� � �������� D, ����� ������ � hl);
; 3 - ������ ������ ����� (� ��������, � �������� �).
;
; 2022-01-24 ����������������� � ���������� SpaceEngineer
;
;----------------------------------------------------------------------------

; ������� DOS
fileGetSetDrive =  0C842h ; ��������/���������� �������� ����������
installDriver   =  0C860h ; ���������� ������� ����������

; �����
IO_KEYB_MODE  =  0FFE3h 
IO_PROG_A     =  0FFE4h 
IO_PROG_B     =  0FFE5h 
IO_PROG_C     =  0FFE6h 
IO_PROG_MODE  =  0FFE7h 

    ORG     0FA00h

    ; ���������� ������� ��� ���������� 7 ("H")
    ld      a, 7
    ld      hl, Driver
    jp      installDriver

    ; ������� ���������� 7 ("H")
    ld      e, 1
    ld      a, 7
    jp      fileGetSetDrive

Driver:
    ld      a, e
    cp      1
    ret z             ; ������ �� ��������������
    push    hl
    push    de
    push    bc

    ; ��c������ ����� ��55
    ld      a, 90h
    ld      (IO_PROG_MODE), a
    ld      a, 0Dh              ; ??? ���� ����������
    ld      (IO_KEYB_MODE), a

    ld      a, e
    cp      3
    jp z,   FuncGetSize
    cp      2
    jp nz,  Exit

    ; ������ �����
    ; ����:
    ; d  - ����� �����
    ; hl - ����� ������ � ������
FuncRead:
    xor     a
    ld      e, a
ReadLoop:
    call    Read
    ld      (hl), a
    inc     hl
    inc     e
    jp z,   Exit
    jp      ReadLoop

    ; ����������� ������ ����������
    ; �����:
    ; a - ���������� ��������
FuncGetSize:
    xor     a
    ld      b, a
    ld      D, a
    ld      e, 4 ; ��������� ���� � �������
LbL3:
    call    Read
    cp      0FFh
    jp nz,  LbL4 ; ��������, ���� �� ����� �����
    inc     b    ; ��������� ������� ������
LbL4:
    inc     e    ; ��������� ���� � �������
    ld      a, e
    cp      0C0h
    jp nz,  LbL3
    ld      a, 0C0h
    sub     b

    ; �������������� ������ � �����
Exit:
    push    af
    ld      a, 0Ch
    ld      (IO_KEYB_MODE), a
    ld      a, 9bh
    ld      (IO_PROG_MODE), a
    pop     af
    pop     bc
    pop     de
    pop     hl
    ret

    ; ������ ������
    ; ����:
    ; de - ����� � �����
    ; �����:
    ; a - ������
Read:
    ex      de, hl
    ld      (IO_PROG_B), hl
    ld      a, (IO_PROG_A)
    ex      de, hl
    ret

    END
