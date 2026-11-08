    INCLUDE "../../source/include/mxos.inc"
    INCLUDE "../../bin/FONTBEN.bios.entries.inc"
    DEFINE FONT_CORPUS_TEST
FONT_COLUMNS = 48
FONT_ROWS = 24
FONT_CORPUS_SIZE = FONT_COLUMNS * FONT_ROWS
textCorpus = 08600h
    ORG 0
    INCLUDE "matrix-main.inc"
    INCLUDE "../textbench/bank-runtime.inc"
bankImage:
    INCBIN "../../bin/FONTBEN.bank.bin"
    INCLUDE "../textbench/standalone.inc"
    INCLUDE "cache.inc"
    INCLUDE "corpus.inc"
    INCLUDE "progress.inc"
    INCLUDE "report.inc"
    INCLUDE "aligned8-check.inc"
    ASSERT $ <= 02000h
    DS 02000h-$,0
    INCBIN "../../bin/FONTBEN.renderer.bin"
    INCLUDE "aligned8.inc"
    ASSERT $ <= 02200h
    DS 02200h-$,0
    INCLUDE "quad.inc"
    INCLUDE "bank-window.inc"
matrixPageText: DB 'Press Enter for next page',13,10,0
    ASSERT $ <= 02800h
    DS 02800h-$,0
    INCBIN "../../assets/system-rom/FONT.FNT"
    ASSERT $ == 03000h
    INCBIN "../../bin/FONTBEN.shifts.bin",0,00800h
    INCLUDE "legacy.inc"
    ASSERT $ <= 04000h
    DS 04000h-$,0
    INCBIN "../../bin/FONTBEN.shifts.bin",01000h,02000h
    ASSERT $ == 06000h
savedDos: DS BANK_TEST_BYTES,0
    ASSERT $ == 06800h
programEnd:
