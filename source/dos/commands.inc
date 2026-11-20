cliTextService:
    ex de,hl
    ld (cliCopyHook),hl
    ex de,hl
    call cliInteractiveCommand
    push af
    ld a,(cliOutput)
    ld b,a
    pop af
    ret
directoryCommand:
    push hl
    ld hl,0
    ld (cliCopyHook),hl
    pop hl
cliInteractiveCommand:
    xor a
    ld (cliQuery),a
    jp cliParseCommand
cliQueryService:
    call cliQueryCommand
    push af
    ld a,(cliOutput)
    ld b,a
    pop af
    ret
cliQueryCommand:
    ld a,1
    ld (cliQuery),a
cliParseCommand:
    xor a
    ld (cliOutput),a
    call cliCopyLine
    ret c
    ld hl,cliLine
    call cliSkipSpaces
    ld (cliArgs),hl
    ld a,(hl)
    or a
    jp z,cliSuccess
    inc hl
    ld a,(hl)
    cp ':'
    jp nz,cliFindCommand
    inc hl
    ld a,(hl)
    cp 021h
    jp c,cliDriveOnly
    cp 05Ch
    jp z,cliDriveRoot
    cp '/'
    jp nz,cliFindCommand
cliDriveRoot:
    inc hl
    ld a,(hl)
    cp 021h
    jp nc,cliFindCommand
    ld a,(cliQuery)
    or a
    jp nz,cliContextChanged
    call cliOnePath
    ret c
    call directoryPathUnchecked
    ret c
    jp cliContextChanged
cliDriveOnly:
    ld a,(cliQuery)
    or a
    jp nz,cliContextChanged
    ld hl,(cliArgs)
    ld a,(hl)
    and 0DFh
    sub 'A'
    call fileChangeDrive
    ret c
    jp cliContextChanged
cliFindCommand:
    ld de,cliCommands
cliNextCommand:
    ld a,(de)
    or a
    jp z,cliUnknown
    ld hl,(cliArgs)
cliMatchCommand:
    ld a,(de)
    inc de
    or a
    jp z,cliMatchEnd
    ld c,a
    ld a,(hl)
    and 0DFh
    cp c
    jp nz,cliSkipCommand
    inc hl
    jp cliMatchCommand
cliMatchEnd:
    ld a,(hl)
    cp 021h
    jp nc,cliSkipPointer
    call cliSkipSpaces
    ld (cliArgs),hl
    ld a,(de)
    inc de
    ld l,a
    ld a,(de)
    ld h,a
    inc de
    ld a,(de)
    cp 4
    jp z,cliAttrib
    cp 3
    jp c,cliCommandFlagsReady
    push hl
    ld hl,(cliArgs)
    ld b,a
    ld a,(hl)
    ld c,a
    ld a,b
    cp 5
    jp z,cliCdFlags
    ld a,c
    or a
    ld a,0
    jp nz,cliCommandFlagsPop
    inc a
    jp cliCommandFlagsPop
cliCdFlags:
    ld a,c
    or a
    ld a,4
    jp nz,cliCommandFlagsPop
    ld a,1
    jp cliCommandFlagsPop
cliCommandFlagsPop:
    pop hl
cliCommandFlagsReady:
    ld (cliOutput),a
    ld a,(cliQuery)
    or a
    jp nz,cliSuccess
    jp (hl)
cliSkipCommand:
    ld a,(de)
    inc de
    or a
    jp nz,cliSkipCommand
cliSkipPointer:
    inc de
    inc de
    inc de
    jp cliNextCommand
cliUnknown:
    ld a,1
    ld (cliOutput),a
    ld a,0FFh
    or a
    ret
cliSkipSpaces:
    ld a,(hl)
    cp ' '
    ret nz
    inc hl
    jp cliSkipSpaces
cliCopyLine:
    ld de,cliLine
    ld b,MAX_PATH-1
cliCopyLineLoop:
    ld a,(hl)
    cp 020h
    jp c,cliCopyLineEnd
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,cliCopyLineLoop
    ld a,(hl)
    cp 020h
    jp nc,blockArgument
cliCopyLineEnd:
    xor a
    ld (de),a
    ret
; HL=курсор, DE=путь MAX_PATH; HL продвинут, BC/AF разрушены.
cliWord:
    call cliSkipSpaces
    ld b,MAX_PATH-1
