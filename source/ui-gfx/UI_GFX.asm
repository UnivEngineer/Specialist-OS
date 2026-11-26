    INCLUDE "../include/mxos.inc"
    INCLUDE "abi.inc"
    INCLUDE "../include/ui.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 01B00h
entry:
    cp MODULE_ACTIVATE
    jp z,ngActivate
    cp GFX_BIND
    jp z,ngBind
    cp GFX_CURSOR
    jp z,ngCursor
    cp GFX_HELP_OPEN
    jp z,ngHelpOpen
    cp GFX_HELP_KEY
    jp z,ngHelpKey
    cp GFX_GLYPH_PADDING
    jp z,ngPadding
    jp ngBatch

;----------------------------------------------------------------------------
; MXOS
;
; Сбросить состояние графического модуля при активации.
;
; вход:
;   Точка выбрана диспетчером по операции MODULE_ACTIVATE. Поля ngHelpActive и
;   ngBatches доступны в MAIN.
;
; выход:
;   Справка закрыта, счётчик пакетов обнулён. a = 0, hl = 0, z = 1, cf = 0; bc и de
;   сохранены.
;----------------------------------------------------------------------------

ngActivate:
    xor a
    ld (ngHelpActive),a
    ld h,a
    ld l,a
    ld (ngBatches),hl
    ld (ngPacketLimit),hl
    ret

;----------------------------------------------------------------------------
; MXOS
; Привязать приёмный пакет клиента и получить состояние рисования.
; вход:
;   HL = начало зарегистрированного клиентом буфера MAIN, BC = ёмкость
;   1..GFX_MAX_PACKET. Весь буфер принадлежит клиенту до конца MODULE_SCOPE.
; выход:
;   При успехе CF=0, A=0, HL = адрес результата в MAIN до выгрузки, DE = база DLL.
;   Справка сброшена. При недопустимом интервале CF=1, A=MEM_ERROR_RANGE,
;   прежняя привязка сохранена. BC и прочие флаги разрушаются.
;----------------------------------------------------------------------------
ngBind:
    ld a,b
    or c
    jp z,ngBad
    ex de,hl
    ld hl,GFX_MAX_PACKET
    ld a,h
    cp b
    jp c,ngBad
    jp nz,ngBindCapacityOK
    ld a,l
    cp c
    jp c,ngBad
ngBindCapacityOK:
    ex de,hl
    push hl
    add hl,bc
    jp c,ngBindBad
    ld de,APP_MEMORY_END
    call bios_cmp_hl_de
    jp z,ngBindCheckDll
    jp nc,ngBindBad
ngBindCheckDll:
    ld de,entry
    call bios_cmp_hl_de
    pop hl
    jp c,ngBindReady
    jp z,ngBindReady
    ld de,ngFileEnd
    call bios_cmp_hl_de
    jp c,ngBad
ngBindReady:
    ld (ngTransfer+4),hl
    ld h,b
    ld l,c
    ld (ngPacketLimit),hl
    call ngHelpReset
    ld a,(bios_upper.cursorCfg)
    ld (ngResult+4),a
    ld hl,FONT_ENGINE_HEADER
    ld de,FONT_ENGINE_HEADER+8
    ld bc,ngResult+8
    call bios_memcpy_bc_hl
    call ngSaveCursor
    ld hl,ngResult
    ld de,entry
    ret
ngBindBad:
    pop hl
    jp ngBad
ngPadding:
    ; HL=first glyph, C=number. Returns D/E = free left/right pixel.
    ld a,(FONT_ENGINE_HEADER+5)
    cp 8
    ld de,0100h
    jp nz,ngStorePadding
    ld a,c
    or a
    jp z,ngStorePadding
    ld h,FONT_ADDR>>8
    ld b,8
    ld e,0
ngPaddingRow:
    push hl
    ld a,c
    ld d,a
ngPaddingGlyph:
    ld a,(hl)
    or e
    ld e,a
    inc l
    dec d
    jp nz,ngPaddingGlyph
    pop hl
    inc h
    dec b
    jp nz,ngPaddingRow
    ld a,e
    cpl
    and 1
    ld d,0
    ld l,a
    ld a,e
    or a
    jp m,ngPaddingReady
    inc d
ngPaddingReady:
    ld e,l
ngStorePadding:
    ld hl,ngResult+16
    ld (hl),d
    inc hl
    ld (hl),e
    xor a
    ret
ngSaveCursor:
    ld hl,(bios_vars.cursorY)
    ld (ngResult),hl
    ld hl,(bios_vars.inverse)
    ld (ngResult+2),hl
    xor a
    ret
; CURSOR: HL = позиция BIOS, B = конфигурация курсора на один вывод,
; C = цвет. Восстанавливает прежнюю конфигурацию и возвращает A=0, CF=0.
ngCursor:
    ld (bios_vars.cursorY),hl
    ld a,c
    ld (IO_COLOR),a
    ld a,(bios_upper.cursorCfg)
    push af
    ld a,b
    ld (bios_upper.cursorCfg),a
    call bios_drawCursorOld
    pop af
    ld (bios_upper.cursorCfg),a
    xor a
    ret

ngBatch:
    ; A=source bank, HL=source, BC=exact bytes, DE=0.
    ld (ngTransfer),a
    ld (ngTransfer+1),hl
    ld h,b
    ld l,c
    ex de,hl
    ld hl,(ngPacketLimit)
    ex de,hl
    call bios_cmp_hl_de
    jp z,ngBatchSizeOK
    jp nc,ngBad
ngBatchSizeOK:
    ld a,h
    or l
    jp z,ngBad
    ld (ngTransfer+6),hl
    ld hl,ngTransfer
    call mx_memcpyFar
    ret c
    ld hl,(ngBatches)
    inc hl
    ld (ngBatches),hl
    ld hl,(ngTransfer+6)
    ex de,hl
    ld hl,(ngPacketBytes)
    add hl,de
    ld (ngPacketBytes),hl
    ld hl,(ngTransfer+4)
    push hl
    add hl,de
    ld (ngEnd),hl
    pop hl
ngRecord:
    ex de,hl
    ld hl,(ngEnd)
    ex de,hl
    call bios_cmp_hl_de
    jp z,ngSaveCursor
    jp nc,ngBad
    ld a,(hl)
    or a
    jp z,ngBad
    cp GFX_TEXT_ROWS+1
    jp nc,ngBad
    ld (ngOperation),a
    inc hl
    ld a,(hl)
    ld (IO_COLOR),a
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ex de,hl
    ld (bios_vars.cursorY),hl
    ex de,hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ex de,hl
    ld (bios_vars.inverse),hl
    ex de,hl
    ld (ngRegisters),hl
    ld de,8
    add hl,de
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld (ngPayload),hl
    add hl,bc
    ld (ngNextRecord),hl
    ex de,hl
    ld hl,(ngEnd)
    ex de,hl
    call bios_cmp_hl_de
    jp z,ngDispatch
    jp nc,ngBad
ngDispatch:
    ld hl,ngNext
    push hl
    ld a,(ngOperation)
    dec a
    add a,a
    ld e,a
    ld d,0
    ld hl,ngVector
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de
    ld hl,(ngRegisters)
    ld de,6
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de
    pop af
    ld hl,(ngRegisters)
    inc hl
    inc hl
    inc hl
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ld hl,(ngRegisters)
    inc hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,(ngRegisters)
    push de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    pop de
    ret
ngNext:
    ld hl,(ngNextRecord)
    jp ngRecord
ngBad:
    ld a,MEM_ERROR_RANGE
    scf
    ret
ngVector:
    DW ngDraw,uiFillRectPixels,uiFrameRectPixels,uiInverseRectPixels
    DW uiFillRectBytes,uiInverseRect,ngText,bios_printChar
    DW uiCopyTextRect,ngCopyColors,ngRecolor,uiDrawCheckbox
    DW ngToolbar,uiBitmapSave,ngRestoreColors,ngTileColumn
    DW ngStore,uiBitmapRestore,ngStoreText,bios_printStringRawLim
    DW ngScrollbar,uiRecolorRectBytes,ngPseudoRows,ngTextPseudo,uiCopyRect,ngTextRows
    INCLUDE "text-rows.inc"
ngScrollbar:
    ld hl,(ngPayload)
    jp uiDrawScrollbar
ngToolbar:
    ld b,d
    ld c,e
    ld hl,(ngPayload)
    jp uiToolbar
ngDraw:
    ld hl,(ngPayload)
    jp uiDraw
ngText:
    ld hl,(ngPayload)
    jp bios_printStringRawLim
ngTextPseudo:
    ld hl,(ngPayload)
    jp bios_printStringRawPseudo
ngStore:
    push de
    ld hl,(ngPayload)
    ex de,hl
    ld hl,(ngNextRecord)
    ex de,hl
    pop bc
    jp bios_memcpy_bc_hl
ngRestoreColors:
    ld hl,(ngPayload)
    jp uiBitmapRestoreColors
ngCopyColors:
    ld hl,(ngPayload)
    ld de,ngColorCopy
    ld b,6
ngCopyHeader:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,ngCopyHeader
    ld (ngColorCopy+6),hl
    ld (ngColorCopy+8),hl
    ld hl,ngColorCopy
    jp uiCopyTextRectColors
ngColorCopy: DS 10,0
ngTileColumn:
    ld a,b
    or a
    ret z
    ld a,c
    or a
    ret z
ngTileNext:
    push bc
    push hl
    ex de,hl
    ld hl,(ngPayload)
    ex de,hl
ngTileByte:
    ld a,(de)
    ld (hl),a
    inc de
    inc l
    dec c
    jp nz,ngTileByte
    pop hl
    pop bc
    inc h
    dec b
    jp nz,ngTileNext
    ret
ngRecolor:
    ; HL=pixel X, D=Y, BC=width, E=height; byte-aligned widgets only.
    ld a,l
    rrca
    rrca
    rrca
    and 31
    ld l,a
    ld a,h
    or a
    ld a,l
    jp z,ngRecolorX
    add a,32
ngRecolorX:
    add a,090h
    ld h,a
    ld l,d
    ld a,c
    rrca
    rrca
    rrca
    and 31
    ld b,a
    ld c,e
    jp uiRecolorRectBytes
; HL=caller-owned text slot, B=slot length, C=paint length, A=text length in batch payload.
ngStoreText:
    push hl
    push bc
    push af
    or a
    jp z,ngPadSlot
    push hl
    ld e,a
    ld d,0
    ld hl,(ngGlyphBytes)
    add hl,de
    ld (ngGlyphBytes),hl
    pop hl
    ld b,h
    ld c,l
    ld hl,(ngPayload)
    ld e,a
    ld d,0
    add hl,de
    ex de,hl
    ld hl,(ngPayload)
    call bios_memcpy_bc_hl
    ld hl,(ngStoredTexts)
    inc hl
    ld (ngStoredTexts),hl
ngPadSlot:
    pop af
    pop bc
    pop hl
    push hl
    ld e,a
    ld d,0
    add hl,de
    ld a,b
    sub e
    jp z,ngSlotReady
ngPadLoop:
    ld (hl),32
    inc hl
    dec a
    jp nz,ngPadLoop
ngSlotReady:
    pop hl
    ld b,0
    jp bios_printStringRawLim
ngBatches:      DW 0
ngStoredTexts:  DW 0
ngPacketBytes:  DW 0
ngGlyphBytes:   DW 0
ngEnd:          DW 0
ngNextRecord:   DW 0
ngRegisters:    DW 0
ngPayload:      DW 0
ngOperation:    DB 0
    DEFINE UI_SCROLL_BAR
    DEFINE UI_PIXEL_RECT
    DEFINE UI_BYTE_RECT
    DEFINE UI_BITMAP
    DEFINE UI_TEXT_RECT
    DEFINE UI_TEXT_RECT_COLORS
    DEFINE UI_MODAL_BORDER
    DEFINE UI_IMPLEMENTATION
    INCLUDE "../include/ui.inc"
    INCLUDE "help.inc"
ngTransfer: DB 0
    DW 0
    DB 0
    DW 0,0
ngPacketLimit: DW 0
ngResult:      DS GFX_RESULT_BYTES,0
ngFileEnd:
IMAGE_MEM_END       = ngFileEnd
IMAGE_CACHEABLE     = IMAGE_OPTION_CACHEABLE|IMAGE_OPTION_REUSABLE
IMAGE_MEMORY_POLICY = IMAGE_POLICY_MANAGED
IMAGE_API           = 2
    END
