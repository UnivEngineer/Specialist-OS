; Алмаз-2 для стандартного Специалиста, МХ/MXOS и Ramfos.
; Сборка SjASMPlus --i8080. Адрес загрузки и запуска — 0000h.
    INCLUDE "config.inc"
    INCLUDE "game.inc"

    IF ALMAZ_PIT
        INCLUDE "audio.inc"
        ASSERT $ <= 00B74h
        DS 00B74h-$, 0FFh
    ELSE
        INCLUDE "sounds.inc"
    ENDIF
    INCLUDE "data.inc"

    IF ALMAZ_COLOR
        INCLUDE "color.inc"
    ENDIF
    IF ALMAZ_HUD
        INCLUDE "hud.inc"
    ENDIF
    IF ALMAZ_OPTIMIZE
        INCLUDE "fast.inc"
    ENDIF
    IF ALMAZ_PIT
        INCLUDE "audio-extra.inc"
        INCLUDE "music.inc"
        INCLUDE "sounds.inc"
    ENDIF
    IF ALMAZ_OPTIMIZE
        INCLUDE "input.inc"
    ENDIF
    IF USE_FRAME_CLOCK
        INCLUDE "frame.inc"
    ENDIF
    IF USE_FRAME_CLOCK
        INCLUDE "level-timer.inc"
    ENDIF
    IF ALMAZ_OPTIMIZE
        INCLUDE "animation.inc"
        INCLUDE "game-extra.inc"
    ENDIF
game_end:
    ASSERT game_end < 07F00h    ; Оставить не менее 256 байт под стек.
    DISPLAY "Almaz2: bytes=", /D, game_end, ", free before stack=", /D, 07F00h-game_end
