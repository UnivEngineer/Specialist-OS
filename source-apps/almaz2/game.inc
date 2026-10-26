; Алмаз-2: восстановленный дизассемблированный код Intel 8080.
; Основа — Almaz2-original.exe; адреса в комментариях служат для сверки с историческим бинарником.
    ORG 0

game_entry:
    jp      game_start ; 0000h

game_new_editor_map:
    IF ALMAZ_PIT
        JP audio_editor_new
    ELSE
    IF ALMAZ_OPTIMIZE
        JP input_editor_new
    ELSE
    jp      editor_create_map
    ENDIF
    ENDIF

game_edit_current_map:
    IF ALMAZ_PIT
        JP audio_editor_existing
    ELSE
    IF ALMAZ_OPTIMIZE
        JP input_editor_existing
    ELSE
    jp      editor_open
    ENDIF
    ENDIF

; Начало игры; стек 7FFFh, рабочая карта 8000h..81FFh.
game_start:
    IF ALMAZ_PIT
        JP audio_game_start
    ELSE
    IF ALMAZ_OPTIMIZE
        JP input_game_start
    ELSE
    ld      sp,07FFFh
    ENDIF
    ENDIF

game_initialize:
    ld      hl,t_music_intro ; 000Ch
    IF ALMAZ_PIT
        CALL audio_music
    ELSE
    call    sound_sequence
    ENDIF
    xor     a ; 0012h
    ld      (v_level),a ; 0013h
    ld      hl,t_levels ; 0016h
    ld      (v_level_pointer),hl ; 0019h
    ld      hl,t_time_presets ; 001Ch
    ld      (v_time_preset_pointer),hl ; 001Fh
    ld      hl,t_diamond_requirements ; 0022h
    ld      a,(hl) ; 0025h
    ld      (v_diamonds_required),a ; 0026h
    jp      game_reload ; 0029h

game_reload:
    ld      hl,t_sprites_phase_zero ; 002Ch
    ld      (v_sprite_bank),hl ; 002Fh
    call    level_load ; 0032h
    call    map_draw ; 0035h

; Игровой кадр: ввод, таймер, четыре прохода по 23*15 клеткам.
game_frame:
    call    keyboard_commands ; 0038h
    cp      01Fh ; 003Bh
    IF ALMAZ_PIT
        JP Z,audio_game_exit
    ELSE
    jp      z,app_bios_return
    ENDIF
    IF USE_FRAME_CLOCK
        NOP                    ; TIME обновляется перед каждым проходом по ВИ53.
        NOP
        NOP
    ELSE
    call    timer_decrement ; 0040h
    ENDIF
    ld      hl,t_sprites_phase_one ; 0043h
    IF ALMAZ_OPTIMIZE
        CALL animation_begin_bank
    ELSE
    IF USE_FRAME_CLOCK
        CALL frame_begin_bank
    ELSE
    ld      (v_sprite_bank),hl ; 0046h
    ENDIF
    ENDIF
    xor     a ; 0049h
    ld      (v_player_processed),a ; 004Ah
    ld      (v_scan_phase),a ; 004Dh
    ld      bc,0170Fh ; 0050h

scan_phase_zero:
    push    bc ; 0053h
    call    cell_update ; 0054h
    pop     bc ; 0057h
    dec     b ; 0058h
    jp      nz,scan_phase_zero ; 0059h
    ld      b,017h ; 005Ch
    dec     c ; 005Eh
    jp      nz,scan_phase_zero ; 005Fh
    ld      hl,t_sprites_phase_two ; 0062h
    IF ALMAZ_OPTIMIZE
        CALL animation_begin_bank
    ELSE
    IF USE_FRAME_CLOCK
        CALL frame_begin_bank
    ELSE
    ld      (v_sprite_bank),hl ; 0065h
    ENDIF
    ENDIF
    xor     a ; 0068h
    ld      (v_player_processed),a ; 0069h
    ld      a,040h ; 006Ch
    ld      (v_scan_phase),a ; 006Eh
    ld      bc,0170Fh ; 0071h

scan_phase_one:
    push    bc ; 0074h
    call    cell_update ; 0075h
    pop     bc ; 0078h
    dec     b ; 0079h
    jp      nz,scan_phase_one ; 007Ah
    ld      b,017h ; 007Dh
    dec     c ; 007Fh
    jp      nz,scan_phase_one ; 0080h
    ld      hl,t_sprites_phase_three ; 0083h
    IF ALMAZ_OPTIMIZE
        CALL animation_begin_bank
    ELSE
    IF USE_FRAME_CLOCK
        CALL frame_begin_bank
    ELSE
    ld      (v_sprite_bank),hl ; 0086h
    ENDIF
    ENDIF
    xor     a ; 0089h
    ld      (v_player_processed),a ; 008Ah
    ld      a,080h ; 008Dh
    ld      (v_scan_phase),a ; 008Fh
    ld      bc,0170Fh ; 0092h

scan_phase_two:
    push    bc ; 0095h
    call    cell_update ; 0096h
    pop     bc ; 0099h
    dec     b ; 009Ah
    jp      nz,scan_phase_two ; 009Bh
    ld      b,017h ; 009Eh
    dec     c ; 00A0h
    jp      nz,scan_phase_two ; 00A1h
    ld      hl,t_sprites_phase_zero ; 00A4h
    IF ALMAZ_OPTIMIZE
        CALL animation_begin_bank
    ELSE
    IF USE_FRAME_CLOCK
        CALL frame_begin_bank
    ELSE
    ld      (v_sprite_bank),hl ; 00A7h
    ENDIF
    ENDIF
    xor     a ; 00AAh
    ld      (v_player_processed),a ; 00ABh
    ld      a,0C0h ; 00AEh
    ld      (v_scan_phase),a ; 00B0h
    ld      bc,0170Fh ; 00B3h

scan_phase_three:
    push    bc ; 00B6h
    call    cell_update ; 00B7h
    pop     bc ; 00BAh
    dec     b ; 00BBh
    jp      nz,scan_phase_three ; 00BCh
    ld      b,017h ; 00BFh
    dec     c ; 00C1h
    jp      nz,scan_phase_three ; 00C2h
    jp      game_frame ; 00C5h

; Десятичный игровой таймер, привязанный к числу кадров.
timer_decrement:
    ld      a,(v_time_fraction) ; 00C8h
    dec     a ; 00CBh
    cp      0FFh ; 00CCh
    jp      z,timer_borrow_units ; 00CEh
    ld      (v_time_fraction),a ; 00D1h
    jp      timer_draw ; 00D4h

timer_borrow_units:
    ld      a,04h ; 00D7h
    ld      (v_time_fraction),a ; 00D9h
    ld      a,(v_time_units) ; 00DCh
    dec     a ; 00DFh
    cp      0FFh ; 00E0h
    jp      z,timer_borrow_tens ; 00E2h
    ld      (v_time_units),a ; 00E5h
    jp      timer_draw ; 00E8h

timer_borrow_tens:
    ld      a,09h ; 00EBh
    ld      (v_time_units),a ; 00EDh
    ld      a,(v_time_tens) ; 00F0h
    dec     a ; 00F3h
    cp      0FFh ; 00F4h
    jp      z,timer_borrow_hundreds ; 00F6h
    ld      (v_time_tens),a ; 00F9h
    jp      timer_draw ; 00FCh

timer_borrow_hundreds:
    ld      a,09h ; 00FFh
    ld      (v_time_tens),a ; 0101h
    ld      a,(v_time_hundreds) ; 0104h
    dec     a ; 0107h
    cp      0FFh ; 0108h
    jp      z,timer_expired ; 010Ah
    ld      (v_time_hundreds),a ; 010Dh
    jp      timer_draw ; 0110h