cliWordLoop:
    ld a,(hl)
    cp 021h
    jp c,cliWordEnd
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,cliWordLoop
    jp blockArgument
cliWordEnd:
    xor a
    ld (de),a
    ret
cliOnePath:
    ld hl,(cliArgs)
    ld de,cliPath1
    call cliWord
    ret c
    call cliSkipSpaces
    ld a,(hl)
    or a
    jp nz,blockArgument
    ld a,(cliPath1)
    or a
    jp z,blockArgument
    ld hl,cliPath1
    ret
cliTwoPaths:
    ld hl,(cliArgs)
    ld de,cliPath1
    call cliWord
    ret c
    ld de,cliPath2
    call cliWord
    ret c
    call cliSkipSpaces
    ld a,(hl)
    or a
    jp nz,blockArgument
    ld a,(cliPath1)
    or a
    jp z,blockArgument
    ld a,(cliPath2)
    or a
    jp z,blockArgument
    ret
cliCd:
    ld hl,(cliArgs)
    ld a,(hl)
    or a
    jp z,cliPwd
    call cliOnePath
    ret c
    call directoryPathUnchecked
    ret c
cliContextChanged:
    ld a,4
    ld (cliOutput),a
    jp cliSuccess
cliPwd:
    ld a,1
    ld (cliOutput),a
    ld hl,cliPath1
    call directoryCwdUnchecked
    ret c
    ld hl,cliPath1
    call bios_printString
    jp cliNewLine
cliMkdir:
    call cliOnePath
    ret c
    jp directoryCreate
cliRmdir:
    call cliOnePath
    ret c
    jp directoryRemove
cliEcho:
    ld hl,(cliArgs)
    ld de,cliEchoOffText
    ld c,4
    call stricmp
    jp z,cliEchoOff
    ld hl,(cliArgs)
    ld de,cliEchoOnText
    ld c,3
    call stricmp
    jp z,cliEchoOn
    ld a,1
    ld (cliOutput),a
    ld hl,(cliArgs)
    call bios_printString
    jp cliNewLine
cliEchoOn:
    ld a,1
    jp cliEchoSet
cliEchoOff:
    xor a
cliEchoSet:
    ld (v_batEcho),a
    xor a
    ld (cliOutput),a
    jp cliSuccess
cliEchoOffText: DB 'OFF',0
cliEchoOnText: DB 'ON',0
cliNewLine:
    ld c,0Ah
    call bios_printChar
    jp cliSuccess
cliCls:
    ld c,01Fh
    call bios_printChar
    jp cliSuccess
cliHelp:
    ld a,1
    ld (cliOutput),a
    ld hl,cliHelpText
    call bios_printString
    jp cliNewLine
cliExit:
    ld a,2
    ld (cliOutput),a
    ; EXIT возвращает из консоли в NC; BAT прекращает свой вызов EXIT.
    ld a,0
    IFDEF BANKED_DOS_BUILD
    call setShell
    ELSE
    MXOS_CALL MXOS_DOS_SET_SHELL
    ENDIF
    jp cliSuccess
; Файловые команды не меняют выбранные пользователем диск и каталог.
cliPrepareSource:
    ld hl,cliPath1
    ld de,cliRequest.source
    call fileNamePrepare
    ret c
    ld a,(v_drive)
    ld (cliRequest.sourceDrive),a
    ld hl,(v_directory)
    ld (cliRequest.sourceDirectory),hl
    ret
cliDelete:
    call cliOnePath
    ret c
    call cliRemember
    call cliPrepareSource
    jp c,cliFileDone
    ld hl,cliRequest.source
    call cliDeleteTree
    jp cliFileDone
cliRename:
    call cliTwoPaths
    ret c
    call cliRemember
    call cliPrepareRename
    jp c,cliFileDone
    ld a,(cliRequest.sourceDrive)
    ld b,a
    ld a,(cliRequest.destinationDrive)
    cp b
    jp nz,cliFileArgument
    ld hl,(cliRequest.destinationDirectory)
    ex de,hl
    ld hl,(cliRequest.sourceDirectory)
    call cmp_hl_de
    jp nz,cliFileArgument
    ld hl,cliRequest.source
    ld de,cliRequest.destination
    ld c,11
    call stricmp
    jp z,cliRenameReady
    ld hl,cliRequest.destination
    call fileFindAny
    jp nc,cliFileExists
    ld a,(v_dosError)
    cp DOS_ERR_NOT_FOUND
    jp nz,cliFileDone
