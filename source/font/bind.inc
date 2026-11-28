    IFNDEF FONT_BENCH_MAIN
; Вывод C800h обоих профилей проходит через исторические векторы Монитора-2.
    ld hl,printChar
    ld (mon2_printCharOld+1),hl
    ld hl,printStringZ
    ld (mon2_printStringOld+1),hl
    ld hl,printStringRawZ
    ld (biosEngineRawZTarget),hl
    ENDIF
