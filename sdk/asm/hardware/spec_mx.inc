; MXOS
; Регистры устройств «Специалист-MX/MX2».

    IFNDEF MXOS_HARDWARE_SPEC_MX_INC
    DEFINE MXOS_HARDWARE_SPEC_MX_INC

MX_SCREEN_BASE  = 09000h
MX_SCREEN_BYTES = 03000h
MX_SCREEN_END   = MX_SCREEN_BASE+MX_SCREEN_BYTES

// U7 - ВВ55 - клавиатура
IO_KEYB_A      = 0FFE0h
IO_KEYB_B      = 0FFE1h
IO_KEYB_C      = 0FFE2h
IO_KEYB_MODE   = 0FFE3h

// U6 - ВВ55 - программатор или ROM/флеш диск
IO_PROG_A      = 0FFE4h
IO_PROG_B      = 0FFE5h
IO_PROG_C      = 0FFE6h
IO_PROG_MODE   = 0FFE7h

// U5 - ВГ93 - контроллер дисковода
IO_FDD_CMD     = 0FFE8h
IO_FDD_TRACK   = 0FFE9h
IO_FDD_SECTOR  = 0FFEAh
IO_FDD_DATA    = 0FFEBh

// U4 - ВИ53 - таймер
IO_PIT_CH0     = 0FFECh
IO_PIT_CH1     = 0FFEDh
IO_PIT_CH2     = 0FFEEh
IO_PIT_CONTROL = 0FFEFh

// U3 - дополнительные порты контроллера дисковода
IO_FDD_REQ     = 0FFF0h
IO_FDD_DENSITY = 0FFF1h  ; 0 — двойная плотность, MFM
IO_FDD_SIDE    = 0FFF2h
IO_FDD_DRIVE   = 0FFF3h

// U2 - резерв
IO_RESERV_U2_0 = 0FFF4h
IO_RESERV_U2_1 = 0FFF5h
IO_RESERV_U2_2 = 0FFF6h
IO_RESERV_U2_3 = 0FFF7h

// U1 - регистр цвета
IO_COLOR       = 0FFF8h  ; регистр цвета
IO_RESERV_U1_1 = 0FFF9h  ; резерв
IO_MX2_STD     = 0FFFAh  ; MX2, STD режим: D0 = выкл/вкл ПЗУ, D0 = 8/5 цветов
IO_RESERV_U1_3 = 0FFFBh  ; резерв

// U0 - регистр режима/страниц
IO_PAGE_RAM    = 0FFFCh  ; порт включения основного ОЗУ
IO_PAGE_RAMD   = 0FFFDh  ; порт включения ОЗУ RAM-дисков
IO_PAGE_ROM    = 0FFFEh  ; порт включения ПЗУ/ROM-диска
IO_PAGE_STD    = 0FFFFh  ; порт включения STD режима

    ENDIF
