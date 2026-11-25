; Enter a monitor itself, not an application beneath a compatibility kernel.
; Raw .MON profiles: 4K Monitor-2, 8K window monitor, split 6880-byte RAMFOS.
; DOS is used only before the image replaces C000h. The final stub is entirely
; in MAIN 8C00h, outside the image/font/IO and outside the video staging area.
monitorOpen:
    ld a,(appArgs)
    or a
    jp nz,argumentError
    ld hl,descriptor
    MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO
    jp c,notFound
    ld hl,(descriptor+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,badImage
    ld hl,(descriptor+FILE_DESCRIPTOR.size)
    ld (monitorSize),hl
    ld de,01000h
    call bios_cmp_hl_de
    ld a,0
    jp z,monitorProfile
    ld de,02000h
    call bios_cmp_hl_de
    ld a,1
    jp z,monitorProfile
    ld de,RAMFOS_IMAGE_SIZE
    call bios_cmp_hl_de
    jp nz,badImage
    ld a,2
monitorProfile:
    ld (monitorKind),a
    ; The staging buffer is video RAM. Hide both its writes and the old NC
    ; screen; error restores COLOR_BIOS before printing a diagnostic.
    xor a
    ld (IO_COLOR),a
    call bios_clearScreenOld
    ld hl,descriptor
    ld de,RAMFOS_TEMP
    call fileReadDescriptor
    jp c,notFound
    ; Reject unrelated files of the same length before modifying the kernel.
    ld a,(RAMFOS_TEMP+0800h)
    cp 0C3h
    jp nz,badImage
    ld a,(monitorKind)
    cp 2
    jp z,monitorCheckRF
    ld a,(RAMFOS_TEMP)
    cp 0C3h
    jp z,monitorLoaded
    cp 0C1h
    jp nz,badImage
    jp monitorLoaded
monitorCheckRF:
    ld a,(RAMFOS_TEMP+RAMFOS_CORE_SIZE)
    cp 0C3h
    jp nz,badImage
    ld a,(RAMFOS_TEMP+RAMFOS_CORE_SIZE+RAMFOS_MONITOR-0F800h)
    cp 0C3h
    jp nz,badImage
monitorLoaded:
    ld a,(callerDrive)
    ld e,1
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld hl,monitorBootImage
    ld de,monitorBootImage+monitorBootEnd-monitorBoot
    ld bc,RAMFOS_BRIDGE
    call bios_memcpy_bc_hl
    ld a,(monitorKind)
    ld (monitorBootKind+1),a
    ld hl,(monitorSize)
    cp 2
    jp nz,monitorLengthReady
    ld hl,RAMFOS_CORE_SIZE
monitorLengthReady:
    ld (monitorBootSize+1),hl
    jp monitorBoot
monitorKind: DB 0
monitorSize: DW 0

monitorBootImage:
    DISP RAMFOS_BRIDGE
monitorBoot:
    di
    ld sp,RAMFOS_BRIDGE_SP
    ld hl,RAMFOS_TEMP
    ld de,0C000h
monitorBootSize: ld bc,0
    call monitorMove
monitorBootKind: ld a,0
    cp 2
    jp nz,monitorSTD
    ld hl,RAMFOS_TEMP+RAMFOS_CORE_SIZE
    ld de,RAMFOS_HIGH_ADDR
    ld bc,RAMFOS_HIGH_SIZE
    call monitorMove
    jp RAMFOS_MONITOR ; already in MX, dedicated isolated shell entry
monitorSTD:
    push af
    ld a,0B6h
    ld (IO_TIMER_MODE),a
    ld a,076h
    ld (IO_TIMER_MODE),a
    ld a,036h
    ld (IO_TIMER_MODE),a
    ld a,00Ah
    ld (IO_KEYB_MODE),a
    ld a,3
    ld (IO_PAGE_STD),a
    ld (IO_COLOR-07FEh),a
    ld a,082h
    ld (0F803h),a
    xor a
    ld (0F802h),a
    ld a,082h
    ld (0FF03h),a
    pop af
    or a
    jp nz,0C800h ; window monitor initializes its own variables/windows
    call 0C444h ; Monitor-2 loader variables; bypass its patched POP BC entry
    jp 0C800h

;----------------------------------------------------------------------------
; MXOS
;
; Скопировать часть образа при установке сменного монитора.
;
; вход:
;   hl = начало источника, de = начало назначения, bc = ненулевая длина. При перекрытии
;   должно быть допустимо копирование вперёд.
;
; выход:
;   hl и de указывают за скопированными данными, bc = 0, a = 0, z = 1, cf = 0.
;----------------------------------------------------------------------------

monitorMove:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,monitorMove
    ret
monitorBootEnd:
    ASSERT $ <= RAMFOS_ARGS
    ENT
