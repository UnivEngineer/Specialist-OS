; Восстановлено из assets/games/Almaz.rks; без внешних бинарных включений.

game_main:
    jp      game_start ; 2000h

; Адаптер соглашения компилятора: X и Y переданы двумя PUSH AF, A=тип.
; Адрес возврата остаётся под аргументами; адаптер снимает аргументы.

tile_draw_stack:
    pop     bc ; 2003h
    ld      c,b ; 2004h
    pop     hl ; 2005h
    ld      b,h ; 2006h
    jp      tile_draw ; 2007h

explosion_draw_stack:
    pop     bc ; 200Ah
    ld      c,b ; 200Bh
    pop     hl ; 200Ch
    ld      b,h ; 200Dh
    jp      explosion_draw ; 200Eh

print_string_de:
    ex      de,hl ; 2011h
    jp      app_bios_print_string ; 2012h

game_start:
    ld      hl,t_level ; 2015h
    ld      (v_level_pointer),hl ; 2018h
    jp      speed_prompt ; 201Bh

; Записать клетку и нарисовать. Вход: стек X, Y (PUSH AF), A=тип.
; Выход: рабочая карта и экран; AF, BC, DE, HL, флаги разрушены.

map_put_stack:
    nop      ; 201Eh
    ld      (v_draw_tile),a ; 201Fh
    pop     bc ; 2022h
    pop     af ; 2023h
    ld      (v_draw_y),a ; 2024h
    pop     af ; 2027h
    ld      (v_draw_x),a ; 2028h
    push    bc ; 202Bh
    ld      hl,(v_draw_y) ; 202Ch
    ld      h,000h ; 202Fh
    ex      de,hl ; 2031h
    push    de ; 2032h
    ld      hl,(v_draw_x) ; 2033h
    ld      h,000h ; 2036h
    ex      de,hl ; 2038h
    ld      hl,v_map ; 2039h
    add     hl,de ; 203Ch
    pop     de ; 203Dh
    jp      map_put_draw ; 203Eh

; Неиспользуемый префикс цепочки сложений; сохранён из оригинала.

multiply_unused_prefix:
    add     hl,de ; 2041h
    add     hl,de ; 2042h
    add     hl,de ; 2043h
    add     hl,de ; 2044h
    add     hl,de ; 2045h
    add     hl,de ; 2046h
    add     hl,de ; 2047h
    add     hl,de ; 2048h
    add     hl,de ; 2049h
    add     hl,de ; 204Ah

; HL += 24*DE. Вход/выход: HL; DE сохранён; флаг CY разрушен.

multiply_24:
    add     hl,de ; 204Bh
    add     hl,de ; 204Ch
    add     hl,de ; 204Dh
    add     hl,de ; 204Eh
    add     hl,de ; 204Fh
    add     hl,de ; 2050h
    add     hl,de ; 2051h
    add     hl,de ; 2052h

; Внутренняя точка: HL += 16*DE, адресация исходной карты по столбцам.

multiply_16:
    add     hl,de ; 2053h
    add     hl,de ; 2054h
    add     hl,de ; 2055h
    add     hl,de ; 2056h
    add     hl,de ; 2057h
    add     hl,de ; 2058h
    add     hl,de ; 2059h
    add     hl,de ; 205Ah
    add     hl,de ; 205Bh
    add     hl,de ; 205Ch
    add     hl,de ; 205Dh
    add     hl,de ; 205Eh
    add     hl,de ; 205Fh
    add     hl,de ; 2060h
    add     hl,de ; 2061h
    add     hl,de ; 2062h
    ret      ; 2063h

map_put_draw:
    call    multiply_24 ; 2064h
    ld      a,(v_draw_tile) ; 2067h
    ld      (hl),a ; 206Ah
    ld      hl,map_put_return ; 206Bh
    push    hl ; 206Eh
    ld      a,(v_draw_x) ; 206Fh
    push    af ; 2072h
    ld      a,(v_draw_y) ; 2073h
    push    af ; 2076h
    ld      a,(v_draw_tile) ; 2077h
    jp      tile_draw_stack ; 207Ah

map_put_return:
    ret      ; 207Dh

; Пауза анимацией: (v_speed_delay+1) повторов, не миллисекунды.
; В каждом повторе перерисовываются игрок и три противника.
; Вход: состояние игры; выход: карта/экран. Все регистры и флаги разрушены.

animation_delay:
    xor     a ; 207Eh
    ld      hl,v_delay_repeat ; 207Fh
    ld      (hl),a ; 2082h

animation_delay_loop:
    push    hl ; 2083h
    ld      a,(v_speed_delay) ; 2084h
    pop     hl ; 2087h
    sub     (hl) ; 2088h
    jp      c,loc_20E3 ; 2089h
    push    hl ; 208Ch
    ld      a,(v_enemy_sprite) ; 208Dh
    xor     001h ; 2090h
    ld      (v_enemy_sprite),a ; 2092h
    ld      a,(v_player_sprite) ; 2095h
    xor     001h ; 2098h
    ld      (v_player_sprite),a ; 209Ah
    ld      a,(v_player_x) ; 209Dh
    push    af ; 20A0h
    ld      a,(v_player_y) ; 20A1h
    push    af ; 20A4h
    ld      a,(v_player_sprite) ; 20A5h
    call    map_put_stack ; 20A8h
    xor     a ; 20ABh
    ld      hl,v_delay_enemy ; 20ACh
    ld      (hl),a ; 20AFh

animation_enemy_loop:
    ld      a,002h ; 20B0h
    sub     (hl) ; 20B2h
    jp      c,loc_20DE ; 20B3h
    push    hl ; 20B6h
    ld      hl,(v_delay_enemy) ; 20B7h
    ld      h,000h ; 20BAh
    ex      de,hl ; 20BCh
    ld      hl,v_enemies ; 20BDh
    add     hl,de ; 20C0h
    add     hl,de ; 20C1h
    add     hl,de ; 20C2h
    ld      a,(hl) ; 20C3h
    push    af ; 20C4h
    ld      hl,(v_delay_enemy) ; 20C5h
    ld      h,000h ; 20C8h
    ex      de,hl ; 20CAh
    ld      hl,v_enemy_first_y ; 20CBh
    add     hl,de ; 20CEh
    add     hl,de ; 20CFh
    add     hl,de ; 20D0h
    ld      a,(hl) ; 20D1h
    push    af ; 20D2h
    ld      a,(v_enemy_sprite) ; 20D3h
    call    map_put_stack ; 20D6h
    pop     hl ; 20D9h
    inc     (hl) ; 20DAh
    jp      nz,animation_enemy_loop ; 20DBh

loc_20DE:
    pop     hl ; 20DEh
    inc     (hl) ; 20DFh
    jp      nz,animation_delay_loop ; 20E0h

loc_20E3:
    ret      ; 20E3h

; Стартовая задержка: 31 полный вызов animation_delay. Музыки здесь нет.

intro_animation:
    xor     a ; 20E4h
    ld      hl,v_intro_repeat ; 20E5h
    ld      (hl),a ; 20E8h

intro_animation_loop:
    ld      a,01Eh ; 20E9h
    sub     (hl) ; 20EBh
    jp      c,loc_20F8 ; 20ECh
    push    hl ; 20EFh
    call    animation_delay ; 20F0h
    pop     hl ; 20F3h
    inc     (hl) ; 20F4h
    jp      nz,intro_animation_loop ; 20F5h

loc_20F8:
    ret      ; 20F8h

speed_prompt:
    ld      de,t_speed_prompt ; 20F9h
    call    print_string_de ; 20FCh

; Первый запуск: исходное v_speed_delay=0, ожидание клавиши пропускается.
; После проигрыша переменная равна FFh: ждать любой код, отличный от FFh.

speed_wait:
    ld      a,(v_speed_delay) ; 20FFh
    inc     a ; 2102h
    jp      nz,speed_decode ; 2103h
    call    app_bios_key_scan ; 2106h
    ld      (v_speed_delay),a ; 2109h
    jp      speed_wait ; 210Ch

; Задержка = 5*(код клавиши AND 0Fh). Цифры не проверяются.
; 0 — самая быстрая игра, 9 — самая медленная; не номер сложности.

speed_decode:
    ld      a,(v_speed_delay) ; 210Fh
    and     00Fh ; 2112h
    ld      e,a ; 2114h
    ld      d,000h ; 2115h
    ld      hl,00005h ; 2117h
    jp      level_start ; 211Ah

; Умножение без знака. Вход: HL, DE; выход: DE=младшие 16 бит произведения.
; AF, BC сохранены; HL разрушён. Флаги восстановлены через POP AF.

multiply_u16:
    push    af ; 211Dh
    push    bc ; 211Eh
    ld      a,l ; 211Fh
    sub     e ; 2120h
    ld      a,h ; 2121h
    sbc     a,d ; 2122h
    jp      nc,multiply_ordered ; 2123h
    ex      de,hl ; 2126h

multiply_ordered:
    ld      b,d ; 2127h
    ld      c,e ; 2128h
    ld      de,00000h ; 2129h

multiply_loop:
    ld      a,b ; 212Ch
    or      c ; 212Dh
    jp      z,multiply_done ; 212Eh
    ex      de,hl ; 2131h
    ld      a,b ; 2132h
    rra      ; 2133h
    ld      b,a ; 2134h
    ld      a,c ; 2135h
    rra      ; 2136h
    ld      c,a ; 2137h
    jp      nc,multiply_shift ; 2138h
    add     hl,de ; 213Bh

multiply_shift:
    ex      de,hl ; 213Ch
    add     hl,hl ; 213Dh
    jp      multiply_loop ; 213Eh

multiply_done:
    pop     bc ; 2141h
    pop     af ; 2142h
    ret      ; 2143h

; Загрузить единственную карту 24*16: источник по столбцам, рабочая по строкам.

level_start:
    call    multiply_u16 ; 2144h
    ld      a,e ; 2147h
    ld      (v_speed_delay),a ; 2148h
    ld      a,009h ; 214Bh
    ld      (v_door_x),a ; 214Dh
    ld      a,005h ; 2150h
    ld      (v_door_y),a ; 2152h
    ld      hl,00000h ; 2155h
    ld      (v_unused_word),hl ; 2158h
    xor     a ; 215Bh
    ld      hl,v_loop_x_enemy ; 215Ch
    ld      (hl),a ; 215Fh

level_column:
    ld      a,017h ; 2160h
    sub     (hl) ; 2162h
    jp      c,actors_initialize ; 2163h
    push    hl ; 2166h
    xor     a ; 2167h
    ld      hl,v_loop_y_frame ; 2168h
    ld      (hl),a ; 216Bh

