;+---------------------------------------------------------------------------
; MXOS
; ���������/��������� ������ �������� �����
;
; �� ����� 
;   hl - ��� �����
;   �  - 0=������, 1=���������
;   de - �����
;
; �� ������ 
;   de - �����
;   bc, hl - �����������
;
; 2013-12-12 ����������������� vinxru
; 2022-01-31 ���������� SpaceEngineer
;----------------------------------------------------------------------------

fileGetSetAddr:   ; ���� ���� � ������ � hl
            call    fileFind
            ret c 

            push    hl

            ; �������� ����� ����� �����
            push    de
            ld      hl, (v_cachedDescrPtr)
            ld      de, FILE_DESCRIPTOR.loadAddress
            add     hl, de
            pop     de

            ; ��������� �����
            ld      a, c
            dec     c ; ZF=1 ���� ����� 1
            jp nz,  fileGetSetAd_0

            ; �������� ����� ��������
            ld      (hl), e
            inc     hl
            ld      (hl), d

            ; ������ ������� �������� ���� isModified
            call    markDirSectorAsModified

            ; ��������� ��������� �� ����
            call    saveSectorCache

            ; �������
            pop     hl
            or      a
            ret

fileGetSetAd_0:
            ; ������ ����� ��������
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

            ; �������
            pop     hl
            or      a
            ret

