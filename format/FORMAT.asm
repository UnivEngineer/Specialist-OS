;----------------------------------------------------------------------------
; MXOS
; FORMAT.COM
;
; ���������:
; - "����� �����" �� ��������� Y (������������� ��� c���c�)
;    �������� FORMAT.COM B: Y
;
; 2022-01-14 ����������������� � ������������ SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

SECTOR_SIZE = 256           ; ������ ������� � ������

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
    jp c,   errAbort        ; ����� � ��, ���� ������ ��� �����
    call    bios_upperCase  ; ������ ����� � ������� �������
    ld      b, a            ; ��������� ����� ����� � b
    jp      ConfirmRequest

ReadParams:
    call    bios_upperCase  ; ������ ����� � ������� �������
    ld      b, a            ; ��������� ����� ����� � b

SearchLoop1:                ; ����� ������� ������� � ������ ����������
    ld      a, (de)
    cp      21h
    jp c,   SearchLoop2
    inc     de
    jp      SearchLoop1

SearchLoop2:                ; ������� ����������� ��������
    ld      a, (de)
    cp      20h
    jp nz,  AnalyzeParam
    inc     de
    jp      SearchLoop2

AnalyzeParam:
    cp      21h
    jp c,   SearchExit      ; ����� ������ ����������
    call    bios_upperCase  ; ������ ����� � ������� �������
    cp      'Y'             ; ���� ������ �������� 'Y', ��������� ���� "������������� ��� �������������"
    jp nz,  AnalyzeParam1
    ld      a, 0FFh
    ld      (v_FormatConfirmed), a
    jp      SearchLoop1     ; ���������� ������������� ������ ����������
AnalyzeParam1:
    cp      'R'             ; ���� ������ �������� 'R', ��������� ���� "������������� ��� RAM-����"
    jp nz,  SearchLoop1     ; ����� ���������� ������������� ������ ����������
    ld      a, 0FFh
    ld      (v_FormatRAMdisk), a
    jp      SearchLoop1     ; ���������� ������������� ������ ����������

SearchExit:
    ld      a, (v_FormatConfirmed)
    or      a
    jp nz,  Confirmed

    ; �������� Y �� ������ - ����� ������� �� ������������� ��������������
ConfirmRequest:
    ld      a, b
    ld      (str_A_Y_N), a  ; �������� 'A' � ������ ��������� �� �������� �����
    ld      hl, str_Format
    call    bios_printString; ����� ��������� 'FORMAT <�����>: [Y/N]?'
    call    bios_getch      ; �������� ������� �������
    call    bios_upperCase  ; ������ ����� � ������� �������
    ld      c, a
    call    bios_printChar
    cp      'Y'             ; ���� ������ �������� 'Y', ������� � ��������������
    jp z,   Confirmed
    jp      errAbort        ; ����� � ��, ���� �� 'Y/y'
    
Confirmed:
    ld      a, b            ; ������������ ����� ����� � a

    ; ����� ����� � �������� a
Format:
    sub     41h             ; ����� �����
    cp      08h             ; ������������ ����� ����� = 7
    jp nc,  errInvalidDrive ; ����� � ��, ���� �������� ����� �����
    ld      b, a            ; ��������� ����� ����� � b

    ; ���������� ��������� ���� �������
    ld      e, 01h          ; ����� ����� � a
    call    bios_fileGetSetDrive

    ; ������ ������ ����� � de
    ld      b, 3            ; ����� 3 - �������� ������
    call    bios_diskDriver ; de = ������ � ��������

    ; ���� de == 0, ������� ����� �� ����������
    ld      a, d
    or      e
    jp z,   errNoDriver     ; ����� � ��, ���� ��� ��������

    ; C�������� ���������� �������� ����� � ��������� ������������ �������
    ex      hl, de
    ld      (v_TotalSectors), hl

    ; ���� ��� �������� 'R', ������ ����� ���� �� "RAM DISK"
    ld      a, (v_FormatRAMdisk)
    or      a               ; ��� �� �������� 'R'?
    jp z,   CalcParams      ; ���� �� ����, ���������

    ; ������ ����� ����
    push    hl
    ld      hl, str_RAM_DISK
    ld      de, str_RAM_DISK + 11
    ld      bc, v_VolLabel
    call    bios_memcpy_bc_hl
    pop     hl

