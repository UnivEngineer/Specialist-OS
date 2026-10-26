; Восстановлено из assets/games/Almaz.rks; без внешних бинарных включений.

; Указатель исходной карты; соседние нулевые байты сохранены.

v_level_pointer:
    db      0B0h,026h ; 26AEh

; Исходная карта: 24 столбца по 16 клеток. Копия меняется при игре.

t_level:
    db      00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh ; 26B0h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,009h,00Dh,00Fh ; 26C0h
    db      00Fh,00Dh,00Dh,009h,00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,00Dh,009h,00Dh,00Dh,00Fh ; 26D0h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh ; 26E0h
    db      00Fh,00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh ; 26F0h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh,00Eh,00Eh,00Eh,00Ah,00Fh ; 2700h
    db      00Fh,009h,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh,00Eh,00Eh,00Eh,00Eh,00Fh ; 2710h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh,00Eh,00Eh,00Eh,00Eh,00Fh ; 2720h
    db      00Fh,00Dh,00Dh,00Dh,009h,00Dh,00Dh,00Dh,009h,00Dh,00Fh,00Eh,00Eh,00Eh,00Eh,00Fh ; 2730h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,008h,009h,00Dh,009h,00Dh,00Fh,00Dh,00Dh,00Dh,00Dh,00Fh ; 2740h
    db      00Fh,009h,00Dh,00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,00Fh,00Eh,00Eh,00Eh,00Ah,00Fh ; 2750h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh,00Eh,00Eh,00Eh,00Eh,00Fh ; 2760h
    db      00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Eh,00Eh,00Eh,00Eh,00Fh ; 2770h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh,00Eh,00Eh,00Eh,00Eh,00Fh ; 2780h
    db      00Fh,009h,00Dh,00Dh,009h,00Dh,010h,00Dh,009h,00Dh,00Fh,00Dh,00Dh,00Dh,00Dh,00Fh ; 2790h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,001h,009h,00Dh,00Dh,00Fh,00Eh,00Eh,00Eh,00Ah,00Fh ; 27A0h
    db      00Fh,00Dh,009h,00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,00Fh,00Eh,00Eh,00Eh,00Eh,00Fh ; 27B0h
    db      00Fh,00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,009h,00Dh,00Fh,00Eh,00Eh,00Eh,00Eh,00Fh ; 27C0h
    db      00Fh,009h,00Dh,00Dh,009h,00Dh,009h,00Dh,00Dh,00Dh,00Fh,00Eh,00Eh,00Eh,00Eh,00Fh ; 27D0h
    db      00Fh,009h,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,00Fh ; 27E0h
    db      00Fh,00Dh,00Dh,009h,00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh ; 27F0h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,009h,00Dh,00Dh,009h,00Dh,00Dh,00Fh ; 2800h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh ; 2810h
    db      00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh ; 2820h

; Рабочая карта 24*16, строки подряд; исходное содержимое сохранено.

v_map:
    db      00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh ; 2830h
    db      00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,009h,00Dh ; 2840h
    db      00Dh,00Dh,009h,00Dh,00Fh,00Dh,009h,00Dh,00Dh,00Dh,009h,009h,00Dh,00Dh,00Dh,00Fh ; 2850h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh,00Dh,00Dh,00Dh ; 2860h
    db      009h,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh,00Fh,00Dh,009h,00Dh,009h,00Dh,00Dh,00Dh ; 2870h
    db      00Dh,00Dh,00Dh,00Dh,00Fh,00Dh,00Dh,00Dh,00Dh,009h,00Dh,00Dh,009h,00Dh,00Dh,00Fh ; 2880h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Fh,00Dh,009h,00Dh ; 2890h
    db      00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,00Fh,00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh ; 28A0h
    db      00Dh,008h,009h,00Dh,00Fh,00Dh,00Dh,00Dh,00Eh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh ; 28B0h
    db      00Fh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,009h,00Dh,00Dh,00Fh,00Dh,010h,00Eh ; 28C0h
    db      009h,00Eh,00Eh,00Dh,00Eh,00Dh,00Dh,00Fh,00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh ; 28D0h
    db      00Dh,00Dh,00Dh,00Dh,00Fh,00Dh,00Dh,009h,00Dh,00Eh,009h,00Eh,009h,00Eh,000h,00Fh ; 28E0h
    db      00Fh,00Dh,00Dh,009h,00Dh,00Dh,00Dh,00Dh,009h,009h,00Dh,00Dh,00Fh,00Dh,009h,00Dh ; 28F0h
    db      00Dh,009h,00Dh,00Dh,00Dh,00Dh,00Dh,00Fh,00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh ; 2900h
    db      00Dh,00Dh,00Dh,00Dh,00Fh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,00Dh,009h,00Dh,00Fh ; 2910h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh ; 2920h
    db      00Fh,00Fh,00Fh,009h,00Dh,00Dh,00Dh,00Fh,00Fh,00Dh,00Dh,00Dh,00Dh,00Eh,00Eh,00Eh ; 2930h
    db      00Eh,00Dh,00Eh,00Eh,00Eh,00Eh,00Dh,00Eh,00Eh,00Eh,00Eh,00Dh,00Dh,00Dh,00Dh,00Fh ; 2940h
    db      00Fh,00Dh,009h,00Dh,00Dh,00Eh,00Eh,00Eh,00Ah,00Dh,00Eh,00Eh,00Eh,00Ah,00Dh,00Eh ; 2950h
    db      00Eh,00Eh,00Ah,00Dh,00Dh,009h,00Dh,00Fh,00Fh,009h,00Dh,00Dh,00Dh,00Eh,00Eh,00Eh ; 2960h
    db      00Eh,00Dh,00Eh,00Eh,00Eh,00Eh,00Dh,00Eh,00Eh,00Eh,00Eh,00Dh,00Dh,00Dh,00Dh,00Fh ; 2970h
    db      00Fh,00Dh,00Dh,00Dh,00Dh,00Eh,00Eh,00Eh,00Eh,00Dh,00Eh,00Eh,00Eh,00Eh,00Dh,00Eh ; 2980h
    db      00Eh,00Eh,00Eh,00Dh,00Dh,00Dh,00Dh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh ; 2990h
    db      00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh,00Fh ; 29A0h

; Три записи по три байта: X, Y, направление (вниз, вправо, вверх, влево).

v_enemies:
    db      008h ; 29B0h

v_enemy_first_y:
    db      00Ch ; 29B1h

v_enemy_first_direction:
    db      002h,00Dh,00Ch,002h,012h,00Ch,002h ; 29B2h

v_player_x:
    db      016h ; 29B9h

v_player_y:
    db      007h ; 29BAh

v_player_sprite:
    db      000h ; 29BBh

v_stone_x:
    db      014h ; 29BCh

v_stone_y:
    db      000h ; 29BDh

; Смещения X: 0,+1,0,-1; смещения Y: +1,0,-1,0.

t_direction_dx:
    db      000h,001h,000h,0FFh ; 29BEh

t_direction_dy:
    db      001h,000h,0FFh,000h ; 29C2h

; Строка KOI-7: «СКОРОСТЬ(0/9):», перед ней очистка экрана 1Fh.

t_speed_prompt:
    db      01Fh,073h,06Bh,06Fh,072h,06Fh,073h,074h,078h,028h,030h,02Fh,039h,029h,03Ah,000h ; 29C6h

v_door_x:
    db      009h ; 29D6h

v_door_y:
    db      005h ; 29D7h

v_stone_active:
    db      000h ; 29D8h

v_unused_word:
    db      000h,000h ; 29D9h

v_loop_x_enemy:
    db      000h ; 29DBh

v_loop_y_frame:
    db      010h,000h ; 29DCh

v_diamonds:
    db      000h ; 29DEh

; Исходное значение 00h объясняет пропуск выбора скорости при первой загрузке.

v_speed_delay:
    db      000h ; 29DFh

v_enemy_sprite:
    db      00Ah ; 29E0h

v_door_open:
    db      000h ; 29E1h

v_draw_x:
    db      012h ; 29E2h

v_draw_y:
    db      00Ch ; 29E3h

v_draw_tile:
    db      00Ah ; 29E4h

v_delay_repeat:
    db      001h ; 29E5h

v_delay_enemy:
    db      003h ; 29E6h

v_intro_repeat:
    db      01Fh ; 29E7h

v_player_direction:
    db      005h ; 29E8h

v_key:
    db      0FFh ; 29E9h

v_player_target:
    db      00Dh ; 29EAh

v_enemy_target:
    db      00Dh ; 29EBh

v_direction_try:
    db      001h ; 29ECh

v_enemy_direction:
    db      001h ; 29EDh

v_stone_below:
    db      00Dh ; 29EEh

v_stone_top:
    db      000h ; 29EFh

v_death_repeat:
    db      0C9h ; 29F0h

; Строка KOI-7 «ИГРА ОКОНЧЕНА! ВЫ УКРАЛИ»; затем число алмазов в HEX.

t_game_over:
    db      01Fh,00Ah,00Ah,00Ah,020h,020h,020h,020h,020h,020h,020h,020h,020h,020h,020h,020h ; 29F1h
    db      069h,020h,067h,020h,072h,020h,061h,020h,020h,020h,06Fh,020h,06Bh,020h,06Fh,020h ; 2A01h
    db      06Eh,020h,07Eh,020h,065h,020h,06Eh,020h,061h,020h,021h,00Ah,00Ah,00Dh,020h,020h ; 2A11h
    db      020h,020h,020h,020h,020h,020h,020h,077h,020h,079h,020h,020h,020h,075h,020h,06Bh ; 2A21h
    db      020h,072h,020h,061h,020h,06Ch,020h,069h,020h,000h ; 2A31h

t_diamonds_suffix:
    db      020h,061h,020h,06Ch,020h,06Dh,020h,061h,020h,07Ah,020h,06Fh,020h,077h,020h,021h ; 2A3Bh
    db      000h ; 2A4Bh

; Неиспользуемая строка авторства: «АВТОР ФИШЕР Е.А.».

t_author_unused:
    db      061h,077h,074h,06Fh,072h,020h,066h,069h,07Bh,065h,072h,020h,065h,02Eh,061h,02Eh ; 2A4Ch
    db      000h ; 2A5Ch

t_zero_padding:
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 2A5Dh
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 2A6Dh
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 2A7Dh
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 2A8Dh
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 2A9Dh
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 2AADh
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 2ABDh
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 2ACDh
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 2ADDh
    db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h ; 2AEDh
    db      000h,000h,000h ; 2AFDh