timer_expired:
    call    level_load ; 0113h
    ld      sp,07FFFh ; 0116h
    jp      game_reload ; 0119h

timer_draw:
    ld      a,(v_time_fraction) ; 011Ch
    cp      01h ; 011Fh
    ret     nz ; 0121h
    push    bc ; 0122h
    ld      a,(v_time_hundreds) ; 0123h
    ld      c,a ; 0126h
    ld      a,(v_time_tens) ; 0127h
    or      c ; 012Ah
    jp      nz,timer_print_value ; 012Bh
    ld      a,(v_time_units) ; 012Eh
    add     030h ; 0131h
    ld      (app_beep_frequency),a ; 0133h
    ld      (app_beep_duration),a ; 0136h
    call    sound_beep ; 0139h

; Общий хвост timer_draw: на стеке обязан находиться сохранённый BC.
; Прямой CALL запрещён; отдельная печать использует level_timer_print.
timer_print_value:
    IF ALMAZ_HUD
        LD HL,0500Bh
    ELSE
    ld      hl,0500Ah
    ENDIF
    ld      (app_cursor_position),hl ; 013Fh
    ld      hl,t_time_text ; 0142h
    IF ALMAZ_COLOR
        CALL hud_print_string
    ELSE
    call    app_bios_print_string
    ENDIF
    ld      a,(v_time_hundreds) ; 0148h
    add     030h ; 014Bh
    ld      c,a ; 014Dh
    IF ALMAZ_COLOR
        CALL hud_print_char
    ELSE
    call    app_bios_print_char
    ENDIF
    ld      a,(v_time_tens) ; 0151h
    add     030h ; 0154h
    ld      c,a ; 0156h
    IF ALMAZ_COLOR
        CALL hud_print_char
    ELSE
    call    app_bios_print_char
    ENDIF
    ld      a,(v_time_units) ; 015Ah
    add     030h ; 015Dh
    ld      c,a ; 015Fh
    IF ALMAZ_HUD
        CALL hud_time_last_char
    ELSE
    IF ALMAZ_COLOR
        CALL hud_print_char
    ELSE
    call    app_bios_print_char
    ENDIF
    ENDIF
    pop     bc ; 0163h
    ret ; 0164h

; Команды переключения уровней, перезапуска и редактора.
keyboard_commands:
    IF ALMAZ_OPTIMIZE
        CALL input_game_command
    ELSE
    call    app_bios_keycheck ; 0165h
    ENDIF
    cp      03Bh ; 0168h
    jp      nz,keyboard_previous_level ; 016Ah
    call    level_next ; 016Dh
    call    level_load ; 0170h
    call    map_draw ; 0173h
    ret ; 0176h

keyboard_previous_level:
    cp      02Dh ; 0177h
    jp      nz,keyboard_restart_level ; 0179h
    call    level_previous ; 017Ch
    call    level_load ; 017Fh
    call    map_draw ; 0182h
    ret ; 0185h

keyboard_restart_level:
    cp      020h ; 0186h
    jp      nz,keyboard_store_map ; 0188h
    call    level_load ; 018Bh
    call    map_draw ; 018Eh
    ret ; 0191h

keyboard_store_map:
    cp      07Ah ; 0192h
    ret     nz ; 0194h
    call    level_store ; 0195h
    ret ; 0198h

; Надпись SCREEN и номер уровня. BC/HL сохраняются.
hud_draw_level:
    push    bc ; 0199h
    push    hl ; 019Ah
    IF ALMAZ_HUD
        CALL hud_screen_position
    ELSE
    ld      hl,t_screen_text
    ENDIF
    IF ALMAZ_COLOR
        CALL hud_print_string
    ELSE
    call    app_bios_print_string
    ENDIF
    ld      a,(v_level) ; 01A1h
    add     040h ; 01A4h
    ld      c,a ; 01A6h
    IF ALMAZ_COLOR
        CALL hud_print_char
    ELSE
    call    app_bios_print_char
    ENDIF
    pop     hl ; 01AAh
    pop     bc ; 01ABh
    ret ; 01ACh

level_next:
    ld      a,(v_level) ; 01ADh
    inc     a ; 01B0h
    and     0Fh ; 01B1h
    ld      (v_level),a ; 01B3h
    add     a ; 01B6h
    ld      d,a ; 01B7h
    ld      e,00h ; 01B8h
    ld      hl,t_levels ; 01BAh
    add     hl,de ; 01BDh
    ld      (v_level_pointer),hl ; 01BEh
    ld      a,(v_level) ; 01C1h
    ld      e,a ; 01C4h
    ld      hl,t_diamond_requirements ; 01C5h
    ld      d,00h ; 01C8h
    add     hl,de ; 01CAh
    ld      a,(hl) ; 01CBh
    ld      (v_diamonds_required),a ; 01CCh
    ld      a,(v_level) ; 01CFh
    add     a ; 01D2h
    add     a ; 01D3h
    ld      e,a ; 01D4h
    ld      d,00h ; 01D5h
    ld      hl,t_time_presets ; 01D7h
    add     hl,de ; 01DAh
    ld      (v_time_preset_pointer),hl ; 01DBh
    ret ; 01DEh

level_previous:
    ld      a,(v_level) ; 01DFh
    dec     a ; 01E2h
    and     0Fh ; 01E3h
    ld      (v_level),a ; 01E5h
    add     a ; 01E8h
    ld      d,a ; 01E9h
    ld      e,00h ; 01EAh
    ld      hl,t_levels ; 01ECh
    add     hl,de ; 01EFh
    ld      (v_level_pointer),hl ; 01F0h
    ld      a,(v_level) ; 01F3h
    ld      e,a ; 01F6h
    ld      hl,t_diamond_requirements ; 01F7h
    ld      d,00h ; 01FAh
    add     hl,de ; 01FCh
    ld      a,(hl) ; 01FDh
    ld      (v_diamonds_required),a ; 01FEh
    ld      a,(v_level) ; 0201h
    add     a ; 0204h
    add     a ; 0205h
    ld      e,a ; 0206h
    ld      d,00h ; 0207h
    ld      hl,t_time_presets ; 0209h
    add     hl,de ; 020Ch
    ld      (v_time_preset_pointer),hl ; 020Dh
    ret ; 0210h

; Сохранить рабочую карту в таблицу уровней. Вход: v_level_pointer.
level_store:
    push    bc ; 0211h
    ld      hl,t_music_level ; 0212h
    IF ALMAZ_PIT
        CALL audio_music
    ELSE
    call    sound_sequence
    ENDIF
    ld      hl,(v_level_pointer) ; 0218h
    ld      de,MAP_BUFFER ; 021Bh
    ld      bc,0200h ; 021Eh

level_store_loop:
    ld      a,(de) ; 0221h
    and     0Fh ; 0222h
    ld      (hl),a ; 0224h
    inc     hl ; 0225h
    inc     de ; 0226h
    dec     bc ; 0227h
    ld      a,c ; 0228h
    or      b ; 0229h
    jp      nz,level_store_loop ; 022Ah
    xor     a ; 022Dh
    ld      (v_exit_open),a ; 022Eh
    ld      (v_diamonds_collected),a ; 0231h
    pop     bc ; 0234h
    ret ; 0235h

