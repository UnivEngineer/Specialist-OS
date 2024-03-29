;+---------------------------------------------------------------------------
; MXOS
; �������� ������ ������ ��������.
; ������� �������� � ����� ���������� ��������� ������ (FILE_INFO).
;
; �� �����
;  hl - ����� ������
;  de - ������ ������ (����. ���������� ���������� ������)
;  bc - � ������ ����������� �������� ���������� ������
;
; �� ������
;  de - ������� ����� ������������ � ��������
;
;  bc, hl - �����������
;
; 2013-12-12 ����������������� vinxru
; 2022-02-11 ������������ SpaceEngineer
;----------------------------------------------------------------------------

fileList:   ; ���� ��� �������� ��� ���� �� �������������� -
            ; ��������� ������ ������ � �������
            ld      a, (v_diskInfo.isValid)
            cp      DISK_VALID
            jp z,   fileListOk

            ; � ������ ������ �������� ������ 0FFh
            ld      (hl), 0FFh

            ; ���������� ���������� ������ � ���������� � de (����)
            ld      de, 0
            ret

fileListOk: ; ���������� ���������
            push    bc  ; [1]
            push    hl  ; [2]
            
            ; � de - �������� ������, �� ��� ���� ��� ���������� �������� ��������
            ld      hl, (v_diskInfo.rootDirMaxFiles)
            call    cmp_hl_de_2
            jp c,   fl_min
            ex      hl, de
fl_min:     ; ��������� �������� ������
            ld      (v_dirMaxFiles), hl

            ; ��������� ����� ������� �����
            ld      h, b
            ld      l, c
            ld      (v_dirFirstFile), hl

            ; �������� �������� ������
            ld      hl, 0
            ld      (v_dirListedFiles), hl
            ld      (v_dirTotalFiles),  hl

            pop     hl  ; [2]
            push    hl  ; [2]

            ; de = ����� ������� ������� ��������
            ; �  = ���������� �������� �������� (��. ����)
            call    fatBeginDirLoop

            ; ���� �� �������� ��������
fileList_dirLoop:
            ; ��������� ����� ������� ��������
            push    de  ; [3]               ; de = ����� ������� ��������

            ; �������� ������ ��������
            push    hl  ; [4]
            xor     a                       ; ���� - ������ �� ����� ��������
            call    getSectorAddressInCache ; �����: hl = ����� ������� � ������
            ex      hl, de                  ; de = ����� ������� � ������
            pop     hl  ; [4]

            ; ���-�� ������������ ������ � ����� ������� �������� (��. ����)
            ld      a, (v_diskInfo.descrPerSector)
            ld      b, a

            ; ���� �� ������������ � �������������� ������� ��������
fileList_sectorLoop:
            ; ���� ������ ������ = FFh, ���������� ������
            ld      a, (de)
            inc     a
            jp z,   fileList_nextDescr

            push    hl  ; [4]
            push    de  ; [5]

            ; ����������� ������� ������ ���������� ������ � ��������
            ld      hl, (v_dirTotalFiles)
            inc     hl
            ld      (v_dirTotalFiles), hl

            ; ���� �� ����� �� ������� ����� (v_dirTotalFiles < v_dirFirstFile),
            ; �� ���������� ������������ �������� ��� ����������� ����������� � �����
            ex      hl, de
            ld      hl, (v_dirFirstFile)
            call    cmp_hl_de
            jp nc,  fileList_skipCopy

            ; ���� ����� ��� �������� (v_dirListedFiles >= v_dirMaxFiles),
            ; �� ���������� ������������ �������� ��� ����������� ����������� � �����
            ld      hl, (v_dirMaxFiles)
            ex      hl, de
            ld      hl, (v_dirListedFiles)
            call    cmp_hl_de
            jp nc,  fileList_skipCopy

            ; ����������� ������� ������������� ������������
            inc     hl
            ld      (v_dirListedFiles), hl

            pop     de  ; [5]
            pop     hl  ; [4]

            ; �������� ���������� �� de � hl
            push    bc  ; [4]
            ;call    copyDescriptor
            call    copyDescriptorCompact
            pop     bc  ; [4]

            ; copyDescriptor ������� de, ������� ��� �������
            ; ������ ���������� ����������� ���� ������������
            ;jp      fileList_next
            jp      fileList_nextDescr

fileList_skipCopy:
            pop     de  ; [5]
            pop     hl  ; [4]

fileList_nextDescr:
            ; ��������� ����� ���������� �����������
            push    hl  ; [4]
            ld      hl, FILE_DESCR_SIZE
            add     hl, de
            ex      de, hl
            pop     hl  ; [4]

fileList_next:
            ; ��� �������� ����� � ������� ��������?
            dec     b
            jp nz,  fileList_sectorLoop

            ; ��� �������� ������� ��������?
            ; ��������������� ����� ������� ��������
            pop     de  ; [3]   de = ����� ������� ��������
            inc     de  ;       ��������� ������ ��������
            dec     c
            jp nz,  fileList_dirLoop

fileList_exit
            ; ���������� � �����
            ; � ����� ������ �������� ������ 0FFh
            ld      (hl), 0FFh

            ; ���������� ���������� ������ � ���������� � de
            ld      hl, (v_dirTotalFiles)
            ex      hl, de

            pop     hl  ; [2]
            pop     bc  ; [1]
            ret

