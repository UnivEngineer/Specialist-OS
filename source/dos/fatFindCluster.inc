;----------------------------------------------------------------------------
; MXOS: выделение свободных кластеров. BC/HL сохраняются, DE=кластер,
; CF=1 и A=1 при заполненном томе. AF/флаги разрушаются.
; На диске сохраняется обычная FAT-цепочка с физическими номерами кластеров.
; Подсказка allocationStep меняет только начало поиска следующего кластера.
;----------------------------------------------------------------------------
fatFindClusterFirst:
            push    hl
            push    bc
            ld      de, 2
            jp      fatFindClusterCommon

fatFindClusterNext:
            push    hl
            push    bc
            ld      hl, (v_findCluster)
            ex      de, hl
            ld      hl, (v_diskInfo.allocationStep)
            ld      a, h
            or      l
            jp z,   fatFindLinear
            ; При одном чипе или многосекторном кластере используем общий поиск.
            push    de
            ex      de, hl
            ld      hl, (v_diskInfo.totalSectors)
            call    cmp_hl_de
            pop     de
            jp c,   fatFindLinear
            jp z,   fatFindLinear
            ld      hl, (v_diskInfo.sectorsPerCluster)
            dec     hl
            ld      a, h
            or      l
            jp nz,  fatFindLinear

            ; Следующий чип, тот же адрес внутри него. После последнего чипа
            ; переходим на следующий сектор первого. Метаданные не адресуем.
            call    fatClusterToSector
            ld      hl, (v_diskInfo.allocationStep)
            add     hl, de
            push    hl
            ld      hl, (v_diskInfo.totalSectors)
            ex      de, hl
            pop     hl
            call    cmp_hl_de
            jp c,   fatFindSectorReady
            call    sub_hl_de
            inc     hl
fatFindSectorReady:
            push    hl
            ld      hl, (v_diskInfo.dataStartSector)
            ex      de, hl
            pop     hl
            call    sub_hl_de
            jp nc,  fatFindClusterReady
            ld      hl, 0
fatFindClusterReady:
            inc     hl
            inc     hl
            ex      de, hl
            jp      fatFindClusterCommon
fatFindLinear:
            ld      hl, (v_findCluster)
            inc     hl
            ex      de, hl

fatFindClusterCommon:
            ld      hl, (v_diskInfo.dataClusters)
            ld      b, h
            ld      c, l
            ld      a, b
            or      c
            jp z,   fatFindClusterFailed
; Цикл 36 байт: 168-191 такт на занятую ячейку сверх тела fatReadCluster.
; Одно чтение FAT-элемента (2 байта) через кэш; промах добавляет чтение сектора.
fatFindClusterLoop:
            ; Полный ограниченный обход 2..dataClusters+1 с возвратом к 2.
            ld      hl, (v_diskInfo.dataClusters)
            inc     hl
            inc     hl
            call    cmp_hl_de
            jp c,   fatFindWrap
            jp nz,  fatFindCandidate
fatFindWrap:
            ld      de, 2
fatFindCandidate:
            ld      h, d
            ld      l, e
            call    fatReadCluster
            jp c,   fatFindClusterReturnError
            ld      a, d
            or      e
            jp z,   fatFindClusterOk
            ld      d, h
            ld      e, l
            inc     de
            dec     bc
            ld      a, b
            or      c
            jp nz,  fatFindClusterLoop
fatFindClusterFailed:
            ld a,DOS_ERR_FULL
            call dosSetError
fatFindClusterReturnError:
            pop     bc
            pop     hl
            ld      a, 1
            scf
            ret
fatFindClusterOk:
            ld      (v_findCluster), hl
            ex      de, hl
            pop     bc
            pop     hl
            or      a
            ret
