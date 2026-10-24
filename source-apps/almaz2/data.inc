; Рабочие переменные, строки, спрайты, таймеры и карты исторической игры.
; Полностью текстовое представление данных; бинарные включения не используются.
    ASSERT $ = 00B74h

v_sprite_bank:
    DB 00h, 0Ch

v_player_processed:
    DB 00h

v_cell_flags:
    DB 00h

v_work_state:
    DB 010h

v_enemy_phase:
    DB 0C0h

v_diamonds_collected:
    DB 00h

v_scan_phase:
    DB 0C0h

v_random_state:
    DB 09h, 03Fh

v_level:
    DB 0Bh

v_level_pointer:
    DB 00h, 02Ah

v_exit_open:
    DB 00h

v_time_fraction:
    DB 04h

v_time_units:
    DB 08h

v_time_tens:
    DB 01h

v_time_hundreds:
    DB 00h

v_diamonds_required:
    DB 016h

v_time_preset_pointer:
    DB 02Ch, 012h

t_screen_text:
    DB 0Ch, 018h, 018h, 018h, 018h, 018h, 018h, 020h, 053h, 043h, 052h, 045h, 045h, 04Eh, 020h, 020h
    DB 020h, 08h, 08h, 00h

t_time_text:
    DB 020h, 054h, 049h, 04Dh, 045h, 020h, 020h, 00h

t_legacy_saved_status:
    DB 00h, 02h, 07h, 06h, 00h, 0FFh, 0ECh, 0Bh, 0Ch, 018h, 018h, 018h, 018h, 018h, 018h, 020h
    DB 053h, 043h, 052h, 045h, 045h, 04Eh, 020h, 020h, 020h, 08h, 08h

t_diamond_requirements:
    DB 022h, 0Fh, 02Bh, 01Ah, 01Eh, 028h, 020h, 01Bh, 012h, 050h, 060h, 016h, 034h, 014h, 022h, 036h

t_legacy_scratch:
    DB 0FEh, 00h, 0F8h, 0FFh, 08h, 0FFh, 060h, 0FFh, 020h, 0FEh, 0FFh, 00h, 00h, 09h, 00h, 01h
    DB 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h
    DB 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h

t_sprites_phase_zero:
; Тайл 0: два столбца по 16 экранных байт.
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
; Тайл 1: два столбца по 16 экранных байт.
    DB 0FFh, 088h, 088h, 088h, 0FFh, 088h, 088h, 088h, 088h, 088h, 088h, 0FFh, 088h, 088h, 088h, 0FFh
    DB 0FFh, 011h, 011h, 011h, 0FFh, 011h, 011h, 011h, 011h, 011h, 011h, 0FFh, 011h, 011h, 011h, 0FFh
; Тайл 2: два столбца по 16 экранных байт.
    DB 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h, 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h
    DB 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h, 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h
; Тайл 3: два столбца по 16 экранных байт.
    DB 0FFh, 0BDh, 0FFh, 0E6h, 0E6h, 0FFh, 0BDh, 0E7h, 0E7h, 0BDh, 0FFh, 0E6h, 0E6h, 0FFh, 0BDh, 0FFh
    DB 0FFh, 0BDh, 0FFh, 067h, 067h, 0FFh, 0BDh, 0E7h, 0E7h, 0BDh, 0FFh, 067h, 067h, 0FFh, 0BDh, 0FFh
; Тайл 4: два столбца по 16 экранных байт.
    DB 03h, 0Ch, 018h, 020h, 050h, 068h, 0D0h, 0C0h, 0D4h, 0E0h, 074h, 070h, 03Dh, 01Eh, 0Fh, 03h
    DB 0C0h, 030h, 08h, 04h, 02h, 02h, 01h, 01h, 01h, 01h, 08Ah, 0Ah, 024h, 098h, 0F0h, 0C0h
; Тайл 5: два столбца по 16 экранных байт.
    DB 01h, 02h, 04h, 09h, 012h, 025h, 04Bh, 097h, 097h, 04Bh, 025h, 012h, 09h, 04h, 02h, 01h
    DB 080h, 040h, 020h, 090h, 048h, 0A4h, 0D2h, 0E9h, 0E9h, 0D2h, 0A4h, 048h, 090h, 020h, 040h, 080h
; Тайл 6: два столбца по 16 экранных байт.
    DB 0FFh, 0FFh, 0FFh, 0E0h, 0EFh, 0E8h, 0EBh, 0EAh, 0EAh, 0EBh, 0E8h, 0EFh, 0E0h, 0FFh, 0FFh, 0FFh
    DB 0FFh, 0FFh, 0FFh, 07h, 0F7h, 017h, 0D7h, 057h, 057h, 0D7h, 017h, 0F7h, 07h, 0FFh, 0FFh, 0FFh
; Тайл 7: два столбца по 16 экранных байт.
    DB 080h, 0C0h, 0A0h, 092h, 0CAh, 0A5h, 0D3h, 0E9h, 0E9h, 0D3h, 0A5h, 0C9h, 090h, 0A0h, 0C0h, 080h
    DB 01h, 03h, 05h, 049h, 053h, 0A5h, 0CBh, 097h, 097h, 0CBh, 0A5h, 093h, 09h, 05h, 03h, 01h
