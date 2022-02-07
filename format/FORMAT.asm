;----------------------------------------------------------------------------
; MXOS
; FORMAT.COM
;
; ���������:
; - "����� �����" �� ��������� Y (������������� ��� c���c�)
;    �������� FORMAT.COM B: Y
;
; 2022-01-14 ����������������� � ���������� SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

    ORG 0F100h

    ld      a, (de)         ; � de ��������� ����� ������ ����������
    cp      20h
    jp nc,  ReadParams      ; ������, ���� ���� ��������

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

ReadParams:
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
    ld      (str_A_Y_N), a  ; �������� 'A' � ������ ��������� �� �������� �����
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
    push    af              ; � � �����

    ; ���������� ��������� ���� �������
    ld      e, 01h          ; ����� ����� � a
    call    bios_fileGetSetDrive

    ; ������ ������ ����� � de
    ld      b, 3            ; ����� 3 - �������� ������
    call    bios_diskDriver ; de = ������ � ���������

    ; ��������� ���������� ������� � "������" ���������
    ld      hl, FAT_SIZE / FAT_ITEM_SIZE
    call    sub_hl_de   ; hl = ���������� ��������� � fat ����� ���������� ��������� �� �����
    push    hl

    ; �������� ������� ����� fat (de * CLUSTER_SIZE ���� 0000h)
                        ; de = ������ � ���������
    ld      hl, buffer  ; hl = ����� ������
    ld      bc, 0       ; bc = ����� ��� ����������
    call    memset

    ; �������� ����� 0001h ("������" ������) � ���������� ������ fat
    ; (� ROM-����� 48 �� ��� ��� ��� ���� 48 ��, � �.�.)
                        ; hl = ����� ������ (���������� ����������)
    pop     de          ; de = ���������� ��������� � fat ����� ���������� ��������� �� �����
    ld      bc, 1       ; bc = ����� ��� ����������
    call    memset

    ; �������� ����� 0001h ("������" ������) � ����������������� ������ fat, ��������������
    ; ����� fat � ��������� ��������
    ld      hl, buffer                      ; hl = ����� ������
    ld      de, FAT_CLUSTERS + DIR_CLUSTERS ; de = ���������� ����� fat � ��������� ��������
    ld      bc, 1                           ; bc = ����� ��� ����������
    call    memset

    ; �������� ����� 0001h ("������" ������) � ������ fat, ��������������
    ; �������� �������� � ����� 64 �� ������, ���� ��� RAM-����
    pop     af  ; a = ����� �����
    cp      1   ; RAM-���� ��� ���� 1 ("B:")
    call z, MarkRamDiskBads

    ; ������ FAT �� ����
    ld      hl, buffer
    ld      de, 0               ; �������� � �������� ����� 0
    ld      c,  FAT_CLUSTERS    ; ������� ���������
    call    WriteBuffer

    ; �������� ������� �������� �������� (DIR_SIZE ���� 0FFh)
    ld      hl, buffer                      ; hl = ����� ������
    ld      de, DIR_SIZE / FAT_ITEM_SIZE    ; de = ���������� ��������
    ld      bc, 0FFFFh                      ; bc = ����� ��� ����������
    call    memset

    ; ������ �������� �� ����
    ld      hl, buffer
    ld      de, FAT_CLUSTERS ; �������� � �������� ����� FAT_CLUSTERS
    ld      c,  DIR_CLUSTERS ; ������� ���������
    call    WriteBuffer

    ; ����� � ��
    ret

;----------------------------------------------------------------------------

; ������ ������ �� ����
; c - ������� ���������
; de - ����� ������� ��������
WriteBuffer:
    ld      b,  1   ; ����� 1 - ������
    ld      hl, buffer
WriteBufferLoop:
    call    bios_diskDriver
    inc     de      ; ��������� �������
    inc     h       ; �������� ���� � ������ (������ �������� 256 ����)
    dec     c
    jp nz,  WriteBufferLoop
    ret

; �������� �������� ������� RAM-����� ��� "����"
MarkRamDiskBads:
    ld      b,  FAT_CLUSTERS / FAT_ITEM_SIZE    ; b = ���������� 64�� ����� RAM-�����
    ld      hl, buffer + 255 * FAT_ITEM_SIZE    ; hl = ����� ������� "����" � ������ fat
    ld      de, 256 * FAT_ITEM_SIZE - 1         ; de = ���������� ������ ��� �������� � ���������� "����"
MarkRamDiskBadsLoop:
    ld      (hl), 1
    inc     hl
    ld      (hl), 0
    add     hl, de
    dec     b
    jp nz,  MarkRamDiskBadsLoop
    ret

; ���������� ������ �� ������ hl ������ bc ����������� ���� de
memset:
    ld      a, d
    or      e
    ret     z   ; ���� de == 0, �������
memsetLoop:
    ld      a, c
    ld      (hl), a
    inc     hl
    ld      a, b
    ld      (hl), a
    inc     hl
    dec     de
    ld      a, d
    or      e
    jp nz,  memsetLoop
    ret

; hl = hl - de
sub_hl_de:
    ld    a, l
    sub   e
    ld    l, a
    ld    a, h
    sbc   d
    ld    h, a
    ret

; ����� ��������� 'ABORTING'
Abort:
    ld      hl, str_Aborting
    jp      bios_printString

    ; ����� ��������� 'INVALID DRIVE LETTER'
InvalidDrive:
    ld      hl, str_InvalidDrive
    jp      bios_printString

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

buffer = 0000h ; 0D100h ; ����� ������

;----------------------------------------------------------------------------

    END
