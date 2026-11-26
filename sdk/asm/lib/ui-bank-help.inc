    INCLUDE "../abi/ui-gfx.inc"
; HL = local ASCIIZ path, immediately preceded by the background palette.
; UI_HELP_PIXELS is a caller-owned MAIN bitmap buffer, or 0 for redraw.
; Like uiShowHelp, returns CF/A on failure.
; UI_HELP_HANDLE, UI_HELP_REGS and UI_HELP_BANK belong to the active client.
uiShowHelp:
    push hl
    ld c,0
uiHelpPathLength:
    ld a,(hl)
    inc hl
    inc c
    or a
    jp z,uiHelpPathReady
    ld a,c
    cp MAX_PATH
    jp c,uiHelpPathLength
    pop hl
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
uiHelpPathReady:
    pop hl
    ld a,(UI_HELP_BANK)
    ld b,a
    ld de,UI_HELP_PIXELS
    ld a,GFX_HELP_OPEN
    call uiHelpBankCall
    ret c
uiHelpRead:
    call UI_HELP_KEY
    ld c,a
    ld a,0
    adc a,0
    ld b,a
    ld a,GFX_HELP_KEY
    call uiHelpBankCall
    ret c
    or a
    jp z,uiHelpRead
    xor a
    ret
uiHelpBankCall:
    ld (UI_HELP_REGS),hl
    ex de,hl
    ld (UI_HELP_REGS+2),hl
    ld h,b
    ld l,c
    ld (UI_HELP_REGS+4),hl
    ld (UI_HELP_REGS+7),a
    ld hl,(UI_HELP_HANDLE)
    ld de,UI_HELP_REGS
    jp mx_routeCall
