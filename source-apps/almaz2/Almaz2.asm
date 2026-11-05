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
    IF !ALMAZ_RAMFOS && !ALMAZ_MONITOR_TARGET
; Классическая карта игры использует стек 7FFFh и буфер 8000h..81FFh.
; До первого их изменения освобождаем DOSMOD через резидентный ABI MXOS.
; Все три внешних входа остаются JMP по прежним адресам 0000h/0003h/0006h.
; Входа нет; AF/флаги разрушены, BC/DE/HL сохраняет releaseDosModule.
mxos_game_entry:
        CALL bios_releaseDosModule
        JP game_start
mxos_editor_new:
        CALL bios_releaseDosModule
    IF ALMAZ_PIT
        JP audio_editor_new
    ELSE
    IF ALMAZ_OPTIMIZE
        JP input_editor_new
    ELSE
        JP editor_create_map
    ENDIF
    ENDIF
mxos_editor_current:
        CALL bios_releaseDosModule
    IF ALMAZ_PIT
        JP audio_editor_existing
    ELSE
    IF ALMAZ_OPTIMIZE
        JP input_editor_existing
    ELSE
        JP editor_open
    ENDIF
    ENDIF
    ENDIF
game_end:
    ASSERT game_end < 07F00h    ; Оставить не менее 256 байт под стек.
    DISPLAY "Almaz2: bytes=", /D, game_end, ", free before stack=", /D, 07F00h-game_end