; Тайл 8: два столбца по 16 экранных байт.
    DB 018h, 020h, 040h, 081h, 086h, 08h, 08h, 011h, 011h, 08h, 08h, 086h, 081h, 040h, 020h, 018h
    DB 018h, 04h, 02h, 081h, 061h, 010h, 010h, 088h, 088h, 010h, 010h, 061h, 081h, 02h, 04h, 018h
; Тайл 9: два столбца по 16 экранных байт.
    DB 00h, 03h, 04h, 03h, 07h, 01h, 01h, 01h, 00h, 01h, 02h, 00h, 00h, 00h, 00h, 03h
    DB 00h, 00h, 080h, 0C0h, 0C0h, 080h, 080h, 080h, 080h, 080h, 080h, 080h, 080h, 080h, 080h, 080h
; Тайл A: два столбца по 16 экранных байт.
    DB 02h, 08h, 022h, 09h, 024h, 092h, 054h, 028h, 089h, 052h, 029h, 0A4h, 049h, 019h, 04h, 012h
    DB 050h, 028h, 080h, 024h, 090h, 042h, 089h, 0A2h, 048h, 055h, 0Ah, 0A5h, 048h, 024h, 048h, 010h
; Тайл B: два столбца по 16 экранных байт.
    DB 00h, 01h, 00h, 00h, 00h, 028h, 054h, 0AAh, 054h, 0AAh, 054h, 028h, 04h, 010h, 05h, 00h
    DB 010h, 00h, 028h, 00h, 024h, 00h, 08h, 00h, 012h, 00h, 00h, 024h, 080h, 00h, 020h, 00h

t_sprites_phase_one:
; Тайл 0: два столбца по 16 экранных байт.
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
; Тайл 1: два столбца по 16 экранных байт.
    DB 044h, 0FFh, 044h, 044h, 044h, 0FFh, 044h, 044h, 044h, 044h, 0FFh, 044h, 044h, 044h, 0FFh, 044h
    DB 022h, 0FFh, 022h, 022h, 022h, 0FFh, 022h, 022h, 022h, 022h, 0FFh, 022h, 022h, 022h, 0FFh, 022h
; Тайл 2: два столбца по 16 экранных байт.
    DB 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h, 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h
    DB 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h, 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h
; Тайл 3: два столбца по 16 экранных байт.
    DB 0FFh, 0BDh, 0FFh, 0E6h, 0E6h, 0FFh, 0BDh, 0E7h, 0E7h, 0BDh, 0FFh, 0E6h, 0E6h, 0FFh, 0BDh, 0FFh
    DB 0FFh, 0BDh, 0FFh, 067h, 067h, 0FFh, 0BDh, 0E7h, 0E7h, 0BDh, 0FFh, 067h, 067h, 0FFh, 0BDh, 0FFh
; Тайл 4: два столбца по 16 экранных байт.
    DB 03h, 0Ch, 018h, 020h, 050h, 068h, 0D0h, 0C0h, 0D4h, 0E0h, 074h, 070h, 03Dh, 01Eh, 0Fh, 03h
    DB 0C0h, 030h, 08h, 04h, 02h, 02h, 01h, 01h, 01h, 01h, 08Ah, 0Ah, 024h, 098h, 0F0h, 0C0h
; Тайл 5: два столбца по 16 экранных байт.
    DB 01h, 02h, 04h, 09h, 013h, 027h, 04Eh, 09Dh, 09Dh, 04Eh, 027h, 013h, 09h, 04h, 02h, 01h
    DB 080h, 040h, 020h, 090h, 0C8h, 0E4h, 072h, 0B9h, 0B9h, 072h, 0E4h, 0C8h, 090h, 020h, 040h, 080h
; Тайл 6: два столбца по 16 экранных байт.
    DB 0FFh, 080h, 0BFh, 0BFh, 0BFh, 0B8h, 0BBh, 0BAh, 0BAh, 0BBh, 0B8h, 0BFh, 0BFh, 0BFh, 080h, 0FFh
    DB 0FFh, 01h, 0FDh, 0FDh, 0FDh, 01Dh, 0DDh, 05Dh, 05Dh, 0DDh, 01Dh, 0FDh, 0FDh, 0FDh, 01h, 0FFh
; Тайл 7: два столбца по 16 экранных байт.
    DB 010h, 010h, 018h, 01Ah, 01Eh, 015h, 013h, 019h, 019h, 013h, 015h, 01Dh, 01Ch, 018h, 018h, 010h
    DB 08h, 08h, 018h, 058h, 078h, 0A8h, 0C8h, 098h, 098h, 0C8h, 0A8h, 0B8h, 038h, 018h, 018h, 08h
; Тайл 8: два столбца по 16 экранных байт.
    DB 030h, 040h, 083h, 08Ch, 010h, 010h, 021h, 022h, 022h, 021h, 010h, 010h, 08Ch, 083h, 040h, 030h
    DB 0Ch, 02h, 0C1h, 031h, 08h, 08h, 084h, 044h, 044h, 084h, 08h, 08h, 031h, 0C1h, 02h, 0Ch
; Тайл 9: два столбца по 16 экранных байт.
    DB 00h, 06h, 09h, 07h, 07h, 01h, 03h, 05h, 05h, 05h, 09h, 01h, 01h, 01h, 01h, 06h
    DB 00h, 060h, 090h, 0E0h, 0E0h, 080h, 0C0h, 0A0h, 0A0h, 0A0h, 090h, 080h, 080h, 080h, 080h, 060h
; Тайл A: два столбца по 16 экранных байт.
    DB 02h, 08h, 022h, 09h, 024h, 092h, 054h, 028h, 089h, 052h, 029h, 0A4h, 049h, 019h, 04h, 012h
    DB 050h, 028h, 080h, 024h, 090h, 042h, 089h, 0A2h, 048h, 055h, 0Ah, 0A5h, 048h, 024h, 048h, 010h
; Тайл B: два столбца по 16 экранных байт.
    DB 00h, 01h, 02h, 05h, 012h, 025h, 012h, 041h, 010h, 040h, 028h, 020h, 0Ah, 08h, 01h, 00h
    DB 00h, 040h, 0A0h, 050h, 0A0h, 050h, 0A0h, 040h, 02h, 01h, 08h, 04h, 040h, 014h, 040h, 00h

t_sprites_phase_two:
; Тайл 0: два столбца по 16 экранных байт.
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
; Тайл 1: два столбца по 16 экранных байт.
    DB 022h, 022h, 0FFh, 022h, 022h, 022h, 0FFh, 022h, 022h, 0FFh, 022h, 022h, 022h, 0FFh, 022h, 022h
    DB 044h, 044h, 0FFh, 044h, 044h, 044h, 0FFh, 044h, 044h, 0FFh, 044h, 044h, 044h, 0FFh, 044h, 044h
; Тайл 2: два столбца по 16 экранных байт.
    DB 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h, 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h
    DB 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h, 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h
; Тайл 3: два столбца по 16 экранных байт.
    DB 0FFh, 0BDh, 0FFh, 0E6h, 0E6h, 0FFh, 0BDh, 0E7h, 0E7h, 0BDh, 0FFh, 0E6h, 0E6h, 0FFh, 0BDh, 0FFh
    DB 0FFh, 0BDh, 0FFh, 067h, 067h, 0FFh, 0BDh, 0E7h, 0E7h, 0BDh, 0FFh, 067h, 067h, 0FFh, 0BDh, 0FFh
; Тайл 4: два столбца по 16 экранных байт.
    DB 03h, 0Ch, 018h, 020h, 050h, 068h, 0D0h, 0C0h, 0D4h, 0E0h, 074h, 070h, 03Dh, 01Eh, 0Fh, 03h
    DB 0C0h, 030h, 08h, 04h, 02h, 02h, 01h, 01h, 01h, 01h, 08Ah, 0Ah, 024h, 098h, 0F0h, 0C0h
; Тайл 5: два столбца по 16 экранных байт.
    DB 01h, 02h, 05h, 0Bh, 016h, 02Dh, 05Ah, 0B5h, 0B5h, 05Ah, 02Dh, 016h, 0Bh, 05h, 02h, 01h
    DB 080h, 040h, 0A0h, 0D0h, 068h, 0B4h, 05Ah, 0ADh, 0ADh, 05Ah, 0B4h, 068h, 0D0h, 0A0h, 040h, 080h
; Тайл 6: два столбца по 16 экранных байт.
    DB 0FFh, 080h, 0BFh, 0A0h, 0AFh, 0AFh, 0AFh, 0AEh, 0AEh, 0AFh, 0AFh, 0AFh, 0A0h, 0BFh, 080h, 0FFh
    DB 0FFh, 01h, 0FDh, 05h, 0F5h, 0F5h, 0F5h, 075h, 075h, 0F5h, 0F5h, 0F5h, 05h, 0FDh, 01h, 0FFh
; Тайл 7: два столбца по 16 экранных байт.
    DB 04h, 04h, 04h, 06h, 06h, 05h, 07h, 07h, 07h, 07h, 07h, 07h, 06h, 04h, 04h, 04h
    DB 020h, 020h, 020h, 060h, 060h, 0A0h, 0E0h, 0E0h, 0E0h, 0E0h, 0E0h, 0E0h, 060h, 020h, 020h, 020h
; Тайл 8: два столбца по 16 экранных байт.
    DB 040h, 083h, 0Ch, 010h, 020h, 021h, 042h, 044h, 044h, 042h, 021h, 020h, 010h, 0Ch, 083h, 040h
    DB 02h, 0C1h, 030h, 030h, 04h, 084h, 042h, 042h, 022h, 042h, 084h, 04h, 08h, 030h, 0C1h, 02h
; Тайл 9: два столбца по 16 экранных байт.
    DB 00h, 00h, 01h, 03h, 03h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h
    DB 00h, 0C0h, 020h, 0C0h, 0E0h, 080h, 080h, 080h, 080h, 080h, 040h, 00h, 00h, 00h, 00h, 0C0h
; Тайл A: два столбца по 16 экранных байт.
    DB 02h, 08h, 022h, 09h, 024h, 092h, 054h, 028h, 089h, 052h, 029h, 0A4h, 049h, 019h, 04h, 012h
    DB 050h, 028h, 080h, 024h, 090h, 042h, 089h, 0A2h, 048h, 055h, 0Ah, 0A5h, 048h, 024h, 048h, 010h
; Тайл B: два столбца по 16 экранных байт.
    DB 00h, 00h, 0Ah, 00h, 08h, 020h, 010h, 00h, 048h, 00h, 024h, 00h, 09h, 00h, 01h, 00h
    DB 00h, 080h, 020h, 088h, 00h, 014h, 02Ah, 055h, 02Ah, 055h, 02Ah, 014h, 00h, 00h, 040h, 00h

