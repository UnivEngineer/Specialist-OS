; WIN8: window monitor D172h-D1A3h, flat glyphs and historical row loop.
; Adapt only cursor storage/advance and font addresses to the FontBen ABI.
; Keep the 32-code block lookup, three DAD H, 8-iteration loop and per-row
; CALL -> JMP -> RET rendering operation. Do not unroll/optimize this control.
    INCLUDE "../../sdk/asm/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 02000h
    jp initialize
descriptor: DB 8,1
    DW scalar,batch,empty,scalar

;----------------------------------------------------------------------------
; MXOS
;
; Подготовить движок измерений шрифта enginew и вернуть его дескриптор.
;
; вход:
;   Движок загружен в MAIN. Регистровые параметры этой реализации не требуются.
;
; выход:
;   hl = адрес локального дескриптора с размерами и точками входа вывода, a = 0, cf =
;   0.
;----------------------------------------------------------------------------

initialize:
    ld hl,descriptor
    xor a
    ret
;----------------------------------------------------------------------------
; Нарисовать один символ заменой восьми экранных строк в текущей позиции.
;
; вход:
;   c = код KOI8. Позиция задана bios_vars.cursorY и bios_vars.cursorX,
;   положение должно допускать запись знакоместа без переноса строки.
;
; выход:
;   bios_vars.cursorX увеличен на 4. AF, BC, DE, HL и флаги меняются.
;----------------------------------------------------------------------------

scalar:
    ld hl,(bios_vars.cursorY)
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,90h
    ld h,a
    ld (screenCursor),hl
    ld a,(bios_vars.cursorX)
    add a,4
    ld (bios_vars.cursorX),a
    ld a,c
glyph:
    ld hl,blockMap               ; D172: original table, relocated
    rrca
    rrca
    rrca
    rrca
    and 0Eh
    add a,l
    ld l,a
    ld a,0
    adc a,h
    ld h,a
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,c
    and 1Fh
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,de
    ex de,hl
    ld hl,(screenCursor)         ; original 8F84 screen address
    ld a,l
    sub 7
    ld l,a
    ld c,8
rows:
    ld a,(de)
    call rowOperation           ; original CALL 8F9D, a JMP trampoline
    ld (hl),a
    inc l
    inc e
    dec c
    jp nz,rows
    ret                         ; window cursor advancement is adapted above
rowOperation: jp plainRow
plainRow: ret                    ; original D1A8, normal replace operation
;----------------------------------------------------------------------------
; Вывести последовательность символов без переноса строки.
;
; вход:
;   hl = адрес кодов KOI8, c = длина 1..255. Все знакоместа должны помещаться
;   в строке. Начальная позиция та же, что у scalar.
;
; выход:
;   Позиция продвинута на длину последовательности. AF, BC, DE, HL сохранены.
;----------------------------------------------------------------------------

batch:
    push af
    push bc
    push de
    push hl
    ld b,c
next:
    push bc
    push hl
    ld c,(hl)
    call scalar
    pop hl
    pop bc
    inc hl
    dec b
    jp nz,next
    pop hl
    pop de
    pop bc
    pop af
    ret
empty: ret
screenCursor:   DW 0
; Identity KOI8 map for the common 256-code corpus. Historical default KOI7
; selects different blocks; the lookup machinery itself is unchanged.
blockMap:       DW font,font+256,font+512,font+768,font+1024,font+1280,font+1536,font+1792
    DS 02800h-$,0
font: INCBIN "../../source/monitor_win/originals/monitor_win.rom",01800h,00800h
    ASSERT $ == 03000h
