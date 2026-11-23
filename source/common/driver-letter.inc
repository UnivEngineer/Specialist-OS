;----------------------------------------------------------------------------
; MXOS
;
; Parse an optional -LETTER argument at the start of a driver command tail.
; Input: HL = tail, C = default drive (0..7).
; Output: CF=0, A = drive index, HL = first unconsumed delimiter.
; CF=1, A = DOS_ERR_ARGUMENT on malformed input. B, DE and flags are scratch.
; C is preserved. The caller decides whether trailing options are supported.
;----------------------------------------------------------------------------
driverLetter:
    push hl
    call argNext
    jp c,driverLetterDefault
    ld a,b
    cp 7
    jp nz,driverLetterDefault
    push hl
    ld hl,driverLetterKey
driverLetterCompare:
    ld a,(de)
    and 0DFh
    cp (hl)
    jp nz,driverLetterSkip
    inc de
    inc hl
    dec b
    jp nz,driverLetterCompare
    pop hl
    pop de
    call argNext
    jp c,driverLetterBad
    ld a,b
    dec a
    jp nz,driverLetterBad
    ld a,(de)
    and 0DFh
    sub 'A'
    cp 8
    jp nc,driverLetterBad
    or a
    ret
driverLetterSkip:
    pop hl
driverLetterDefault:
    pop hl
    ld a,c
    or a
    ret
driverLetterBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
driverLetterKey: DB 0Dh,"LETTER"

; Input: HL = tail, C = sole supported drive. Output: CF/A as driverLetter.
; Rejects extra arguments before the driver probes or modifies the device.
driverFixedLetter:
    call driverLetter
    ret c
    cp c
    jp nz,driverLetterBad
    call argNext
    jp nc,driverLetterBad
    ld a,c
    or a
    ret
