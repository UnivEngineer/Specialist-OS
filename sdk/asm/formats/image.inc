; MXOS
; Формат контейнера MXOS2 для COM, EXE, DLL и FNT.

    IFNDEF MXOS_FORMAT_IMAGE_INC
    DEFINE MXOS_FORMAT_IMAGE_INC

; COM/EXE/DLL/FNT: общий контейнер v2, заголовок 32 байта, затем тело.
; Все word little-endian. Заголовок не копируется в адрес загрузки.
; 00h: CD 20 "MX" 02 (5 Б); 05h: FLAGS (0/DRIVER/BANK/MAIN).
; 06h: load; 08h: entry; 0Ah: bodyBytes; 0Ch: bodyChecksum.
; 0Eh: BSS bytes (FFFFh — неизвестный объём); 10h: memoryBegin;
; 12h: memoryEnd исключительно; 14h: minHeap; 16h: policy;
; 17h: API (0/2); 18h: stackBytes; 1Ah: MODE; 1Bh: headerBytes=32;
; 1Ch: options (bit 0 CACHEABLE, bit 1 REUSABLE); 1Dh: reserved=0;
; 1Eh: headerChecksum по байтам 00h..1Dh; 20h: начало тела.
; bodyChecksum — штатная сумма Монитора-2 по телу, хвостовой суммы нет.
; Размер файла строго bodyBytes+32, entry внутри тела. Релокации нет.
; memoryBegin <= load; memoryEnd >= load+bodyBytes+BSS (если BSS известен).
; BSS и прочая рабочая память резервируются, но не обнуляются загрузчиком.
; Инициализацию выполняет программа; C CRT обнуляет собственный BSS.
; EXCLUSIVE=0: освобождение прикладной MAIN; неизвестный BSS требует её.
; PRESERVE=1: отказ при пересечении; MANAGED=2: можно вытеснять CACHEABLE.
; Активный код не вытесняется. DOS-арена освобождает лишь явный кэш.
; MODE=AUTO/MX/STD/MON/RF (0..4). FLAGS=0 передаёт выбор среды RUN;
; MODE!=AUTO позволяет RUN выбрать известное стандартное ядро без BAT/меню.
; Имя произвольного монитора и сценарии остаются в BAT с вызовом RUN.
; CPU: load, entry, monitor; далее ID=VALUE. Символы ASM метаданных:
; IMAGE_BSS_SIZE, IMAGE_MEM_BEGIN, IMAGE_MEM_END, IMAGE_MIN_HEAP_SIZE,
; IMAGE_STACK_SIZE, IMAGE_MEMORY_POLICY, IMAGE_API, IMAGE_COMPAT_MODE,
; IMAGE_CACHEABLE. Эти имена не совпадают с константами смещений полей.

; One signature for the kernel and local container readers.
    MACRO MXOS_IMAGE_SIGNATURE
        DB 0CDh,020h,"MX",2
    ENDM
IMAGE_HEADER_SIZE = 32

IMAGE_BSS              = 14
IMAGE_MEMORY_BEGIN     = 16
IMAGE_MEMORY_END       = 18
IMAGE_MIN_HEAP         = 20
IMAGE_POLICY           = 22
IMAGE_API_VERSION      = 23
IMAGE_STACK            = 24
IMAGE_MODE             = 26
IMAGE_MODE_AUTO        = 0
IMAGE_MODE_MX          = 1
IMAGE_MODE_STD         = 2
IMAGE_MODE_MON         = 3
IMAGE_MODE_RF          = 4
IMAGE_HEADER_BYTES     = 27
IMAGE_OPTIONS          = 28
IMAGE_HEADER_CHECKSUM  = 30
IMAGE_POLICY_EXCLUSIVE = 0
IMAGE_POLICY_PRESERVE  = 1
IMAGE_POLICY_MANAGED   = 2
IMAGE_POLICY_TRANSIENT = 3     ; Временная MAIN-программа без записи статической области
IMAGE_OPTION_CACHEABLE = 1
IMAGE_OPTION_REUSABLE  = 2     ; opt-in MAIN DLL activation on cold load and reuse
MODULE_ACTIVATE        = 0F0h  ; A at entry; C=0 fresh / 1 reuse; CF=reject

IMAGE_BSS_UNKNOWN = 0FFFFh
IMAGE_FLAG_DRIVER = 01h     ; байт 5 COM/EXE: инициализатор в странице DOS
IMAGE_FLAG_BANK   = 02h     ; требуется отдельный исполняемый банк
IMAGE_FLAG_MAIN   = 04h     ; явно подтверждена совместимость с main/MXOS BIOS

    ENDIF
