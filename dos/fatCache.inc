;----------------------------------------------------------------------------
; MXOS
; ��� �������� fat
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

    STRUCT  FAT_CACHE_DATA          ; ������ ��������� 8 ����
number:         DW  0FFFFh          ; ����� �������
address:        DW  FAT_CACHE_ADDR  ; ����� ��������������� ������� � ���
accessTime:     DW  0               ; ��������� ����� ��������� � �������
isModified:     DB  0               ; ������������� �� ������
_align:         DB  0               ; ��� ������������
    ENDS

; ������ �� FAT_CACHE_CAPACITY �������� ������ FAT_CACHE_DATA, ��������������� ������
cacheData:      BLOCK   FAT_CACHE_CAPACITY * FAT_CACHE_DATA, 0

;----------------------------------------------------------------------------
; MXOS
; ������������� ���� fat
;
; ������� �� �����������!
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

resetSectorCache:
        ; bc = ��������� �����
        call    getSystemTime

        ; � ����� ���������� ��������� �������� �� ���� address
        ld      de, FAT_CACHE_ADDR      ; ����� ������ ��������
        ld      hl, cacheData           ; ����� ������� �������� � �������
        ld      a,  FAT_CACHE_CAPACITY  ; ������� �����

resetCacheLoop:
        ; ���� number = 0FFFFh
        ld      (hl), 0FFh
        inc     hl
        ld      (hl), 0FFh
        inc     hl
        ; ���� address = de
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     hl
        ; ���� accessTime = bc
        ld      (hl), c
        inc     hl
        ld      (hl), b
        inc     hl
        ; ���� isModified = 0
        ld      (hl), 0
        inc     hl
        ; ���� isLoaded = 0
        ld      (hl), 0
        inc     hl

        ; de += SECTOR_SIZE
        inc     d   ; ��� �������� ������ � ��������� �� 256 ����

        ; ������ �����
        dec     a
        jp nz,  resetCacheLoop

        ret

;----------------------------------------------------------------------------
; MXOS
; ��������� ������ ��������������� ������� � ������.
; ���� ������ �� ������ � ����, �� ����������� � �������� ����������.
;
; ����:
;   de = ����� �������
;   a  = 1 - ���������� ���� isModified �������
;
; �����:
;   hl = ����� ������� � ������
;
;   �������� de, bc �����������
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

getSectorAddressInCache:
        push    bc
        push    de
        ld      b, a    ; b = isModified
        ex      hl, de

        ; ������� ������ � ����; ���� ���� - ��������
        ; ����:  hl = ����� ������� �� �����
        ; �����: hl = ����� ������� � ���
        call    cacheSector

        pop     de
        pop     bc
        ret

;----------------------------------------------------------------------------
; MXOS
; ��������� ������ ������ fat.
; ���� ������ fat �� ������ � ����, �� ����������� � �������� ����������.
;
; ����:
;   de = ����� ������ fat
;   a  = 1 - ���������� ���� isModified �������
;
; �����:
;   hl = ����� ������ � ������
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

getFatCellAddress:
        push    bc
        ld      b, a ; b = isModified

        ; ��������� ����� �������, ��� ��������� ������� ������ fat
        ; fatSectorNum  = (N * 2) / v_diskInfo.sectorSize + v_diskInfo.resSectors;
        ; fatCellOffset = (N * 2) % v_diskInfo.sectorSize;
        ld      hl, (v_diskInfo.sectorSize)
        ex      hl, de
        add     hl, hl      ; hl = N * 2
        call    div_hl_de   ; �����: hl = ����� ������� �� ������ fat; de = �������� ������ � �������
        push    de          ; de = �������� ������ � �������
        ex      hl, de
        ld      hl, (v_diskInfo.resSectors)
        add     hl, de      ; hl += v_diskInfo.resSectors

        ; ������� ������ � ����; ���� ���� - ��������
        ; ����:  hl = ����� ������� �� �����
        ; �����: hl = ����� ������� � ���
        call    cacheSector

        ; ������� �������� ������ fat � ���� �������
        pop     de      ; de = �������� ������ � �������
        add     hl, de  ; hl = ����� ������ � �������

        pop     bc
        ret

;----------------------------------------------------------------------------
; MXOS
; ����� ������� � ����. ���� ������ �� ������, �� �� ����������� � ����������.
;
; ����:
;   hl = ����� �������
;   b  = ���� �����������
;
; �����:
;   hl = ����� ������ ������� � ������
;
; �������� de, bc �����������
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

cacheSector:
        ; ��������� ��������
        push    de
        push    bc

        ; ������������� ������ �������� cacheData � ������� ������� � ������� hl
        ex      hl, de                                      ; de = ����� ��� ������
        ld      hl, cacheData + FAT_CACHE_DATA.number       ; hl = ����� ���� number � ������ �������� �������
        ld      c,  FAT_CACHE_CAPACITY                      ; ������� �����
cff_FindSector:
        call    cmp_m_de            ; ���������� [hl] � de
        jp z,   cff_SectorFound     ; ���� �����, ������ ���� ����� ������ � ����
        push    bc                  ; ����� ��������� �� �������� ������� �������
        ld      bc, FAT_CACHE_DATA  ; bc = ������ ��������� FAT_CACHE_DATA
        add     hl, bc
        pop     bc
        dec     c                   ; ������ �����
        jp      nz, cff_FindSector

        ; �� �� ����� ������ � �������, ��������� ��� � ���������� �� ����� �������� ����� ������������������
        push    de  ; de = ����� �������, ������� ���� ���������

        ; ���� �������� ����� ����������������� ������ (�.�. � �������� ����� ��������� accessTime)
        ld      de, 0FFFFh                                  ; de = ������������ �������� accessTime �������
        ld      hl, cacheData + FAT_CACHE_DATA.accessTime   ; hl = ����� ���� accessTime � ������ �������� �������
        ld      bc, FAT_CACHE_CAPACITY                      ; b  = 0, c = ������� �����
cff_FindOldestSector:
        call    cmp_m_de            ; ���������� [hl] � de
        jp nc,  cff_next            ; �������, ���� [hl] >= de
        ld      e, (hl)             ; ��� [hl] < de, ���������� accessTime ������� � de
        inc     hl
        ld      d, (hl)
        dec     hl
        ld      b, c                ; ���������� �������� ����� ������� � b
cff_next:
        push    bc                  ; ��������� �� �������� ������� �������
        ld      bc, FAT_CACHE_DATA  ; bc = ������ ��������� FAT_CACHE_DATA
        add     hl, bc
        pop     bc
        dec     c                   ; ������ �����
        jp      nz, cff_FindOldestSector

        ; ����� �������� ����� ����������������� ������
        ld      a, FAT_CACHE_CAPACITY
        sub     b       ; a = ������ ���������� �������
        rla
        rla
        rla             ; a = a * 8 (������ ��������� FAT_CACHE_DATA)
        ld      c, a
        ld      b, 0    ; bc = �������� ������� � ������� cacheData
        ld      hl, cacheData
        add     hl, bc  ; hl = ����� �������� �������

        ; ������ ��� ��������� ����� ������, ������ ���� ���������, ���� ��� isModified �� ����
        call    saveSectorFromCache    ; ���������� ������ �� ����������

        ; ��������� ����� ������ �� ����� �������
        pop     de              ; de = ����� �������, ������� ���� ���������
        call    loadSectorToCache    ; ��������� ������ � ����������

        ; ������ ������ ��� ��������, ���������� ��� �����
        ; ����� hl = ����� �������� ������� cacheData ����� �������
cff_SectorFound:

        inc     hl
        inc     hl      ; ������ hl = ����� ���� address

        ; ������ ����� ������� � de
        ld      e, (hl)
        inc     hl
        ld      d, (hl) ; de = ����� �������
        inc     hl      ; ������ hl = ����� ���� accessTime

        ; �c����������� accessTime �������
        call    getSystemTime;  bc = ��������� �����

        ld      (hl), c
        inc     hl
        ld      (hl), b ; de = ����� �������
        inc     hl      ; ������ hl = ����� ���� isModified

        ; ������������� isModified, ���� � �������� b �� ����� � ������� ��� �� ����
        pop     bc      ; b = ���� �����������
        ld      a, (hl)
        or      b
        ld      (hl), a

        ; hl = ����� �������
        ex      hl, de

        ; ��������������� �������� � �������
        pop     de
        ret

;----------------------------------------------------------------------------
; MXOS
; �������� ������� � ���������� � ���
;
; ����:
;   hl = ����� ��������� FAT_CACHE_DATA � ��������� �������
;   de = ����� �������
;
; �������� hl, de, bc �����������
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

loadSectorToCache:
        push    hl

        ; ���������� ����� ������� � ���������
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     hl      ; ������ hl = ����� ���� address
        push    de

        ; ������ � de ����� �������� �������
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        inc     hl

        ; ��������� ������
        ; ����� ������� = ����� �������, �.�. fat ���������� � ������ �����
        ex      hl, de  ; hl = ����� ������
        pop     de      ; de = ����� �������
        ld      b, 2    ; ����� - ������
        call    j_diskDriver

        ; ��������������� �������� � �������
        pop     hl
        ret

;----------------------------------------------------------------------------
; MXOS
; ������ ������� �� ���� �� ����������, ���� ��� isModified �� ����
;
; ����:
;   hl = ����� ��������� FAT_CACHE_DATA � ��������� �������
;
;   �������� hl, de, bc �����������
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

saveSectorFromCache:
        push    hl
        push    de

        ; ���������, ��� �� ������� ������
        push    bc
        ld      bc, FAT_CACHE_DATA.isModified   ; bc = �������� ���� isModified
        add     hl, bc      ; hl = ����� ���� isModified
        pop     bc
        ld      a, (hl)     ; a = isModified
        or      a           ; a == 0?
        jp z,   ssfc_Exit   ; ���� isModified == 0, ������ �� ��� �������, � ��� �� ���� ���������

        ; �������� isModified
        ld      (hl), 0

        dec     hl  ; ������ hl = ����� ���� accessTime (��. ����)

        dec     hl
        dec     hl  ; ������ hl = ����� ���� address (��. ����)

        ; ������ � de ����� �������� �������
        ld      d, (hl)
        dec     hl
        ld      e, (hl)
        push    de  ; ��������� ��� � ����

        dec     hl  ; ������ hl = ����� ���� number (��. ����)

        ; ������ � de ����� �������
        ld      d, (hl)
        dec     hl
        ld      e, (hl)

        ; ���������� ������
        ; de = ����� �������
        pop     hl      ; hl = ����� �������� �������
        ld      b, 1    ; ����� - ������
        call    j_diskDriver

ssfc_Exit:
        ; ��������������� �������� � �������
        pop     de
        pop     hl
        ret

;----------------------------------------------------------------------------
; MXOS
; ������ ���� �������������� �������� �� ����������, ���� �� isModified �� ����
;
; �������� hl, de, bc �����������
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

saveSectorCache:
        ; ��������� ��������
        push    hl
        push    de
        push    bc

        ld      hl, cacheData           ; hl = ������ ������� � ������� cacheData
        ld      de, FAT_CACHE_DATA      ; de = ������ �������� ������� cacheData
        ld      c,  FAT_CACHE_CAPACITY  ; ������� �����
saveSectorCacheLoop:
        call    saveSectorFromCache     ; ���������� ������ �� ����������
        add     hl, de
        dec     c
        jp nz,  saveSectorCacheLoop

        ; ��������������� �������� � �������
        pop     bc
        pop     de
        pop     hl
        ret

;----------------------------------------------------------------------------
; MXOS
; ���������� ������ ������� �� ������ ��������
;
; ����:
;   de = ����� ��������
;
; �������:
;   de = (de - 2) * v_diskInfo.sectorsPerCluster + v_diskInfo.dataStartSector
;
; �����:
;   de = ����� �������
;
;   �������� bc, hl �����������
;
; 2022-02-14 ����������� SpaceEngineer
;----------------------------------------------------------------------------

fatClusterToSector:
        push    hl

        dec     de
        dec     de          ; de -= 2 (��������� ��������� ���������� � 2)

        ld      hl, (v_diskInfo.sectorsPerCluster)
        call    mul_hl_de   ; hl = de * BPB_SecPerClus
        ex      hl, de

        ld      hl, (v_diskInfo.dataStartSector)
        add     hl, de
        ex      hl, de

        pop     hl
        ret

;----------------------------------------------------------------------------
; MXOS
; ������ ����� �� ������ � ��������
;
; �����:
;   de = ����� ������� ������� ��������
;   �  = ���������� �������� �������� (��. ����)
;
;   ��������� �������� �����������
;
; 2022-02-14 ����������� SpaceEngineer
;----------------------------------------------------------------------------

fatBeginDirLoop:
        ex      hl, de
        ld      hl, (v_diskInfo.dirStartSector)
        ex      hl, de                      ; de = ����� ������� ������� ��������
        ld      a, (v_diskInfo.dirSectors)  ; a = ���������� �������� �������� (��. ����)
        ld      c,  a                       ; �  = ������� �����
        ret

;----------------------------------------------------------------------------
; MXOS
; ��������� ������� �������� ����� isModified
;
; ����:
;  v_cachedSector - ����� �������
;
; �����:
;   bc = ��������� �����
;
;   ������� bc �����������
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

markDirSectorAsModified:
        ld      hl, (v_cachedSector)
        ex      hl, de  ; de = ����� �������
        ld      a, 1    ; ���� - ������ ����� �������

        ; �������� ������� � ���; ���� ������ ��� � ����,
        ; ������� ������ ������ ��� � ������� ����
        jp      getSectorAddressInCache

;----------------------------------------------------------------------------
; MXOS
; ��������� �������� ���������� �������
;
; � "�����������" ��� ����� ��������� ������� (RTC), ������� ��������� �����
; - ��� ������ �������, ��������������� ������ ��� ��� ���������. � �������
; ����� ����� ������� ����� ����� ������ ��53, ��� ����� ������� RTC, ��� ���
; ������� � �������.
;
; �����:
;   bc = ��������� �����
;
; ��� ��������� �������� �����������
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

getSystemTime:
        push    hl
        ld      hl, (v_fakeSystemTime)
        inc     hl
        ld      (v_fakeSystemTime), hl
        ld      b, h
        ld      c, l
        pop     hl
        ret

;----------------------------------------------------------------------------
; MXOS
; ��������� ����� �� ������ hl � de
; ���������: cp [hl], de
;
; �����:
;   ����� c, z
;
; 2022-02-08 ����������� SpaceEngineer
;----------------------------------------------------------------------------

cmp_m_de:
        inc   hl
        ld    a, (hl)
        dec   hl
        cp    d
        ret   c
        ret   nz
        ld    a, (hl)
        cp    e
        ret
