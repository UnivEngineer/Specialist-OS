; MXOS
; Публичные входы MAIN/DOS/EXEC, пакеты и операции MEM.

    IFNDEF MXOS_ABI_MEMORY_INC
    DEFINE MXOS_ABI_MEMORY_INC

mx_callPacket   = 0F800h  ; B=группа, C=номер, DE=пакет MX_REGS
mx_callCPM      = 0F803h  ; Резерв для будущего адаптера CP/M; пока UNSUPPORTED
mx_callRegs     = 0F806h  ; CALL + DW операции; регистры как у исполнителя
mx_queueBind    = 0F809h  ; HL=local flush callback, 0 disables; AF destroyed
mx_queueFlush   = 0F80Ch  ; explicit flush; all registers and flags preserved
mx_memcpyFar    = 0F80Fh  ; HL=MEM_COPY_REQUEST; CF/A=статус, BC/DE/HL сохранены
mx_memInfo      = 0F812h  ; HL=версия, DE=возможности, A=FAR-код, B=банки, C=тип; CF=0
mx_memory       = 0F815h  ; A=MEM_OP_*, регистры/ошибки: source/mem/api.md
mx_routeCall    = 0F818h  ; HL=bound DLL handle, DE=local MX_REGS, results in CPU
mx_printChar    = 0F81Bh  ; C=символ с обработкой управляющих кодов
mx_getCursorPos = 0F81Eh  ; HL=координаты BIOS
mx_setCursorPos = 0F821h  ; HL=новые координаты BIOS
mx_printCharRaw = 0F824h  ; A=код глифа
mx_printHexByte = 0F827h  ; A=байт
mx_printHexWord = 0F82Ah  ; HL=слово
mx_getMemTop    = 0F830h  ; HL=верхняя граница MAIN включительно
mx_setMemTop    = 0F833h  ; HL=новая верхняя граница MAIN

MXOS_GROUP_BDOS   = 0
MXOS_GROUP_MEMORY = 3
; Публичный memcpy: банки 0=main, 1..16=ДОЗУ; слова little-endian.
; Нулевая длина возвращает успех. Исключающий конец не выше FE00h. В одной странице допускается перекрытие.
; Пакет, код и стек инициатора не должны попадать в область назначения.
MEM_ABI_VERSION = 00104h  ; Публичный ABI MEM 1.4