level_row:
    ld      a,00Fh ; 216Ch
    sub     (hl) ; 216Eh
    jp      c,level_next_column ; 216Fh
    push    hl ; 2172h
    ld      a,(v_loop_x_enemy) ; 2173h
    push    af ; 2176h
    ld      a,(v_loop_y_frame) ; 2177h
    push    af ; 217Ah
    ld      hl,(v_loop_x_enemy) ; 217Bh
    ld      h,000h ; 217Eh
    ex      de,hl ; 2180h
    ld      hl,(v_level_pointer) ; 2181h
    call    multiply_16 ; 2184h
    push    hl ; 2187h
    ld      hl,(v_loop_y_frame) ; 2188h
    ld      h,000h ; 218Bh
    ex      de,hl ; 218Dh
    pop     hl ; 218Eh
    add     hl,de ; 218Fh
    ld      a,(hl) ; 2190h
    call    map_put_stack ; 2191h
    pop     hl ; 2194h
    inc     (hl) ; 2195h
    jp      nz,level_row ; 2196h

level_next_column:
    pop     hl ; 2199h
    inc     (hl) ; 219Ah
    jp      nz,level_column ; 219Bh

actors_initialize:
    xor     a ; 219Eh
    ld      (v_diamonds),a ; 219Fh
    ld      a,00Fh ; 21A2h
    ld      (v_player_x),a ; 21A4h
    ld      a,006h ; 21A7h
    ld      (v_player_y),a ; 21A9h
    xor     a ; 21ACh
    ld      hl,v_loop_x_enemy ; 21ADh
    ld      (hl),a ; 21B0h

enemy_initialize_loop:
    ld      a,002h ; 21B1h
    sub     (hl) ; 21B3h
    jp      c,intro_start ; 21B4h
    push    hl ; 21B7h
    ld      hl,(v_loop_x_enemy) ; 21B8h
    ld      h,000h ; 21BBh
    ex      de,hl ; 21BDh
    ld      hl,v_enemy_first_y ; 21BEh
    add     hl,de ; 21C1h
    add     hl,de ; 21C2h
    add     hl,de ; 21C3h
    ld      a,00Eh ; 21C4h
    ld      (hl),a ; 21C6h
    ld      hl,(v_loop_x_enemy) ; 21C7h
    ld      h,000h ; 21CAh
    ex      de,hl ; 21CCh
    ld      hl,v_enemy_first_direction ; 21CDh
    add     hl,de ; 21D0h
    add     hl,de ; 21D1h
    add     hl,de ; 21D2h
    xor     a ; 21D3h
    ld      (hl),a ; 21D4h

; Вычисляемый переход через таблицу t_enemy_initializers, возврат через стек.
    ld      hl,enemy_initialize_next ; 21D5h
    push    hl ; 21D8h
    ld      hl,(v_loop_x_enemy) ; 21D9h
    ld      h,000h ; 21DCh
    ex      de,hl ; 21DEh
    ld      hl,t_enemy_initializers ; 21DFh
    add     hl,de ; 21E2h
    ret     c ; 21E3h
    add     hl,de ; 21E4h
    ret     c ; 21E5h
    pop     de ; 21E6h
    push    de ; 21E7h
    ld      a,l ; 21E8h
    sub     e ; 21E9h
    ld      a,h ; 21EAh
    sbc     a,d ; 21EBh
    ret     nc ; 21ECh
    ld      a,(hl) ; 21EDh
    inc     hl ; 21EEh
    ld      h,(hl) ; 21EFh
    ld      l,a ; 21F0h
    jp      (hl) ; 21F1h

enemy_start_left:
    ld      hl,(v_loop_x_enemy) ; 21F2h
    ld      h,000h ; 21F5h
    ex      de,hl ; 21F7h
    ld      hl,v_enemies ; 21F8h
    add     hl,de ; 21FBh
    add     hl,de ; 21FCh
    add     hl,de ; 21FDh
    ld      a,005h ; 21FEh
    ld      (hl),a ; 2200h
    ret      ; 2201h

enemy_start_middle:
    ld      hl,(v_loop_x_enemy) ; 2202h
    ld      h,000h ; 2205h
    ex      de,hl ; 2207h
    ld      hl,v_enemies ; 2208h
    add     hl,de ; 220Bh
    add     hl,de ; 220Ch
    add     hl,de ; 220Dh
    ld      a,00Ah ; 220Eh
    ld      (hl),a ; 2210h
    ret      ; 2211h

enemy_start_right:
    ld      hl,(v_loop_x_enemy) ; 2212h
    ld      h,000h ; 2215h
    ex      de,hl ; 2217h
    ld      hl,v_enemies ; 2218h
    add     hl,de ; 221Bh
    add     hl,de ; 221Ch
    add     hl,de ; 221Dh
    ld      a,00Fh ; 221Eh
    ld      (hl),a ; 2220h
    ret      ; 2221h

; Три адреса инициализации противников; это данные, а не инструкции.

t_enemy_initializers:
    dw      enemy_start_left,enemy_start_middle,enemy_start_right ; 2222h

enemy_initialize_next:
    pop     hl ; 2228h
    inc     (hl) ; 2229h
    jp      nz,enemy_initialize_loop ; 222Ah

intro_start:
    xor     a ; 222Dh
    ld      (v_player_sprite),a ; 222Eh
    ld      a,00Ah ; 2231h
    ld      (v_enemy_sprite),a ; 2233h
    call    intro_animation ; 2236h

