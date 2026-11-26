; Временный инициализатор шрифта FIN1, версия 1. Нативный DLL для MAIN,
; заголовок контейнера длиной IMAGE_HEADER_SIZE лежит перед входом.
; По адресу входа JMP обработчика, со смещения +3 — DB "FIN1",1.
; A = FONT_INIT_IMPORT или FONT_INIT_GENERATE, HL = адрес FONT_INIT_REQUEST.
; Регистры могут меняться. CF=0 при успехе, CF=1 и A=код DOS при отказе.
; Инициализатор не выделяет память, не печатает и не обращается к файлам.
; Дескриптор FNT1 по смещению +46 от тела движка содержит резидентный
; указатель на имя инициализатора в формате ASCIZ (8.3).

    IFNDEF MXOS_FORMAT_FONT_INIT_INC
    DEFINE MXOS_FORMAT_FONT_INIT_INC

FONT_INIT_ADDR     = 05000h
FONT_INIT_END      = 05800h
FONT_IMPORT_ADDR   = 04000h
FONT_INIT_IMPORT   = 0
FONT_INIT_GENERATE = 1

    STRUCT FONT_INIT_REQUEST
engine DW 0     ; Staged engine BODY, not its native executable header.
flat   DW 0     ; Read-only flat input: 256 glyphs * 8 bytes.
lower  DW 0     ; Writable reserved planes; ignored during IMPORT.
    ENDS

    ENDIF