MEM_CAP_COPY        = 1
MEM_CAP_LAZY_WINDOW = 2   ; Исторический бит 1.3, в 1.4 сброшен
MEM_CAP_MEMORY_OPS  = 4
MEM_CAP_MODULES     = 8
MEM_OP_MOVE         = 0
MX_QUEUE_BIOS       = 0
MX_QUEUE_DOS        = 1
MX_QUEUE_DLL        = 2
MEM_OP_GET8         = 1
MEM_OP_GET16        = 2
MEM_OP_PUT8         = 3
MEM_OP_PUT16        = 4
MEM_OP_PRINT        = 5
MEM_OP_ALLOC        = 6
MEM_OP_FREE         = 7
MEM_OP_REALLOC      = 8
MEM_OP_ARENA        = 9
MEM_OP_RAMLIMIT     = 10
MEM_OP_FILL         = 11
MEM_OP_QUERY        = 12
MEM_OP_DISKCLAIM    = 13
MEM_OP_RAMDISK      = 14
MEM_OP_RESIDENT     = 15  ; B=0 query/1 replace, C=system slot, HL=bytes, DE=protected end
; MAINCLAIM: HL=start, BC=bytes, evicts overlapping inactive MAIN DLLs (code+BSS).
; No allocation/writes; active DLLs reject with ACTIVE, font/system slots survive.
MEM_OP_MAINCLAIM = 16
; MAINREGISTER: B=0, HL=nonzero MAIN base, DE=bytes. Exact owned range,
; no writes/moves/eviction or arena change. Overlap=ACTIVE; release with FREE.
; Tracked by normal image/module ownership and destructive FILE_EXEC handoff.
; Success: B=0, HL=base, DE=bytes, A=0, CF=0; error leaves registry unchanged.
MEM_OP_MAINREGISTER = 17
MEM_OP_MAINRETIRE   = 18  ; Release the current MAIN image and its allocations before an irreversible handoff.
MEM_RESIDENT_SLOTS  = 4
MEM_RESIDENT_FONT   = 0
MEM_RAMDISK_STATUS  = 0
; STATUS: B=банки, C=владелец, D=1 при потере хвоста MEM до bootstrap, E=0.
MEM_RAMDISK_COMMIT       = 1
MEM_RAMDISK_CHECK_LETTER = 2
RAM_ALTERNATE_DRIVE      = 3
RAMDISK_SERVICE_KEY      = 04452h  ; Исторический ключ RAMDSVC; штатный B: принадлежит DOS.
MEM_CAP_ALLOCATOR        = 16
MEM_CAP_COMMAND_QUEUE    = 32
MEM_CAP_BIOS_REGISTERS   = 64      ; Прямые регистровые входы в таблице F800h
RESIDENT_RETAIN          = 0
RESIDENT_FIND            = 1
RESIDENT_ENUM            = 2
RESIDENT_SHELL_SET       = 3       ; DE=preferred resident key, or 0 for the MAIN shell.
RESIDENT_SHELL_RESUME    = 4       ; Resume configured resident; CF=1 if absent/failed
NC_BANK_KEY              = 0434Eh
RESIDENT_DIRECTORY_EPOCH = 5       ; HL=low word, DE=high word of mutation generation
RESIDENT_EPOCH_ALL       = 0FEh    ; B=FEh: copy eight 32-bit drive epochs to NEAR HL
RESIDENT_RELEASE         = 6       ; Current resident drops its key; DOS unloads after RET
RESIDENT_POLICY          = 7       ; B=приоритет, C=флаги, HL=абсолютный путь DLL с нулём (до 32 байт)
RESIDENT_EXEC            = 8       ; HL=команда оболочки для запуска после возврата из CORE
RESIDENT_VETO            = 9       ; C=код (0 снимает), HL=имя ASCIIZ и текст ASCIIZ подряд, DE=адрес флага (0 всегда)
RESIDENT_SHELL_STATE     = 10      ; B=0 читать / 1 записать, HL=буфер, DE=35 байт
RESIDENT_STATE_BYTES     = 35
RESIDENT_RELOAD          = 3       ; Вход вновь загруженной DLL: B=2, C=буква диска
RESIDENT_PRIORITY_APP    = 100
RESIDENT_PRIORITY_EDIT   = 150
RESIDENT_PRIORITY_NC     = 200
RESIDENT_PRIORITY_DISK   = 254
RESIDENT_PRIORITY_PINNED = 255
RESIDENT_AUTO_RESTORE    = 1
RESIDENT_APP_CANT_UNLOAD = 0E8h    ; B=банк строк, C=причина, HL=имя, DE=текст ASCIIZ
; SHELL_SET: DE=0 selects NC_MN, otherwise DE is the preferred resident key.
; SHELL_RESUME: invoke RESTORE (A=1, other regs zero) on a fresh EXEC stack.
; When absent, returns CF=1, A=0 and HL=preferred key. No PC/SP snapshot.
; DIRECTORY_EPOCH: B=0 -> HL:DE low/high 32-bit global mutation generation;
; B=RESIDENT_EPOCH_ALL, HL=32-byte NEAR output -> eight little-endian drive
; generations A:..H:. Includes attempted writes, survives observer disconnect.
; RELEASE is self-only: it drops the key, assigns the inactive module to DOS,
; and leaves code alive until RET. The shell dispatcher then unloads it.

MEM_ALLOC_ANY = 0FFh

MEM_ERROR_NOMEM       = 0E3h
MEM_ERROR_OWNER       = 0E4h
MEM_ERROR_BANK        = 0E0h
MEM_ERROR_STACK       = 0E1h
MEM_ERROR_RANGE       = 0E2h
MEM_ERROR_HANDLE      = 0E5h
MEM_ERROR_ACTIVE      = 0E6h
MEM_ERROR_UNSUPPORTED = 0E7h

MODULE_TARGET_MAIN = 0
MODULE_TARGET_BANK = 1
MODULE_TARGET_DOS  = 2  ; DLL в одной из арен DOS; регистрируется точный диапазон

    STRUCT MEM_COPY_REQUEST
sourceBank          DB 0
sourceOffset        DW 0
destinationBank     DB 0
destinationOffset   DW 0
count               DW 0
    ENDS

    ASSERT MEM_COPY_REQUEST == 8

MEM_DATA_LIMIT        = 0FE00h  ; исключающий конец для FAR
APP_MEMORY_END        = 08C00h  ; штатный исключающий предел MAIN
LEGACY_APP_MEMORY_END = 08C00h  ; физическая граница обычной памяти
; Текущий доступный диапазон MAIN уточняется через mx_getMemTop.

    ENDIF