CalcParams:

    ; ����������� ������ ������� fat
    add     hl, hl              ; hl * 2 - ������ fat � ������
    ld      (v_FatBytes), hl
    ld      de, SECTOR_SIZE     ; de = ������ ������� � ������
    call    bios_div_hl_de      ; hl = ���������� �������� fat
    ld      (v_FatSectors), hl  ; ��������� ��������

    ; ����������� ����� ������� ������� ��������
    inc     hl                      ; hl = ������ fat + 1 ����������� ������
    ld      (v_DirStartSector), hl  ; ��������� ��������

    ; ����������� ������ ��������
    ld      hl,(v_DirFiles)     ; ���������� ������������ � �������� ��������
    ld      de, FILE_DESCR_SIZE ; ������ ����������� � ������
    call    bios_mul_hl_de      ; ������ �������� � ������
    ld      (v_DirBytes), hl
    ld      de, SECTOR_SIZE
    call    bios_div_hl_de      ; hl = ���������� �������� ��������
    ld      (v_DirSectors), hl

    ; ����������� ����� ������� ������� ������� ������
    ex      hl, de
    ld      hl, (v_DirStartSector)
    add     hl, de                  ; hl = ������ �������� + ������ fat + 1 ����������� ������
    ld      (v_DataStartSector), hl ; ��������� ��������

    ; ���������� ������ ������� ������
    ex      hl, de
    ld      hl, (v_TotalSectors); hl = ���������� �������� �� �����
    call    bios_sub_hl_de      ; hl = ���������� �������� �� ����� ����� ��� �����������������
    ld      (v_DataSectors), hl ; ��������� ��������

;----------------------------------------------------------------------------
; ����������� ������

    ; �������� ��������� ������������ ������� � �����
    ld      hl, v_HeaderStart
    ld      de, v_HeaderEnd
    ld      bc, buffer
    call    bios_memcpy_bc_hl

    ; ��������� ��������� ������ 0FFh
    ld      hl, buffer      + v_HeaderEnd - v_HeaderStart
    ld      de, SECTOR_SIZE - v_HeaderEnd + v_HeaderStart
    ld      b, 0FFh
    call    memset

    ; ���������� ����������� ������ �� ����
    ld      hl, buffer          ; ����� ������
    ld      de, 0               ; ������ ����� 0
    ld      c,  1               ; ������� ��������
    call    WriteBuffer

;----------------------------------------------------------------------------
; FAT

    ; ������� ����� fat ������ ������������
    ld      hl, (v_DataStartSector) ; hl = ������� �������� ������ ����������� ��������, fat � ���������
    dec     hl                      ; ������ ��� ������ fat ���������������
    dec     hl
    add     hl, hl                  ; hl *= 2 - ������� ���� � ����� ������� fat ������ ������������ (������ = �������!)
    push    hl

    ; �������� ��� ������ fat (������ �������), ����� ���������
    ld      hl, (v_FatBytes)    ; ������ fat � ������
    pop     de                  ; ������� ���� � ����� ������� fat ������ ������������
    push    de
    call    bios_sub_hl_de      ; ��������� fat �� �������
    ex      hl, de              ; de = ������� ���� ���������
    ld      hl, buffer          ; ����� ������
    ld      b, 0                ; ���� ��� ����������
    call    memset

    ; �������� ����� FFFFh (����������������� �������) � ��������� ������ fat
    pop     de                  ; ������� ���� �������� �� ����� ������� fat
    ld      b, 0FFh             ; ���� ��� ����������
    call    memset

    ; �������� ����� FFFFh (����������������� �������) � ������ ��� ������������������ ������ fat
    ld      hl, 0FFFFh
    ld      (buffer),   hl
    ld      (buffer+2), hl

    ; ���� ��� RAM-����, �������� ����� FFF7h (������ ������) � ������ fat,
    ; �������������� �������� �������� � ����� 64 �� ������
    ld      a, (v_FormatRAMdisk)
    or      a                   ; ��� �� �������� 'R'?
    call nz,MarkRamDiskBads

    ; ���������� FAT �� ����
    ld      hl, (v_FatSectors)  ; ���������� �������� fat
    ld      c, l                ; ���������� �������� fat (�������, ��� �� ������ 256)
    ld      hl, buffer          ; ����� ������
    ld      de, 1               ; �������� � ������� ����� 1
    call    WriteBuffer

;----------------------------------------------------------------------------
; �������� �������

    ; �������� ������� �������� (de ���� 0FFh)
    ld      hl, (v_DirBytes)    ; ������ �������� � ������
    ex      hl, de
    ld      hl, buffer          ; ����� ������
    ld      b, 0FFh             ; ���� ��� ����������
    call    memset

    ; ���������� ������� �� ����
    ld      a, (v_DirSectors)
    ld      c, a                ; ���������� �������� �������� (�������, ��� �� ������ 256)
    ld      hl, (v_FatSectors)  ; ���������� �������� fat
    inc     hl                  ; ���������� �������� fat + 1 = ��������� ������ ��������
    ex      hl, de              ; ��������� ������ ��������
    ld      hl, buffer          ; ����� ������
    call    WriteBuffer

    ; ����� � ��
    ret

;----------------------------------------------------------------------------
; ������ ������ �� ����
; c - ������� ��������
; de - ����� ������� �������

WriteBuffer:
    ld      b,  1   ; ����� 1 - ������
    ld      hl, buffer
WriteBufferLoop:
    call    bios_diskDriver
    inc     de      ; ��������� ������
    inc     h       ; �������� ���� � ������ (������ ������� 256 ����)
    dec     c
    jp nz,  WriteBufferLoop
    ret

;----------------------------------------------------------------------------
; �������� �������� ������� RAM-����� ��� ������
; ������� ���������� ������ ������� �� ������ ��������
; sector = (cluster - 2) * BPB_SecPerClus + v_DataStartSector
; �������� �������:
; cluster = (sector - v_DataStartSector) / BPB_SecPerClus + 2


MarkRamDiskBads:
    ; ��������� ����� ��������, ��������������� ������� ������� ������� �� RAM-�����
    ld      hl, (v_DataStartSector)
    ld      de, 255 ; ������ ������ - ��������� � 64� �����
    ex      de, hl
    call    bios_sub_hl_de
    ; ��� ����� ���� hl = hl / BPB_SecPerClus, �� � ��� ���� ������ = �������
    inc     hl
    inc     hl

    ; ��������� ����� �������� � ����� ������ � ������ fat
    add     hl, hl  ; hl *= 2
    ld      de, buffer
    add     hl, de

    ; ���������� ������ ��� �������� � ���������� ������� �������
    ; -1 ������ ��� � ����� ���� ��� �������� inc hl
    ld      de, 65536/256 * 2 - 1   ; = 511 ����

    ; � ����� �������� ������ ������� ������ 64� �����
    ld      a, (bios_vars.ramPageCount) ; a = ���������� 64� ����� RAM-�����
    ld      b, a                        ; ������� �����
MarkRamDiskBadsLoop:
    ld      (hl), FAT16_BAD & 0FFh
    inc     hl
    ld      (hl), FAT16_BAD >> 8
    add     hl, de
    dec     b
    jp nz,  MarkRamDiskBadsLoop
    ret

;----------------------------------------------------------------------------
; ���������� ������ �� ������ hl ������ b ����������� ���� de

memset:
    ld      (hl), b
    inc     hl
    dec     de
    ld      a, d
    or      e
    jp nz,  memset
    ret

;----------------------------------------------------------------------------
; ����� ��������� 'INVALID DRIVE LETTER'

errInvalidDrive:
    ld      hl, str_InvalidDrive
    jp      bios_printString

;----------------------------------------------------------------------------
; ����� ��������� 'NO DRIVER'

errNoDriver:
    ld      hl, str_NoDriver
    jp      bios_printString

;----------------------------------------------------------------------------
; ����� ��������� 'DRIVE SIZE IS UNKNOWN'

;errSizeUnknown:
;    ld      hl, str_SizeUnknown
;    jp      bios_printString

;----------------------------------------------------------------------------
; ����� ��������� 'ABORTING'

errAbort:
    ld      hl, str_Aborting
    jp      bios_printString

