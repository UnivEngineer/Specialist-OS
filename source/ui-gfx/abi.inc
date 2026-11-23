; MAIN graphics executor. The caller registers its packet and bitmap buffers.
GFX_BASE = 01B00h
; Result: cursor DW, inverse DW, cursor config, three reserved bytes,
; FNT1 header (8 bytes), left/right glyph padding.
GFX_RESULT       = 03360h
GFX_RESULT_BYTES = 18
GFX_PACKET       = 03380h
GFX_MAX_PACKET   = 3072
; BIND takes BC=capacity at GFX_PACKET.
GFX_BIND          = 0F1h
GFX_CURSOR        = 0F2h
GFX_GLYPH_PADDING = 081h  ; L=first glyph, C=count. Updates padding in result.
; HL=FAR path offset, B=bank, C=length including NUL. A palette byte
; precedes the path. DE=MAIN bitmap storage (32*148 bytes), or 0.
GFX_HELP_OPEN = 084h
GFX_HELP_KEY  = 085h  ; C=character, B bit 0=Ctrl
; Record: operation, color, cursor, inverse, HL/DE/BC/AF, payload length, payload.
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
GFX_COPY_COLORS           = 10  ; Inline: source DW, destination DW, width DB, height DB, colors per 10px row.
GFX_RECOLOR               = 11
GFX_CHECKBOX              = 12
GFX_TOOLBAR               = 13  ; Inline field list; D/E are key/label palettes.
GFX_BITMAP_SAVE           = 14  ; HL=VRAM, DE=client pixels, B=byte width, C=height.
GFX_BITMAP_RESTORE_COLORS = 15  ; Inline 12-byte uiBitmapRestoreColors descriptor.
GFX_TILE_COLUMN           = 16  ; HL=VRAM, B=repeat count, C=height bytes of inline pixels.
GFX_STORE                 = 17  ; Inline bytes, DE=client MAIN destination.
GFX_BITMAP_RESTORE        = 18  ; Same registers as SAVE. Record supplies palette.
GFX_TEXT_STORE            = 19  ; HL=client slot, B=slot length, C=paint length; inline glyphs, then space padding.
GFX_TEXT_BUFFER           = 20  ; HL=client MAIN glyphs, BC=paint length.
GFX_SCROLLBAR             = 21  ; Inline UI_SCROLLBAR.
GFX_RECOLOR_BYTES         = 22  ; HL=VRAM, B=byte width, C=pixel height.
GFX_PSEUDO_ROWS           = 23  ; B=rows, C=glyphs/row, HL=MAIN source or 0 for inline rows, DE=optional MAIN destination, A=slot stride. Pitch 10.
GFX_TEXT_PSEUDO           = 24  ; BC=glyph count, inline glyphs rendered by printStringRawPseudo.
GFX_COPY_RECT             = 25  ; HL=source VRAM, BC=destination VRAM, D=byte width, E=pixel height.
GFX_TEXT_ROWS             = 26  ; Same layout as GFX_PSEUDO_ROWS, rendered by printStringRawLim.
