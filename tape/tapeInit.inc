;+---------------------------------------------------------------------------
; MXOS
; TAPE.SYS - �������������
;
; 2022-02-02 SpaceEngineer
;----------------------------------------------------------------------------

tapeInit:
        ; �������� ������� ��������� � DOS.SYS � �������
        ld      hl, jumpsStart
        ld      de, jumpsEnd
        ld      bc, TAPE_DRIVER_JUMPS
        jp      bios_memcpy_bc_hl

        ; ������ ���������
jumpsStart:
        jp      tapeRead
        jp      tapeWrite
        jp      tapeLoad
        jp      tapeSave
        jp      tapeLoadInt
        jp      tapeReadError
jumpsEnd:
