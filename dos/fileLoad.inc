;+---------------------------------------------------------------------------
; MXOS
; ��������� ���� �� ���������� ������
;
; �� �����
;  hl - ���
;  de - ����� ��������
;
; �� ������
;  bc, de, hl - �����������
;  v_foundDescrPtr - ����� ����� ����������� ���������� �����
;  cf - ������
;
; 2013-12-12 ����������������� vinxru
; 2022-02-11 ������������ SpaceEngineer
;----------------------------------------------------------------------------

fileLoad2:  ; ���� ���� � ������ � hl
            call    fileFind
            ret c 

            ; ��������� ��������
            push    hl
            push    bc
            push    de

            jp      fileLoadStep2

;----------------------------------------------------------------------------
; ��������� ���� �� ������ ���������� � ���������
;
; �� �����
;  hl - ���
;
; �� ������
;  bc, de, hl - �����������
;  v_foundDescrPtr - ����� ����� ����������� ���������� �����
;  cf - ������
;----------------------------------------------------------------------------

fileLoad:   ; ���� ���� � ������ � hl
            call    fileFind
            ret c 

            ; ��������� ��������
            push    hl
            push    bc
            push    de

            ; hl = ��������� �� ����� ��������
            ld      hl, (v_foundDescrPtr)
            ld      de, FILE_DESCRIPTOR.loadAddress
            add     hl, de

            ; de = ����� ��������
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

fileLoadStep2:
            ; �������� ����� ��������
            ex      hl, de
            ld      (v_input_start), hl
            ex      hl, de

            ; hl = ��������� �� ����� ������� �������� �����
            ld      hl, (v_foundDescrPtr)
            ld      bc, FILE_DESCRIPTOR.firstCluster
            add     hl, bc

            ; de = ������ ������� �����
            ld      e, (hl)
            inc     hl
            ld      d, (hl)
            push    de

            ; de = ������ ������ ����� � ������� ������
            call    fatClusterToSector

            ; hl = ��������� �� ����� ���������� �����
            ld      hl, (v_foundDescrPtr)
            ld      bc, FILE_DESCRIPTOR.size + 1
            add     hl, bc

            ; c = ���������� ��������� �����. ������ ������ ������� ����
            ; �������, �.�. ������� �������� 256 ����.
            ; TODO: ��� �� ����� �������� � ������ �������� ��������!
            ld      c, (hl)
            inc     c

            ; hl = ����� ��������
            ld      hl, (v_input_start)

            ; ����� �������� - ������
            ld      b, 2
fileLoadLoop:
            ; ������ ������� � ������
            call    j_diskDriver    ; de = ����� �������, hl = ����� ���� ������

            inc     h               ; ��������� ���� � ������ (TODO: ��� �������� ������ ��� ������� 256 ����)

            ; ��������� ��������� ������� � fat
            pop     de              ; de = ����� �������� ��������
            call    fatReadCluster  ; de = ����� ���������� ��������
            push    de

            ; ��������� � ����� �������
            call    fatClusterToSector  ; de = ��������� ������ ����� � ������� ������

            dec     c               ; ������� ��������� ���������
            jp nz,  fileLoadLoop    ; ������ �����

            ; ��������������� ��������
            pop     de
            pop     de
            pop     bc
            pop     hl

            jp      fileLoadExit

; ---------------------------------------------------------------------------
; �������� ���������� � ����� (����������)
;
; �� �����
;  hl - ���
;
; �� ������
;  bc, de, hl - �����������
;  v_foundDescrPtr - ����� ����� ����������� ���������� �����
;  cf - ������
;----------------------------------------------------------------------------

fileLoadInfo:
            ; ���� ���� � ������ � hl
            call    fileFind
            ret c 

; ---------------------------------------------------------------------------

fileLoadExit:
            ; ��������� ��������
            push    hl
            push    bc
            push    de

            ; de = ��������� �� ������ ���� ����� ����������
            ld      bc, FILE_DESCRIPTOR.attrib
            add     hl, bc
            ex      de, hl

            ; ���������� � �����, �������� ���
            ld      hl, (v_foundDescrPtr)
            add     hl, bc

            ; �������� ���������� �� hl � de, �������� ��� � ����������
            ld      b, FILE_DESCR_SIZE - FILE_NAME_LENGTH - 3
fileLoadCDLoop:
            ld      a, (hl)
            ld      (de), a
            inc     hl
            inc     de
            dec     b
            jp nz,  fileLoadCDLoop

            ; ����� CF=0
            or      a
            jp      popDBH_ret

