;+---------------------------------------------------------------------------
; MXOS
; �������� ����� � ���� (�����, ������� ���������� �� ����� 0FFC0h)
;
; 2013-12-12 ����������������� � ���������� vinxru
; 2022-02-02 ���������� SpaceEngineer
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; ���������� ������� ������� � ���� � ���� ����������� ������
; �����������-MX (FFC0h-FFDEh)
;
; ����:
;  b = 0 - ���������� ������� �������, b = 1 - ���������� ���������� �������
;
; 2022-02-02 SpaceEngineer
;----------------------------------------------------------------------------

setRAMDDriver:
            push  hl
            push  de
            push  bc
            push  af
            dec   b
            jp z, setRAMDDriver_1

            ; ������ �������� ��������
            ld    hl, driverBlockStart
            ld    de, driverBlockEnd
            jp    setRAMDDriver_2

setRAMDDriver_1:
            ; ������ ����������� ��������
            ld    hl, driverByteStart
            ld    de, driverByteEnd

            ; �������� ������� �� ����� bc = bios_RAMDRead = 0FFC0h
setRAMDDriver_2:
            ld    bc, bios_RAMDRead
            call  memcpy_bc_hl

            ; ��������������� �������� � �������
            pop   af
            pop   bc
            pop   de
            pop   hl
            ret

;----------------------------------------------------------------------------
; ������� �������, ������������ ����� ���� 256 ����
;
; FFC0h - ������ 256-�������� �����
;   ����:
;     c = ����� ��������
;     d = ����� ����� 
;     e = 0
;     hl = ����� ������ � ������
;
; FFD0h - ������ 256-�������� �����
;   ����:
;     c = ����� ��������
;     d = ����� ����� 
;     e = 0
;     hl = ����� ������ � ������
;
;----------------------------------------------------------------------------

; ������ 256-�������� ����� � �����

driverBlockStart: ; ��� �/� ���������� �� FFC0h
            ld      a, c
            ld      (IO_PAGE_RAMD), a
            ld      a, (de)
            ld      (IO_PAGE_RAM), a
            ld      (hl), a
            inc     hl
            inc     e
            jp nz,  0FFC0h
            ret

            ; �������� �����: 1 ����
            nop

; ������ 256-�������� ����� �� ����

driverBlockWrite: ; ��� �/� ���������� �� FFD0h
            ld      b, (hl)
            ld      a, c
            ld      (IO_PAGE_RAMD), a
            ld      a, b
            ld      (de), a
            ld      (IO_PAGE_RAM), a
            inc     hl
            inc     e
            jp nz,  0FFD0h
            ret

            ; �������� �����: 0 ����

driverBlockEnd:

;----------------------------------------------------------------------------
; ���������� �������, ������������ ������ 1 ����
;
; FFC0h - ������ �����
;   ����:
;     a  = ����� ��������
;     hl = ����� ����� � �������� ����
;   �����:
;     c  = ��������� ����
;
; FFD0h - ������ �����
;   ����:
;     a  = ����� ��������
;     hl = ����� � �������� ����
;     c  = ������������ ����
;
;----------------------------------------------------------------------------

driverByteStart: ; ��� �/� ���������� �� FFC0h
            ld      (IO_PAGE_RAMD), a
            ld      c, (hl)
            ld      (IO_PAGE_RAM), a
            ret

            ; �������� �����: 8 ����
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop

driverByteWrite: ; ��� �/� ���������� �� FFD0h
            ld      (IO_PAGE_RAMD), a
            ld      (hl), c
            ld      (IO_PAGE_RAM), a
            ret

            ; �������� �����: 8 ����
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop

driverByteEnd:

; ---------------------------------------------------------------------------

; ��������, ��� �������� �������� � 32 ����� ����������� ������ �����������-MX

    IF driverBlockWrite-driverBlockStart+bios_RAMDRead != bios_RAMDWrite
        ASSERT 0
        DISPLAY /l, "Error! Block briver entry point FFD0 has been shifted: ", driverBlockWrite-driverBlockStart+bios_RAMDRead, " != 0xFFD0"
    ENDIF

    IF driverByteWrite-driverByteStart+bios_RAMDRead != bios_RAMDWrite
        ASSERT 0
        DISPLAY /l, "Error! Byte briver entry point FFD0 has been shifted: ", driverByteWrite-driverByteStart+bios_RAMDRead, " != 0xFFD0"
    ENDIF

    IF driverBlockEnd-driverBlockStart > 32
        ASSERT 0
        DISPLAY /d, "Error! Block briver did not fit: ", driverBlockEnd-driverBlockStart, " > 32 bytes"
    ENDIF

    IF driverByteEnd-driverByteStart > 32
        ASSERT 0
        DISPLAY /d, "Error! Byte briver did not fit: ", driverByteEnd-driverByteStart, " > 32 bytes"
    ENDIF
