; MXOS
; Банковый ввод и состояние закреплённого потока клавиатуры.

    IFNDEF MXOS_ABI_KEYBOARD_INC
    DEFINE MXOS_ABI_KEYBOARD_INC

    INCLUDE "timer.inc"

KEY_REPEAT_FIRST = 12  ; clock units of 32.768 ms at 2 MHz
KEY_REPEAT_NEXT  = 2
KEY_CURSOR_HALF  = 15

IO_KEY_MOD_SHIFT = 1
IO_KEY_MOD_CTRL  = 2
; Private state of a keyboard consumer in DOS/EXEC. Instantiate once per input stream.
    STRUCT IO_KEY_STATE
lastKey         DB 0FFh
repeatCount     DB 0
locks           DB 0    ; bit 7 Caps Lock, bit 0 Russian layout
koi7            DB 0    ; 0 = KOI8, FFh = KOI7
column          DW 0    ; driven column of held switch
rowMask         DB 0
repeatStart     DB 0    ; timestamp of the last reported press/repeat
timed           DB 0    ; positive after TIMER_CLOCK succeeds
softDivider     DB 0    ; fallback only, when the clock is unavailable
softTicks       DB 0
    ENDS

    ASSERT IO_KEY_STATE == 11

; Для DOS/EXEC. HL = IO_KEY_STATE там, где входу нужно состояние.
; Poll возвращает Z при отсутствии события, иначе A = код, CF = Ctrl/ПС.
io_bankGetCh        = 0F040h
io_bankKeyPoll      = 0F043h
io_bankKeyScan      = 0F046h
io_bankGetChCursor  = 0F049h
io_bankKeyModifiers = 0F04Ch
io_bankKeyShift     = 0F04Fh
io_bankKeyTicks     = 0F057h  ; HL=IO_KEY_STATE -> A, сохраняет BC/DE/HL
io_bankClockRead    = 0F05Ah  ; HL = счёт с поправкой на щелчок, 128 мкс при 2 МГц
IO_BANK_KEYS        = io_bankGetCh

    MACRO MXOS_KEY_BEGIN state
        ld a,(state+IO_KEY_STATE.timed)
        or a
        jp nz,.ready
        ld a,TIMER_CLOCK
        MXOS_CALL_REGS MXOS_DOS_TIMER
        sbc a,a
        inc a
        ld (state+IO_KEY_STATE.timed),a
        ld a,0FFh
        ld (state+IO_KEY_STATE.lastKey),a
.ready:
    ENDM

    MACRO MXOS_KEY_END state
        ld a,(state+IO_KEY_STATE.timed)
        or a
        jp z,.done
        xor a
        ld (state+IO_KEY_STATE.timed),a
        ld a,TIMER_RELEASE
        MXOS_CALL_REGS MXOS_DOS_TIMER
.done:
    ENDM

    ENDIF