; Один игровой шаг: ввод игрока, три противника, камни, animation_delay.
; Опрос клавиш однократный; во время паузы стрелки не накапливаются.

game_step:
    ld      a,005h ; 2239h
    ld      (v_player_direction),a ; 223Bh
    ld      a,(v_player_y) ; 223Eh
    dec     a ; 2241h
    ld      e,a ; 2242h
    ld      d,000h ; 2243h
    push    de ; 2245h
    ld      hl,(v_player_x) ; 2246h
    ld      h,000h ; 2249h
    ex      de,hl ; 224Bh
    ld      hl,v_map ; 224Ch
    add     hl,de ; 224Fh
    pop     de ; 2250h
    call    multiply_24 ; 2251h
    ld      a,(hl) ; 2254h
    ld      (v_player_target),a ; 2255h
    ld      a,(v_player_target) ; 2258h
    xor     009h ; 225Bh
    jp      nz,player_input ; 225Dh
    ld      a,(v_player_x) ; 2260h
    ld      (v_stone_x),a ; 2263h
    ld      a,(v_player_y) ; 2266h
    dec     a ; 2269h
    ld      (v_stone_y),a ; 226Ah
    ld      a,0FFh ; 226Dh
    ld      (v_stone_active),a ; 226Fh

; Коды BIOS: вниз 1Ah, вверх 19h, вправо 18h, влево 08h.
; Остальные клавиши и отсутствие клавиши оставляют направление 5 (стоять).

player_input:
    call    app_bios_key_scan ; 2272h
    ld      (v_key),a ; 2275h
    ld      a,(v_key) ; 2278h
    xor     01Ah ; 227Bh
    jp      nz,player_check_up ; 227Dh
    xor     a ; 2280h
    ld      (v_player_direction),a ; 2281h

player_check_up:
    ld      a,(v_key) ; 2284h
    xor     019h ; 2287h
    jp      nz,player_check_right ; 2289h
    ld      a,002h ; 228Ch
    ld      (v_player_direction),a ; 228Eh

player_check_right:
    ld      a,(v_key) ; 2291h
    xor     018h ; 2294h
    jp      nz,player_check_left ; 2296h
    ld      a,001h ; 2299h
    ld      (v_player_direction),a ; 229Bh
    ld      a,(v_player_sprite) ; 229Eh
    and     0FDh ; 22A1h
    ld      (v_player_sprite),a ; 22A3h

player_check_left:
    ld      a,(v_key) ; 22A6h
    xor     008h ; 22A9h
    jp      nz,player_target ; 22ABh
    ld      a,003h ; 22AEh
    ld      (v_player_direction),a ; 22B0h
    ld      a,(v_player_sprite) ; 22B3h
    or      002h ; 22B6h
    ld      (v_player_sprite),a ; 22B8h

player_target:
    ld      a,(v_player_direction) ; 22BBh
    xor     005h ; 22BEh
    jp      z,enemies_update ; 22C0h
    ld      de,t_direction_dy ; 22C3h
    ld      hl,(v_player_direction) ; 22C6h
    ld      h,000h ; 22C9h
    add     hl,de ; 22CBh
    ld      a,(v_player_y) ; 22CCh
    add     a,(hl) ; 22CFh
    ld      e,a ; 22D0h
    ld      d,000h ; 22D1h
    push    de ; 22D3h
    ld      de,t_direction_dx ; 22D4h
    ld      hl,(v_player_direction) ; 22D7h
    ld      h,000h ; 22DAh
    add     hl,de ; 22DCh
    ld      a,(v_player_x) ; 22DDh
    add     a,(hl) ; 22E0h
    ld      e,a ; 22E1h
    ld      d,000h ; 22E2h
    ld      hl,v_map ; 22E4h
    add     hl,de ; 22E7h
    pop     de ; 22E8h
    call    multiply_24 ; 22E9h
    ld      a,(hl) ; 22ECh
    ld      (v_player_target),a ; 22EDh
    ld      a,(v_player_target) ; 22F0h
    xor     009h ; 22F3h
    cp      001h ; 22F5h
    sbc     a,a ; 22F7h
    ld      b,a ; 22F8h
    ld      a,(v_player_target) ; 22F9h
    xor     00Fh ; 22FCh
    cp      001h ; 22FEh
    sbc     a,a ; 2300h
    or      b ; 2301h
    rrca     ; 2302h
    jp      c,enemies_update ; 2303h

player_check_diamond:
    ld      a,(v_player_target) ; 2306h
    xor     008h ; 2309h
    jp      nz,player_check_door ; 230Bh
    ld      a,0FFh ; 230Eh
    ld      (v_door_open),a ; 2310h
    ld      a,(v_player_sprite) ; 2313h
    xor     004h ; 2316h
    ld      (v_player_sprite),a ; 2318h
    ld      a,(v_door_x) ; 231Bh
    push    af ; 231Eh
    ld      a,(v_door_y) ; 231Fh
    push    af ; 2322h
    ld      a,00Eh ; 2323h
    call    map_put_stack ; 2325h
    ld      a,(v_diamonds) ; 2328h
    inc     a ; 232Bh
    ld      (v_diamonds),a ; 232Ch

player_check_door:
    ld      a,(v_player_target) ; 232Fh
    xor     010h ; 2332h
    jp      nz,player_move ; 2334h
    ld      a,(v_door_open) ; 2337h
    inc     a ; 233Ah
    jp      nz,player_door_done ; 233Bh
    xor     a ; 233Eh
    ld      (v_door_open),a ; 233Fh
    ld      a,(v_door_x) ; 2342h
    push    af ; 2345h
    ld      a,(v_door_y) ; 2346h
    push    af ; 2349h
    ld      a,008h ; 234Ah
    call    map_put_stack ; 234Ch
    ld      a,(v_player_sprite) ; 234Fh
    xor     004h ; 2352h
    ld      (v_player_sprite),a ; 2354h

player_door_done:
    jp      enemies_update ; 2357h

player_move:
    ld      a,(v_player_x) ; 235Ah
    push    af ; 235Dh
    ld      a,(v_player_y) ; 235Eh
    push    af ; 2361h
    ld      a,00Eh ; 2362h
    call    map_put_stack ; 2364h
    ld      de,t_direction_dy ; 2367h
    ld      hl,(v_player_direction) ; 236Ah
    ld      h,000h ; 236Dh
    add     hl,de ; 236Fh
    ld      a,(v_player_y) ; 2370h
    add     a,(hl) ; 2373h
    ld      (v_player_y),a ; 2374h
    ld      de,t_direction_dx ; 2377h
    ld      hl,(v_player_direction) ; 237Ah
    ld      h,000h ; 237Dh
    add     hl,de ; 237Fh
    ld      a,(v_player_x) ; 2380h
    add     a,(hl) ; 2383h
    ld      (v_player_x),a ; 2384h
    ld      a,(v_player_x) ; 2387h
    push    af ; 238Ah
    ld      a,(v_player_y) ; 238Bh
    push    af ; 238Eh
    ld      a,(v_player_sprite) ; 238Fh
    call    map_put_stack ; 2392h

enemies_update:
    xor     a ; 2395h
    ld      hl,v_loop_x_enemy ; 2396h
    ld      (hl),a ; 2399h

enemy_loop:
    ld      a,002h ; 239Ah
    sub     (hl) ; 239Ch
    jp      c,stone_update ; 239Dh
    push    hl ; 23A0h
    xor     a ; 23A1h
    ld      hl,v_direction_try ; 23A2h
    ld      (hl),a ; 23A5h

enemy_try_direction:
    ld      a,003h ; 23A6h
    sub     (hl) ; 23A8h
    jp      c,enemy_next ; 23A9h
    push    hl ; 23ACh
    ld      hl,(v_loop_x_enemy) ; 23ADh
    ld      h,000h ; 23B0h
    ex      de,hl ; 23B2h
    ld      hl,v_enemy_first_direction ; 23B3h
    add     hl,de ; 23B6h
    add     hl,de ; 23B7h
    add     hl,de ; 23B8h
    ld      a,(v_direction_try) ; 23B9h
    add     a,(hl) ; 23BCh
    dec     a ; 23BDh
    ld      e,a ; 23BEh
    ld      d,000h ; 23BFh
    ld      hl,00004h ; 23C1h
    jp      enemy_direction_modulo ; 23C4h

; Деление без знака. Вход: DE=делимое, HL=делитель.
; Выход: DE=частное, HL=остаток; AF, BC и флаги сохранены.

divide_u16:
    push    af ; 23C7h
    push    bc ; 23C8h
    ld      b,d ; 23C9h
    ld      c,e ; 23CAh
    ld      a,h ; 23CBh
    cpl      ; 23CCh
    ld      d,a ; 23CDh
    ld      a,l ; 23CEh
    cpl      ; 23CFh
    ld      e,a ; 23D0h
    inc     de ; 23D1h
    ld      hl,00000h ; 23D2h
    ld      a,011h ; 23D5h

divide_loop:
    push    hl ; 23D7h
    add     hl,de ; 23D8h
    jp      nc,divide_rotate ; 23D9h
    ex      (sp),hl ; 23DCh

divide_rotate:
    pop     hl ; 23DDh
    push    af ; 23DEh
    ld      a,c ; 23DFh
    rla      ; 23E0h
    ld      c,a ; 23E1h
    ld      a,b ; 23E2h
    rla      ; 23E3h
    ld      b,a ; 23E4h
    ld      a,l ; 23E5h
    rla      ; 23E6h
    ld      l,a ; 23E7h
    ld      a,h ; 23E8h
    rla      ; 23E9h
    ld      h,a ; 23EAh
    pop     af ; 23EBh
    dec     a ; 23ECh
    jp      nz,divide_loop ; 23EDh
    or      a ; 23F0h
    ld      a,h ; 23F1h
    rra      ; 23F2h
    ld      h,a ; 23F3h
    ld      a,l ; 23F4h
    rra      ; 23F5h
    ld      l,a ; 23F6h
    ld      d,b ; 23F7h
    ld      e,c ; 23F8h
    pop     bc ; 23F9h
    pop     af ; 23FAh
    ret      ; 23FBh

