;----------------------------------------------------------------------------
; MXOS
; ����������� ����������� ����� (FILE_DESCRIPTOR)
;
; ����:
;  de = ����� ������
;  hl = ����� ����
;
; �����:
;  de = ����� ���������� �����������
;  hl = ����� ���������� �����������
;
; 2022-02-11 SpaceEngineer
;----------------------------------------------------------------------------

copyDescriptor:
            ; �������� FILE_DESCR_SIZE ���� �� de � hl
            ld      b, FILE_DESCR_SIZE
cfd_loop:   ld      a, (de)
            ld      (hl), a
            inc     hl
            inc     de
            dec     b
            jp nz,  cfd_loop
            ret

;----------------------------------------------------------------------------
; MXOS
; ��������������� ����������� ����� � ���������� �����
; FILE_DESCRIPTOR -> FILE_INFO
;
; ����:
;  de = ����� ������
;  hl = ����� ����
;
; �����:
;  hl = ����� ���������� ���������
;
;  de - �����������
;
; 2022-02-16 SpaceEngineer
;----------------------------------------------------------------------------

copyDescriptorCompact:
            push    de

            ; �������� ��� ����� + ���������� + ���� ���������
            ld      b, FILE_NAME_LENGTH + 4
cfdc_loop:  ld      a, (de)
            ld      (hl), a
            inc     hl
            inc     de
            dec     b
            jp nz,  cfdc_loop

            ; �������� ���� ������ ��������
            ex      hl, de
            ld      bc, FILE_DESCRIPTOR.loadAddress - FILE_DESCRIPTOR.attrib - 1   ; ������������� �������� ���� loadAddr
            add     hl, bc
            ex      hl, de

            ld      a, (de)
            ld      (hl), a
            inc     hl
            inc     de
            ld      a, (de)
            ld      (hl), a
            inc     hl

            ; �������� ���� �������
            ex      hl, de
            ld      bc, FILE_DESCRIPTOR.size - FILE_DESCRIPTOR.loadAddress - 1  ; ������������� �������� ���� size
            add     hl, bc
            ex      hl, de

            ld      a, (de)
            ld      (hl), a
            inc     hl
            inc     de
            ld      a, (de)
            ld      (hl), a
            inc     hl

            pop     de
            ret
