;----------------------------------------------------------------------------
; MXOS — DEBUG.COM, монитор памяти и отладчик (ранее MON2.COM)
;
; 2022-02-07 Дизассемблировано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../common/system.inc"
IMAGE_FLAGS         = IMAGE_FLAG_MAIN
IMAGE_MEMORY_POLICY = IMAGE_POLICY_EXCLUSIVE
IMAGE_BSS_SIZE      = IMAGE_BSS_UNKNOWN

    INCLUDE "state.inc" ; Рабочая память и размещение программы
    INCLUDE "command.inc" ; Приглашение, ввод и диспетчер директив
    INCLUDE "disk.inc" ; Выход, каталог и дисковые директивы
    INCLUDE "debug.inc" ; Чтение ленты, регистры и программная точка останова
    INCLUDE "memory.inc" ; Поиск, сравнение, перенос, заполнение и текстовый дамп
    INCLUDE "input.inc" ; Разбор HEX-параметров и вход ввода строки
    INCLUDE "console.inc" ; Редактор памяти, ввод HEX, пауза и HEX-дамп
    INCLUDE "vectors.inc" ; Фиксированная таблица совместимости F800h
    INCLUDE "legacy.inc" ; Остатки старого обмена и общий ввод строки
debugPrinterNative:
    MXOS_CALL_REGS MXOS_BIOS_PRINTER
    ret
debugKeyScanNative:
    MXOS_CALL_REGS MXOS_BIOS_KEY_SCAN
    ret
debugTapeLoadNative:
    MXOS_CALL_REGS MXOS_BIOS_TAPE_LOAD
    ret
debugTapeSaveNative:
    MXOS_CALL_REGS MXOS_BIOS_TAPE_SAVE
    ret

; Собственные фиксированные буферы допустимы только ниже текущего memtop.

;----------------------------------------------------------------------------
; MXOS
;
; Проверить, помещается ли рабочая область DEBUG в MAIN.
;
; вход:
;   mx_getMemTop возвращает последний доступный адрес. Рабочие данные начинаются по
;   vars1.
;
; выход:
;   При достаточной памяти управление передаётся debugMemoryReady. Иначе выводится
;   сообщение NOT ENOUGH MEMORY и программа возвращается вызывающему. af, de и hl
;   рабочие.
;----------------------------------------------------------------------------

debugCheckMemory:
        CALL mx_getMemTop
        LD DE,vars1+MON2_VARS_1-1
        CALL mon2_cmp_hl_de
        JP NC,debugMemoryReady
        LD HL,debugNoMemory
        CALL mon2_printStringOld
        RET
debugNoMemory: DB "NOT ENOUGH MEMORY",0

    INCLUDE "file-name.inc" ; Пакетная печать поля 8.3

    ASSERT $ <= APP_MEMORY_END
    INCLUDE "../../sdk/asm/lib/file-io.inc"
    INCLUDE "../../sdk/asm/lib/file-image.inc"
    END
