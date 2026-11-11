; Standalone MAIN DLL; descriptor width/quantum/scalar/batch/reset.
    INCLUDE "../../source/include/mxos.inc"
    DEFINE FONT_ENGINE_DLL
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 02000h
entry: jp initialize
descriptor: DB 6,4
    DW monitor.scalar,serialBIOS,stageReset
    DS 06000h-$,0
initialize:
    ld (variant),a
    ld a,b
    ld (preEnabled),a
    call buildLinear
    ld hl,monitor.scalar
    ld (descriptor+2),hl
    ld hl,serialBIOS
    ld (descriptor+4),hl
    ld a,(variant)
    or a
    jp z,ready
    dec a
    jp z,selectOpt
    ld a,(preEnabled)
    or a
    jp z,withoutPre
    call buildShifts
    ld a,(variant)
    cp 2
    jp z,selectFull
    cp 3
    jp z,selectCache
    cp 4
    jp z,selectQuadFull
    cp 7
    jp z,selectDynamic
    cp 8
    jp z,selectHalf
    cp 9
    jp z,selectPacked
    call prepareCache
    ld hl,cacheRender
    ld (descriptor+2),hl
    ld hl,quadRender
    jp batchReady
withoutPre:
    ld a,(variant)
    cp 4
    jp c,ready
selectDynamic:
    ld hl,quadDynamicRender
    jp batchReady
selectOpt:
    ld a,0C3h
    ld (monitor.drawEntry),a
    ld hl,legacyOpt.optimizedDraw
    ld (monitor.drawEntry+1),hl
    jp ready
selectFull:
    ld hl,shift.render
    ld (descriptor+2),hl
    ld hl,serialFull
    jp batchReady
selectCache:
    call prepareCache
    ld hl,cacheRender
    ld (descriptor+2),hl
    ld hl,serialCache
    jp batchReady
selectQuadFull:
    ld hl,shift.render
    ld (descriptor+2),hl
    ld hl,quadFullRender
    jp batchReady
selectHalf:
    call quadPrepareHalf
    ret c
    ld hl,quadHalfRender
    jp batchReady
selectPacked:
    call quadPreparePacked
    ret c
    ld hl,packedBatch
batchReady:
    ld (descriptor+4),hl
ready:
    ld hl,descriptor
    xor a
    ret
prepareCache:
    ld hl,03000h
    ld (cachePrivateStart),hl
    ld hl,03800h
    ld (cachePrivateEnd),hl
    jp cachePrepare
stageReset:
    ld a,(variant)
    cp 3
    jp z,cachePrepare
    cp 5
    jp z,cachePrepare
    cp 6
    jp z,cachePrepare
    ret
; Unrepresented packed glyphs use a local full-font fallback, not BIOS.
packedBatch:
    push af
    push bc
    push de
    push hl
    ld b,c
packedCheck:
    ld a,(hl)
    cp 32
    jp z,packedNext
    push hl
    ld l,a
    ld h,0
    ld de,packedIndex
    add hl,de
    ld a,(hl)
    pop hl
    or a
    jp z,packedScalar
packedNext:
    inc hl
    dec b
    jp nz,packedCheck
    pop hl
    pop de
    pop bc
    pop af
    jp quadPackedRender
packedScalar:
    pop hl
    pop de
    pop bc
    pop af
    jp serialBIOS
buildLinear:
    ld de,02800h
    ld c,0
linearGlyph:
    ld h,FONT_ADDR >> 8
    ld l,c
    DUP 8
    ld a,(hl)
    ld (de),a
    inc h
    inc de
    EDUP
    inc c
    jp nz,linearGlyph
    ret
buildShifts:
    ld hl,02800h
    ld de,03000h
    ld bc,2048
shiftZero:
    ld a,(hl)
    add a,a
    add a,a
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,shiftZero
    ld de,04000h
    ld a,2
    call buildPair
    ld de,05000h
    ld a,4
buildPair:
    ld (pairRotations+1),a
    ld hl,02800h
    ld bc,2048
pairByte:
    push bc
    ld a,(hl)
pairRotations: ld b,2
pairRotate:
    rrca
    dec b
    jp nz,pairRotate
    ld c,a
    ld a,(pairRotations+1)
    cp 2
    ld a,c
    jp nz,pairFour
    and 00Fh
    ld (de),a
    inc de
    ld a,c
    and 0C0h
    jp pairWrite
pairFour:
    and 3
    ld (de),a
    inc de
    ld a,c
    and 0F0h
pairWrite:
    ld (de),a
    inc de
    inc hl
    pop bc
    dec bc
    ld a,b
    or c
    jp nz,pairByte
    ret
renderFont8: DW 02800h/8
variant: DB 0
preEnabled: DB 0
    MODULE shift
SHIFT_ORIGIN = $
    INCLUDE "engines/SHIFT.asm"
    ENDMODULE
CACHE_GLYPHBODY = shift.glyphBody
CACHE_RENDERERRETURN = shift.rendererReturn
CACHE_ONEBYTE = shift.oneByte
CACHE_TWOBYTES = shift.twoBytes
    INCLUDE "engines/monitor.inc"
    DEFINE FONTBEN_EMBED_OPT
    DEFINE OPT_DRAW_ORIGIN $
    MODULE legacyOpt
    INCLUDE "engines/OPT_DRAW.asm"
    ENDMODULE
    INCLUDE "engines/cache.inc"
    INCLUDE "engines/serial-engine.inc"
    INCLUDE "engines/quad.inc"
    INCLUDE "engines/quad-packed.inc"
packedOverlay: DB 0
bench_copy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,bench_copy
    ret
    ASSERT $ <= 07C00h
end:
