;----------------------------------------------------------------------------
; MXOS
; ROM.COM - ������� ���-����� ����� ��55
; ��������� ������������� DISK-H.COM
; �������������� ������ ���� 64� ��������
;
; ������� ����� ��� ������� (����� ��������� � �������� b):
;   b == 1 - �������� ������ (256 ����, ����� ������� � de, ����� ������ � hl);
;   b == 2 - �������  ������ (256 ����, ����� ������� � de, ����� ������ � hl);
;   b == 3 - ������ ������ ����� � �������� (�������� � de, ����� DISK_INFO � hl).
;
; 2022-02-15 ����������� SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; ������� ��� �������� ������� ��55
LATCH_0 = 0Ah   ; ��� ������� ��22 = 0
LATCH_1 = 0Bh   ; ��� ������� ��22 = 1
WRITE_0 = 0Ch   ; ��� ������ = 0
WRITE_1 = 0Dh   ; ��� ������ = 1
READ_0  = 0Eh   ; ��� ������ = 0
READ_1  = 0Fh   ; ��� ������ = 1

; ����� ����������� ����� ��55
MASK_STADBY = 0C0h  ; ��� ������� ��22 = 0, ��� ������ = 1, ��� ������ = 1
MASK_READ   = 040h  ; ��� ������� ��22 = 0, ��� ������ = 1, ��� ������ = 0

; ����������, ���� ����������� �������
DRIVE       = 7     ; "H"

; ����� ���������� �� ��������� (64 ��)
DISK_SECTORS = 64 * 4

; ����� ���-�����:
; ���� A - ������ (����)
; ���� B - ����� [A0-A7]  (�����)
; ���� C - ����� [A8-A15] (�����)
; ����� �6 ����� ���������� (��� 11) - ��������� ��� (���. 1)

;----------------------------------------------------------------------------

    ORG     0DE00h

    ; ���������� ������� ��� ���������� 7 ("H")
    ld      a, DRIVE
    ld      hl, Driver
    call    bios_installDriver

    ; ���������� ����� ��������� DISK_INFO � �������
    ld      (v_diskInfo), hl
    ret

Driver:

    ; ������ ������ �������
    ld      a, b
    cp      1
    ret z               ; ������ - �� ��������������
    cp      2
    jp z,   FuncRead    ; ������
    cp      3
    jp z,   FuncSize    ; ������
    ret                 ; ����� �����

;-----------------------------------------------------------------------------------
; ������� 3 - ����������� ������ ����������
; �����:
;  de = ���������� �������� (1 - ���� �� ��������������)

FuncSize:
    ; ������ ���� isValid �� ��������� DISK_INFO
    ld      hl, (v_diskInfo)            ; hl = ����� ���������
    ld      de, DISK_INFO.isValid       ; de = �������� ����
    add     hl, de                      ; hl = ����� ����
    ld      a, (hl)                     ; a = isValid

    ; ���� ���� �� ��������������, ���������� de = DISK_SECTORS
    ld      de, DISK_SECTORS
    cp      DISK_VALID
    jp nz,  fsExit

    ; ����� ���������� � de ���� totalSectors �� ��������� DISK_INFO
    ld      hl, (v_diskInfo)            ; hl = ����� ���������
    ld      de, DISK_INFO.totalSectors  ; de = �������� ����
    add     hl, de                      ; hl = ����� ����
    ld      e, (hl)
    inc     hl
    ld      d, (hl)

fsExit:
    ; ���������� � hl ����� ��������� DISK_INFO
    ld      hl, (v_diskInfo)
    ret

;----------------------------------------------------------------------------
; ������� 2 - ������ �������
; ����:
;   de = ����� �������
;   hl = ����� ������ � ������

FuncRead:
    ; ���������� ���������
    push    hl
    push    de
    push    bc

    ; ��������� ������ ��55
    ld      a, 90h              ; ���� A - ����, ����� B � C - ����� 
    ld      (IO_PROG_MODE), a
    ld      a, 0Dh              ; ����� �6 ����� ���������� = 1
    ld      (IO_KEYB_MODE), a

    ; de = ����� ������ ������� � 64� �������� ���-�����
    ; � ����� �������� (d) ��������, �.�. �������������� ������ ���� ��������
    ld      d, e
    ld      e, 0

ReadLoop:
    ex      de, hl          ; hl = ����� � ���, de = ����� � ������
    ld      (IO_PROG_B), hl ; ���� B = ������� ���� ������, ���� C = ������� ���� ������
    ld      a, (IO_PROG_A)  ; a = ���� �� ���
    ex      de, hl          ; hl = ����� � ������, de = ����� � ���
    ld      (hl), a         ; ��������� ���� � ������
    inc     hl              ; ��������� ����� � ������
    inc     e               ; ��������� ����� � ���
    jp nz,  ReadLoop        ; ������ �����, ���� ����� � ��� �� ��������� XX00h

    ; �������������� ������, ��������� � �����
    push    af
    ld      a, 0Ch          ; ����� �6 ����� ���������� = 0
    ld      (IO_KEYB_MODE), a
    ld      a, 9Bh          ; ����� A, B, C - ����
    ld      (IO_PROG_MODE), a
    pop     af
    pop     bc
    pop     de
    pop     hl
    ret

;----------------------------------------------------------------------------
; ������ ����� �� ���� ���� ������
; ����:
;   de = ����� � 64� ��������
; �����:
;   a  = ���� ������
;   ��� ������ ������� �������� (C7 = 0)

ReadByteFromChip:
    ex      de, hl
    ld      (IO_PROG_B), hl
    ld      a, (IO_PROG_A)
    ex      de, hl
    ret

;----------------------------------------------------------------------------
; ����������

v_diskInfo: DW 0

;----------------------------------------------------------------------------

    ; �������� - ROM.COM �� ������ �������� �� ��� �������
    ASSERT_DONT_FIT 0E000h

    END