cliRenameReady:
    ld hl,cliRequest.source
    ld de,cliRequest.destination
    call cliRenamePrepared
    jp cliFileDone
cliPrepareRename:
    call cliPrepareSource
    ret c
    ld a,(cliOldDrive)
    call fileChangeDrive
    ret c
    jp cliPairFile
cliPreparePair:
    call cliPrepareSource
    ret c
    ld a,(cliOldDrive)
    call fileChangeDrive
    ret c
    ld hl,cliPath2
    call directoryPathUnchecked
    jp c,cliPairFile
    ld a,(v_drive)
    ld (cliRequest.destinationDrive),a
    ld hl,(v_directory)
    ld (cliRequest.destinationDirectory),hl
    ld hl,cliRequest.source
    ld de,cliRequest.destination
    ld bc,11
    call cliCopyBytes
    xor a
    ld (v_dosError),a
    ret
cliPairFile:
    ld hl,cliPath2
    ld de,cliRequest.destination
    call fileNamePrepare
    ret c
    ld a,(v_drive)
    ld (cliRequest.destinationDrive),a
    ld hl,(v_directory)
    ld (cliRequest.destinationDirectory),hl
    xor a
    ld (v_dosError),a
    ret
cliCopy:
    xor a
    jp cliTransfer
cliMove:
    ld a,1
cliTransfer:
    ld (cliMoveFlag),a
    call cliTwoPaths
    ret c
    ld hl,cliPath1
cliTransferMask:
    ld a,(hl)
    or a
    jp z,cliTransferOne
    cp '*'
    jp z,cliTransferMany
    cp '?'
    jp z,cliTransferMany
    inc hl
    jp cliTransferMask
cliTransferOne:
    call cliRemember
    call cliPreparePair
    jp c,cliFileDone
    ld hl,(cliCopyHook)
    ld (cliRequest.onExists),hl
    ld hl,cliIoBuffer
    ld (cliRequest.buffer),hl
    ld hl,256
    ld (cliRequest.bufferSize),hl
    ld a,(cliMoveFlag)
    or a
    jp z,cliTransferCopy
    call cliMovePrepared
    jp cliFileDone
cliTransferCopy:
    call cliCopyInternal
    jp cliFileDone
 ; A wildcard source expands through the same enumerator as DIR. Destination
; must be an existing directory; destination rename masks are not supported.
cliTransferMany:
    call cliRemember
    ld hl,cliPath2
    call directoryPathUnchecked
    call cliFileDone
    ret c
    ld hl,cliPath1
    ld c,ENUM_CHECK_CANCEL
    xor a
    call enumRun
    ret c
    xor a
    ld (cliTransferMatched),a
cliTransferManyNext:
    ld a,ENUM_NEXT
    call enumRun
    jp c,cliTransferManyEnd
    ld a,h
    or l
    jp z,cliTransferManyNext
    ld de,32
    add hl,de
    ld de,cliPath1
    ld bc,MAX_PATH
    call cliCopyBytes
    ld a,1
    ld (cliTransferMatched),a
    call cliTransferOne
    jp nc,cliTransferManyNext
cliTransferManyEnd:
    push af
    ld a,ENUM_CLOSE
    call enumRun
    pop af
    or a
    jp nz,dosSetError
    ld a,(cliTransferMatched)
    or a
    jp nz,cliSuccess
    ld a,DOS_ERR_NOT_FOUND
    jp dosSetError
 ; DIR consumes the same lazy enumerator used by applications.
cliDir:
    ld a,1
    ld (cliOutput),a
    ld hl,(cliArgs)
    ld a,(hl)
    or a
    jp z,cliDirDefault
    ld de,cliPath1
    call cliWord
    ret c
    call cliSkipSpaces
    ld a,(hl)
    or a
    ld c,ENUM_CHECK_CANCEL
    jp z,cliDirSingle
    cp '-'
    jp nz,blockArgument
    inc hl
    ld a,(hl)
    and 0DFh
    cp 'S'
    jp nz,blockArgument
    inc hl
    call cliSkipSpaces
    ld a,(hl)
    or a
    jp nz,blockArgument
    ld c,ENUM_RECURSIVE+ENUM_CHECK_CANCEL
    jp cliDirOpen
cliDirSingle:
    ld hl,cliPath1
    ld a,(hl)
    cp '-'
    jp nz,cliDirOpen
    inc hl
    ld a,(hl)
    and 0DFh
    cp 'S'
    jp nz,cliDirOpen
    inc hl
    ld a,(hl)
    or a
    jp nz,cliDirOpen
    ld (cliPath1),a
    ld c,ENUM_RECURSIVE+ENUM_CHECK_CANCEL
    jp cliDirOpen
cliDirDefault:
    ld (cliPath1),a
    ld c,ENUM_CHECK_CANCEL
cliDirOpen:
    ld a,c
    and ENUM_RECURSIVE
    ld (cliDirRecursive),a
    ld hl,cliPath1
    xor a
    ld (cliDirHadItems+1),a
    call enumRun
    ret c
cliDirLoop:
    ld a,ENUM_NEXT
    call enumRun
    jp c,cliDirEnd
    ld a,h
    or l
    jp z,cliDirLoop
    ld (cliDirHadItems+1),a
    ld a,(cliDirRecursive)
    or a
    jp z,cliDirShort
    ld de,32
    add hl,de
    call bios_printString
    jp cliDirNext
cliDirShort:
    push hl
    ld b,8
    call cliPrintNamePart
    pop hl
    push hl
    ld de,8
    add hl,de
    ld a,(hl)
    cp ' '
    jp z,cliDirNoExt
    ld c,'.'
    call bios_printChar
    ld b,3
    call cliPrintNamePart
cliDirNoExt:
    ; Колонка 15 (с нуля): 15 символов по 6 пикселей, X хранится в полупикселях.
    MXOS_CALL MXOS_BIOS_GET_CURSOR_POS
    ld h,45
    MXOS_CALL MXOS_BIOS_SET_CURSOR_POS
    pop hl
    ld de,11
    add hl,de
    ld a,(hl)
    and 010h
    jp z,cliDirSize
    ld hl,cliDirText
    call bios_printString
    jp cliDirNext
cliDirSize:
    ld de,17
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld b,' '
    call bios_printDecWord
cliDirNext:
    ld c,0Ah
    call bios_printChar
    jp cliDirLoop
cliDirEnd:
    push af
    ld a,ENUM_CLOSE
    call enumRun
    pop af
    or a
    jp nz,dosSetError
cliDirHadItems:
    ld a,0 ; Patched only for this DIR; yielding without a record is not a match.
    or a
    jp nz,cliSuccess
    ld hl,cliDirNoMatches
    call bios_printString
    jp cliNewLine
cliDirNoMatches: DB 'No matching files',0
cliPrintNamePart:
    ld a,(hl)
    cp ' '
    ret z
    ld c,a
    call bios_printChar
    inc hl
    dec b
    jp nz,cliPrintNamePart
    ret