enemy_direction_modulo:
    call    divide_u16 ; 23FCh
    ld      a,l ; 23FFh
    ld      (v_enemy_direction),a ; 2400h
    ld      hl,(v_loop_x_enemy) ; 2403h
    ld      h,000h ; 2406h
    ex      de,hl ; 2408h
    ld      hl,v_enemy_first_y ; 2409h
    add     hl,de ; 240Ch
    add     hl,de ; 240Dh
    add     hl,de ; 240Eh
    ld      b,(hl) ; 240Fh
    ld      de,t_direction_dy ; 2410h
    ld      hl,(v_enemy_direction) ; 2413h
    ld      h,000h ; 2416h
    add     hl,de ; 2418h
    ld      a,(hl) ; 2419h
    add     a,b ; 241Ah
    ld      e,a ; 241Bh
    ld      d,000h ; 241Ch
    push    de ; 241Eh
    ld      hl,(v_loop_x_enemy) ; 241Fh
    ld      h,000h ; 2422h
    ex      de,hl ; 2424h
    ld      hl,v_enemies ; 2425h
    add     hl,de ; 2428h
    add     hl,de ; 2429h
    add     hl,de ; 242Ah
    ld      b,(hl) ; 242Bh
    ld      de,t_direction_dx ; 242Ch
    ld      hl,(v_enemy_direction) ; 242Fh
    ld      h,000h ; 2432h
    add     hl,de ; 2434h
    ld      a,(hl) ; 2435h
    add     a,b ; 2436h
    ld      e,a ; 2437h
    ld      d,000h ; 2438h
    ld      hl,v_map ; 243Ah
    add     hl,de ; 243Dh
    pop     de ; 243Eh
    call    multiply_24 ; 243Fh
    ld      a,(hl) ; 2442h
    ld      (v_enemy_target),a ; 2443h
    ld      a,(v_enemy_target) ; 2446h
    sub     008h ; 2449h
    jp      nc,enemy_check_empty ; 244Bh
    pop     hl ; 244Eh
    pop     hl ; 244Fh
    jp      death_by_enemy ; 2450h

enemy_check_empty:
    ld      a,(v_enemy_target) ; 2453h
    xor     00Eh ; 2456h
    jp      nz,enemy_next_direction ; 2458h
    ld      a,004h ; 245Bh
    ld      (v_direction_try),a ; 245Dh
    ld      hl,(v_loop_x_enemy) ; 2460h
    ld      h,000h ; 2463h
    ex      de,hl ; 2465h
    ld      hl,v_enemy_first_direction ; 2466h
    add     hl,de ; 2469h
    add     hl,de ; 246Ah
    add     hl,de ; 246Bh
    ld      a,(v_enemy_direction) ; 246Ch
    ld      (hl),a ; 246Fh
    ld      hl,(v_loop_x_enemy) ; 2470h
    ld      h,000h ; 2473h
    ex      de,hl ; 2475h
    ld      hl,v_enemies ; 2476h
    add     hl,de ; 2479h
    add     hl,de ; 247Ah
    add     hl,de ; 247Bh
    ld      a,(hl) ; 247Ch
    push    af ; 247Dh
    ld      hl,(v_loop_x_enemy) ; 247Eh
    ld      h,000h ; 2481h
    ex      de,hl ; 2483h
    ld      hl,v_enemy_first_y ; 2484h
    add     hl,de ; 2487h
    add     hl,de ; 2488h
    add     hl,de ; 2489h
    ld      a,(hl) ; 248Ah
    push    af ; 248Bh
    ld      a,00Eh ; 248Ch
    call    map_put_stack ; 248Eh
    ld      hl,(v_loop_x_enemy) ; 2491h
    ld      h,000h ; 2494h
    ex      de,hl ; 2496h
    ld      hl,v_enemy_first_y ; 2497h
    add     hl,de ; 249Ah
    add     hl,de ; 249Bh
    add     hl,de ; 249Ch
    push    hl ; 249Dh
    ld      hl,(v_loop_x_enemy) ; 249Eh
    ld      h,000h ; 24A1h
    ex      de,hl ; 24A3h
    ld      hl,v_enemy_first_y ; 24A4h
    add     hl,de ; 24A7h
    add     hl,de ; 24A8h
    add     hl,de ; 24A9h
    ld      b,(hl) ; 24AAh
    ld      de,t_direction_dy ; 24ABh
    ld      hl,(v_enemy_direction) ; 24AEh
    ld      h,000h ; 24B1h
    add     hl,de ; 24B3h
    ld      a,(hl) ; 24B4h
    add     a,b ; 24B5h
    pop     hl ; 24B6h
    ld      (hl),a ; 24B7h
    ld      hl,(v_loop_x_enemy) ; 24B8h
    ld      h,000h ; 24BBh
    ex      de,hl ; 24BDh
    ld      hl,v_enemies ; 24BEh
    add     hl,de ; 24C1h
    add     hl,de ; 24C2h
    add     hl,de ; 24C3h
    push    hl ; 24C4h
    ld      hl,(v_loop_x_enemy) ; 24C5h
    ld      h,000h ; 24C8h
    ex      de,hl ; 24CAh
    ld      hl,v_enemies ; 24CBh
    add     hl,de ; 24CEh
    add     hl,de ; 24CFh
    add     hl,de ; 24D0h
    ld      b,(hl) ; 24D1h
    ld      de,t_direction_dx ; 24D2h
    ld      hl,(v_enemy_direction) ; 24D5h
    ld      h,000h ; 24D8h
    add     hl,de ; 24DAh
    ld      a,(hl) ; 24DBh
    add     a,b ; 24DCh
    pop     hl ; 24DDh
    ld      (hl),a ; 24DEh
    ld      hl,(v_loop_x_enemy) ; 24DFh
    ld      h,000h ; 24E2h
    ex      de,hl ; 24E4h
    ld      hl,v_enemies ; 24E5h
    add     hl,de ; 24E8h
    add     hl,de ; 24E9h
    add     hl,de ; 24EAh
    ld      a,(hl) ; 24EBh
    push    af ; 24ECh
    ld      hl,(v_loop_x_enemy) ; 24EDh
    ld      h,000h ; 24F0h
    ex      de,hl ; 24F2h
    ld      hl,v_enemy_first_y ; 24F3h
    add     hl,de ; 24F6h
    add     hl,de ; 24F7h
    add     hl,de ; 24F8h
    ld      a,(hl) ; 24F9h
    push    af ; 24FAh
    ld      a,(v_enemy_sprite) ; 24FBh
    call    map_put_stack ; 24FEh

