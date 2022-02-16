;+---------------------------------------------------------------------------
; MXOS
; TAPE.SYS - Инициализация
;
; 2022-02-02 SpaceEngineer
;----------------------------------------------------------------------------

tapeInit:
        ; Копируем таблицу переходов в DOS.SYS и выходим
        ld      hl, jumpsStart
        ld      de, jumpsEnd
        ld      bc, TAPE_DRIVER_JUMPS
        jp      bios_memcpy_bc_hl

        ; Таблца переходов
jumpsStart:
        jp      tapeRead
        jp      tapeWrite
        jp      tapeLoad
        jp      tapeSave
        jp      tapeLoadInt
        jp      tapeReadError
jumpsEnd:
