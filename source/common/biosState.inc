; Эти данные нужны монитор-совместимым сервисам без обращения к данным DOS.
    IFDEF DOS_SPLIT_BUILD
v_memTop: DW DOS_CODE_START-1
    ELSE
v_memTop: DW APP_MEMORY_END-1
    ENDIF
v_char:   BLOCK 13,0FFh
v_input_start: DW 0
v_input_end:   DW 0

; Физический BIOS хранит выбранный накопитель и его общий контекст.
v_drive: DB 1
; Адреса драйверов для 8 накопителей. Начальные значения:
; = diskDriver для накопителей A: и B: (встроенный драйвер ROM и RAM диска),
; = diskDriverDummy для отальных (пустой драйвер).
v_drives:           DW diskDriver,      diskDriver  ; A:, B:
                    DW diskDriverDummy, diskDriverDummy  ; C:, D:
                    DW diskDriverDummy, diskDriverDummy  ; E:, F:
                    DW diskDriverDummy, diskDriverDummy  ; G:, H:

; Информация о текущем накопителе
v_diskInfo          DISK_INFO