;----------------------------------------------------------------------------
; ������

str_Format:         DB 0Ah,"Format drive "
str_A_Y_N:          DB "A: [Y/N]? ",0
str_ChoseDrive:     DB 0Ah,"Choose drive: ",0
str_InvalidDrive:   DB 0Ah,"Invalid drive letter",0
str_NoDriver:       DB 0Ah,"No driver",0
;str_SizeUnknown:    DB 0Ah,"Drive size is unkown",0
str_Aborting:       DB 0Ah,"Aborting",0
str_RAM_DISK:       DB "RAM DISK   "

v_FormatConfirmed:  DB 0
v_FormatRAMdisk:    DB 0
v_FatBytes:         DW 0
v_DirBytes:         DW 0
v_DirSectors:       DW 0
v_DirStartSector:   DW 0
v_DataSectors:      DW 0
v_DataStartSector:  DW 0

buffer = 0000h ; 0D100h ; ����� ������

; ������ ������������ �������.
; ����, ���������� *, ������������� ���� ����������.
v_HeaderStart:
    DB 0, 0, 0          ; BS_JmpBoot     - ��������  00h,   3 ���� - jmp �� ��������� - �� �������������
    DB "SPETSMX2"       ; BS_OEMName     - ��������  03h,   8 ���� - ��������� "MSWIN 4.1" ��� "MSDOS 5.0", �� ����� � ���� :)
    DW SECTOR_SIZE      ; BPB_BytsPerSec - ��������  0Bh,   2 ���� - ������ ������� � ������
    DB 1                ; BPB_SecPerClus - ��������  0Dh,   1 ���� - ������� �������� � ��������
    DW 1                ; BPB_RsvdSecCnt - ��������  0Eh,   2 ���� - ������� �������� � ���������������� (�����������) �������
    DB 1                ; BPB_NumFATs    - ��������  10h,   1 ���� - ���������� ����� FAT
v_DirFiles:
    DW 64               ; BPB_RootEntCnt - ��������  11h,   2 ���� - ���������� ������ � �������� �������� (512 - �������� ��� FAT16)
v_TotalSectors:
    DW 0            ; * ; BPB_TotSec16   - ��������  13h,   2 ���� - ������� ����� �������� �� ����� (16-������ ��������)
    DB 0F8h             ; BPB_Media      - ��������  15h,   1 ���� - ��������� F0h, F8h ... FFh, ����� �� �������� ������ ���� � ������� 8 ����� FAT[0]
v_FatSectors:
    DW 0            ; * ; BPB_FATSz16    - ��������  16h,   2 ���� - ������� �������� �������� FAT
    DW 0                ; BPB_SecPerTrk  - ��������  18h,   2 ���� - ���������� �������� �� �������
    DW 0                ; BPB_NumHeads   - ��������  1Ah,   2 ���� - ���������� �������
    DB 0, 0, 0, 0       ; BPB_HiddSec    - ��������  1Ch,   4 ���� - ���������� ������� �������� ����� ���� ��������
    DB 0, 0, 0, 0       ; BPB_TotSec32   - ��������  20h,   4 ���� - ������� ����� �������� �� ����� (32-������ ��������)
    DB 80h              ; BS_DrvNum      - ��������  24h,   1 ���� - ����� ���������� IBM PC: 0 ��� ���������, 80h ��� �������� �����
    DB 0                ; BS_Reserved    - ��������  25h,   1 ���� - ��������������� ��� Windows NT
    DB 29h              ; BS_BootSig     - ��������  26h,   1 ���� - ����������� ��������� ������������ ������� (29h): ��������� ��� ���� �������
    DB 0, 0, 0, 0       ; BS_VolID       - ��������  27h,   4 ���� - �������� ����� ����
v_VolLabel:
    DB "NO NAME    "; * ; BS_VolLab      - ��������  2Bh,  11 ���� - ����� ����
    DB "FAT16   "       ; BS_FilSysType  - ��������  36h,   8 ���� - ��������� "FAT12   ", "FAT16   " ��� "FAT     "

v_HeaderEnd:            ; ��������� ����������� ������ 0FFh

;----------------------------------------------------------------------------

    END
