;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F8 - удаление файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F8:
            call ncRealSelection
            jp c,mainLoop
            call buildSelectedFilePath
            jp c,mainLoop
            ld hl,ncDeleteModal
            xor a
            ld bc,UI_FOCUS_PAIR*256
            call uiModalFocusInit
            ld hl,ncDeleteModal
            call ncModalOpen

            ; Выводим "DELETE FROM A:"
            ld hl,state.panelA_drive
            call readBytePanel
            add a,'A'
            ld (aDeleteFrom+12),a
            ld hl,aDeleteFrom
            call uiModalTitle

            ld hl,vars.pathArg
            ld de,vars.pathArg+MAX_PATH
            ld bc,vars.input
            call mon2_memcpy_bc_hl
            ld hl,240
            call ncProgressFieldWidth
            ld hl,vars.pathArg
            call compactPath
            call uiModalBodyText

ncDeleteKey:
            call uiModalChoose
            jp c,ncRefreshDirty
ncDeleteConfirm:
            or a
            jp nz,ncRefreshDirty
            ld          hl, state.panelA_drive
            call        readBytePanel
            MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
            jp c,       deleteFileError
            call restorePanelDirectory
            jp c,       deleteFileError
            ld hl,state.panelA_drive
            call readBytePanel
            ld (vars.copyRequest.sourceDrive),a
            ld a,DIR_OP_GET
            MXOS_CALL_REGS MXOS_DOS_DIRECTORY
            ex de,hl
            ld (vars.copyRequest.sourceDirectory),hl
            call getSelectedFile
            ld de,vars.copyRequest.source
            ld b,11
ncDeleteName:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            dec b
            jp nz,ncDeleteName
            ld a,CLI_OP_DELETE
            call ncFileops
            jp c,deleteFileJobError
            ; Количество файлов и курсор обновляются по реальному каталогу.
            jp          ncRefreshDirty

deleteFileError:
            MXOS_CALL_REGS MXOS_DOS_GET_ERROR
deleteFileJobError:
            call dosErrorText
            jp copyFileShowError

ncDeleteModal:
    UI_MODAL_BEGIN 12,1,2
    UI_MODAL_BUTTON ncInputOkText,0,0,1
    UI_MODAL_BUTTON ncInputCancelText,0,1,0
    UI_MODAL_END
