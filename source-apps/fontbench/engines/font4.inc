; Три формата 4x8: байт на строку, два глифа в байте, повторённый полубайт.
; Нативный стенд: таблица 3000h, без менеджера памяти и нового API.
FONT4_BASE = 03000h
font4RestorePhase0:
    ld hl,02800h
    ld de,FONT4_BASE
    ld bc,2048
font4RestoreByte:
    ld a,(hl)
    add a,a
    add a,a
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,font4RestoreByte
    ret
; A=формат 0/1/2. Распаковка настоящего шрифта CP/M вне таймера.
font4Prepare:
    ld (font4Format),a
    call font4SelectFormat
    ld hl,font4Source
    ld de,FONT4_BASE
    ld b,128
font4UnpackPair:
    push bc
    ld c,8
font4UnpackHigh:
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 00Fh
    ld (de),a
    inc hl
    inc de
    dec c
    jp nz,font4UnpackHigh
    ld bc,-8
    add hl,bc
    ld c,8
font4UnpackLow:
    ld a,(hl)
    and 00Fh
    ld (de),a
    inc hl
    inc de
    dec c
    jp nz,font4UnpackLow
    pop bc
    dec b
    jp nz,font4UnpackPair
    ld a,(font4Format)
    cp 2
    jp nz,font4PreparePacked
    ld hl,FONT4_BASE
    ld bc,2048
font4RepeatByte:
    ld a,(hl)
    ld d,a
    rrca
    rrca
    rrca
    rrca
    or d
    ld (hl),a
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,font4RepeatByte
    jp font4Transpose
font4PreparePacked:
    ld a,(font4Format)
    cp 1
    jp nz,font4Transpose
    ld hl,FONT4_BASE
    ld de,FONT4_BASE
    ld b,128
font4PackPair:
    push bc
    ld b,8
font4PackRow:
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    ld c,a
    push hl
    ld a,l
    add a,8
    ld l,a
    ld a,(hl)
    or c
    pop hl
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,font4PackRow
    ld a,l
    add a,8
    ld l,a
    jp nc,font4PackNoCarry
    inc h
font4PackNoCarry:
    pop bc
    dec b
    jp nz,font4PackPair
    ret

font4Scalar:
    push af
    push bc
    push de
    push hl
    ld a,c
    ld (font4Code+1),a
    ld c,1
    call font4Validate
    jp c,font4Error
    ld hl,font4Return
    ld (font4SingleContinue+1),hl
font4Code:
    ld a,0
    jp font4Single
    IFDEF FONT_ENGINE_DLL
; Trusted ABI scalar wrapper already owns register preservation. Keep the
; original coordinate validation; only the duplicate PUSH/POP set is removed.
font4ScalarHelper:
    ld a,c
    ld (font4Code+1),a
    ld c,1
    call font4Validate
    ret c
    ld hl,font4ScalarDone
    ld (font4SingleContinue+1),hl
    jp font4Code
font4ScalarDone:
    ret
    ENDIF
font4Batch:
    push af
    push bc
    push de
    push hl
    ld a,c
    or a
    jp z,font4Return
    call font4Validate
    jp c,font4Error
    ld (font4Remaining),a
    ld (font4Read+1),hl
    ld a,(bios_vars.cursorX)
    and 2
    jp z,font4BatchAligned
    ld hl,font4HeadDone
    ld (font4SingleContinue+1),hl
    call font4Read
    jp font4Single
font4HeadDone:
    ld hl,font4Remaining
    dec (hl)
    jp z,font4Return
font4BatchAligned:
    ld hl,font4Return
    ld (font4SingleContinue+1),hl
    ld a,(font4Remaining)
    cp 2
    jp c,font4BatchTail
    and 1
    ld (font4Tail),a
    ld a,(font4Remaining)
    and 0FEh
    ld b,a
    add a,a
    ld hl,(bios_vars.cursorY)
    add a,h
    ld (font4EndX),a
    ld a,b
    or a
    rra
    ld (font4Pairs),a
    ld a,h
    rrca
    rrca
    add a,090h
    ld d,a
    ld a,l
    sub 7
    ld e,a
    ; Как в CP/M: SP читает пары кодов, до восстановления стека нет CALL/PUSH.
    ld hl,0
    add hl,sp
    ld (font4SavedSP+1),hl
    ld hl,(font4Read+1)
    ld sp,hl
font4BatchCore:
    jp font4RawLoop
font4RawLoop:
    pop hl
    ld c,l
    ld l,h
    ld h,FONT4_BASE/256
    ld b,h
font4PairOrdinary:
    DUP 7
        ld a,(bc)
        rrca
        rrca
        rrca
        rrca
        or (hl)
        ld (de),a
        inc b
        inc h
        inc e
    EDUP
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    or (hl)
    ld (de),a
    jp font4FastNext
font4RepeatLoop:
    pop hl
    ld c,l
    ld l,h
    ld h,FONT4_BASE/256
    ld b,h
font4PairRepeated:
    DUP 7
        ld a,(bc)
        xor (hl)
        and 0F0h
        xor (hl)
        ld (de),a
        inc b
        inc h
        inc e
    EDUP
    ld a,(bc)
    xor (hl)
    and 0F0h
    xor (hl)
    ld (de),a
    ; DUP falls straight into the shared pair continuation: no JMP per pair.
font4FastNext:
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,font4Pairs
    dec (hl)
font4FastContinue:
    jp nz,font4RawLoop
    ld hl,0
    add hl,sp
    ld (font4Read+1),hl
font4SavedSP:
    ld sp,0
    ld a,(font4EndX)
    ld (bios_vars.cursorX),a
    ld a,(font4Tail)
    or a
    jp z,font4Return
font4BatchTail:
    call font4Read
    jp font4Single
font4PackedLoop:
    pop hl
    ld a,h
    and 1
    ld b,a
    ld a,l
    and 1
    add a,a
    or b
    ld (font4Parity),a
    ld c,l
    ld a,h
    rlca
    rlca
    ld h,a
    and 0F8h
    ld l,a
    ld a,h
    and 3
    add a,FONT4_BASE/256
    ld h,a
    ld a,c
    rlca
    rlca
    ld c,a
    and 3
    add a,FONT4_BASE/256
    ld b,a
    ld a,c
    and 0F8h
    ld c,a
    ld a,(font4Parity)
    or a
    jp z,font4PairUpper
    dec a
    jp z,font4PairPackedMerge
    dec a
    jp z,font4PairResult
    jp font4PairLower
font4PairPackedMerge:
    DUP 7
        ld a,(bc)
        xor (hl)
        and 0F0h
        xor (hl)
        ld (de),a
        inc c
        inc l
        inc e
    EDUP
    ld a,(bc)
    xor (hl)
    and 0F0h
    xor (hl)
    ld (de),a
    jp font4FastNext
font4PairLower:
    DUP 7
        ld a,(bc)
        rrca
        rrca
        rrca
        rrca
        xor (hl)
        and 0F0h
        xor (hl)
        ld (de),a
        inc c
        inc l
        inc e
    EDUP
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    xor (hl)
    and 0F0h
    xor (hl)
    ld (de),a
    jp font4FastNext
font4PairResult:
    DUP 7
        ld a,(bc)
        xor (hl)
        and 00Fh
        xor (hl)
        rrca
        rrca
        rrca
        rrca
        ld (de),a
        inc c
        inc l
        inc e
    EDUP
    ld a,(bc)
    xor (hl)
    and 00Fh
    xor (hl)
    rrca
    rrca
    rrca
    rrca
    ld (de),a
    jp font4FastNext
font4PairUpper:
    ld a,h
    ld h,b
    ld b,a
    ld a,l
    ld l,c
    ld c,a
    DUP 7
        ld a,(bc)
        rrca
        rrca
        rrca
        rrca
        xor (hl)
        and 00Fh
        xor (hl)
        ld (de),a
        inc c
        inc l
        inc e
    EDUP
    ld a,(bc)
    rrca
    rrca
    rrca
    rrca
    xor (hl)
    and 00Fh
    xor (hl)
    ld (de),a
    jp font4FastNext
font4Return:
    pop hl
    pop de
    pop bc
    pop af
    or a
    ret
font4Error:
    pop hl
    pop de
    pop bc
    pop af
    scf
    ret
font4Read:
    ld hl,0
    ld a,(hl)
    inc hl
    ld (font4Read+1),hl
    ret
font4Pointer:
    jp font4PointerPlane
font4PointerPlane:
    ld l,a
    ld h,FONT4_BASE/256
    ret
font4PointerPacked:
    rlca
    rlca
    ld h,a
    and 0F8h
    ld l,a
    ld a,h
    and 3
    add a,FONT4_BASE/256
    ld h,a
    ret
font4Single:
    ld (font4SingleCode),a
    call font4Pointer
    ex de,hl
    ld hl,(bios_vars.cursorY)
    ld a,h
    and 2
    ld b,a
    ld c,0F0h
    jp z,font4SingleMask
    ld c,00Fh
font4SingleMask:
    ld a,h
    rrca
    rrca
    and 03Fh
    add a,090h
    ld h,a
    ld a,l
    sub 7
    ld l,a
font4SingleFormat:
    jp font4SingleRaw
font4SingleRaw:
    ld a,2
    jp font4SinglePhase
font4SinglePacked:
    ld a,(font4SingleCode)
    and 1
    add a,a
font4SinglePhase:
    xor b
    jp z,font4SinglePlain
    call font4SingleRotated
    jp font4SingleDone
font4SinglePlain:
    call font4SinglePixels
font4SingleDone:
    ld a,(bios_vars.cursorX)
    add a,2
    ld (bios_vars.cursorX),a
font4SingleContinue:
    jp font4Return
; Длина 0..48, X кратен четырём пикселям; правый край и все восемь строк.
font4Validate:
    push hl
    ld a,c
    cp 49
    jp nc,font4Invalid
    ld hl,(bios_vars.cursorY)
    ld a,h
    and 1
    jp nz,font4Invalid
    ld a,l
    cp 7
    jp c,font4Invalid
    ld a,c
    add a,a
    add a,h
    jp c,font4Invalid
    cp 193
    jp nc,font4Invalid
    ld a,c
    pop hl
    or a
    ret