t_sprites_phase_three:
; Тайл 0: два столбца по 16 экранных байт.
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
; Тайл 1: два столбца по 16 экранных байт.
    DB 011h, 011h, 011h, 0FFh, 011h, 011h, 011h, 0FFh, 0FFh, 011h, 011h, 011h, 0FFh, 011h, 011h, 011h
    DB 088h, 088h, 088h, 0FFh, 088h, 088h, 088h, 0FFh, 0FFh, 088h, 088h, 088h, 0FFh, 088h, 088h, 088h
; Тайл 2: два столбца по 16 экранных байт.
    DB 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h, 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h
    DB 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h, 060h, 06Fh, 06Fh, 060h, 06h, 0F6h, 0F6h, 06h
; Тайл 3: два столбца по 16 экранных байт.
    DB 0FFh, 0BDh, 0FFh, 0E6h, 0E6h, 0FFh, 0BDh, 0E7h, 0E7h, 0BDh, 0FFh, 0E6h, 0E6h, 0FFh, 0BDh, 0FFh
    DB 0FFh, 0BDh, 0FFh, 067h, 067h, 0FFh, 0BDh, 0E7h, 0E7h, 0BDh, 0FFh, 067h, 067h, 0FFh, 0BDh, 0FFh
; Тайл 4: два столбца по 16 экранных байт.
    DB 03h, 0Ch, 018h, 020h, 050h, 068h, 0D0h, 0C0h, 0D4h, 0E0h, 074h, 070h, 03Dh, 01Eh, 0Fh, 03h
    DB 0C0h, 030h, 08h, 04h, 02h, 02h, 01h, 01h, 01h, 01h, 08Ah, 0Ah, 024h, 098h, 0F0h, 0C0h
; Тайл 5: два столбца по 16 экранных байт.
    DB 01h, 03h, 06h, 0Dh, 01Ah, 035h, 06Ah, 0D5h, 0D5h, 06Ah, 035h, 01Ah, 0Dh, 06h, 03h, 01h
    DB 080h, 0C0h, 060h, 0B0h, 058h, 0ACh, 056h, 0ABh, 0ABh, 056h, 0ACh, 058h, 0B0h, 060h, 0C0h, 080h
; Тайл 6: два столбца по 16 экранных байт.
    DB 0FFh, 080h, 0BFh, 0A0h, 0AFh, 0A8h, 0ABh, 0ABh, 0ABh, 0ABh, 0A8h, 0AFh, 0A0h, 0BFh, 080h, 0FFh
    DB 0FFh, 01h, 0FDh, 05h, 0F5h, 015h, 0D5h, 0D5h, 0D5h, 0D5h, 015h, 0F5h, 05h, 0FDh, 01h, 0FFh
; Тайл 7: два столбца по 16 экранных байт.
    DB 010h, 010h, 018h, 01Ah, 01Eh, 015h, 013h, 019h, 019h, 013h, 015h, 01Dh, 01Ch, 018h, 018h, 010h
    DB 08h, 08h, 018h, 058h, 078h, 0A8h, 0C8h, 098h, 098h, 0C8h, 0A8h, 0B8h, 038h, 018h, 018h, 08h
; Тайл 8: два столбца по 16 экранных байт.
    DB 087h, 018h, 020h, 040h, 043h, 084h, 088h, 088h, 088h, 088h, 084h, 043h, 040h, 020h, 018h, 087h
    DB 0E1h, 018h, 04h, 02h, 0C2h, 021h, 011h, 011h, 011h, 011h, 021h, 0C2h, 02h, 04h, 018h, 0E1h
; Тайл 9: два столбца по 16 экранных байт.
    DB 00h, 06h, 0Fh, 07h, 07h, 01h, 03h, 05h, 05h, 05h, 09h, 01h, 01h, 01h, 01h, 06h
    DB 00h, 060h, 0F0h, 0E0h, 0E0h, 080h, 0C0h, 0A0h, 0A0h, 0A0h, 090h, 080h, 080h, 080h, 080h, 060h
; Тайл A: два столбца по 16 экранных байт.
    DB 02h, 08h, 022h, 09h, 024h, 092h, 054h, 028h, 089h, 052h, 029h, 0A4h, 049h, 019h, 04h, 012h
    DB 050h, 028h, 080h, 024h, 090h, 042h, 089h, 0A2h, 048h, 055h, 0Ah, 0A5h, 048h, 024h, 048h, 010h
; Тайл B: два столбца по 16 экранных байт.
    DB 00h, 08h, 00h, 015h, 00h, 028h, 00h, 00h, 0A0h, 01h, 082h, 05h, 02h, 05h, 02h, 01h
    DB 00h, 080h, 024h, 00h, 054h, 01h, 08h, 01h, 04h, 042h, 0A0h, 052h, 0A4h, 050h, 0A0h, 040h

t_time_presets:
    LEVEL_TIME_PRESET t_time_budget_01, 01h, 01h, 01h, 00h
    LEVEL_TIME_PRESET t_time_budget_02, 01h, 01h, 03h, 00h
    LEVEL_TIME_PRESET t_time_budget_03, 01h, 01h, 07h, 00h
    LEVEL_TIME_PRESET t_time_budget_04, 01h, 01h, 07h, 00h
    LEVEL_TIME_PRESET t_time_budget_05, 01h, 01h, 07h, 00h
    LEVEL_TIME_PRESET t_time_budget_06, 01h, 01h, 06h, 00h
    LEVEL_TIME_PRESET t_time_budget_07, 01h, 01h, 07h, 00h
    LEVEL_TIME_PRESET t_time_budget_08, 01h, 01h, 07h, 00h
    LEVEL_TIME_PRESET t_time_budget_09, 01h, 01h, 09h, 00h
    LEVEL_TIME_PRESET t_time_budget_10, 01h, 01h, 06h, 00h
    LEVEL_TIME_PRESET t_time_budget_11, 01h, 01h, 09h, 00h
    LEVEL_TIME_PRESET t_time_budget_12, 01h, 01h, 02h, 00h
    LEVEL_TIME_PRESET t_time_budget_13, 01h, 01h, 02h, 01h
    LEVEL_TIME_PRESET t_time_budget_14, 01h, 01h, 08h, 00h
    LEVEL_TIME_PRESET t_time_budget_15, 01h, 01h, 08h, 00h
    LEVEL_TIME_PRESET t_time_budget_16, 01h, 01h, 01h, 00h

t_legacy_reserved:
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h, 09h
    DB 09h, 00h, 00h, 00h, 00h, 00h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 00h, 04h
    DB 00h, 00h, 04h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 00h, 04h
    DB 00h, 00h, 04h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 00h, 04h
    DB 04h, 04h, 04h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 04h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 00h, 04h
    DB 00h, 00h, 04h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 09h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

t_levels:

t_level_01:
; Уровень 1: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 01h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 05h, 05h, 05h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 00h, 05h, 05h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 05h, 05h, 00h, 05h, 05h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 05h, 00h, 05h, 00h, 05h, 00h, 00h, 00h, 00h, 00h, 00h, 05h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 05h, 05h, 05h, 05h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 00h, 05h, 05h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 04h, 04h, 04h, 00h, 00h, 00h, 00h, 00h, 00h, 04h, 04h, 04h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 00h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 00h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 00h, 04h, 04h, 04h, 04h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 04h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 00h, 04h, 00h, 00h, 04h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 09h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

t_level_02:
; Уровень 2: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 0Fh, 0Fh, 00h, 00h, 0Fh
    DB 03h, 0Ah, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 01h, 03h, 0Fh, 00h, 00h, 0Fh, 0Fh, 00h, 00h, 0Fh
    DB 03h, 04h, 0Ah, 0Ah, 04h, 04h, 04h, 05h, 02h, 0Ah, 04h, 04h, 04h, 0Ah, 0Ah, 02h, 05h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 03h, 0Ah, 00h, 00h, 0Fh, 0Fh, 00h, 00h, 0Fh
    DB 03h, 04h, 0Ah, 0Ah, 0Ah, 04h, 04h, 04h, 02h, 0Ah, 04h, 05h, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 0Ah, 04h, 04h, 0Ah, 03h, 0Ah, 00h, 00h, 0Fh, 0Fh, 00h, 00h, 0Fh
    DB 03h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 04h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 02h, 05h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 0Fh, 0Fh, 00h, 00h, 0Fh
    DB 03h, 05h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 0Fh, 0Fh, 00h, 00h, 0Fh
    DB 03h, 04h, 04h, 05h, 04h, 0Ah, 0Ah, 0Ah, 02h, 04h, 0Ah, 0Ah, 0Ah, 04h, 05h, 02h, 04h, 0Ah, 0Ah, 04h, 04h, 0Ah, 04h, 03h, 0Ah, 00h, 00h, 0Fh, 0Fh, 00h, 00h, 0Fh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 04h, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 04h, 05h, 04h, 0Ah, 04h, 03h, 0Ah, 00h, 00h, 0Fh, 0Fh, 00h, 00h, 0Fh
    DB 03h, 0Ah, 0Ah, 0Ah, 04h, 0Ah, 0Ah, 04h, 02h, 0Ah, 0Ah, 0Ah, 00h, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 04h, 04h, 04h, 0Ah, 04h, 03h, 0Ah, 00h, 09h, 0Fh, 02h, 00h, 0Bh, 0Fh
    DB 03h, 0Ah, 0Ah, 0Ah, 00h, 0Ah, 0Ah, 04h, 02h, 0Ah, 0Ah, 0Ah, 05h, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 00h, 03h, 0Ah, 00h, 03h, 0Fh, 00h, 00h, 09h, 0Fh
    DB 03h, 0Ah, 04h, 0Ah, 00h, 0Ah, 04h, 05h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 05h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 00h, 03h, 0Ah, 00h, 0Dh, 0Fh, 04h, 00h, 0Bh, 0Fh
    DB 03h, 0Ah, 0Ah, 0Ah, 00h, 0Ah, 04h, 0Ah, 02h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 00h, 03h, 0Ah, 00h, 0Fh, 0Fh, 04h, 00h, 0Dh, 0Fh
    DB 03h, 04h, 0Ah, 04h, 04h, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 09h, 0Fh, 06h, 00h, 09h, 0Fh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 02h, 04h, 04h, 04h, 0Ah, 0Ah, 0Ah, 02h, 04h, 04h, 05h, 04h, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 0Eh, 0Fh, 02h, 00h, 09h, 0Fh
    DB 03h, 09h, 0Ah, 04h, 05h, 0Ah, 0Ah, 05h, 02h, 04h, 05h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 05h, 03h, 0Ah, 00h, 05h, 0Fh, 06h, 00h, 0Fh, 0Fh
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 06h, 0Fh, 04h, 00h, 09h, 0Fh

t_level_03:
; Уровень 3: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 01h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 02h, 02h, 02h, 02h, 0Ah, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 0Ah, 02h, 02h, 02h, 02h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 09h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh

t_level_04:
; Уровень 4: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 01h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 07h, 0Ah, 00h, 0Ah, 00h, 0Ah, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 06h, 0Ah, 00h, 0Ah, 00h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 00h, 0Ah, 07h, 0Ah, 00h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 00h, 0Ah, 06h, 0Ah, 00h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 00h, 0Ah, 00h, 0Ah, 07h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 00h, 0Ah, 00h, 0Ah, 06h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 09h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh

t_level_05:
; Уровень 5: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 01h, 03h, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 03h, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 03h, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 03h, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 03h, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 03h, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 03h, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Fh, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 00h, 00h, 00h, 08h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 08h, 00h, 00h, 00h, 0Ah, 03h, 0Fh, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 09h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 06h, 00h, 06h, 00h, 06h, 00h, 06h, 00h, 06h, 00h, 06h, 00h, 06h, 00h, 06h, 00h, 06h, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

t_level_06:
; Уровень 6: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 00h, 00h, 02h, 04h, 04h, 04h, 04h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 01h, 03h, 0Ah, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 00h, 00h, 00h, 02h, 04h, 04h, 04h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 02h, 04h, 04h, 04h, 04h, 02h, 00h, 00h, 00h, 00h, 02h, 04h, 04h, 02h, 02h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 03h, 0Ah, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 00h, 02h, 04h, 04h, 04h, 02h, 00h, 00h, 00h, 00h, 00h, 0Ah, 0Ah, 02h, 00h, 02h, 04h, 04h, 04h, 04h, 04h, 02h, 03h, 0Ah, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 00h, 00h, 02h, 04h, 04h, 02h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 02h, 00h, 00h, 02h, 04h, 04h, 04h, 02h, 00h, 03h, 0Ah, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 00h, 00h, 00h, 0Ah, 0Ah, 02h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 02h, 00h, 00h, 00h, 0Ah, 0Ah, 0Ah, 00h, 00h, 03h, 0Ah, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh, 0Fh
    DB 03h, 00h, 00h, 00h, 00h, 00h, 02h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 02h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 02h, 00h, 00h, 00h, 00h, 02h, 02h, 02h, 02h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 02h, 00h, 00h, 00h, 00h, 02h, 0Ah, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 09h, 00h, 00h, 00h, 02h, 00h, 00h, 00h, 00h, 02h, 0Ah, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

t_level_07:
; Уровень 7: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 02h, 04h, 02h, 04h, 04h, 04h, 04h, 04h, 02h, 04h, 04h, 04h, 04h, 0Ah, 05h, 02h, 0Ah, 0Ah, 01h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 05h, 04h, 0Ah, 02h, 05h, 02h, 04h, 04h, 04h, 0Ah, 04h, 02h, 04h, 04h, 07h, 04h, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 04h, 04h, 0Ah, 0Ah, 0Ah, 02h, 02h, 04h, 04h, 04h, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 04h, 04h, 0Ah, 0Ah, 0Ah, 02h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 02h, 0Ah, 04h, 04h, 0Ah, 0Ah, 02h, 06h, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 04h, 04h, 04h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 06h, 04h, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 05h, 05h, 05h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 06h, 04h, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 05h, 0Ah, 0Ah, 0Ah, 02h, 07h, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 06h, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 02h, 0Ah, 0Ah, 0Ah, 02h, 02h, 02h, 02h, 02h, 02h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 07h, 0Ah, 0Ah, 02h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 02h, 0Ah, 0Ah, 0Ah, 04h, 04h, 04h, 04h, 04h, 06h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 05h, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 0Ah, 04h, 05h, 05h, 05h, 04h, 06h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 0Ah, 04h, 05h, 05h, 05h, 04h, 06h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 0Ah, 04h, 05h, 05h, 05h, 04h, 06h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 09h, 0Ah, 04h, 05h, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 0Ah, 04h, 05h, 05h, 05h, 04h, 06h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh

t_level_08:
; Уровень 8: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 01h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 0Ah, 0Ah, 0Ah, 0Ah, 09h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

t_level_09:
; Уровень 9: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 0Ah, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 01h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 0Ah, 0Ah, 0Ah, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 08h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 08h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 0Ah, 09h, 0Ah, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 0Ah, 0Ah, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 0Ah, 0Ah, 0Ah, 06h, 00h, 06h, 00h, 06h, 00h, 06h, 00h, 06h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

