;+---------------------------------------------------------------------------
; MXOS
; ������������� ����
;
; �� ����� 
;  hl - �������� ���
;  de - ����� ���
;
; �� ������
;  cf - ������
;  bc, de, hl - �����������
;
; 2013-12-12 ����������������� vinxru
;----------------------------------------------------------------------------

fileRename: ; ����� ���� � ������ �� hl
            call    fileFind
            ret c 

            ; ��������� ��������
            push    hl
            push    bc
            push    de
            
            ld      hl, (v_cachedDescrPtr)

            ; ���������� ����� ���� � ����� ������
            ex      de, hl
            call    fileFind

            ; ���� ����� ���� ����, �� �������
            call nc,deleteFileInt
            
            ; �������� ���
            ld      c, FILE_NAME_LENGTH + 3        
loc_CD2C:   ld      a, (hl)
            ld      (de),a
            inc     hl
            inc     de
            dec     c
            jp nz,  loc_CD2C

            ; ������ ������� �������� ���� isModified
            call    markDirSectorAsModified

            ; ��������� ��������� �� ����
            call    saveSectorCache

            ; �����
            or      a
            jp      popDBH_ret

