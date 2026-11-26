; Эти данные нужны монитор-совместимым сервисам без обращения к данным DOS.
v_memTop: DW DOS_CODE_START-1
v_input_start: DW 0
v_input_end:   DW 0

; Физический BIOS хранит выбранный накопитель и его общий контекст.
v_drive:        DB 1
; Адреса драйверов для 8 накопителей. Начальные значения:
; = diskDriver для накопителей A: и B: (встроенный драйвер ROM и RAM диска),
; = diskDriverDummy для отальных (пустой драйвер).
v_drives:       DW diskDriverDummy, diskDriver      ; A:, B:
                DW diskDriver, diskDriverDummy      ; C:, D:
                    DW diskDriverDummy, diskDriverDummy  ; E:, F:
                    DW diskDriverDummy, diskDriverDummy  ; G:, H:

; Информация о текущем накопителе
v_diskInfo          DISK_INFO
