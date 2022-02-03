;----------------------------------------------------------------------------
; MXOS
; Таблица перехвата вызовов драйвера магнитофона
;
; Изначально все функции просто перезапускают систему.
; Адреса заменяются на реальные при установке драйвера.
;
; 2022-02-03 SpaceEngineer
;----------------------------------------------------------------------------

t_tapeRead:      jp bios_reboot
t_tapeWrite:     jp bios_reboot
t_tapeLoad:      jp bios_reboot
t_tapeSave:      jp bios_reboot
t_tapeLoadInt:   jp bios_reboot
t_tapeReadError: jp bios_reboot
