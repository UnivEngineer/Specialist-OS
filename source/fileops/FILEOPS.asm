; Lazy DOS file-operation engine. Native MXOS container, DRIVER placement.
    INCLUDE "../include/mxos.inc"
    DEFINE BANKED_DOS_BUILD 1
KD_OBJECT = DOS_BANK_OBJECT_ADDR
    INCLUDE "../../bin/fileops-imports.inc"
    ORG DOS_BANK_DRIVER_ADDR
    jp foEntry
    INCLUDE "host.inc"
foEntry:
    cp 11
    jp z,foDeleteSync
    cp 12
    jp z,foGetPath
    cp 13
    jp z,foSkipPrepare
    cp 10
    jp z,foMkdir
    cp 4
    jp nc,foControl
    push af
    xor a
    ld (foAsync),a
    pop af
    push af
    push hl
    call foContextSave
    pop hl
    pop af
    call foSync
    push af
    call foContextRestore
    pop af
    ret
foSync:
    push af
    xor a
    ld (foPathText),a
    pop af
    cp 2
    jp nc,foInternal
    or a
    jp z,foCopy
    jp foCopyForMove
foInternal:
    sub 2
foInternalMode:
    ld (treeMoveMode),a
    xor a
    ld (v_dosError),a
    ld (v_copyRequest),hl
    ld e,FILE_COPY_REQUEST.bufferSize
    call fc_ReadWord
    ld a,h
foCapacity:
    ld (v_copyCapacity),a
    jp fc_RangeReady
    INCLUDE "step.inc"
    INCLUDE "copy.inc"
    INCLUDE "tree.inc"
    INCLUDE "confirm.inc"
    INCLUDE "mkdir.inc"
    INCLUDE "prepare.inc"
    INCLUDE "delete.inc"
fileopsEnd:
    ASSERT $ < DOS_BANK_DRIVER_END
    END
