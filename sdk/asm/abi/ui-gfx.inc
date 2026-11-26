; Протокол UI_GFX.DLL для клиентов MAIN и EXEC.
    IFNDEF MXOS_ABI_UI_GFX_INC
    DEFINE MXOS_ABI_UI_GFX_INC

; GFX_BIND: HL = начало зарегистрированного приёмника в MAIN, BC = ёмкость.
; Ответ: HL = результат в MAIN (действителен до выгрузки), DE = база DLL.
; Курсор DW, инверсия DW, конфигурация курсора, три резервных байта,
; заголовок FNT1 (8 байт), два байта отступов глифа.
GFX_RESULT_BYTES  = 18
GFX_MAX_PACKET    = 3072
GFX_BIND          = 0F1h
GFX_CURSOR        = 0F2h
GFX_GLYPH_PADDING = 081h  ; L = первый глиф, C = число глифов. Обновляет отступы в результате.
; B:HL = банк и смещение пути, C = длина с нулём. Перед путём лежит байт палитры.
; DE = буфер фона в MAIN (32 × 148 байт) или 0.
GFX_HELP_OPEN     = 084h
GFX_HELP_KEY      = 085h  ; C = символ, бит 0 регистра B = Ctrl.

; Поля записи: операция, цвет, курсор, инверсия, HL/DE/BC/AF, длина данных, данные.
GFX_RECORD_HEADER         = 16
GFX_DRAW                  = 1
GFX_FILL                  = 2
GFX_FRAME                 = 3
GFX_INVERSE               = 4
GFX_BYTES                 = 5
GFX_INVERSE_BYTES         = 6
GFX_TEXT                  = 7
GFX_CHAR                  = 8
GFX_COPY                  = 9
GFX_COPY_COLORS           = 10  ; В записи: источник DW, приёмник DW, ширина DB, высота DB, цвета для каждого ряда в 10 px.
GFX_RECOLOR               = 11
GFX_CHECKBOX              = 12
GFX_TOOLBAR               = 13  ; В записи список полей, D/E = палитры клавиш и подписей.
GFX_BITMAP_SAVE           = 14  ; HL = VRAM, DE = буфер клиента, B = ширина в байтах, C = высота.
GFX_BITMAP_RESTORE_COLORS = 15  ; В записи 12-байтный дескриптор uiBitmapRestoreColors.
GFX_TILE_COLUMN           = 16  ; HL = VRAM, B = число повторов, C = число байтов пикселей в записи.
GFX_STORE                 = 17  ; Байты из записи копируются по адресу DE в MAIN.
GFX_BITMAP_RESTORE        = 18  ; Регистры как у SAVE. Палитра в записи.
GFX_TEXT_STORE            = 19  ; HL = поле клиента, B = длина поля, C = длина вывода. Глифы из записи дополняются пробелами.
GFX_TEXT_BUFFER           = 20  ; HL = глифы клиента в MAIN, BC = длина вывода.
GFX_SCROLLBAR             = 21  ; В записи структура UI_SCROLLBAR.
GFX_RECOLOR_BYTES         = 22  ; HL = VRAM, B = ширина в байтах, C = высота в пикселях.
GFX_PSEUDO_ROWS           = 23  ; B = строки, C = глифы в строке, HL = источник в MAIN (0 для данных из записи), DE = приёмник в MAIN или 0, A = шаг поля. Высота строки 10 px.
GFX_TEXT_PSEUDO           = 24  ; BC = число глифов в записи для printStringRawPseudo.
GFX_COPY_RECT             = 25  ; HL и BC = адреса источника и приёмника в VRAM, D = ширина в байтах, E = высота в пикселях.
GFX_TEXT_ROWS             = 26  ; Формат GFX_PSEUDO_ROWS, вывод через printStringRawLim.

    ENDIF