; TYPE использует точный размер файла и поток, без терминатора в файле.
cliType:
    ld a,1
    ld (cliOutput),a
    call cliOnePath
    ret c
    call cliRemember
    call cliPrepareSource
    jp c,cliFileDone
    ld hl,cliRequest.source
    call fileLoadInfo
    jp c,cliFileDone
    ld hl,(cliRequest.source+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,cliFileArgument
    ld hl,(cliRequest.source+FILE_DESCRIPTOR.size)
    ld (cliRemaining),hl
    ld hl,(cliRequest.source+FILE_DESCRIPTOR.firstCluster)
    ld (cliCursor),hl
    ld hl,0
    ld (cliCursor+2),hl
cliTypeLoop:
    call cliCheckCancel
    jp c,cliFileDone
    ld hl,(cliRemaining)
    ld a,h
    or l
    jp z,cliFileDone
    call cliChunk
    push bc
    ld de,cliCursor
    ld hl,cliIoBuffer
    ld a,2
    call cliStream
    pop bc
    jp c,cliFileDone
    ld hl,cliIoBuffer
cliTypePrint:
    ld a,(hl)
    inc hl
    push bc
    push hl
    ld c,a
    call bios_printChar
    pop hl
    pop bc
    dec bc
    ld a,b
    or c
    jp nz,cliTypePrint
    jp cliTypeLoop
; HL=остаток -> BC=min(256,остаток), остаток уменьшен.
cliResolveExternal:
    call cliCopyLine
    ret c
    ld hl,cliLine
    call cliSkipSpaces
    ld de,cliPath1
    call cliWord
    ret c
    ld (cliTail),hl
    call cliRemember
    ld hl,cliPath1
    ld de,cliRequest.source
    call fileNamePrepare
    jp c,cliResolveFail
    ld a,(cliRequest.source+8)
    cp ' '
    ld a,0
    jp nz,cliExtensionChosen
    inc a
cliExtensionChosen:
    ld (cliNeedExtension),a
    xor a
    ld (cliSearchDrive),a
    ld hl,v_cliPath
    ld (cliPathCursor),hl
cliSearchAgain:
    ld a,(cliNeedExtension)
    or a
    jp z,cliTryExplicit
    ld hl,cliExtensions
    ld (cliExtPtr),hl
cliTryExtension:
    ld hl,(cliExtPtr)
    ld a,(hl)
    or a
    jp z,cliSearchNextDirectory
    ld de,cliRequest.source+8
    ld bc,3
    call cliCopyBytes
    ld (cliExtPtr),hl
    ld hl,cliRequest.source
    call fileLoadInfo
    jp nc,cliFoundExtension
    ld a,(v_dosError)
    cp DOS_ERR_NOT_FOUND
    jp nz,cliResolveFail
    jp cliTryExtension
cliTryExplicit:
    ld hl,cliRequest.source
    call fileLoadInfo
    jp nc,cliFoundExtension
    ld a,(v_dosError)
    cp DOS_ERR_NOT_FOUND
    jp nz,cliResolveFail
cliSearchNextDirectory:
    call cliCanSearchSystem
    jp c,cliResolveNotFound
    ld hl,(cliPathCursor)
    ld a,(hl)
    or a
    jp z,cliResolveNotFound
    ld de,cliSearchDirectory
    ld b,MAX_PATH-1
cliReadSearchDirectory:
    ld a,(hl)
    or a
    jp z,cliSearchDirectoryEnd
    inc hl
    cp ';'
    jp z,cliSearchDirectoryEnd
    ld (de),a
    inc de
    dec b
    jp nz,cliReadSearchDirectory
    jp blockArgument
cliSearchDirectoryEnd:
    xor a
    ld (de),a
    ld (cliPathCursor),hl
    ld a,(cliSearchDirectory)
    or a
    jp z,cliSearchNextDirectory
    ; Каждый относительный элемент PATH разрешается от исходного CWD.
    ld hl,cliSavedDirectories
    ld de,v_directories
    ld bc,16
    call cliCopyBytes
    ld a,(cliOldDrive)
    call fileChangeDrive
    jp c,cliResolveFail
    ld hl,cliSearchDirectory
    call directoryPathUnchecked
    jp c,cliSearchNextDirectory
    ld a,1
    ld (cliSearchDrive),a
    jp cliSearchAgain
cliFoundExtension:
    ld a,(cliSearchDrive)
    or a
    jp z,cliFoundLocal
    ld hl,cliPath2
    call directoryCwdUnchecked
    jp c,cliResolveFail
    ld de,cliPath2
    ld b,MAX_PATH-1
cliFoundPrefixLength:
    ld a,(de)
    or a
    jp z,cliFoundPrefixEnd
    ld c,a
    inc de
    dec b
    jp nz,cliFoundPrefixLength
    jp blockArgument
cliFoundPrefixEnd:
    ld a,c
    cp 05Ch
    jp z,cliFoundCopy
    ld a,05Ch
    call cliEmit
    ret c
    jp cliFoundCopy
cliFoundLocal:
    ld de,cliPath2
    ld b,MAX_PATH-1
cliFoundCopy:
    ld hl,cliPath1
    call cliEmitString
    ret c
    ld a,(cliNeedExtension)
    or a
    jp z,cliFoundReady
    ld a,'.'
    call cliEmit
    ret c
    ld hl,cliRequest.source+8
    ld a,(hl)
    call cliEmit
    ret c
    inc hl
    ld a,(hl)
    call cliEmit
    ret c
    inc hl
    ld a,(hl)
    call cliEmit
    ret c
cliFoundReady:
    xor a
    ld (de),a
    ld hl,cliPath2
    ld de,cliPath1
    ld bc,MAX_PATH
    call cliCopyBytes
cliResolveAssociation:
    ; Подтверждённый COM/EXE обрабатывается ядром до ассоциаций.
    ld hl,cliRequest.source+8
    ld de,aCom
    ld c,3
    call stricmp
    jp z,cliCheckNativeImage
    ld hl,cliRequest.source+8
    ld de,aExe
    ld c,3
    call stricmp
    jp nz,cliFindAssociation
cliCheckNativeImage:
    ld hl,cliRequest.source
    call imageOpen
    jp c,cliResolveFail
    ld a,(v_tmpFileDescr+5)
    or a
    jp nz,cliAssociationAbsent
cliFindAssociation:
    ld hl,cliRequest.source+8
    ld de,cliAssociationBuffer
    ld bc,512
    call fileAssociation
    jp c,cliNoAssociation
    ld de,CMD_LINE
    ld b,MAX_PATH-1
cliResolvePrefix:
    ld a,(hl)
    cp 0FFh
    jp z,cliResolveSpace
    cp 020h
    jp c,cliResolveSpace
    call cliEmit
    ret c
    inc hl
    jp cliResolvePrefix
cliResolveSpace:
    ld a,' '
    call cliEmit
    ret c
    jp cliResolvePath
cliNoAssociation:
    ld a,(v_dosError)
    or a
    jp z,cliAssociationAbsent
    cp DOS_ERR_NOT_FOUND
    jp nz,cliResolveFail
cliAssociationAbsent:
    ld de,CMD_LINE
    ld b,MAX_PATH-1
cliResolvePath:
    ld hl,cliPath1
    call cliEmitString
    ret c
    ld hl,(cliTail)
    call cliEmitString
    ret c
    xor a
    ld (de),a
    xor a
    call cliFileDone
    ret c
    ld hl,CMD_LINE
    ret
cliResolveNotFound:
    ld a,DOS_ERR_NOT_FOUND
    call dosSetError
cliResolveFail:
    call cliFileDone
    scf
    ret
cliEmitString:
    ld a,(hl)
    or a
    ret z
    call cliEmit
    ret c
    inc hl
    jp cliEmitString
cliEmit:
    ld c,a
    ld a,b
    or a
    jp z,blockArgument
    ld a,c
    ld (de),a
    inc de
    dec b
    or a
    ret

; Внутренние буферы модуля не принадлежат приложению; публичная проверка
; memtop для них неприменима. Вызываем те же циклы с известными границами.
cliCanSearchSystem:
    ld hl,cliPath1
cliSearchPathCheck:
    ld a,(hl)
    or a
    ret z
    cp ':'
    jp z,cliSearchPathExplicit
    cp '/'
    jp z,cliSearchPathExplicit
    cp 05Ch
    jp z,cliSearchPathExplicit
    inc hl
    jp cliSearchPathCheck
cliSearchPathExplicit:
    scf
    ret

; PATH [каталог[;каталог...]]; PATH ; очищает список, текущий каталог ищется всегда.
cliPathCommand:
    ld hl,(cliArgs)
    ld a,(hl)
    or a
    jp z,cliPathPrint
    cp ';'
    jp nz,cliPathSet
    inc hl
    ld a,(hl)
    or a
    dec hl
    jp nz,cliPathSet
    xor a
    ld (v_cliPath),a
    jp cliSuccess
cliPathSet:
    ld de,v_cliPath
    ld b,MAX_PATH-1
cliPathSetLoop:
    ld a,(hl)
    ld (de),a
    or a
    jp z,cliSuccess
    inc hl
    inc de
    dec b
    jp nz,cliPathSetLoop
    jp blockArgument
cliPathPrint:
    ld a,1
    ld (cliOutput),a
    ld hl,v_cliPath
    call bios_printString
    jp cliNewLine

; Cooperative cancellation; no interrupts and no forced abort inside a driver.
cliCheckCancel:
    push bc
    push de
    push hl
    MXOS_CALL MXOS_BIOS_KEY_SCAN
    cp 01Bh
    jp z,cliCancelled
    cp 3
    jp z,cliCancelled
    or a
    pop hl
    pop de
    pop bc
    ret
cliCancelled:
    pop hl
    pop de
    pop bc
    ld a,DOS_ERR_CANCELLED
    jp dosSetError
