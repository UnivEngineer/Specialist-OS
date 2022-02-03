;----------------------------------------------------------------------------
; MXOS
; FLPAGE.COM - ������������� ������� ����-����� ��55
;
; 2022-01-25 ����������� SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

    ORG 0F100h

    ; ����� ������� ��������
    ld      hl, txtCurrentPage
    call    bios_printString
    ld      a, (bios_vars.flashPage)
    call    bios_printHexByte

Repeat:
    ; ����� �������
    ld      hl, txtEnterPage
    call    bios_printString

    ; ���� ������
    ld      hl, v_input
    ld      de, v_input + 23
    call    bios_input

    ; ������ ������
    ex      de,hl
    call    bios_strToHex
    jp z,   Repeat

    ; ��������� ��������
    ld      de, 20h ; ����. 1Fh + 1
    call    cmp_hl_de
    jp nc,  Repeat

    ; ���������� ���������
    ld      a, l
    ld      (bios_vars.flashPage), a
    ret

cmp_hl_de:
    ld      a, h
    cp      d
    ret nz  
    ld      a, l
    cp      e
    ret

txtCurrentPage:
    DB 0Ah,"CURRENT PAGE NUMBER: ",0

txtEnterPage:
    DB 0Ah,"ENTER NEW PAGE NUMBER (0-1F): ",0

v_input:

    END
