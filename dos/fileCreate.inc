;----------------------------------------------------------------------------
; MXOS
; ������� ����
;
; �� �����
;  hl - ����� ����������� ����� � ������� FAT (���, ��������, ������ � �.�.)
;
; �� ������
;  �f - ������
;
; TODO: � ������ �������� ����� �� �����, �� ��� �������� �������� ����.
; ���� ������� ��, ��� ��������.
;
; 2013-12-12 ����������������� vinxru
; 2022-02-11 ������������ SpaceEngineer
;----------------------------------------------------------------------------

fileCreate: ; ��������� ��������
            push    hl  ; [1]
            push    bc  ; [2]
            push    de  ; [3]

            ; ��������� ����� ��������������� ����������� ������������ �����
            ld      (v_newDescrPtr), hl

            ; ���� ���� ����������, �� ������� ���
            call    fileFind
            call nc,deleteFileInt

            ; de = ����� ������� ������� ��������
            ; �  = ���������� �������� �������� (��. ����)
            call    fatBeginDirLoop

            ; ���� �� �������� ��������
fileCreate_dirLoop:
            ; ��������� ����� ������� ��������
            push    de  ; [4]               ; de = ����� ������� ��������

            ; �������� ������ ��������
            xor     a                       ; ���� - ������ �� ����� ��������
            call    getSectorAddressInCache ; �����: hl = ����� ������� � ������

            ; ���-�� ������������ ������ � ����� ������� �������� (��. ����)
            ld      a, (v_diskInfo.descrPerSector)
            ld      b, a

            ; ������ �����������
            ld      de, FILE_DESCR_SIZE

            ; ���� �� ������������ � �������������� ������� ��������
fileCreate_sectorLoop:
            ld      a, (hl)                 ; ������ ���� �� ��������
            inc     a                       ; ���� ���� = 0FFh, ������ ������ ��������
            jp z,   fileCreate_placeFound   ; �������, ���� ��������� ������ �������

            ; ��������� ����� ���������� �����������
            add     hl, de

            ; ��� �������� ����� � ������� ��������?
            dec     b
            jp nz,  fileCreate_sectorLoop

            ; ��� �������� ������� ��������?
            ; ��������������� ����� ������� ��������
            pop     de  ; [4]   de = ����� ������� ��������
            inc     de  ;       ��������� ������ ��������

            dec     c
            jp nz,  fileCreate_dirLoop

            ;-----------------------------------------------------------
            ; ��������� ������� ���

            ; ��������� a=0, ZF=1
            xor     a           

stc_popDBH_ret:
            ; ��������� ����� CF=1
            scf

popDBH_ret: ; �������������� ��������� � �����
            pop     de  ; [3]
            pop     bc  ; [2]
            pop     hl  ; [1]
            ret

            ;-----------------------------------------------------------
            ; ��������� ������ �������

fileCreate_placeFound:

            ; ��������� ��������� � ���� ����� ���������� �����������
            ld      (v_cachedDescrPtr), hl

            ; ��������� ����� ������� ��������
            pop     de  ; [4]   de = ����� ������� ��������
            ex      hl, de
            ld      (v_cachedSector), hl

            ; ������� ������ ��������� �������
            call    fatFindClusterFirst ; de = ������ ��������� �������
            jp c,   stc_popDBH_ret      ; ���� ��� ��������� ��������� - ������� � �������

            ; ��������� ��� � ���������� v_fileFirstCluster
            ex      hl, de
            ld      (v_fileFirstCluster), hl
            ex      hl, de

            ; ��������� ����� ������� �������� � ���������� �����
            ; de = ����� ������� ��������
            ; hl = ����� ����������� �����
            ld      hl, (v_newDescrPtr)
            ld      bc, FILE_DESCRIPTOR.firstCluster
            add     hl, bc  ; hl = ����� ���� firstCluster (��. ����) � ����������� �����
            ld      (hl), e
            inc     hl      ; hl = ����� ���� firstCluster (��. ����) � ����������� �����
            ld      (hl), d


            ;---------------------------------------------------------------------
            ; ���������� ���������� � �������, ���� ������ �������� ��� ��� � ����

            ; ������ ������� �������� ���� isModified
            call    markDirSectorAsModified

            ; �������� �������������� ���������� ����� � �������������� ������ ��������
            ld      hl, (v_newDescrPtr)     ; ����� ��������������� �����������
            ex      hl, de
            ld      hl, (v_cachedDescrPtr)  ; ����� ���������� ����������� � ����

            ; ����������� c ���� �� de � hl
            push    bc
            call    copyDescriptor
            pop     bc

            ;---------------------------------------------------------------------

		    ; ������ ����� �������� �����
            ; de = ����� �������� �����
            ld      hl, (v_newDescrPtr)
            ld      bc, FILE_DESCRIPTOR.loadAddress
            add     hl, bc  ; hl = ����� ���� loadAddress (��. ����) � ����������� �����
            ld      e, (hl)
            inc     hl      ; hl = ����� ���� loadAddress (��. ����) � ����������� �����
            ld      d, (hl) ; de = ����� ��������

            ; ��������� ����� �������� � ���������� v_input_start
            ex      de, hl
            ld      (v_input_start), hl ; v_input_start = ����� �������� �����

            ; ������ ������ �����
            ; de = ������ �����
            ld      hl, (v_newDescrPtr)
            ld      bc, FILE_DESCRIPTOR.size
            add     hl, bc  ; hl = ����� ���� size (��. ����) � ����������� �����
            ld      e, (hl)
            inc     hl      ; hl = ����� ���� size (��. ����) � ����������� �����
            ld      d, (hl)

            ; ��������� ������ ����� � ���������� v_input_end
            ex      de, hl
            ld      (v_input_end), hl   ; v_input_end = ������ �����

            ; ������� ��������� �����
            ld      c, h        ; c  = ������� ���� ������� = ���������� ��������� ����� - 1, �.�. ������� �������� 256 ����.
            inc     c           ; c  = ���������� ��������� �����. TODO: ��� �� ����� �������� � ������ �������� ��������!
            ld      b, c        ; ��������� � b - ���������� �����

            ; ������ ������� ����� � fat
            ld      hl, (v_fileFirstCluster)    ; hl = ����� ������� �������� �����
            ld      d, h
            ld      e, l                        ; de = hl - ���� ���� ������� �� ������ ��������, ������ fat ����� ��������� �� ���� ����

            jp      writeFirstCluster

; ---------------------------------------------------------------------------
; ���������� �������� ����� � ������� fat �� ����������

allocClusters:
            call    fatFindClusterNext  ; de = ��������� ��������� �������
            jp c,   stc_popDBH_ret      ; ���� ������ ��� ��������� ��������� - ������� � �������

writeFirstCluster:
            ; ���������� ����� �������� � fat
            ; hl = ����� ������ fat
            ; de = ����� ��������
            call    fatWriteCluster

            dec     c                   ; ������� ���������� ���������
            jp nz,  allocClusters       ; ������ �����

            ; �����, ��������� ������� ����� ��������� ��� �� ����
            ; hl == de
            call    fatWriteCluster

; ---------------------------------------------------------------------------
; ��������� ���� �� ����

            ; ����� ������� ��������
            ld      hl, (v_fileFirstCluster)
            ex      hl, de                      ; de = ����� ������� �������� �����
            push    de

            ; ��������� � ����� �������
            call    fatClusterToSector          ; de = ����� ������� ������� �����

            ld      hl, (v_input_start)         ; hl = ����� �������� �����
            ld      c, b                        ; c = ���������� ��������� �����
            ld      b, 1                        ; ����� �������� 1 - ������

writeFileLoop:
            call    j_diskDriver
            inc     h                   ; c�������� ���� ����� � ������ (TODO: ��� �������� ������ ��� �������� 256 ����)

            ; ������� ��������� ������� � fat
            pop     de                  ; de = ����� �������� ��������
            call    fatReadCluster      ; de = ����� ���������� ��������
            push    de

            ; ��������� � ����� �������
            call    fatClusterToSector  ; de = ��������� ������ ����� � ������� ������

            dec     c                   ; ������� ���������� ���������
            jp nz,  writeFileLoop       ; ������ �����

            pop     de

; ---------------------------------------------------------------------------
; ������ ������������ ����������� �����
; ��� ����� �� ������������ ���� � ������������ ��

;            ld      hl, (v_input_end)   ; hl = ������ ����� - 1 �� v_input_end
;            inc     hl                  ; hl = ������ �����
;            ex      de, hl              ; de = ������ �����
;            ld      hl, (v_input_start) ; hl = ����� �������� ����� �� v_input_start
;            ld      b, 0                ; ��� ����� ������������� �����
;fileCreateCRCLoop:
;            ld      a, d
;            or      e
;            jp z,   fileCreateCRCDone   ; ���� ������� �������� �� ����, �������
;            ld  a,  (hl)                ; ������ ���� �����
;            add     a, b                ; ����������� �����
;            ld      b, a
;            inc     hl                  ; ��������� ����
;            dec     de                  ; ��������� �������
;            jp      fileCreateCRCLoop
;fileCreateCRCDone:
;            ; ��������� ����������� ����� � ���������� �����, � ���� ntReserved
;            ld      a, b                ; a = ����������� ����������� �����
;            ld      hl, (v_newDescrPtr)
;            ld      bc, FILE_DESCRIPTOR.ntReserved
;            add     hl, bc              ; hl = ����� ���� ntReserved � ����������� �����
;            ld      (hl), a             ; ��������� ���������� �����

; ------------------------------------------------------------------------------------------------------------------------
; ��������� ��� � �������

            call    saveSectorCache ; ��������� ��������� � ���� �� ����
            or      a               ; ������� � ������ cf = 0
            jp      popDBH_ret

