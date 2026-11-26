; Raw PRE payload: exact expected byte count, no checksums or staging copy.
; On failure caller regenerates all planes before publishing the new module.
checkWorkspace:
    ld hl,0
    ld bc,FONT_INIT_END
    ld a,MEM_OP_MAINCLAIM
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret c
    ld hl,FONT_INIT_END
    ld (workspaceEnd),hl
    ld de,FONT_INIT_END-1
    call bios_getMemTop
    call bios_cmp_hl_de
    ret nc
    ld a,MEM_ERROR_NOMEM
    scf
    ret
loadPrepared:
    ld a,(useFile)
    or a
    jp z,generateResources
    ld hl,(staging+IMAGE_HEADER_SIZE+44)
    ld a,h
    or l
    jp z,badImage
    ld (preparedIO+FILE_PATH_IO.count),hl
    ld hl,(lowerBase)
    ld (preparedIO+FILE_PATH_IO.buffer),hl
    ld hl,(request+FILE_PATH_IO.path)
    ld de,preparedPath
    call copyPath
    ld hl,preparedPath
    call findExtension
    ld de,preExtension
    call copyToHL
    ld hl,preparedIO
    FONT_STAMP 16
    call loadPath
    FONT_STAMP 18
    ret c
    ld hl,(staging+IMAGE_HEADER_SIZE+44)
    ex de,hl
    ld hl,(preparedIO+FILE_PATH_IO.actual)
    call bios_cmp_hl_de
    jp nz,badImage
    xor a
    ret
generateResources:
    ld hl,(lowerBase)
    ld (initContext+4),hl
    ld hl,initContext
    ld a,1
    jp FONT_INIT_ADDR
workspaceEnd:   DW 0
argTail:        DW 0
useFile:        DB 0
preparedError:  DB 0
preparedPath:   DS MAX_PATH,0
preExtension:   DB 'pre',0
preparedIO:     DW preparedPath,0,0,0
    DS FILE_DESCRIPTOR,0
