;+---------------------------------------------------------------------------
; MXOS
; ������ RKS �����
;
; 2021-01-27 ����������� SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"
    
; ����� �������� ��������
MON_ADDR        =  0C000h 

; ����� ���������� ������ �������� ��������
MON_ADDR_TEMP   =  0E000h 

    ORG 0F800h

    ; � de ��������� ����� ������ ����������
    ld      a, (de)
    cp      20h
    jp c,   noargsRet ; �������� �� �����, �������

    push    de
    ld      hl, txtLoading
    call    bios_printString
    pop     hl
    push    hl
    call    bios_printString
    pop     hl

    ; �������� ���������
    ; ���������� ����� ����� � ������������ ����������
    ld      de, nameBuffer
    call    bios_fileNamePrepare   ; hl = ��� ����� = ������ ����������
    ex      de, hl

    ; ��������� ����
    ld      hl, nameBuffer
    call    bios_fileLoad
    jp c,   fileNotFoundRet

    ; �������� ����� �������� (= ����� �������) ����� � de
    ld      de, FILE_DESCRIPTOR.loadAddress
    add     hl, de
    ld      e, (hl)
    inc     hl
    ld      d, (hl)

    ; �������� � ���� ��� �������� ��������
    push    de

    ld      hl, txtLoading
    call    bios_printString
    ld      hl, txtMonitorPath
    call    bios_printString

    ; �������� ��������
    ; ���������� ����� ����� a:MON2.SYS
    ld      hl, txtMonitorPath
    ld      de, nameBuffer
    call    bios_fileNamePrepare

    ; ��������� ���� MON2.SYS �� ��������� ����� - �� ������ nc.com
    ld      hl, nameBuffer
    ld      de, MON_ADDR_TEMP   ; �������� ����� �������� ����� �� de
    call    bios_fileLoad2      ; ����� ������������ �������! BIOS 4.50 � ������ 
    jp c,   popFileNotFoundRet

    ; �������� ������ ����� �������� � de
    ld      de, FILE_DESCRIPTOR.size
    add     hl, de
    ld      e, (hl)
    inc     hl
    ld      d, (hl)

    ; hl = MON_ADDR_TEMP
    ; de = MON_ADDR_TEMP + ������ ��������
    ld      hl, MON_ADDR_TEMP
    ex      de, hl
    add     hl, de
    ex      de, hl

    ; �������� ������� �� ����� C000h. ��� ���� �� ����� BIOS, � ��������
    ; ������� �� ������ ����������. ������� � ����� ��� ��������� �����.
    ld      bc, MON_ADDR
    call    memcpy

    ; �������������� STD ���������� �����
    ld      a, 82h              ; ����� A, C - �����, ���� B - ����
    ld      (IO_KEYB_MODE), a
    ld      a, 0h               ; ����� ����
    ld      (IO_KEYB_C), a

    ; ������ ��������. ������� ��� ��������� � ����� STD, ����������������,
    ; � ��������� ��������� �� ������ �� ������� �����.
    jp      MON_ADDR

    ; ������������ �� hl � bc � ����������� �������, ���� hl �� ����� de
memcpy:
    ld      a, (hl)
    ld      (bc), a
    call    cmp_hl_de
    ret z
    inc     hl
    inc     bc
    jp      memcpy

cmp_hl_de:
    ld      a, l
    cp      e
    ret nz  
    ld      a, h
    cp      d
    ret

popFileNotFoundRet:
    pop     de ; ��������������� ���� ��� ����������� ���������� ������
fileNotFoundRet:
    ld      hl, txtFileNotFound
    jp      bios_printString

noargsRet:
    ld      hl, txtNoArgs
    jp      bios_printString

    ; Esc + ) �������� KOI-8 �� ����� ��������� ������
txtLoading:
    DB 0Ah,"LOADING ",0

txtFileNotFound:
    DB 0Ah,"FILE NOT FOUND",0

txtNoArgs:
    DB 0Ah,"USAGE: LAUNCH.COM <FILE.RKS>",0

txtMonitorPath:
    DB "A:MON2.MON",0

nameBuffer:
    BLOCK 10,0

    END
