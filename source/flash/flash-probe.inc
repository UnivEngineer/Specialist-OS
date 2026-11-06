; B=выбор физического гнезда, выход B=manufacturer/C=device.
; AF/DE/HL разрушены. ID всегда покидается, SDP и данные не меняются.
ParallelProbe:
    ld a,PPI_MODE_WRITE
    ld (IO_PROG_MODE),a
    ld c,090h
    call ProbeCommand
    call ProbeDelay
    ld a,PPI_MODE_READ
    ld (IO_PROG_MODE),a
    ld de,0
    call ProbeRead
    ld c,a
    inc e
    call ProbeRead
    push af
    push bc
    ld a,PPI_MODE_WRITE
    ld (IO_PROG_MODE),a
    ld c,0F0h
    call ProbeCommand
    call ProbeDelay
    ld a,MASK_STANDBY
    ld (IO_PROG_C),a
    pop bc
    pop af
    ld b,c
    ld c,a
    ret
ProbeCommand:
    ld de,05555h
    ld a,0AAh
    call ProbeWrite
    ld de,02AAAh
    ld a,055h
    call ProbeWrite
    ld de,05555h
    ld a,c
ProbeWrite:
    ld (IO_PROG_A),a
    call ProbeLatch
    ld a,e
    ld (IO_PROG_B),a
    ld a,b
    or MASK_STANDBY
    ld (IO_PROG_C),a
    ld a,WRITE_0
    ld (IO_PROG_MODE),a
    ld a,WRITE_1
    ld (IO_PROG_MODE),a
    ret
ProbeLatch:
    ld a,LATCH_1
    ld (IO_PROG_MODE),a
    ld a,d
    ld (IO_PROG_B),a
    ld a,LATCH_0
    ld (IO_PROG_MODE),a
    ret
ProbeRead:
    call ProbeLatch
    ld a,e
    ld (IO_PROG_B),a
    ld a,b
    or MASK_READ
    ld (IO_PROG_C),a
    ld a,(IO_PROG_A)
    ret
