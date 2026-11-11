; NC использует обычный рендерер, не резервируя 10 КиБ под пререндер.
; Сбрасываем режим BIOS при входе и перед передачей управления.
ncFontRenderEnd:
            ld a,FONT_RENDER_DISABLE
            MXOS_CALL MXOS_BIOS_FONT_RENDER
            ret
