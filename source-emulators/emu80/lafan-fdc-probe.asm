; Диагностика шины ВГ93 в отдельном профиле Специалист-MX.

FDC_COMMAND    EQU 0FFE8h
FDC_TRACK      EQU 0FFE9h
FDC_SECTOR     EQU 0FFEAh
FDC_DATA       EQU 0FFEBh
FDC_SYNC       EQU 0FFF0h
FDC_SIDE       EQU 0FFF2h
FDC_DRIVE      EQU 0FFF3h

PROBE_RESULT   EQU 02F00h
ST_SINGLE      EQU PROBE_RESULT
SEC_SINGLE     EQU PROBE_RESULT + 1
ST_SEARCH      EQU PROBE_RESULT + 2
SEC_SEARCH     EQU PROBE_RESULT + 3
ST_LAST        EQU PROBE_RESULT + 4
SINGLE_BUFFER  EQU 03000h
MULTI_BUFFER   EQU 04000h

        ORG 02000h

wait_entry:
        ld sp, 08C00h
        call read_sectors
wait_search:
        ld a, (FDC_COMMAND)
        ld (ST_LAST), a
        jp wait_search

abort_entry:
        ld sp, 08C00h
        call read_sectors
        ld a, 0D0h
        ld (FDC_COMMAND), a
        ld a, (FDC_COMMAND)
        ld (ST_LAST), a
abort_done:
        jp abort_done

read_sectors:
        xor a
        ld (FDC_DRIVE), a
        ld (FDC_SIDE), a
        ld (FDC_TRACK), a
        ld a, 1
        ld (FDC_SECTOR), a
        ld a, 080h
        ld (FDC_COMMAND), a
        ld hl, SINGLE_BUFFER
        ld bc, 0400h
        call read_block
        ld a, (FDC_COMMAND)
        ld (ST_SINGLE), a
        ld a, (FDC_SECTOR)
        ld (SEC_SINGLE), a

        ld a, 1
        ld (FDC_SECTOR), a
        ld a, 090h
        ld (FDC_COMMAND), a
        ld hl, MULTI_BUFFER
        ld bc, 01400h
        call read_block
        ld a, (FDC_COMMAND)
        ld (ST_SEARCH), a
        ld a, (FDC_SECTOR)
        ld (SEC_SEARCH), a
        ret

read_block:
        ld (FDC_SYNC), a
        ld a, (FDC_DATA)
        ld (hl), a
        inc hl
        dec bc
        ld a, b
        or c
        jp nz, read_block
        ret
