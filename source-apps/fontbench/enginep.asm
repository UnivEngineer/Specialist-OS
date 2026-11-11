; Preserved full post-row standalone engines; MAIN descriptor ABI.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
PAGED_FONT_BASE = 02800h
PAGED_PRE_BASE = 03000h
    ORG 02000h
entry: jp initialize
descriptor: DB 6,4
    DW scalarSix,batch6Raw,empty
    DS 06000h-$,0
initialize:
    ld (variant),a
    ld a,b
    ld (preEnabled),a
    IFDEF FONT_BLOCK32
    call blockBuild
    ELSE
    ld hl,FONT_ADDR
    ld de,PAGED_FONT_BASE
    ld bc,2048
    call copy
    ENDIF
    ld a,(variant)
    or a
    jp z,six
    ld a,8
    ld (descriptor),a
    ld a,1
    ld (descriptor+1),a
    jp ready
six:
    ld a,(preEnabled)
    or a
    jp z,ready
    ld de,PAGED_PRE_BASE
    ld a,0
phase:
    ld (operation+1),a
    ld hl,PAGED_FONT_BASE
    IFDEF FONT_BLOCK32
    ld bc,1280
    ELSE
    ld bc,2048
    ENDIF
build:
    ld a,(hl)
    push bc
    ld c,a
operation: ld a,0
    or a
    ld a,c
    jp z,left2
    ld a,(operation+1)
    dec a
    ld a,c
    jp z,right2
    ld a,(operation+1)
    cp 2
    ld a,c
    jp z,left6
    ld a,(operation+1)
    cp 3
    ld a,c
    jp z,right4
    add a,a
    add a,a
left2:
    add a,a
    add a,a
    jp store
right2:
    rrca
    rrca
    and 00Fh
    jp store
left6:
    rrca
    rrca
    and 0C0h
    jp store
right4:
    rrca
    rrca
    rrca
    rrca
    and 3
store:
    ld (de),a
    inc de
    inc hl
    pop bc
    dec bc
    ld a,b
    or c
    jp nz,build
    IFDEF FONT_BLOCK32
    ex de,hl
    ld bc,0300h ; five 256-byte blocks in each 2 KiB phase slot
    add hl,bc
    ex de,hl
    ENDIF
    ld a,(operation+1)
    inc a
    cp 5
    jp nz,phase
ready:
    ; Install the exact scalar/batch kernel once. No variant/preEnabled
    ; selection remains on the measured scalar or packet path.
    ld a,(variant)
    or a
    jp z,readySix
    ld hl,scalar8
    ld (descriptor+2),hl
    ld hl,batch8
    jp readyBatch
readySix:
    ld hl,batch6Raw
    ld a,(preEnabled)
    or a
    jp z,readyBatch
    ld hl,batch6Pre
readyBatch:
    IFDEF FONT_BLOCK32
    ld (blockRender+1),hl
    ld hl,blockBatch
    ENDIF
    ld (descriptor+4),hl
    ld hl,descriptor
    xor a
    ret
; Independent cursor adapters, chosen by the descriptor at initialization.
; Public hooks own the contract; each batch saves it once, helpers do not.
    MACRO CURSOR_BATCH name, step, renderer
name:
    push af
    push bc
    push de
    push hl
    ld hl,(bios_vars.cursorY)
    ld a,h
    rrca
    rrca
    and 03Fh
    add a,090h
    ld d,a
    ld a,l
    sub 7
    ld e,a
    ld a,c
    add a,a
    IF step = 3
    add a,c
    ELSE
    add a,a
    ENDIF
    add a,h
    ld (bios_vars.cursorX),a
    pop hl
    push hl
    call renderer
    pop hl
    pop de
    pop bc
    pop af
    ret
    ENDM
    CURSOR_BATCH batch6Raw,3,paged.raw6Helper
    CURSOR_BATCH batch6Pre,3,paged.pre6Helper
    IFNDEF FONT_BLOCK32
    CURSOR_BATCH batch8,4,paged.raw8Helper
    ELSE
batch8: ret
    ENDIF
; Private scalar callback: public hooks preserve AF/BC/DE/HL once.
; String heads/tails preserve only their live HL around this callback.
scalar8:
    ld e,c
    ld hl,(bios_vars.cursorY)
    ld a,h
    rrca
    rrca
    and 03Fh
    add a,090h
    ld d,a
    ld a,e
    ld e,l
    ld l,a
    ld a,e
    sub 7
    ld e,a
    ld a,h
    add a,4
    ld (bios_vars.cursorX),a
    ld h,PAGED_FONT_BASE >> 8
    DUP 7
    ld a,(hl)
    ld (de),a
    inc h
    inc e
    EDUP
    ld a,(hl)
    ld (de),a
    ret
scalar:
scalarSix:
    IFDEF FONT_BLOCK32
    ld d,FONT_ADDR >> 8
    ELSE
    ld d,PAGED_FONT_BASE >> 8
    ENDIF
    ld e,c
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
    jp scalar6Paged.scalar6Draw
empty: ret
copy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,copy
    ret
variant: DB 0
preEnabled: DB 0
    IFDEF FONT_BLOCK32
    INCLUDE "engines/block32.inc"
    INCLUDE "engines/block-adapter.inc"
    ELSE
    INCLUDE "engines/paged.inc"
    ENDIF
    DEFINE SCALAR6_PAGED
    INCLUDE "engines/paged-scalar.inc"
    ASSERT $ <= 07C00h
end:
