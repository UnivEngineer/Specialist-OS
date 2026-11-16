; The module owns the paged layout; no per-glyph foreign-font probe.
scalar:
    ld d,FONT_ADDR >> 8
    ld e,c
    call printChar_position
    jp scalar6.scalar6Draw
    INCLUDE "scalar-position.inc"