; Загрузить 512 байт уровня и начальные цифры таймера. BC сохраняется.
level_load:
    push    bc ; 0236h
    ld      hl,t_music_level ; 0237h
    IF ALMAZ_PIT
        CALL audio_music
    ELSE
    call    sound_sequence
    ENDIF
    ld      hl,(v_level_pointer) ; 023Dh
    ld      de,MAP_BUFFER ; 0240h
    ld      bc,0200h ; 0243h

level_load_loop:
    ld      a,(hl) ; 0246h
    and     0Fh ; 0247h
    ld      (de),a ; 0249h
    inc     hl ; 024Ah
    inc     de ; 024Bh
    dec     bc ; 024Ch
    ld      a,c ; 024Dh
    or      b ; 024Eh
    jp      nz,level_load_loop ; 024Fh
    xor     a ; 0252h
    ld      (v_exit_open),a ; 0253h
    ld      (v_diamonds_collected),a ; 0256h
    ld      hl,(v_time_preset_pointer) ; 0259h
    ld      a,(hl) ; 025Ch
    ld      (v_time_fraction),a ; 025Dh
    inc     hl ; 0260h
    ld      a,(hl) ; 0261h
    ld      (v_time_units),a ; 0262h
    inc     hl ; 0265h
    ld      a,(hl) ; 0266h
    ld      (v_time_tens),a ; 0267h
    inc     hl ; 026Ah
    ld      a,(hl) ; 026Bh
    IF USE_FRAME_CLOCK
        CALL level_timer_convert
    ELSE
    ld      (v_time_hundreds),a ; 026Ch
    ENDIF
    pop     bc ; 026Fh
    ret ; 0270h

; Обновить клетку: B=X, C=Y. Состояние движения — в старшей тетраде.
cell_update:
    IF ALMAZ_OPTIMIZE
        JP fast_cell_update
    ELSE
    IF ALMAZ_PIT
        CALL audio_read_cell
    ELSE
    IF ALMAZ_OPTIMIZE
        CALL input_read_cell
    ELSE
    call    map_get_cell
    ENDIF
    ENDIF
    ENDIF
    cp      00h ; 0274h
    jp      z,smoke_update ; 0276h
    cp      01h ; 0279h
    jp      z,exit_update ; 027Bh
    cp      02h ; 027Eh
    ret     z ; 0280h
    cp      03h ; 0281h
    ret     z ; 0283h
    cp      04h ; 0284h
    jp      z,rock_update ; 0286h
    cp      05h ; 0289h
    jp      z,diamond_update ; 028Bh
    cp      06h ; 028Eh
    jp      z,square_update ; 0290h
    cp      07h ; 0293h
    jp      z,butterfly_update ; 0295h
    cp      08h ; 0298h
    jp      z,fungus_update ; 029Ah
    cp      09h ; 029Dh
    jp      z,player_update ; 029Fh
    cp      0Ah ; 02A2h
    jp      z,soil_update ; 02A4h
    ret ; 02A7h

smoke_update:
    ld      a,(v_cell_flags) ; 02A8h
    or      a ; 02ABh
    ret     z ; 02ACh
    cp      010h ; 02ADh
    jp      nz,smoke_animate ; 02AFh
    xor     a ; 02B2h
    call    map_put_cell ; 02B3h
    call    tile_draw ; 02B6h
    ret ; 02B9h

smoke_animate:
    ld      a,0Bh ; 02BAh
    call    tile_draw ; 02BCh
    ld      a,(v_cell_flags) ; 02BFh
    sub     010h ; 02C2h
    call    map_put_cell ; 02C4h
    ret ; 02C7h

exit_update:
    ld      a,(v_diamonds_collected) ; 02C8h
    ld      e,a ; 02CBh
    ld      a,(v_diamonds_required) ; 02CCh
    cp      e ; 02CFh
    jp      nc,exit_check_open ; 02D0h
    ld      a,0FFh ; 02D3h
    ld      (v_exit_open),a ; 02D5h

exit_check_open:
    ld      a,(v_exit_open) ; 02D8h
    cp      0FFh ; 02DBh
    ret     nz ; 02DDh
    ld      a,01h ; 02DEh
    call    tile_draw ; 02E0h
    push    bc ; 02E3h
    dec     b ; 02E4h
    call    map_get_cell ; 02E5h
    cp      09h ; 02E8h
    jp      z,exit_enter_next_level ; 02EAh
    inc     b ; 02EDh
    inc     c ; 02EEh
    call    map_get_cell ; 02EFh
    cp      09h ; 02F2h
    jp      z,exit_enter_next_level ; 02F4h
    pop     bc ; 02F7h
    ret ; 02F8h

exit_enter_next_level:
    ld      sp,07FFFh ; 02F9h
    call    level_next ; 02FCh
    call    level_load ; 02FFh
    call    map_draw ; 0302h
    jp      game_reload ; 0305h

; Гравитация камней: падение, перекатывание и столкновения.
rock_update:
    call    map_get_cell ; 0308h
    ld      a,(v_cell_flags) ; 030Bh
    and     080h ; 030Eh
    jp      z,rock_check_below ; 0310h
    ld      a,(v_cell_flags) ; 0313h
    and     070h ; 0316h
    or      04h ; 0318h
    call    map_put_cell ; 031Ah
    ret ; 031Dh

rock_check_below:
    inc     c ; 031Eh
    ld      a,(v_cell_flags) ; 031Fh
    ld      (v_work_state),a ; 0322h
    call    map_get_cell ; 0325h
    cp      00h ; 0328h
    jp      nz,rock_blocked_below ; 032Ah
    ld      a,074h ; 032Dh
    call    map_put_cell ; 032Fh
    ld      a,04h ; 0332h
    call    tile_draw ; 0334h
    dec     c ; 0337h
    xor     a ; 0338h
    call    map_put_cell ; 0339h
    call    tile_draw ; 033Ch
    ret ; 033Fh

rock_blocked_below:
    ld      e,a ; 0340h
    ld      a,(v_work_state) ; 0341h
    cp      00h ; 0344h
    jp      z,rock_try_roll_left ; 0346h
    ld      a,e ; 0349h
    cp      09h ; 034Ah
    jp      nz,rock_hits_square ; 034Ch
    call    explosion_smoke ; 034Fh
    dec     c ; 0352h
    ret ; 0353h

rock_hits_square:
    cp      06h ; 0354h
    jp      nz,rock_hits_butterfly ; 0356h
    call    explosion_smoke ; 0359h
    dec     c ; 035Ch
    ret ; 035Dh

rock_hits_butterfly:
    cp      07h ; 035Eh
    jp      nz,rock_stop_falling ; 0360h
    dec     c ; 0363h
    call    explosion_diamonds ; 0364h
    ret ; 0367h

rock_try_roll_left:
    dec     c ; 0368h
    push    bc ; 0369h
    inc     c ; 036Ah
    dec     b ; 036Bh
    call    map_get_cell ; 036Ch
    cp      00h ; 036Fh
    jp      nz,rock_try_roll_right ; 0371h
    dec     c ; 0374h
    call    map_get_cell ; 0375h
    cp      00h ; 0378h
    jp      nz,rock_try_roll_right ; 037Ah
    ld      a,084h ; 037Dh
    call    map_put_cell ; 037Fh
    call    tile_draw ; 0382h
    inc     b ; 0385h
    xor     a ; 0386h
    call    map_put_cell ; 0387h
    call    tile_draw ; 038Ah
    pop     bc ; 038Dh
    ret ; 038Eh

