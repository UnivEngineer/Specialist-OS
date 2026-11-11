    INCLUDE "../../source/include/mxos.inc"
    INCLUDE "../../bin/FONTBEN.bios.entries.inc"
    DEFINE FONT_CORPUS_TEST
FONT_COLUMNS = 48
FONT_ROWS = 24
FONT_CORPUS_SIZE = FONT_COLUMNS * FONT_ROWS
textCorpus = 08600h
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 0
    INCLUDE "matrix-main.inc"
    INCLUDE "../textbench/bank-runtime.inc"
bankImage:
    DS 00800h,0
    INCLUDE "../textbench/standalone.inc"
    INCLUDE "engines/cache.inc"
    INCLUDE "corpus.inc"
    INCLUDE "progress.inc"
    INCLUDE "report.inc"
    INCLUDE "aligned8-check.inc"
    INCLUDE "narrow-check.inc"
    INCLUDE "options-runtime.inc"
    ALIGN 8,0
font4Source:
    INCBIN "../../bin/FONTBEN.font4.bin"
    ASSERT $ <= 02000h
    DS 02000h-$,0
    INCBIN "../../bin/FONTBEN.renderer.bin"
    INCLUDE "engines/aligned8.inc"
    ASSERT $ <= 02200h
    DS 02200h-$,0
    INCLUDE "engines/quad.inc"
matrixPageText: DB 'Press Enter for next page',13,10,0
    ASSERT $ <= 02800h
    DS 02800h-$,0
    INCBIN "../../assets/system-rom/FONT.FNT"
    ASSERT $ == 03000h
    INCBIN "../../bin/FONTBEN.shifts.bin",0,00800h
    INCLUDE "engines/legacy.inc"
    ASSERT $ <= 04000h
    DS 04000h-$,0
    INCLUDE "options-start.inc"
    ASSERT $ <= 05000h
    DS 05000h-$,0
    INCBIN "../../bin/FONTBEN.shifts.bin",02000h,01000h
    ASSERT $ == 06000h
    INCLUDE "engines/monitor.inc"
    INCLUDE "engines/kernel-api.inc"
    INCLUDE "engines/paged-bench.inc"
    ASSERT $ <= 06800h
    DS 06800h-$,0
    ASSERT $ == 06800h
    INCLUDE "engines/font4.inc"
    INCLUDE "matrix-data.inc"
    IFDEF FONT_BLOCK32
    INCLUDE "engines/block32-support.inc"
    ENDIF
    ASSERT $ <= 07C00h
programEnd:
