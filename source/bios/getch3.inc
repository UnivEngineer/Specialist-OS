; Shared nonblocking decoder. B = raw matrix code, D = KOI7, E = locks.
; B preserved, E returns locks. Z = language key, NZ = event, A = code, CF = Ctrl.
keyDecode:
    ld a,b
    cp 081h
    jp z,keyToggle
    cp 021h
    jp c,keyModifiers
    cp 040h
    jp c,keyPunctuation
    cp 07Fh
    jp nc,keyModifiers
    ld a,(IO_KEYB_B)
    and 2
    rrca
    rrca                    ; bit 7 = Shift released
    xor e
    and 080h
    rrca
    rrca                    ; bit 5 = change case
    xor b
    ld c,a
    ld a,e
    rrca                    ; carry = Russian layout
    ld a,c
    jp nc,keyModifiers
    ld a,d
    or a
    ld a,c
    jp nz,keyRussianCode    ; keep the historical KOI-7 mapping
    xor 020h                ; KOI8-R Cyrillic lowercase starts at C0h
keyRussianCode:
    xor 080h
    jp keyModifiers
keyPunctuation:
    ld a,(IO_KEYB_B)
    and 2
    ld a,b
    jp nz,keyModifiers
    xor 010h
    cp 020h
    jp nz,keyModifiers
    ld a,05Fh
keyModifiers:
    ld c,a
    call setKeybMode83
    ld a,0F7h
    ld (IO_KEYB_A),a
    ld a,(IO_KEYB_B)
    cpl
    ld d,a
    call setKeybMode82
    ld a,1
    or a
    ld a,d
    rrca
    rrca
    rrca
    ld a,c
loc_C311:                   ; key ready in A, profiling breakpoint
    ret
keyToggle:
    ld a,(IO_KEYB_B)
    and 2
    jp z,keyCaps
    ld a,d
    or a
    ld a,1
    jp z,keyToggleLock
keyCaps:
    ld a,080h
keyToggleLock:
    xor e
    ld e,a
    xor a
    ret

; Private register entry via MEM bankCall. B = hide caret on delivery.
; The same matrix table and decoder serve both Monitor-2 and banked input.
bankKeyEvent:
    call keyScan2
    ld c,b                  ; remember requested hide
    ld b,a                  ; return raw key for repeat tracking
    cp 0FFh
    ret z                   ; released before reaching MAIN; Z=1, CF=0
    push bc
    call keyDecode
    pop hl                  ; H = raw, L = hide
    jp z,keySwitchCoords
    push af
    ld a,l
    or a
    call nz,bankCursorHide
    pop af
keySwitchCoords:
    push af
    ld a,(keyRow)
    ld c,a
    ld a,1
keyEventRowMask:
    rrca
    dec c
    jp nz,keyEventRowMask
    ld c,a
    ld hl,(keyColumn)
    pop af
    ret

; B = desired visibility, return actual in A. Callers finish/hide the
; outstanding caret before changing BIOS cursor geometry or overwriting it.
bankCursorHide:
    push bc
    ld b,0
    call bankCursorSet
    pop bc
    ret
bankCursorSet:
    ld a,(bankCaretVisible)
    cp b
    ret z
    ld a,b
    or a
    jp z,keyDrawCaret
    ld a,(bios_upper.cursorCfg)
    and 08Fh
    jp z,keyNoCaret
    jp m,keyNoCaret
keyDrawCaret:
    ld a,b
    ld (bankCaretVisible),a
    push af
    call drawCursor2
    pop af
    ret
keyNoCaret:
    xor a
    ret
keyColumn: DW 0
keyRow: DB 0
bankCaretVisible: DB 0
