;+---------------------------------------------------------------------------
; MXOS
; �������� �����
;
; �� �����
;  hl - ��� �����
;
; �� ������
;  cf - ������
;  bc, de, hl - �����������
;
; 2013-12-12 ����������������� vinxru
; 2022-02-11 ������������ SpaceEngineer
;----------------------------------------------------------------------------

fileDelete: ; ����� ���� � ������ �� hl
            call    fileFind
            ret     c 

            ; ������� ����
            call    deleteFileInt

            ; ��������� ��������� �� ����
            call    saveSectorCache

            ; ���������
            or      a
            ret
            
; ---------------------------------------------------------------------------

deleteFileInt:
            ; ��������� ��������
            push    hl
            push    bc
            push    de

            ; �������� ���� � ����, ��� ���������, ������� ������ ����� ��� ����� �� 0FFh
            ld      hl, (v_cachedDescrPtr)
            ld      (hl), 0FFh
            push    hl

            ; ������ ������� �������� ���� isModified
            call    markDirSectorAsModified

            ; �������� ������ ������� ����� � de
            pop     hl
            ld      de, FILE_DESCRIPTOR.firstCluster
            add     hl, de
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

deleteFileLoop:
            ld      h, d
            ld      l, e

            ; �������� ������� ����� de � �������� �����
            ; ����������� �������� ����� � de
            call    fatFreeCluster

            ; ���� ��� ��� �� ��������� �������, ����������.
            ld      a, d
            cp      h
            jp nz,  deleteFileLoop
            ld      a, e
            cp      l
            jp nz,  deleteFileLoop

            ; �������������� �������� � �������
            jp      popDBH_ret

