; Resident facade. FILEOPS owns the recursive engine in the shared DOS arena.
; No user data or on-disk formats are changed by loading the engine.
cliDeleteTree:
    call fileFindAny
    ret c
    ld hl,v_tmpFileDescr
    ld de,cliRequest.source
    ld bc,FILE_DESCR_SIZE
    call cliCopyBytes
    ld a,(v_drive)
    ld (cliRequest.sourceDrive),a
    ld hl,(v_directory)
    ld (cliRequest.sourceDirectory),hl
    ld hl,cliRequest
    ld a,11
    call fileopsRun
    ret nc
    jp dosIoError
fileCopy:
    xor a
    jp fileopsRun
fileCopyForMove:
    ld a,1
fileopsRun:
    IFDEF BANKED_DOS_BUILD
    push af
    push hl
    call fileopsEnsure
    pop hl
    pop bc
    jp nc,fileopsRunReady
    ld a,FILE_OP_ERROR
    ret
fileopsRunReady:
    ld a,b
    inc a
    ld (fileopsActive),a
    ld a,b
    call DOS_FILEOPS_ADDR
    push af
    xor a
    ld (fileopsActive),a
    pop af
    ret
    ELSE
    jp blockArgument
    ENDIF

    IFDEF BANKED_DOS_BUILD
fileopsEnsure:
    ld a,(fileopsActive)
    or a
    jp nz,overlayBusy
    ld hl,fileopsOverlay
    jp overlayEnsure
    INCLUDE "overlays.inc"
    ENDIF
; Cross-volume MOVE consumes these results after FILEOPS returns.
treeMoveMode: DB 0
treeHandled: DB 0
copySkipped: DB 0
