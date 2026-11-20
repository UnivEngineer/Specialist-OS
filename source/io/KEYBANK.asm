; MX keyboard. Only an edge, a due repeat, or a requested caret change enters MAIN.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    INCLUDE "../../bin/banked/key-imports.inc"
    ORG IO_BANK_KEYS
    jp keyGet
    jp keyPoll
    jp keyRaw
    jp keyGetCursor
    ASSERT $ == io_bankKeyModifiers
    jp keyReadModifiers
    ASSERT $ == io_bankKeyShift
    ; Shift is wired directly to active-low PB1. Normal keyboard mode has B as input.
    ld a,(IO_KEYB_B)
    cpl
    rrca
    and IO_KEY_MOD_SHIFT
    ret

    ASSERT $ == io_bankKeyTicks
    jp keyTicks
    ASSERT $ == io_bankClockRead
keyClockRead:
    push de
    ld hl,(keyBias)
    ex de,hl
    ld hl,IO_TIMER_MODE
    ld (hl),080h
    dec hl
    ld a,(hl)
    ld h,(hl)
    ld l,a
    add hl,de
    pop de
    ret
; Local elapsed-time clock, modulo 256. No bank switch, no timer setup.
keyTicks:
    push hl
    push de
    ld de,IO_KEY_STATE.timed
    add hl,de
    ld a,(hl)
    or a
    jp z,keySoftwareClock
    call keyClockRead
    ld a,h
    cpl
    jp keyClockDone
keySoftwareClock:
    inc hl
    inc (hl)
    inc hl
    jp nz,keySoftRead
    inc (hl)
keySoftRead:
    ld a,(hl)
keyClockDone:
    pop de
    pop hl
    ret

; Isolate the ПС contact (column PA3, row PB2). Sample Shift in the same read.
; Restore the standard 82h matrix configuration; A and flags report the mask.
keyReadModifiers:
    ld a,083h
    ld (IO_KEYB_MODE),a
    ld a,0F7h
    ld (IO_KEYB_A),a
    ld a,(IO_KEYB_B)
    cpl
    rrca
    and IO_KEY_MOD_SHIFT | IO_KEY_MOD_CTRL
    push af
    call keyMode82
    pop af
    ret

; Mode 82 selects all columns. FF means no ordinary key (Shift is independent).
keyMode82:
    ld a,082h
    ld (IO_KEYB_MODE),a
    ret
keyPresent:
    call keyMode82
    ld a,(IO_KEYB_B)
    or 3
    cp 0FFh
    ret z
    cp 0FBh
    ret nz
    ; Row 2 alone may be the Ctrl/ПС contact. Leave all other columns
    ; selected to detect a real key in that row, even with Ctrl held.
    ld a,8
    ld (IO_KEYB_A),a
    ld a,(IO_KEYB_B)
    or 3
    push af
    xor a
    ld (IO_KEYB_A),a
    pop af
    cp 0FFh
    ret

keyRaw:
    call keyPresent
    ret z
    push hl
    push bc
    push de
    ld hl,bios_keyScan
    xor a
    call bankCall
    jp keyReturn

; HL = caller's IO_KEY_STATE. NZ = event (including F1=0), CF = Ctrl.
keyGet:
    call keyPoll
    jp z,keyGet
    ret
keyPoll:
    push hl
    push bc
    push de
    ld b,0
    jp keySample

; Like keyPoll but ask BIOS to hide a visible caret when it delivers a key.
keyPollCursor:
    push hl
    push bc
    push de
    ld b,e
keySample:
    call keyPresent
    jp z,keyReleased
    ld a,(hl)
    cp 0FFh
    jp z,keyNew
    ; Test just the previously held physical switch. If it was replaced
    ; between two polls, dispatch the new press without waiting for repeat.
    push hl
    ld de,IO_KEY_STATE.column
    add hl,de
    ld a,(hl)
    ld (IO_KEYB_A),a
    inc hl
    ld a,(hl)
    ld (IO_KEYB_C),a
    inc hl
    ld a,(IO_KEYB_B)
    and (hl)
    call keyMode82
    jp nz,keyChanged
    inc hl                  ; repeatStart follows rowMask
    ld d,(hl)
    pop hl
    call keyTicks
    sub d
    inc hl
    cp (hl)
    dec hl
    jp c,keyEmpty
    ld a,(hl)
    cp 081h
    jp z,keyEmpty           ; language lock changes once per press
    ld c,KEY_REPEAT_NEXT
    jp keyRequest
keyChanged:
    pop hl
keyNew:
    ld c,KEY_REPEAT_FIRST
keyRequest:
    push hl
    inc hl
    ld (hl),c
    inc hl
    ld e,(hl)               ; E = private lock bits, D = KOI7 mode
    inc hl
    ld d,(hl)
    ld hl,bankKeyEvent
    xor a
    call bankCall
    ; HL = physical column, C = row number, B = raw key, E = locks.
    ex (sp),hl
    ld (hl),b
    inc hl
    inc hl
    ld (hl),e
    pop de
    push af
    inc b
    jp z,keyResult
    inc hl
    inc hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (hl),c               ; row mask supplied by the decoder
    inc hl
    push hl
    inc hl                  ; timed
    ld a,(hl)
    or a
    ld de,IO_KEY_STATE.repeatCount-IO_KEY_STATE.timed
    add hl,de
    jp z,keyStamp
    ld a,(hl)
    cp KEY_REPEAT_FIRST
    call z,keyClick
keyStamp:
    dec hl
    call keyTicks
    pop hl
    ld (hl),a
keyResult:
    pop af
    jp keyReturn
keyReleased:
    ld (hl),a
keyEmpty:
    xor a
keyReturn:
    pop de
    pop bc
    pop hl
    ret

; Optional built-in blink at the current BIOS text position. E holds actual
; visibility, so a disabled/zero-height cursor never leads to periodic calls.
keyGetCursor:
    push hl
    push bc
    push de
    ld e,0
    call keyTicks
    sub KEY_CURSOR_HALF
    ld c,a
keyCursorWait:
    call keyPollCursor
    jp nz,keyReturn
    call keyTicks
    sub c
    cp KEY_CURSOR_HALF
    jp c,keyCursorWait
    push hl
    ld a,e
    xor 1
    ld b,a
    ld hl,bankCursorSet
    xor a
    call bankCall
    pop hl
    ld e,a
    cp b
    jp nz,keyCursorOff
    call keyTicks
    ld c,a
    jp keyCursorWait
keyCursorOff:
    call keyGet
    jp keyReturn
; Same down counter before, during and after the hardware one-shot.
; Offset is private to the current bank. Only the timer owner may use it.
keyBias: DW 0

keyClick:
    push hl
    call keyClockRead
    ld de,-KEY_CLICK_TICKS
    add hl,de
    ld (keyBias),hl
    ld hl,IO_TIMER_CH2      ; reload mode 0, OUT2 goes low until terminal count
    ld (hl),KEY_CLICK_TICKS
    ld (hl),0
    pop hl
    ret
    ASSERT $ <= MEM_IMAGE_LOCAL_START
