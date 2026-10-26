; Алмаз для Intel 8080: побайтное восстановление исторического RKS.
; Загружается и запускается с 1EFDh, стек наследуется от Монитора.
    INCLUDE "config.inc"
    ORG GAME_LOAD
    INCLUDE "video.inc"
    ASSERT $ == 02000h
    INCLUDE "game.inc"
    ASSERT $ == 026AEh
    INCLUDE "data.inc"
    ASSERT $ == 02B00h
    INCLUDE "graphics.inc"
game_end:
    ASSERT game_end == 03100h
    DISPLAY "Almaz: payload bytes=", /D, game_end-GAME_LOAD