font4Invalid:
    pop hl
    scf
    ret
font4SinglePixels:
    ld a,(de)
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SinglePixelsAdvance0:
    inc d
    inc l
    ld a,(de)
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SinglePixelsAdvance1:
    inc d
    inc l
    ld a,(de)
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SinglePixelsAdvance2:
    inc d
    inc l
    ld a,(de)
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SinglePixelsAdvance3:
    inc d
    inc l
    ld a,(de)
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SinglePixelsAdvance4:
    inc d
    inc l
    ld a,(de)
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SinglePixelsAdvance5:
    inc d
    inc l
    ld a,(de)
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SinglePixelsAdvance6:
    inc d
    inc l
    ld a,(de)
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
    ret
font4SingleRotated:
    ld a,(de)
    rrca
    rrca
    rrca
    rrca
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SingleRotatedAdvance0:
    inc d
    inc l
    ld a,(de)
    rrca
    rrca
    rrca
    rrca
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SingleRotatedAdvance1:
    inc d
    inc l
    ld a,(de)
    rrca
    rrca
    rrca
    rrca
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SingleRotatedAdvance2:
    inc d
    inc l
    ld a,(de)
    rrca
    rrca
    rrca
    rrca
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SingleRotatedAdvance3:
    inc d
    inc l
    ld a,(de)
    rrca
    rrca
    rrca
    rrca
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SingleRotatedAdvance4:
    inc d
    inc l
    ld a,(de)
    rrca
    rrca
    rrca
    rrca
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SingleRotatedAdvance5:
    inc d
    inc l
    ld a,(de)
    rrca
    rrca
    rrca
    rrca
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
font4SingleRotatedAdvance6:
    inc d
    inc l
    ld a,(de)
    rrca
    rrca
    rrca
    rrca
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
    ret
; Выбор всех трёх путей один раз при установке таблицы, вне измерений.
font4SelectFormat:
    ld hl,font4RawLoop
    ld de,font4PointerPlane
    ld bc,font4SingleRaw
    ld a,(font4Format)
    or a
    jp z,font4SelectReady
    cp 2
    ld hl,font4RepeatLoop
    ld bc,font4SinglePlain
    jp z,font4SelectReady
    ld hl,font4PackedLoop
    ld de,font4PointerPacked
    ld bc,font4SinglePacked
font4SelectReady:
    ld (font4BatchCore+1),hl
    ld (font4FastContinue+1),hl
    ex de,hl
    ld (font4Pointer+1),hl
    ld h,b
    ld l,c
    ld (font4SingleFormat+1),hl
    ld a,(font4Format)
    cp 1
    ld a,014h
    jp nz,font4SelectAdvance
    ld a,01Ch
font4SelectAdvance:
    ld (font4SinglePixelsAdvance0),a
    ld (font4SinglePixelsAdvance1),a
    ld (font4SinglePixelsAdvance2),a
    ld (font4SinglePixelsAdvance3),a
    ld (font4SinglePixelsAdvance4),a
    ld (font4SinglePixelsAdvance5),a
    ld (font4SinglePixelsAdvance6),a
    ld (font4SingleRotatedAdvance0),a
    ld (font4SingleRotatedAdvance1),a
    ld (font4SingleRotatedAdvance2),a
    ld (font4SingleRotatedAdvance3),a
    ld (font4SingleRotatedAdvance4),a
    ld (font4SingleRotatedAdvance5),a
    ld (font4SingleRotatedAdvance6),a
    ret
; Строки по страницам, как в CP/M. Временный буфер фазы 4 восстанавливается.
font4Transpose:
    ld hl,FONT4_BASE
    ld de,04000h
    ld b,0
font4TransposeGlyph:
    ld c,8
font4TransposeRow:
    ld a,(hl)
    ld (de),a
    inc hl
    inc d
    dec c
    jp nz,font4TransposeRow
    ld d,040h
    inc e
    dec b
    jp nz,font4TransposeGlyph
    ld hl,04000h
    ld de,FONT4_BASE
    ld bc,2048
    call bench_copy
    ld a,1
    ld (packedOverlay),a
    jp font4RestorePackedOverlay
font4Format: DB 0
font4Remaining: DB 0
font4SingleCode: DB 0
font4Parity: DB 0
font4Pairs: DB 0
font4Tail: DB 0
font4EndX: DB 0
; После резервного FONTKOI 4072 восстанавливаем фазу 4 перед следующим тестом.
font4RestorePackedOverlay:
    ld a,(packedOverlay)
    or a
    ret z
    xor a
    ld (packedOverlay),a
    ld hl,02800h
    ld de,04000h
    ld b,8
font4RestorePhase4Byte:
    ld a,(hl)
    rrca
    rrca
    ld c,a
    and 00Fh
    ld (de),a
    inc de
    ld a,c
    and 0C0h
    ld (de),a
    inc de
    inc hl
    ld a,l
    or a
    jp nz,font4RestorePhase4Byte
    dec b
    jp nz,font4RestorePhase4Byte
    ret
packedOverlay: DB 0
