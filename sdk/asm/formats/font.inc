; MXOS
; Формат и входы сменного шрифтового движка.

    IFNDEF MXOS_FORMAT_FONT_INC
    DEFINE MXOS_FORMAT_FONT_INC

    INCLUDE "image.inc"

; Fixed resident slot, entirely visible in STD. Private module layout is free.
    IFDEF FONT_BENCH_MAIN
; Compile the very same public engines into the FontBen MAIN DLL arena.
FONT_ENGINE_BASE = 02000h
FONT_ENGINE_END  = 04880h
    ELSE
FONT_ENGINE_BASE = 0D800h
FONT_ENGINE_END  = 0EC80h
    ENDIF
FONT_ENGINE_INIT     = FONT_ENGINE_BASE
FONT_ENGINE_CHAR     = FONT_ENGINE_BASE+3
FONT_ENGINE_RAW      = FONT_ENGINE_BASE+6
FONT_ENGINE_Z        = FONT_ENGINE_BASE+9
FONT_ENGINE_RAWZ     = FONT_ENGINE_BASE+12
FONT_ENGINE_LIM      = FONT_ENGINE_BASE+15
FONT_ENGINE_ENCODING = FONT_ENGINE_BASE+18  ; D812h: A=0 KOI8, A!=0 KOI7; AF/BC/DE/HL preserved; idempotent, physical bitmap/index swap
FONT_ENGINE_SCALAR   = FONT_ENGINE_BASE+21
FONT_ENGINE_SERVICE  = FONT_ENGINE_BASE+24
FONT_ENGINE_PSEUDO   = FONT_ENGINE_BASE+27
FONT_ENGINE_TEXT_EXT = FONT_ENGINE_BASE+30  ; DW FONT_ENGINE_TEXT_LIM для ограниченных строк, 0 у прежних FNT1
; FNT1: +5 cell width, +6 height, +7 renderer kind:
; 0 Rotate, 1 Packed6K, 2 Fast12K, 3 Compact 6×8, 4 Fast8, 5 Packed 4×8.
; Kind identifies the engine, independently of imported glyphs/file names.
FONT_ENGINE_HEADER        = FONT_ENGINE_BASE+32
FONT_ENGINE_TEXT_LIM      = FONT_ENGINE_BASE+48  ; HL=текст с управлением, BC=длина, HL=конец
FONT_ENGINE_VERSION       = 2
FONT_FORMAT_4_PACKED      = 5                    ; Дублированные полубайты, постраничная таблица
FONT_FORMAT_GRAPH8        = 6                    ; Fast8 variant with exact two-pixel cursor coordinates
FONT_FORMAT_GRAPH8_PACKED = 7                    ; three pagesets of rotated bytes
FONT_FORMAT_GRAPH8_FULL   = 8                    ; six pagesets of separated bitmap parts
FONT_FORMAT_4_COMPACT     = 9                    ; Два соседних кода в каждой восьмибайтной записи
FONT_FORMAT_4_FAST        = 10                   ; Верхняя таблица старших полубайтов и нижняя таблица младших
FONT_FORMAT_KOI7          = 11                   ; Fixed KOI-7R Rotate, Packed, Fast, full Fast benchmark
    IFDEF FONT_BENCH_MAIN
FONT_ADDR = 04000h
    ELSE
FONT_ADDR = 0E400h  ; Полный построчный шрифт, восемь страниц по 256 байт
    ENDIF
FONT_BYTES           = 00800h
FONT_PRERENDER_BYTES = 02800h  ; Три сдвинутые фазы 6x8 для 256 кодов
FONT_RENDER_QUERY    = 0
FONT_RENDER_BUILD    = 1
FONT_RENDER_DISABLE  = 2

FONT_IMAGE_HEADER_SIZE = IMAGE_HEADER_SIZE

    ENDIF
