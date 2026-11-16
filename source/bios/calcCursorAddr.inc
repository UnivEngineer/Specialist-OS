;+---------------------------------------------------------------------------
; MXOS
; Расчет адреса курсора в видеопамяти. И перевод строки, если курсор был
; за правым краем экрана.
;
; На выходе
;  hl - координаты курсора
;  a * 256 + l - адрес в видеопамяти
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

calcCursorAddr:   ; В hl положение курсора
            ld    hl, (bios_vars.cursorY)

            ; An aligned 4px cell at X=380 is valid; only X>=384 wraps.
            ; Иначе рассчитываем адрес старшего байта a = cursorX / 4 + 90h
            ld    a,(FONT_ENGINE_HEADER+5)
            cp    6
            ld    a,h
            jp z,calcCursorLegacyEdge
            cp    192
            jp calcCursorEdgeReady
calcCursorLegacyEdge:
            cp    190
calcCursorEdgeReady:
            jp c, calcCursorAd_1
            ld    c, 10
            jp    calcCursorAd_0