rock_try_roll_right:
    pop     bc ; 038Fh
    push    bc ; 0390h
    inc     c ; 0391h
    inc     b ; 0392h
    call    map_get_cell ; 0393h
    cp      00h ; 0396h
    jp      nz,rock_stay ; 0398h
    dec     c ; 039Bh
    call    map_get_cell ; 039Ch
    cp      00h ; 039Fh
    jp      nz,rock_stay ; 03A1h
    ld      a,04h ; 03A4h
    call    map_put_cell ; 03A6h
    call    tile_draw ; 03A9h
    pop     bc ; 03ACh
    xor     a ; 03ADh
    call    map_put_cell ; 03AEh
    call    tile_draw ; 03B1h
    ret ; 03B4h

rock_stay:
    pop     bc ; 03B5h
    ld      a,04h ; 03B6h
    call    map_put_cell ; 03B8h
    ret ; 03BBh

rock_stop_falling:
    dec     c ; 03BCh
    ld      a,04h ; 03BDh
    call    map_put_cell ; 03BFh
    ld      hl,t_sound_stone ; 03C2h
    call    sound_sequence ; 03C5h
    ret ; 03C8h

; Взрыв с облаком: центр и восемь соседей, кроме неразрушаемой стены.
explosion_smoke:
    ld      hl,t_sound_smoke ; 03C9h
    call    sound_sequence ; 03CCh
    call    explosion_mark_smoke ; 03CFh
    inc     b ; 03D2h
    call    explosion_mark_smoke ; 03D3h
    dec     c ; 03D6h
    call    explosion_mark_smoke ; 03D7h
    dec     b ; 03DAh
    call    explosion_mark_smoke ; 03DBh
    dec     b ; 03DEh
    call    explosion_mark_smoke ; 03DFh
    inc     c ; 03E2h
    call    explosion_mark_smoke ; 03E3h
    inc     c ; 03E6h
    call    explosion_mark_smoke ; 03E7h
    inc     b ; 03EAh
    call    explosion_mark_smoke ; 03EBh
    inc     b ; 03EEh
    call    explosion_mark_smoke ; 03EFh
    dec     c ; 03F2h
    dec     b ; 03F3h
    ret ; 03F4h

explosion_mark_smoke:
    call    map_get_cell ; 03F5h
    cp      03h ; 03F8h
    ret     z ; 03FAh
    ld      a,0F0h ; 03FBh
    call    map_put_cell ; 03FDh
    ld      a,0Bh ; 0400h
    call    tile_draw ; 0402h
    ret ; 0405h

; Взрыв бабочки с превращением соседей в алмазы.
explosion_diamonds:
    ld      hl,t_sound_diamond_explosion ; 0406h
    call    sound_sequence ; 0409h
    call    map_get_cell ; 040Ch
    cp      03h ; 040Fh
    ret     z ; 0411h
    ld      a,05h ; 0412h
    call    map_put_cell ; 0414h
    call    tile_draw ; 0417h
    inc     b ; 041Ah
    call    map_get_cell ; 041Bh
    cp      03h ; 041Eh
    ret     z ; 0420h
    ld      a,05h ; 0421h
    call    map_put_cell ; 0423h
    call    tile_draw ; 0426h
    dec     c ; 0429h
    call    map_get_cell ; 042Ah
    cp      03h ; 042Dh
    ret     z ; 042Fh
    ld      a,05h ; 0430h
    call    map_put_cell ; 0432h
    call    tile_draw ; 0435h
    dec     b ; 0438h
    call    map_get_cell ; 0439h
    cp      03h ; 043Ch
    ret     z ; 043Eh
    ld      a,05h ; 043Fh
    call    map_put_cell ; 0441h
    call    tile_draw ; 0444h
    dec     b ; 0447h
    call    map_get_cell ; 0448h
    cp      03h ; 044Bh
    ret     z ; 044Dh
    ld      a,05h ; 044Eh
    call    map_put_cell ; 0450h
    call    tile_draw ; 0453h
    inc     c ; 0456h
    call    map_get_cell ; 0457h
    cp      03h ; 045Ah
    ret     z ; 045Ch
    ld      a,05h ; 045Dh
    call    map_put_cell ; 045Fh
    call    tile_draw ; 0462h
    inc     c ; 0465h
    call    map_get_cell ; 0466h
    cp      03h ; 0469h
    ret     z ; 046Bh
    ld      a,05h ; 046Ch
    call    map_put_cell ; 046Eh
    call    tile_draw ; 0471h
    inc     b ; 0474h
    call    map_get_cell ; 0475h
    cp      03h ; 0478h
    ret     z ; 047Ah
    ld      a,05h ; 047Bh
    call    map_put_cell ; 047Dh
    call    tile_draw ; 0480h
    inc     b ; 0483h
    call    map_get_cell ; 0484h
    cp      03h ; 0487h
    ret     z ; 0489h
    ld      a,05h ; 048Ah
    call    map_put_cell ; 048Ch
    call    tile_draw ; 048Fh
    dec     c ; 0492h
    dec     b ; 0493h
    ret ; 0494h

; Гравитация алмазов, симметричная обработке камней.
diamond_update:
    call    map_get_cell ; 0495h
    ld      a,(v_cell_flags) ; 0498h
    and     080h ; 049Bh
    jp      z,diamond_check_below ; 049Dh
    ld      a,(v_cell_flags) ; 04A0h
    and     070h ; 04A3h
    or      05h ; 04A5h
    call    map_put_cell ; 04A7h
    ret ; 04AAh

diamond_check_below:
    inc     c ; 04ABh
    ld      a,(v_cell_flags) ; 04ACh
    ld      (v_work_state),a ; 04AFh
    call    map_get_cell ; 04B2h
    cp      00h ; 04B5h
    jp      nz,diamond_blocked_below ; 04B7h
    ld      a,075h ; 04BAh
    call    map_put_cell ; 04BCh
    ld      a,05h ; 04BFh
    call    tile_draw ; 04C1h
    dec     c ; 04C4h
    xor     a ; 04C5h
    call    map_put_cell ; 04C6h
    call    tile_draw ; 04C9h
    ret ; 04CCh

diamond_blocked_below:
    ld      e,a ; 04CDh
    ld      a,(v_work_state) ; 04CEh
    cp      00h ; 04D1h
    jp      z,diamond_try_roll_left ; 04D3h
    ld      a,e ; 04D6h
    cp      09h ; 04D7h
    jp      nz,diamond_hits_square ; 04D9h
    dec     c ; 04DCh
    ld      a,05h ; 04DDh
    call    tile_draw ; 04DFh
    ret ; 04E2h

diamond_hits_square:
    cp      06h ; 04E3h
    jp      nz,diamond_hits_butterfly ; 04E5h
    call    explosion_smoke ; 04E8h
    dec     c ; 04EBh
    ret ; 04ECh

diamond_hits_butterfly:
    cp      07h ; 04EDh
    jp      nz,diamond_stop_falling ; 04EFh
    dec     c ; 04F2h
    call    explosion_diamonds ; 04F3h
    ret ; 04F6h

diamond_try_roll_left:
    dec     c ; 04F7h
    push    bc ; 04F8h
    inc     c ; 04F9h
    dec     b ; 04FAh
    call    map_get_cell ; 04FBh
    cp      00h ; 04FEh
    jp      nz,diamond_try_roll_right ; 0500h
    dec     c ; 0503h
    call    map_get_cell ; 0504h
    cp      00h ; 0507h
    jp      nz,diamond_try_roll_right ; 0509h
    ld      a,085h ; 050Ch
    call    map_put_cell ; 050Eh
    call    tile_draw ; 0511h
    inc     b ; 0514h
    xor     a ; 0515h
    call    map_put_cell ; 0516h
    call    tile_draw ; 0519h
    pop     bc ; 051Ch
    ret ; 051Dh

diamond_try_roll_right:
    pop     bc ; 051Eh
    push    bc ; 051Fh
    inc     c ; 0520h
    inc     b ; 0521h
    call    map_get_cell ; 0522h
    cp      00h ; 0525h
    jp      nz,diamond_stay ; 0527h
    dec     c ; 052Ah
    call    map_get_cell ; 052Bh
    cp      00h ; 052Eh
    jp      nz,diamond_stay ; 0530h
    ld      a,05h ; 0533h
    call    map_put_cell ; 0535h
    call    tile_draw ; 0538h
    pop     bc ; 053Bh
    xor     a ; 053Ch
    call    map_put_cell ; 053Dh
    call    tile_draw ; 0540h
    ret ; 0543h

diamond_stay:
    pop     bc ; 0544h
    ld      a,05h ; 0545h
    call    map_put_cell ; 0547h
    IF ALMAZ_OPTIMIZE
        CALL animation_static_draw
    ELSE
    call    tile_draw ; 054Ah
    ENDIF
    ret ; 054Dh

diamond_stop_falling:
    dec     c ; 054Eh
    ld      a,05h ; 054Fh
    call    map_put_cell ; 0551h
    call    tile_draw ; 0554h
    ld      hl,t_sound_diamond ; 0557h
    call    sound_sequence ; 055Ah
    ret ; 055Dh

; Квадратики: движение вдоль стен и поворот по часовой стрелке.
square_update:
    push    bc ; 055Eh
    call    map_get_cell ; 055Fh
    ld      a,(v_cell_flags) ; 0562h
    and     0C0h ; 0565h
    ld      (v_enemy_phase),a ; 0567h
    ld      a,(v_cell_flags) ; 056Ah
    and     030h ; 056Dh
    ld      (v_work_state),a ; 056Fh
    ld      a,(v_enemy_phase) ; 0572h
    ld      e,a ; 0575h
    ld      a,(v_scan_phase) ; 0576h
    cp      e ; 0579h
    jp      nz,square_redraw ; 057Ah
    ld      a,(v_work_state) ; 057Dh
    sub     010h ; 0580h
    and     030h ; 0582h
    call    direction_move ; 0584h
    call    map_get_cell ; 0587h
    cp      00h ; 058Ah
    jp      nz,square_hits_player ; 058Ch
    ld      a,(v_work_state) ; 058Fh
    sub     010h ; 0592h
    and     030h ; 0594h
    or      06h ; 0596h
    call    enemy_pack_state ; 0598h
    call    map_put_cell ; 059Bh
    ld      a,06h ; 059Eh
    call    tile_draw ; 05A0h
    pop     bc ; 05A3h
    xor     a ; 05A4h
    call    map_put_cell ; 05A5h
    call    tile_draw ; 05A8h
    ret ; 05ABh

square_hits_player:
    cp      09h ; 05ACh
    jp      nz,square_hits_fungus ; 05AEh
    call    explosion_smoke ; 05B1h
    pop     bc ; 05B4h
    ret ; 05B5h

square_hits_fungus:
    cp      08h ; 05B6h
    jp      nz,square_turn_clockwise ; 05B8h
    call    explosion_smoke ; 05BBh
    pop     bc ; 05BEh
    ret ; 05BFh

square_turn_clockwise:
    pop     bc ; 05C0h
    ld      a,(v_work_state) ; 05C1h
    add     010h ; 05C4h
    and     030h ; 05C6h
    or      06h ; 05C8h
    call    enemy_pack_state ; 05CAh
    call    map_put_cell ; 05CDh
    ld      a,06h ; 05D0h
    call    tile_draw ; 05D2h
    ret ; 05D5h

square_redraw:
    pop     bc ; 05D6h
    ld      a,06h ; 05D7h
    call    tile_draw ; 05D9h
    ret ; 05DCh

; Бабочки: движение вдоль стен в противоположную сторону.
butterfly_update:
    push    bc ; 05DDh
    call    map_get_cell ; 05DEh
    ld      a,(v_cell_flags) ; 05E1h
    and     0C0h ; 05E4h
    ld      (v_enemy_phase),a ; 05E6h
    ld      a,(v_cell_flags) ; 05E9h
    and     030h ; 05ECh
    ld      (v_work_state),a ; 05EEh
    ld      a,(v_enemy_phase) ; 05F1h
    ld      e,a ; 05F4h
    ld      a,(v_scan_phase) ; 05F5h
    cp      e ; 05F8h
    jp      nz,butterfly_redraw ; 05F9h
    ld      a,(v_work_state) ; 05FCh
    add     010h ; 05FFh
    and     030h ; 0601h
    call    direction_move ; 0603h
    call    map_get_cell ; 0606h
    cp      00h ; 0609h
    jp      nz,butterfly_hits_player ; 060Bh
    ld      a,(v_work_state) ; 060Eh
    add     010h ; 0611h
    and     030h ; 0613h
    or      07h ; 0615h
    call    enemy_pack_state ; 0617h
    call    map_put_cell ; 061Ah
    ld      a,07h ; 061Dh
    call    tile_draw ; 061Fh
    pop     bc ; 0622h
    xor     a ; 0623h
    call    map_put_cell ; 0624h
    call    tile_draw ; 0627h
    ret ; 062Ah

butterfly_hits_player:
    cp      09h ; 062Bh
    jp      nz,butterfly_hits_fungus ; 062Dh
    call    explosion_diamonds ; 0630h
    pop     bc ; 0633h
    ret ; 0634h

butterfly_hits_fungus:
    cp      08h ; 0635h
    jp      nz,butterfly_turn_counterclockwise ; 0637h
    call    explosion_diamonds ; 063Ah
    pop     bc ; 063Dh
    ret ; 063Eh

butterfly_turn_counterclockwise:
    pop     bc ; 063Fh
    ld      a,(v_work_state) ; 0640h
    sub     010h ; 0643h
    and     030h ; 0645h
    or      07h ; 0647h
    call    enemy_pack_state ; 0649h
    call    map_put_cell ; 064Ch
    ld      a,07h ; 064Fh
    call    tile_draw ; 0651h
    ret ; 0654h

butterfly_redraw:
    pop     bc ; 0655h
    ld      a,07h ; 0656h
    call    tile_draw ; 0658h
    ret ; 065Bh

enemy_pack_state:
    ld      e,a ; 065Ch
    ld      a,(v_enemy_phase) ; 065Dh
    add     040h ; 0660h
    or      e ; 0662h
    ret ; 0663h

; Сдвиг координат B/C по направлению A=00h/10h/20h/30h.
direction_move:
    cp      00h ; 0664h
    jp      nz,direction_down ; 0666h
    inc     b ; 0669h
    ret ; 066Ah

direction_down:
    cp      010h ; 066Bh
    jp      nz,direction_left ; 066Dh
    inc     c ; 0670h
    ret ; 0671h

direction_left:
    cp      020h ; 0672h
    jp      nz,direction_up ; 0674h
    dec     b ; 0677h
    ret ; 0678h

direction_up:
    dec     c ; 0679h
    ret ; 067Ah

; Грибы: анимация и случайное размножение в пустоту или землю.
fungus_update:
    ld      a,08h ; 067Bh
    IF ALMAZ_OPTIMIZE
        CALL animation_static_draw
    ELSE
    call    tile_draw ; 067Dh
    ENDIF
    call    random_sparse_event ; 0680h
    and     01h ; 0683h
    ret     z ; 0685h
    push    bc ; 0686h
    call    random_next ; 0687h
    and     030h ; 068Ah
    call    direction_move ; 068Ch
    call    map_get_cell ; 068Fh
    cp      00h ; 0692h
    jp      z,fungus_grow ; 0694h
    cp      0Ah ; 0697h
    jp      z,fungus_grow ; 0699h
    pop     bc ; 069Ch
    ret ; 069Dh

fungus_grow:
    ld      a,08h ; 069Eh
    call    map_put_cell ; 06A0h
    call    tile_draw ; 06A3h
    pop     bc ; 06A6h
    ret ; 06A7h

; ГПСЧ: обновить 16-битное состояние, вернуть A. HL/DE сохраняются.
random_next:
    push    hl ; 06A8h
    push    de ; 06A9h
    ld      de,06D69h ; 06AAh
    ld      hl,(v_random_state) ; 06ADh
    add     hl,hl ; 06B0h
    ld      (v_random_state),hl ; 06B1h
    ld      a,h ; 06B4h
    and     040h ; 06B5h
    jp      z,random_result ; 06B7h
    ld      a,l ; 06BAh
    xor     e ; 06BBh
    ld      l,a ; 06BCh
    ld      a,h ; 06BDh
    xor     d ; 06BEh
    ld      h,a ; 06BFh
    ld      (v_random_state),hl ; 06C0h

random_result:
    ld      a,l ; 06C3h
    xor     h ; 06C4h
    pop     de ; 06C5h
    pop     hl ; 06C6h
    ret ; 06C7h

random_sparse_event:
    push    bc ; 06C8h
    ld      bc,05FFh ; 06C9h

random_sparse_loop:
    call    random_next ; 06CCh
    and     c ; 06CFh
    ld      c,a ; 06D0h
    dec     b ; 06D1h
    jp      nz,random_sparse_loop ; 06D2h
    ld      a,c ; 06D5h
    pop     bc ; 06D6h
    ret ; 06D7h

; Игрок: ввод стрелок, толкание камней, земля, сбор алмазов.
player_update:
    ld      a,00h ; 06D8h
    call    map_put_cell ; 06DAh
    call    tile_draw ; 06DDh
    push    bc ; 06E0h
    ld      a,(v_player_processed) ; 06E1h
    cp      00h ; 06E4h
    jp      nz,player_commit_move ; 06E6h
    IF ALMAZ_OPTIMIZE
        CALL input_take_direction
    ELSE
    call    app_bios_keycheck ; 06E9h
    ENDIF
player_direction_code:
    cp      08h ; 06ECh
    jp      nz,player_push_right ; 06EEh
    dec     b ; 06F1h
    call    map_get_cell ; 06F2h
    cp      04h ; 06F5h
    jp      nz,player_test_destination ; 06F7h
    dec     b ; 06FAh
    call    map_get_cell ; 06FBh
    cp      00h ; 06FEh
    jp      nz,player_keep_position ; 0700h
    ld      hl,t_sound_stone ; 0703h
    call    sound_sequence ; 0706h
    ld      a,04h ; 0709h
    call    map_put_cell ; 070Bh
    call    tile_draw ; 070Eh
    xor     a ; 0711h
    inc     b ; 0712h
    call    map_put_cell ; 0713h
    call    tile_draw ; 0716h
    jp      player_test_destination ; 0719h

player_push_right:
    cp      018h ; 071Ch
    jp      nz,player_move_up ; 071Eh
    inc     b ; 0721h
    call    map_get_cell ; 0722h
    cp      04h ; 0725h
    jp      nz,player_test_destination ; 0727h
    inc     b ; 072Ah
    call    map_get_cell ; 072Bh
    cp      00h ; 072Eh
    jp      nz,player_keep_position ; 0730h
    ld      hl,t_sound_stone ; 0733h
    call    sound_sequence ; 0736h
    ld      a,04h ; 0739h
    call    map_put_cell ; 073Bh
    call    tile_draw ; 073Eh
    xor     a ; 0741h
    dec     b ; 0742h
    call    map_put_cell ; 0743h
    call    tile_draw ; 0746h
    jp      player_test_destination ; 0749h

player_move_up:
    cp      019h ; 074Ch
    jp      nz,player_move_down ; 074Eh
    dec     c ; 0751h
    jp      player_test_destination ; 0752h

player_move_down:
    cp      01Ah ; 0755h
    jp      nz,player_test_destination ; 0757h
    inc     c ; 075Ah

player_test_destination:
    IF PLAYER_DIAGONAL_MOVEMENT && ALMAZ_OPTIMIZE
    call    player_diagonal_destination
    ELSE
    call    map_get_cell ; 075Bh
    ENDIF
    cp      00h ; 075Eh
    jp      nz,player_test_soil ; 0760h

player_commit_move:
    ld      a,09h ; 0763h
    call    map_put_cell ; 0765h
    call    tile_draw ; 0768h
    ld      a,0FFh ; 076Bh
    ld      (v_player_processed),a ; 076Dh
    pop     bc ; 0770h
    ret ; 0771h

player_test_soil:
    cp      0Ah ; 0772h
    jp      nz,player_test_diamond ; 0774h
    ld      hl,t_sound_step ; 0777h
    call    sound_sequence ; 077Ah
    jp      player_commit_move ; 077Dh

player_test_diamond:
    cp      05h ; 0780h
    jp      nz,player_blocked ; 0782h
    ld      hl,t_sound_diamond ; 0785h
    call    sound_sequence ; 0788h
    ld      a,(v_diamonds_collected) ; 078Bh
    inc     a ; 078Eh
    ld      (v_diamonds_collected),a ; 078Fh
    jp      player_commit_move ; 0792h

player_blocked:
    pop     bc ; 0795h
    push    bc ; 0796h
    jp      player_commit_move ; 0797h

player_keep_position:
    ld      a,09h ; 079Ah
    pop     bc ; 079Ch
    call    map_put_cell ; 079Dh
    call    tile_draw ; 07A0h
    ret ; 07A3h

soil_update:
    ret ; 07A4h

; Редактор: создать карту с рамкой и заполнением землёй.
editor_create_map:
    ld      hl,t_sprites_phase_zero ; 07A5h
    ld      (v_sprite_bank),hl ; 07A8h
    ld      b,018h ; 07ABh

editor_column_loop:
    ld      c,00h ; 07ADh
    ld      a,03h ; 07AFh
    call    map_put_cell ; 07B1h
    ld      c,0Fh ; 07B4h
    call    map_put_cell ; 07B6h
    ld      c,0Eh ; 07B9h

editor_fill_soil:
    ld      a,0Eh ; 07BBh
    call    map_get_cell ; 07BDh
    and     01h ; 07C0h
    jp      z,editor_fill_soil_alternative ; 07C2h
    ld      a,0Ah ; 07C5h
    jp      editor_store_soil ; 07C7h

editor_fill_soil_alternative:
    ld      a,0Ah ; 07CAh

editor_store_soil:
    call    map_put_cell ; 07CCh
    ld      a,c ; 07CFh
    dec     a ; 07D0h
    ld      c,a ; 07D1h
    cp      01h ; 07D2h
    jp      nz,editor_fill_soil ; 07D4h
    ld      c,0Ah ; 07D7h
    ld      a,0Ah ; 07D9h
    call    map_put_cell ; 07DBh
    ld      c,0Dh ; 07DEh

editor_fill_lower_soil:
    ld      a,0Ah ; 07E0h
    call    map_put_cell ; 07E2h
    dec     c ; 07E5h
    ld      a,c ; 07E6h
    cp      09h ; 07E7h
    jp      nz,editor_fill_lower_soil ; 07E9h
    dec     b ; 07ECh
    jp      nz,editor_column_loop ; 07EDh
    ld      c,0Fh ; 07F0h

editor_draw_border:
    ld      b,00h ; 07F2h
    ld      a,03h ; 07F4h
    call    map_put_cell ; 07F6h
    ld      b,017h ; 07F9h
    call    map_put_cell ; 07FBh
    dec     c ; 07FEh
    jp      nz,editor_draw_border ; 07FFh
    ld      bc,0C08h ; 0802h
    xor     a ; 0805h
    call    map_put_cell ; 0806h

; Редактор: выбор тайла цифрами и перемещение стрелками.
editor_open:
    ld      hl,t_sprites_phase_zero ; 0809h
    ld      (v_sprite_bank),hl ; 080Ch
    call    map_draw ; 080Fh
    ld      bc,0C08h ; 0812h

editor_input_loop:
    call    keyboard_commands ; 0815h
    IF ALMAZ_PIT
        CALL audio_editor_getch
    ELSE
    IF ALMAZ_OPTIMIZE
        CALL input_editor_getch
    ELSE
    call    app_bios_getch
    ENDIF
    ENDIF
    cp      08h ; 081Bh
    jp      nz,editor_move_right ; 081Dh
    call    map_get_cell ; 0820h
    and     0Fh ; 0823h
    call    tile_draw ; 0825h
    dec     b ; 0828h
    ld      a,09h ; 0829h
    call    tile_draw ; 082Bh
    jp      editor_input_loop ; 082Eh

editor_move_right:
    cp      018h ; 0831h
    jp      nz,editor_move_up ; 0833h
    call    map_get_cell ; 0836h
    and     0Fh ; 0839h
    call    tile_draw ; 083Bh
    inc     b ; 083Eh
    ld      a,09h ; 083Fh
    call    tile_draw ; 0841h
    jp      editor_input_loop ; 0844h

editor_move_up:
    cp      019h ; 0847h
    jp      nz,editor_move_down ; 0849h
    call    map_get_cell ; 084Ch
    and     0Fh ; 084Fh
    call    tile_draw ; 0851h
    dec     c ; 0854h
    ld      a,09h ; 0855h
    call    tile_draw ; 0857h
    jp      editor_input_loop ; 085Ah

editor_move_down:
    cp      01Ah ; 085Dh
    jp      nz,editor_test_escape ; 085Fh
    call    map_get_cell ; 0862h
    and     0Fh ; 0865h
    call    tile_draw ; 0867h
    inc     c ; 086Ah
    ld      a,09h ; 086Bh
    call    tile_draw ; 086Dh
    jp      editor_input_loop ; 0870h

editor_test_escape:
    cp      01Fh ; 0873h
    jp      nz,editor_select_tile ; 0875h
    jp      game_start ; 0878h

editor_select_tile:
    sub     030h ; 087Bh
    and     0Fh ; 087Dh
    call    map_put_cell ; 087Fh
    jp      editor_input_loop ; 0882h

; Тайл 16*16: A=тип, B=X, C=Y, v_sprite_bank=банк. AF/HL/BC сохраняются; DE разрушается.
tile_draw:
    push    hl ; 0885h
    IF ALMAZ_COLOR
        JP tile_draw_color
    ELSE
    push    af
    and     0Fh
    ENDIF

tile_draw_pixels:
    ld      l,a ; 0889h
    ld      h,00h ; 088Ah
    add     hl,hl ; 088Ch
    add     hl,hl ; 088Dh
    add     hl,hl ; 088Eh
    add     hl,hl ; 088Fh
    add     hl,hl ; 0890h
    ex      de,hl ; 0891h
    IF ALMAZ_OPTIMIZE
        CALL animation_sprite_bank
    ELSE
    ld      hl,(v_sprite_bank) ; 0892h
    ENDIF
    ex      de,hl ; 0895h
    add     hl,de ; 0896h
    ex      de,hl ; 0897h
    ld      a,b ; 0898h
    add     a ; 0899h
    add     090h ; 089Ah
    ld      h,a ; 089Ch
    ld      a,c ; 089Dh
    rla ; 089Eh
    rla ; 089Fh
    rla ; 08A0h
    rla ; 08A1h
    and     0F0h ; 08A2h
    ld      l,a ; 08A4h
    push    hl ; 08A5h
    ld      a,(de) ; 08A6h
    ld      (hl),a ; 08A7h
    inc     hl ; 08A8h
    inc     de ; 08A9h
    ld      a,(de) ; 08AAh
    ld      (hl),a ; 08ABh
    inc     hl ; 08ACh
    inc     de ; 08ADh
    ld      a,(de) ; 08AEh
    ld      (hl),a ; 08AFh
    inc     hl ; 08B0h
    inc     de ; 08B1h
    ld      a,(de) ; 08B2h
    ld      (hl),a ; 08B3h
    inc     hl ; 08B4h
    inc     de ; 08B5h
    ld      a,(de) ; 08B6h
    ld      (hl),a ; 08B7h
    inc     hl ; 08B8h
    inc     de ; 08B9h
    ld      a,(de) ; 08BAh
    ld      (hl),a ; 08BBh
    inc     hl ; 08BCh
    inc     de ; 08BDh
    ld      a,(de) ; 08BEh
    ld      (hl),a ; 08BFh
    inc     hl ; 08C0h
    inc     de ; 08C1h
    ld      a,(de) ; 08C2h
    ld      (hl),a ; 08C3h
    inc     hl ; 08C4h
    inc     de ; 08C5h
    ld      a,(de) ; 08C6h
    ld      (hl),a ; 08C7h
    inc     hl ; 08C8h
    inc     de ; 08C9h
    ld      a,(de) ; 08CAh
    ld      (hl),a ; 08CBh
    inc     hl ; 08CCh
    inc     de ; 08CDh
    ld      a,(de) ; 08CEh
    ld      (hl),a ; 08CFh
    inc     hl ; 08D0h
    inc     de ; 08D1h
    ld      a,(de) ; 08D2h
    ld      (hl),a ; 08D3h
    inc     hl ; 08D4h
    inc     de ; 08D5h
    ld      a,(de) ; 08D6h
    ld      (hl),a ; 08D7h
    inc     hl ; 08D8h
    inc     de ; 08D9h
    ld      a,(de) ; 08DAh
    ld      (hl),a ; 08DBh
    inc     hl ; 08DCh
    inc     de ; 08DDh
    ld      a,(de) ; 08DEh
    ld      (hl),a ; 08DFh
    inc     hl ; 08E0h
    inc     de ; 08E1h
    ld      a,(de) ; 08E2h
    ld      (hl),a ; 08E3h
    inc     hl ; 08E4h
    inc     de ; 08E5h
    pop     hl ; 08E6h
    inc     h ; 08E7h
    ld      a,(de) ; 08E8h
    ld      (hl),a ; 08E9h
    inc     hl ; 08EAh
    inc     de ; 08EBh
    ld      a,(de) ; 08ECh
    ld      (hl),a ; 08EDh
    inc     hl ; 08EEh
    inc     de ; 08EFh
    ld      a,(de) ; 08F0h
    ld      (hl),a ; 08F1h
    inc     hl ; 08F2h
    inc     de ; 08F3h
    ld      a,(de) ; 08F4h
    ld      (hl),a ; 08F5h
    inc     hl ; 08F6h
    inc     de ; 08F7h
    ld      a,(de) ; 08F8h
    ld      (hl),a ; 08F9h
    inc     hl ; 08FAh
    inc     de ; 08FBh
    ld      a,(de) ; 08FCh
    ld      (hl),a ; 08FDh
    inc     hl ; 08FEh
    inc     de ; 08FFh
    ld      a,(de) ; 0900h
    ld      (hl),a ; 0901h
    inc     hl ; 0902h
    inc     de ; 0903h
    ld      a,(de) ; 0904h
    ld      (hl),a ; 0905h
    inc     hl ; 0906h
    inc     de ; 0907h
    ld      a,(de) ; 0908h
    ld      (hl),a ; 0909h
    inc     hl ; 090Ah
    inc     de ; 090Bh
    ld      a,(de) ; 090Ch
    ld      (hl),a ; 090Dh
    inc     hl ; 090Eh
    inc     de ; 090Fh
    ld      a,(de) ; 0910h
    ld      (hl),a ; 0911h
    inc     hl ; 0912h
    inc     de ; 0913h
    ld      a,(de) ; 0914h
    ld      (hl),a ; 0915h
    inc     hl ; 0916h
    inc     de ; 0917h
    ld      a,(de) ; 0918h
    ld      (hl),a ; 0919h
    inc     hl ; 091Ah
    inc     de ; 091Bh
    ld      a,(de) ; 091Ch
    ld      (hl),a ; 091Dh
    inc     hl ; 091Eh
    inc     de ; 091Fh
    ld      a,(de) ; 0920h
    ld      (hl),a ; 0921h
    inc     hl ; 0922h
    inc     de ; 0923h
    ld      a,(de) ; 0924h
    ld      (hl),a ; 0925h
    inc     hl ; 0926h
    inc     de ; 0927h
    pop     af ; 0928h
    pop     hl ; 0929h
    ret ; 092Ah

; Перерисовать 24*16 клеток рабочей карты, затем надпись уровня.
map_draw:
    ld      hl,MAP_BUFFER ; 092Bh
    ld      bc,0170Fh ; 092Eh

map_draw_loop:
    call    map_draw_cell ; 0931h
    dec     c ; 0934h
    ld      a,c ; 0935h
    cp      0FFh ; 0936h
    jp      nz,map_draw_loop ; 0938h
    ld      c,0Fh ; 093Bh
    dec     b ; 093Dh
    ld      a,b ; 093Eh
    cp      0FFh ; 093Fh
    jp      nz,map_draw_loop ; 0941h
    IF USE_FRAME_CLOCK
        CALL frame_map_drawn
    ELSE
    IF ALMAZ_OPTIMIZE
        CALL input_map_drawn
    ELSE
    IF ALMAZ_OPTIMIZE
        CALL animation_map_drawn
    ELSE
    call    hud_draw_level ; 0944h
    ENDIF
    ENDIF
    ENDIF
    ret ; 0947h

map_draw_cell:
    push    hl ; 0948h
    IF ALMAZ_PIT
        CALL audio_read_cell
    ELSE
    IF ALMAZ_OPTIMIZE
        CALL input_read_cell
    ELSE
    call    map_get_cell
    ENDIF
    ENDIF
    and     0Fh ; 094Ch
    call    map_put_cell ; 094Eh
    call    tile_draw ; 0951h
    pop     hl ; 0954h
    ret ; 0955h

; Читать клетку B=X/C=Y. A=тип 0..15; v_cell_flags=старшая тетрада. HL/DE сохраняются, флаги меняются.
map_get_cell:
    push    hl ; 0956h
    push    de ; 0957h
    ld      l,c ; 0958h
    ld      h,00h ; 0959h
    add     hl,hl ; 095Bh
    add     hl,hl ; 095Ch
    add     hl,hl ; 095Dh
    add     hl,hl ; 095Eh
    add     hl,hl ; 095Fh
    ld      e,b ; 0960h
    ld      d,00h ; 0961h
    add     hl,de ; 0963h
    ld      de,MAP_BUFFER ; 0964h
    add     hl,de ; 0967h
    ld      a,(hl) ; 0968h
    ld      e,a ; 0969h
    and     0F0h ; 096Ah
    ld      (v_cell_flags),a ; 096Ch
    ld      a,e ; 096Fh
    and     0Fh ; 0970h
    pop     de ; 0972h
    pop     hl ; 0973h
    ret ; 0974h

; Записать A в клетку B=X/C=Y. AF/BC/HL/DE сохраняются.
map_put_cell:
    push    hl ; 0975h
    push    de ; 0976h
    ld      l,c ; 0977h
    ld      h,00h ; 0978h
    add     hl,hl ; 097Ah
    add     hl,hl ; 097Bh
    add     hl,hl ; 097Ch
    add     hl,hl ; 097Dh
    add     hl,hl ; 097Eh
    ld      e,b ; 097Fh
    ld      d,00h ; 0980h
    add     hl,de ; 0982h
    ld      de,MAP_BUFFER ; 0983h
    add     hl,de ; 0986h
    ld      (hl),a ; 0987h
    pop     de ; 0988h
    pop     hl ; 0989h
    ret ; 098Ah

; Звуковая последовательность: HL=таблица. Старый путь блокирующий, ВИ53 — асинхронный.
sound_sequence:
    IF ALMAZ_PIT
        JP audio_start
        DS 027h-3, 0FFh
    ELSE
    ld      a,(hl) ; 098Bh
    inc     hl ; 098Ch
    cp      00h ; 098Dh
    jp      nz,sound_test_end ; 098Fh
    call    sound_rest ; 0992h
    jp      sound_sequence ; 0995h

sound_test_end:
    cp      0FFh ; 0998h
    jp      nz,sound_play_note ; 099Ah
    ret ; 099Dh

sound_play_note:
    ld      (app_beep_frequency),a ; 099Eh
    ld      a,(hl) ; 09A1h
    inc     hl ; 09A2h
    ld      (app_beep_duration),a ; 09A3h
    call    sound_beep ; 09A6h
    IF ALMAZ_OPTIMIZE
        CALL input_read_command
    ELSE
    call    app_bios_keycheck ; 09A9h
    ENDIF
    cp      01Fh ; 09ACh
    ret     z ; 09AEh
    jp      sound_sequence ; 09AFh
    ENDIF
    ASSERT $ = 009B2h

; Один тон по app_beep_frequency/duration. BC/DE/HL сохраняются.
sound_beep:
    IF ALMAZ_PIT
        JP audio_countdown
        DS 10-3, 0FFh
    ELSE
    push    hl ; 09B2h
    push    de ; 09B3h
    push    bc ; 09B4h
    call    app_bios_beep ; 09B5h
    pop     bc ; 09B8h
    pop     de ; 09B9h
    pop     hl ; 09BAh
    ret ; 09BBh
    ENDIF
    ASSERT $ = 009BCh

sound_long_delay:
    push    hl ; 09BCh
    ld hl,02000h ; 09BDh

sound_long_delay_loop:
    dec     hl ; 09C0h
    ld      a,l ; 09C1h
    or      h ; 09C2h
    jp      nz,sound_long_delay_loop ; 09C3h
    pop     hl ; 09C6h
    ret ; 09C7h

; Пауза программного проигрывателя; HL сохраняется, AF разрушается.
sound_rest:
    push    hl ; 09C8h
    ld hl,01000h ; 09C9h

sound_rest_loop:
    dec     hl ; 09CCh
    ld      a,l ; 09CDh
    or      h ; 09CEh
    jp      nz,sound_rest_loop ; 09CFh
    pop     hl ; 09D2h
    ret ; 09D3h
    ASSERT $ = 009D4h