t_level_10:
; Уровень 10: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 01h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 05h, 05h, 04h, 04h, 04h, 05h, 04h, 04h, 04h, 04h, 04h, 05h, 04h, 05h, 05h, 05h, 05h, 05h, 04h, 04h, 04h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 05h, 04h, 05h, 05h, 04h, 05h, 05h, 05h, 04h, 04h, 04h, 05h, 04h, 04h, 05h, 04h, 05h, 05h, 04h, 05h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 05h, 05h, 04h, 04h, 05h, 04h, 05h, 05h, 04h, 05h, 04h, 04h, 05h, 04h, 04h, 05h, 04h, 05h, 05h, 04h, 04h, 04h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 05h, 05h, 04h, 04h, 05h, 04h, 04h, 04h, 04h, 04h, 04h, 05h, 05h, 04h, 05h, 04h, 05h, 05h, 04h, 05h, 04h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 05h, 05h, 05h, 04h, 05h, 05h, 04h, 04h, 05h, 04h, 04h, 04h, 05h, 04h, 04h, 04h, 04h, 04h, 05h, 04h, 04h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 05h, 05h, 04h, 04h, 04h, 04h, 04h, 05h, 05h, 04h, 05h, 04h, 05h, 05h, 05h, 05h, 05h, 04h, 05h, 05h, 05h, 04h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 04h, 05h, 04h, 04h, 04h, 04h, 04h, 05h, 04h, 05h, 04h, 04h, 04h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 05h, 04h, 04h, 04h, 05h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 05h, 04h, 05h, 05h, 05h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 04h, 04h, 05h, 04h, 05h, 05h, 05h, 05h, 05h, 04h, 04h, 04h, 05h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 05h, 04h, 05h, 04h, 04h, 04h, 04h, 05h, 04h, 04h, 05h, 04h, 05h, 04h, 05h, 05h, 04h, 05h, 04h, 04h, 04h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 05h, 04h, 05h, 05h, 05h, 04h, 05h, 04h, 04h, 05h, 04h, 05h, 04h, 04h, 04h, 05h, 05h, 04h, 04h, 04h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 05h, 04h, 04h, 05h, 05h, 04h, 05h, 05h, 04h, 05h, 04h, 05h, 05h, 05h, 05h, 05h, 04h, 05h, 04h, 05h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 09h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 04h, 05h, 04h, 04h, 05h, 05h, 05h, 05h, 05h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

t_level_11:
; Уровень 11: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 08h, 04h, 04h, 04h, 04h, 04h, 04h, 08h, 04h, 04h, 04h, 04h, 04h, 01h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 09h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 02h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 00h, 07h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

t_level_12:
; Уровень 12: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 028h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 01h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 00h, 0Ah, 00h, 0Ah, 06h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 06h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 0Ah, 00h, 0Ah, 07h, 0Ah, 00h, 0Ah, 00h, 0Ah, 07h, 0Ah, 00h, 0Ah, 00h, 0Ah, 07h, 0Ah, 00h, 0Ah, 00h, 0Ah, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 06h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 06h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 07h, 0Ah, 00h, 0Ah, 00h, 0Ah, 07h, 0Ah, 00h, 0Ah, 00h, 0Ah, 00h, 0Ah, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 09h, 00h, 00h, 00h, 00h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

t_level_13:
; Уровень 13: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 01h, 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah
    DB 03h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 03h, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 08h, 0Ah, 0Ah
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 04h, 04h, 0Ah, 08h, 08h, 08h, 08h
    DB 03h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 03h, 0Ah, 04h, 04h, 04h, 08h, 08h, 08h, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 00h, 00h, 06h, 00h, 00h, 0Ah, 06h, 00h, 00h, 00h, 00h, 0Ah, 00h, 00h, 00h, 00h, 00h, 0Ah, 0Ah, 03h, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 07h, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 00h, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 06h, 0Ah, 0Ah, 03h, 0Ah, 04h, 04h, 04h, 00h, 04h, 04h, 04h
    DB 03h, 04h, 0Ah, 0Ah, 02h, 05h, 0Ah, 0Ah, 00h, 0Ah, 02h, 05h, 00h, 0Ah, 00h, 0Ah, 02h, 05h, 0Ah, 0Ah, 00h, 0Ah, 04h, 03h, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 02h, 05h, 05h, 0Ah, 00h, 0Ah, 02h, 05h, 05h, 0Ah, 07h, 0Ah, 02h, 05h, 05h, 0Ah, 00h, 0Ah, 0Ah, 03h, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 02h, 05h, 05h, 05h, 00h, 0Ah, 02h, 05h, 05h, 05h, 00h, 0Ah, 02h, 05h, 05h, 05h, 07h, 0Ah, 0Ah, 03h, 0Ah, 0Ah, 04h, 00h, 04h, 04h, 0Ah, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 02h, 02h, 02h, 02h, 02h, 0Ah, 02h, 02h, 02h, 02h, 02h, 0Ah, 02h, 02h, 02h, 02h, 02h, 0Ah, 0Ah, 03h, 0Ah, 04h, 04h, 0Eh, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 04h, 04h, 00h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 00h, 00h, 0Ch, 04h, 00h, 0Ch, 00h
    DB 03h, 09h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 06h, 06h, 06h, 06h, 06h, 06h, 06h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 06h, 06h, 06h, 06h, 06h, 06h, 06h

t_level_14:
; Уровень 14: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 08h, 08h, 00h, 08h, 08h, 08h, 00h
    DB 03h, 0Ah, 04h, 04h, 05h, 02h, 04h, 0Ah, 0Ah, 05h, 02h, 05h, 04h, 05h, 02h, 04h, 04h, 0Ah, 0Ah, 05h, 05h, 02h, 01h, 03h, 08h, 08h, 08h, 08h, 08h, 08h, 08h, 08h
    DB 03h, 0Ah, 04h, 04h, 04h, 02h, 04h, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 02h, 04h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 00h, 03h, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 04h, 04h, 02h, 02h, 04h, 04h, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 02h, 00h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 04h, 0Ah, 02h, 0Ah, 04h, 04h, 04h, 04h, 02h, 02h, 04h, 0Ah, 02h, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 04h, 02h, 00h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 05h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 02h, 00h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 04h, 02h, 04h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 04h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 02h, 04h, 0Ah, 02h, 0Ah, 0Ah, 05h, 0Ah, 0Ah, 0Ah, 04h, 0Ah, 02h, 0Ah, 02h, 04h, 0Ah, 0Ah, 0Ah, 02h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 02h, 04h, 0Ah, 02h, 05h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 05h, 0Ah, 0Ah, 0Ah, 05h, 02h, 0Ah, 0Ah, 02h, 02h, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 05h, 02h, 0Ah, 05h, 02h, 0Ah, 05h, 03h, 0Ah, 00h, 00h, 00h, 00h, 00h, 00h, 00h
    DB 03h, 0Ah, 02h, 02h, 02h, 02h, 02h, 04h, 04h, 02h, 02h, 04h, 0Ah, 0Ah, 04h, 04h, 02h, 02h, 0Ah, 0Ah, 02h, 04h, 0Ah, 03h, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 02h, 05h, 05h, 00h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 04h, 04h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 04h, 0Ah, 03h, 0Ah, 00h, 0Eh, 00h, 0Eh, 00h, 0Eh, 00h
    DB 03h, 0Ah, 02h, 04h, 04h, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 05h, 04h, 0Ah, 03h, 0Ah, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh, 0Eh
    DB 03h, 0Ah, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 02h, 02h, 02h, 04h, 02h, 04h, 04h, 04h, 04h, 02h, 04h, 02h, 04h, 0Ah, 03h, 0Ah, 00h, 0Eh, 00h, 0Eh, 00h, 0Eh, 00h
    DB 03h, 09h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 05h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 06h, 06h, 06h, 06h, 06h, 06h, 06h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 06h, 06h, 06h, 06h, 06h, 06h, 06h

t_level_15:
; Уровень 15: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 04h, 00h, 00h, 00h, 04h, 00h, 00h
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 02h, 04h, 02h, 00h, 04h, 00h, 04h, 00h, 04h, 00h, 00h, 01h, 03h, 04h, 04h, 0Ah, 08h, 0Ah, 04h, 04h, 04h
    DB 03h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 02h, 0Ah, 04h, 02h, 0Ah, 04h, 0Ah, 04h, 05h, 04h, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 04h, 04h, 08h, 04h, 04h, 04h, 04h
    DB 03h, 02h, 04h, 04h, 04h, 04h, 04h, 04h, 04h, 02h, 0Ah, 0Ah, 04h, 02h, 0Ah, 04h, 05h, 04h, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 04h, 04h, 08h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 02h, 04h, 04h, 04h, 04h, 04h, 02h, 0Ah, 0Ah, 0Ah, 04h, 02h, 0Ah, 04h, 0Ah, 04h, 0Ah, 04h, 0Ah, 0Ah, 05h, 03h, 0Ah, 04h, 04h, 08h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 02h, 04h, 04h, 04h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 04h, 05h, 04h, 0Ah, 0Ah, 05h, 03h, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 02h, 04h, 02h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 04h, 0Ah, 04h, 0Ah, 0Ah, 05h, 03h, 0Ah, 04h, 08h, 04h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 05h, 04h, 0Ah, 04h, 0Ah, 04h, 04h, 03h, 0Ah, 04h, 0Ah, 04h, 00h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 00h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 04h, 05h, 04h, 0Ah, 04h, 04h, 03h, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 0Ah, 0Ah, 00h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 04h, 0Ah, 04h, 0Ah, 04h, 04h, 03h, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 06h, 06h, 06h, 06h, 06h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 05h, 04h, 05h, 04h, 0Ah, 05h, 05h, 03h, 0Ah, 04h, 08h, 08h, 08h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 06h, 07h, 07h, 07h, 06h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 04h, 0Ah, 04h, 0Ah, 05h, 05h, 03h, 0Ah, 04h, 04h, 04h, 04h, 04h, 04h, 04h
    DB 03h, 0Ah, 0Ah, 06h, 07h, 00h, 07h, 06h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 04h, 0Ah, 04h, 0Ah, 05h, 05h, 03h, 0Ah, 04h, 04h, 08h, 08h, 08h, 08h, 08h
    DB 03h, 0Ah, 0Ah, 06h, 07h, 07h, 07h, 06h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 05h, 04h, 0Ah, 04h, 0Ah, 05h, 05h, 03h, 0Ah, 04h, 04h, 0Ah, 04h, 04h, 04h, 04h
    DB 03h, 09h, 0Ah, 06h, 06h, 06h, 06h, 06h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 02h, 0Ah, 04h, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 0Ah, 03h, 0Ah, 06h, 06h, 06h, 06h, 06h, 06h, 06h
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 06h, 06h, 06h, 06h, 06h, 06h, 06h

t_level_16:
; Уровень 16: 16 строк, 24 видимые клетки и 8 служебных байт в строке.
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 00h, 01h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 05h, 00h, 05h, 00h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 05h, 05h, 00h, 00h, 00h, 00h, 00h, 00h, 05h, 05h, 05h, 05h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 00h, 09h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 05h, 00h, 05h, 00h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 05h, 00h, 00h, 05h, 00h, 05h, 05h, 00h, 05h, 00h, 00h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 00h, 00h, 05h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 05h, 05h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 05h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 05h, 05h, 00h, 00h, 00h, 00h, 00h, 05h, 05h, 05h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 00h, 00h, 00h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 00h, 00h, 00h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 00h, 00h, 00h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 07h, 00h, 00h, 00h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    DB 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
    ASSERT $ = 03400h
