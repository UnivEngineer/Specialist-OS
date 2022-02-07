;+---------------------------------------------------------------------------
; MXOS
; FORMAT.COM
;
; 2022-01-14 ����������������� � ���������� SpaceEngineer
;
; ���������:
; - "����� �����" �� ��������� Y (������������� ��� c���c�)
;    �������� FORMAT.COM B: Y
;
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

    ORG 0F100h

    ld      a, (de)         ; � de ��������� ����� ������ ����������
    cp      20h
    jp nc,  Readparams      ; ������, ���� ���� ��������

    ; ������ ����� ����� ��� ��������������
chooseDrive:
    ld      hl, str_ChoseDrive
    call    bios_printString; ����� ��������� 'CHOOSE DRIVE: '
    call    bios_getch      ; �������� ������� �������
    ld      c, a
    call    bios_printChar
    cp      21h             ; ��������� c ��������
    jp c,   Abort           ; ����� � ��, ���� ������ ��� �����
    ld      b, a            ; ��������� ����� ����� � b
    jp      ConfirmRequest

Readparams:
    ld      b, a            ; ��������� ����� ����� � b

SearchLoop1:                ; ����� ������� ������� � ������ ����������
    ld      a,(de)
    cp      21h
    jp c,   SearchLoop2
    inc     de
    jp      SearchLoop1

SearchLoop2:                ; ������� ����������� ��������
    ld      a,(de)
    cp      20h
    jp nz,  SearchLoopExit
    inc     de
    jp      SearchLoop2

SearchLoopExit:     
    cp      'Y'             ; ���� ������ �������� 'Y', ������� � ��������������
    jp z,   Confirmed

    ; ������������� ��������������
ConfirmRequest:
    ld      a, b
    ld      (str_A_Y_N),a   ; �������� 'A' � ������ ��������� �� �������� �����
    ld      hl, str_Format
    call    bios_printString; ����� ��������� 'FORMAT <�����>: [Y/N]?'
    call    bios_getch      ; �������� ������� �������
    ld      c, a
    call    bios_printChar
    cp      'Y'             ; ��������� c 'Y'
    jp nz,  Abort           ; ����� � ��, ���� �� 'Y'
    
Confirmed:
    ld      a, b            ; ������������ ����� ����� � a

    ; ����� ����� � �������� a
Format:
    sub     41h             ; ����� �����
    cp      08h             ; ������������ ����� ����� = 7
    jp nc,  InvalidDrive    ; �����, ���� �������� ����� �����
    ld      b, a            ; ��������� ����� ����� � b

    ; ���������� ��������� ���� �������
    ld      a, b    ; ����� ����� � a
    ld      e, 01h
    call    bios_fileGetSetDrive

    ; ������ ������ ����� � a
    ld      e, 03h
    call    bios_diskDriver
    ld      e, a    ; ��������� ������ ����� � e
    dec     a

    ; �������� ������ ��������� FAT (� ���� �����)
    ld      hl, buffer
CreateFATLoop1:
    ld      (hl), 0
    inc     l
    dec     e
    jp nz,  CreateFATLoop1

    ; �������� ��� 01h � ��������, ��������������� "������" ��������
    ; (� RAM-����� ��� ���������, � ROM-����� 48 �� ��� ��� ��� ���� 48 ��, � �.�.)
CreateFATLoop2:
    inc     a
    jp z,   WriteToDisk
    ld      (hl), 01h
    inc     l
    jp      CreateFATLoop2

    ; ������ FAT �� ����
    ; d - ����� �������
    ; e - ��� ��������
WriteToDisk:
    ld      de, 0001h  ; ������ ������� ����� 0
    call    bios_diskDriver

    ; �������� ������ ��������� �������� (256 ����)
CreateCatLoop:
    ld      (hl), 0FFh
    inc     l
    jp nz,  CreateCatLoop

    ; ������ �������� c ������ 3 �� 1
    ld      d, 03h
WriteLoop:
    call    bios_diskDriver
    dec     d
    jp nz,  WriteLoop

    ; ����� � ��
    ret

Abort:
    ld      hl, str_Aborting
    jp      bios_printString     ; ����� ��������� 'ABORTING'

InvalidDrive:
    ld      hl, str_InvalidDrive
    jp      bios_printString     ; ����� ��������� 'INVALID DRIVE LETTER'

;----------------------------------------------------------------------------
; ������

str_Format:
    DB 0Ah,"FORMAT "

str_A_Y_N:
    DB "A: [Y/N]? ",0

str_ChoseDrive:
    DB 0Ah,"CHOOSE DRIVE: ",0

str_InvalidDrive:
    DB 0Ah,"INVALID DRIVE LETTER",0

str_Aborting:
    DB 0Ah,"ABORTING",0

buffer = 0D100h ; �����

;----------------------------------------------------------------------------

    END
