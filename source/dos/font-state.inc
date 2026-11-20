; FONT profile survives destructive MAIN applications and hardware Reset.
; REGISTER receives a NEAR command; PREPARE returns it to a caller buffer.
; Only BIOS restores it, before entering a shell. No saved renderer bitmap.
FONT_CONFIG_REGISTER = 0
FONT_CONFIG_PREPARE = 1
FONT_CONFIG_FINISH = 2
kdFontConfig:
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    ld a,(kdRegs+7)
    or a
    jp nz,kfcNotRegister
    ld de,KD_PATH
    ld bc,MAX_PATH
    call kdFetch
    jp c,kdBad
kfcNotRegister:
    ld a,(kdRegs+7)
    ld hl,KD_PATH
    call fontConfigService
    jp c,kdError
    or a
    jp z,kfcDone
    push af
    ld hl,KD_PATH
    push hl
    ld hl,(kdClientObject)
    ex de,hl
    pop hl
    ld bc,MAX_PATH
    call kdStore
    pop af
kfcDone:
    call kdCapture
    jp kdFinish
fontConfigService:
    or a
    jp z,fontConfigRegister
    dec a
    jp z,fontConfigPrepare
    dec a
    jp z,fontConfigFinish
    jp blockArgument
fontConfigRegister:
    ld a,(fontConfigRestoring)
    or a
    jp nz,fontConfigReinstall
    ld a,(hl)
    or a
    jp z,blockArgument
    push hl
    ld b,MAX_PATH
fontConfigEnd:
    ld a,(hl)
    or a
    jp z,fontConfigCommandReady
    inc hl
    dec b
    jp nz,fontConfigEnd
    pop hl
    jp blockArgument
fontConfigCommandReady:
    pop hl
    ld de,fontConfigCommand
    ld bc,MAX_PATH
    call moveBytesForward
    ld a,(v_drive)
    ld (fontConfigDrive),a
    ld hl,(v_directory)
    ld (fontConfigDirectory),hl
    ld hl,v_directories
    ld de,fontConfigDirectories
    ld bc,16
    call moveBytesForward
    ld a,1
    ld (fontConfigValid),a
    xor a
    ld (fontConfigPending),a
    ld b,0
    ret
fontConfigReinstall:
    ld b,1
    xor a
    ret
fontConfigPrepare:
    ld a,(fontConfigPending)
    or a
    ret z
    xor a
    ld (fontConfigPending),a
    ld a,(v_drive)
    ld (fontConfigReturnDrive),a
    ld hl,(v_directory)
    ld (fontConfigReturnDirectory),hl
    ld hl,v_directories
    ld de,fontConfigReturnDirectories
    ld bc,16
    call moveBytesForward
    ld a,(fontConfigDrive)
    call fileSelectDrive
    ret c
    ld hl,(fontConfigDirectory)
    ld (v_directory),hl
    ld hl,fontConfigDirectories
    ld de,v_directories
    ld bc,16
    call moveBytesForward
    ld hl,fontConfigCommand
    ld de,KD_PATH
    ld bc,MAX_PATH
    call moveBytesForward
    ld a,1
    ld (fontConfigRestoring),a
    or a
    ret
fontConfigFinish:
    ld a,(fontConfigRestoring)
    or a
    ret z
    xor a
    ld (fontConfigRestoring),a
    ld a,(fontConfigReturnDrive)
    call fileSelectDrive
    ld hl,(fontConfigReturnDirectory)
    ld (v_directory),hl
    ld hl,fontConfigReturnDirectories
    ld de,v_directories
    ld bc,16
    call moveBytesForward
fontConfigNone:
    xor a
    ret
fontConfigReset:
    ld a,(fontConfigValid)
    ld (fontConfigPending),a
    xor a
    ld (fontConfigRestoring),a
    jp memoryResidentReset
fontConfigValid = DOS_FONT_STATE_ADDR
fontConfigPending = fontConfigValid+1
fontConfigRestoring = fontConfigValid+2
fontConfigDrive = fontConfigValid+3
fontConfigDirectory = fontConfigValid+4
fontConfigDirectories = fontConfigValid+6
fontConfigReturnDrive = fontConfigValid+22
fontConfigReturnDirectory = fontConfigValid+23
fontConfigReturnDirectories = fontConfigValid+25
fontConfigCommand = fontConfigValid+41
    ASSERT fontConfigCommand+MAX_PATH <= DOS_BANK_OBJECT_ADDR