enemy_next_direction:
    pop     hl ; 2501h
    inc     (hl) ; 2502h
    jp      nz,enemy_try_direction ; 2503h

enemy_next:
    pop     hl ; 2506h
    inc     (hl) ; 2507h
    jp      nz,enemy_loop ; 2508h

stone_update:
    ld      a,(v_stone_active) ; 250Bh
    inc     a ; 250Eh
    jp      nz,game_step_delay ; 250Fh
    ld      a,(v_stone_y) ; 2512h
    dec     a ; 2515h
    ld      e,a ; 2516h
    ld      d,000h ; 2517h
    push    de ; 2519h
    ld      hl,(v_stone_x) ; 251Ah
    ld      h,000h ; 251Dh
    ex      de,hl ; 251Fh
    ld      hl,v_map ; 2520h
    add     hl,de ; 2523h
    pop     de ; 2524h
    call    multiply_24 ; 2525h
    ld      a,(hl) ; 2528h
    ld      (v_stone_below),a ; 2529h
    ld      a,(v_stone_below) ; 252Ch
    xor     009h ; 252Fh
    jp      z,stone_remember_top ; 2531h
    xor     a ; 2534h
    ld      (v_stone_active),a ; 2535h
    jp      stone_check_below ; 2538h

stone_remember_top:
    ld      a,(v_stone_y) ; 253Bh
    dec     a ; 253Eh
    ld      (v_stone_top),a ; 253Fh

stone_check_below:
    ld      a,(v_stone_y) ; 2542h
    inc     a ; 2545h
    ld      e,a ; 2546h
    ld      d,000h ; 2547h
    push    de ; 2549h
    ld      hl,(v_stone_x) ; 254Ah
    ld      h,000h ; 254Dh
    ex      de,hl ; 254Fh
    ld      hl,v_map ; 2550h
    add     hl,de ; 2553h
    pop     de ; 2554h
    call    multiply_24 ; 2555h
    ld      a,(hl) ; 2558h
    ld      (v_stone_below),a ; 2559h

stone_fall_test:
    ld      a,(v_stone_below) ; 255Ch
    xor     00Eh ; 255Fh
    cp      001h ; 2561h
    sbc     a,a ; 2563h
    ld      b,a ; 2564h
    ld      a,(v_stone_below) ; 2565h
    sub     008h ; 2568h
    sbc     a,a ; 256Ah
    or      b ; 256Bh
    rrca     ; 256Ch
    jp      nc,stone_repeat ; 256Dh
    ld      a,(v_stone_x) ; 2570h
    push    af ; 2573h
    ld      a,(v_stone_y) ; 2574h
    push    af ; 2577h
    ld      a,00Eh ; 2578h
    call    map_put_stack ; 257Ah
    ld      a,(v_stone_y) ; 257Dh
    inc     a ; 2580h
    ld      (v_stone_y),a ; 2581h
    ld      a,(v_stone_x) ; 2584h
    push    af ; 2587h
    ld      a,(v_stone_y) ; 2588h
    push    af ; 258Bh
    ld      a,009h ; 258Ch
    call    map_put_stack ; 258Eh
    ld      a,(v_stone_below) ; 2591h
    sub     008h ; 2594h
    jp      c,death_by_stone ; 2596h
    ld      a,(v_stone_y) ; 2599h
    inc     a ; 259Ch
    ld      e,a ; 259Dh
    ld      d,000h ; 259Eh
    push    de ; 25A0h
    ld      hl,(v_stone_x) ; 25A1h
    ld      h,000h ; 25A4h
    ex      de,hl ; 25A6h
    ld      hl,v_map ; 25A7h
    add     hl,de ; 25AAh
    pop     de ; 25ABh
    call    multiply_24 ; 25ACh
    ld      a,(hl) ; 25AFh
    ld      (v_stone_below),a ; 25B0h
    jp      stone_fall_test ; 25B3h

stone_repeat:
    ld      a,(v_stone_top) ; 25B6h
    ld      (v_stone_y),a ; 25B9h
    jp      stone_update ; 25BCh

game_step_delay:
    call    animation_delay ; 25BFh
    jp      game_step ; 25C2h

; Гибель от противника: 201 повтор по две фазы взрыва.

death_by_enemy:
    xor     a ; 25C5h
    ld      hl,v_death_repeat ; 25C6h
    ld      (hl),a ; 25C9h

death_enemy_outer:
    ld      a,0C8h ; 25CAh
    sub     (hl) ; 25CCh
    jp      c,death_enemy_end ; 25CDh
    push    hl ; 25D0h
    xor     a ; 25D1h
    ld      hl,v_loop_y_frame ; 25D2h
    ld      (hl),a ; 25D5h

loc_25D6:
    ld      a,001h ; 25D6h
    sub     (hl) ; 25D8h
    jp      c,death_enemy_next_repeat ; 25D9h
    push    hl ; 25DCh
    ld      hl,death_enemy_next_frame ; 25DDh
    push    hl ; 25E0h
    ld      hl,(v_loop_x_enemy) ; 25E1h
    ld      h,000h ; 25E4h
    ex      de,hl ; 25E6h
    ld      hl,v_enemies ; 25E7h
    add     hl,de ; 25EAh
    add     hl,de ; 25EBh
    add     hl,de ; 25ECh
    ld      a,(hl) ; 25EDh
    push    af ; 25EEh
    ld      hl,(v_loop_x_enemy) ; 25EFh
    ld      h,000h ; 25F2h
    ex      de,hl ; 25F4h
    ld      hl,v_enemy_first_y ; 25F5h
    add     hl,de ; 25F8h
    add     hl,de ; 25F9h
    add     hl,de ; 25FAh
    ld      a,(hl) ; 25FBh
    push    af ; 25FCh
    ld      a,(v_loop_y_frame) ; 25FDh
    jp      explosion_draw_stack ; 2600h

death_enemy_next_frame:
    pop     hl ; 2603h
    inc     (hl) ; 2604h
    jp      nz,loc_25D6 ; 2605h

death_enemy_next_repeat:
    pop     hl ; 2608h
    inc     (hl) ; 2609h
    jp      nz,death_enemy_outer ; 260Ah

death_enemy_end:
    jp      game_over ; 260Dh

; Гибель от камня: 201 повтор по две фазы для трёх противников.

death_by_stone:
    xor     a ; 2610h
    ld      hl,v_death_repeat ; 2611h
    ld      (hl),a ; 2614h

death_stone_outer:
    ld      a,0C8h ; 2615h
    sub     (hl) ; 2617h
    jp      c,game_over ; 2618h
    push    hl ; 261Bh
    xor     a ; 261Ch
    ld      hl,v_loop_y_frame ; 261Dh
    ld      (hl),a ; 2620h

death_stone_frame:
    ld      a,001h ; 2621h
    sub     (hl) ; 2623h
    jp      c,death_stone_next_repeat ; 2624h
    push    hl ; 2627h
    xor     a ; 2628h
    ld      hl,v_loop_x_enemy ; 2629h
    ld      (hl),a ; 262Ch

death_stone_enemy:
    ld      a,002h ; 262Dh
    sub     (hl) ; 262Fh
    jp      c,death_stone_next_frame ; 2630h
    push    hl ; 2633h
    ld      hl,death_stone_next_enemy ; 2634h
    push    hl ; 2637h
    ld      hl,(v_loop_x_enemy) ; 2638h
    ld      h,000h ; 263Bh
    ex      de,hl ; 263Dh
    ld      hl,v_enemies ; 263Eh
    add     hl,de ; 2641h
    add     hl,de ; 2642h
    add     hl,de ; 2643h
    ld      a,(hl) ; 2644h
    push    af ; 2645h
    ld      hl,(v_loop_x_enemy) ; 2646h
    ld      h,000h ; 2649h
    ex      de,hl ; 264Bh
    ld      hl,v_enemy_first_y ; 264Ch
    add     hl,de ; 264Fh
    add     hl,de ; 2650h
    add     hl,de ; 2651h
    ld      a,(hl) ; 2652h
    push    af ; 2653h
    ld      a,(v_loop_y_frame) ; 2654h
    add     a,00Ah ; 2657h
    jp      tile_draw_stack ; 2659h

death_stone_next_enemy:
    pop     hl ; 265Ch
    inc     (hl) ; 265Dh
    jp      nz,death_stone_enemy ; 265Eh

death_stone_next_frame:
    pop     hl ; 2661h
    inc     (hl) ; 2662h
    jp      nz,death_stone_frame ; 2663h

death_stone_next_repeat:
    pop     hl ; 2666h
    inc     (hl) ; 2667h
    jp      nz,death_stone_outer ; 2668h

game_over:
    ld      de,t_game_over ; 266Bh
    call    print_string_de ; 266Eh
    ld      a,(v_diamonds) ; 2671h
    call    app_bios_print_hex ; 2674h
    ld      de,t_diamonds_suffix ; 2677h
    call    print_string_de ; 267Ah
    xor     a ; 267Dh
    ld      hl,v_loop_x_enemy ; 267Eh
    ld      (hl),a ; 2681h

; Задержка конца игры: 101*(7+7+10+11+7+7+250*(5+12*15+5+10)+10+10+10)
; и заключительная проверка 24 такта: 5058003 такта, 2,529 с при 2 МГц.

game_over_wait:
    ld      a,064h ; 2682h
    sub     (hl) ; 2684h
    jp      c,game_restart ; 2685h
    push    hl ; 2688h
    ld      a,0FAh ; 2689h
    ld      d,00Ch ; 268Bh

wait_period:
    ld      e,d ; 268Dh

wait_inner:
    dec     e ; 268Eh
    jp      nz,wait_inner ; 268Fh
    dec     a ; 2692h
    jp      nz,wait_period ; 2693h
    pop     hl ; 2696h
    inc     (hl) ; 2697h
    jp      nz,game_over_wait ; 2698h

game_restart:
    ld      a,0FFh ; 269Bh
    ld      (v_speed_delay),a ; 269Dh
    jp      speed_prompt ; 26A0h

; Недостижимый хвост компилятора: внешние адреса 0040h/0020h.
; Не является игровым выходом и не вызывается из достижимого кода.

legacy_unreachable:
    call    00040h ; 26A3h
    jp      00020h ; 26A6h
    db      000h,000h,000h,000h,000h ; 26A9h
