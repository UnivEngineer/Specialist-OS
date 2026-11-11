; Scalar-only collection. The common FontBen hooks own AF/BC/DE/HL.
    INCLUDE "../../source/include/mxos.inc"
    DEFINE FONT_ENGINE_DLL
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 02000h
entry: jp initialize
descriptor: DB 6,1
    DW historical.monitor2,batch,empty
    DS 06000h-$,0
initialize:
    ld (variant),a
    ld a,b
    ld (preEnabled),a
    call buildLinear
    ld a,(variant)
    or a
    ld hl,historical.monitor2
    jp z,selected
    dec a
    ld hl,historical.ramfos
    jp z,selected
    dec a
    ld hl,linearScalar
    jp z,selected
    ld a,(preEnabled)
    or a
    ld hl,linearScalar
    jp z,selected
    ld a,(variant)
    cp 4
    jp z,columnInit
    call buildShifts
    ; The public hook preserves registers. Enter the unwrapped FULL body.
    ld a,0C9h
    ld (shift.rendererReturn),a
    ld hl,shift.glyphBody
    jp selected
columnInit:
    call columnPre.build
    ld hl,columnPre.scalar
selected:
    ld (descriptor+2),hl
    ld (scalarCall+1),hl
    ld hl,descriptor
    xor a
    ret
batch:
    push af
    push bc
    push de
    push hl
    ld b,c
batchNext:
    ld a,b
    or a
    jp z,batchDone
    push bc
    push hl
    ld c,(hl)
scalarCall: call 0
    pop hl
    pop bc
    inc hl
    dec b
    jp batchNext
batchDone:
    pop hl
    pop de
    pop bc
    pop af
    or a
    ret
empty: ret
linearScalar:
    ld l,c
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,02800h
    add hl,de
    ex de,hl
    ld hl,(bios_vars.cursorY)
    ld a,h
    rrca
    rrca
    and 03Fh
    add a,090h
    ld b,a
    inc h
    inc h
    inc h
    ld (bios_vars.cursorY),hl
    ld a,l
    sub 7
    ld c,a
    jp scalar6.scalar6Draw
    INCLUDE "engines/scalar-build.inc"
    INCLUDE "engines/historical-scalar.inc"
    INCLUDE "engines/column-scalar.inc"
    INCLUDE "engines/paged-scalar.inc"
    DS 07000h-$,0
    DEFINE SHIFT_ORIGIN 07000h
    MODULE shift
    INCLUDE "engines/SHIFT.asm"
    ENDMODULE
    ASSERT $ <= 07C00h
end:
