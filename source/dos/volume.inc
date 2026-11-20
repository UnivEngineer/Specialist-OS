; Файловые метаданные обслуживает встроенное FAT-ядро.
; Драйверы физических блоков независимы от файлового слоя.
; Общий вход сохраняет входные регистры/флаги и нормализует DOS error.
    MACRO FAT_SERVICE name,target
name:
    push hl
    ld hl,target
    jp volumeDispatch
    ENDM
    FAT_SERVICE fileList,fatFileList
    FAT_SERVICE fileDelete,fatFileDelete
    FAT_SERVICE fileRename,fatFileRename
    FAT_SERVICE fileLoadInfo,fatFileLoadInfo
    FAT_SERVICE fileGetSetAttr,fatFileGetSetAttr
    FAT_SERVICE getDriveFreeSpace,fatGetDriveFreeSpace
    FAT_SERVICE fileCreatePrepare,fatFileCreatePrepare
    FAT_SERVICE flushVolume,fatVolumeFlush
    FAT_SERVICE volumePrepareName,fatFileNamePrepare
    FAT_SERVICE volumeMountDispatch,fatReadBootSector

; HL = [диск:]имя, DE = буфер. Префикс диска принадлежит DOS,
; оставшуюся часть имени разбирает общий FAT-разборщик.
fileNamePrepare:
            push bc
            push de
            call directoryDrive
            jp c,volumePathReturn
            push hl
            ld de,0
            ld b,MAX_PATH
volumePathScan:
            ld a,(hl)
            cp 021h
            jp c,volumePathScanned
            call directorySeparator
            jp nz,volumePathNext
            ld d,h
            ld e,l
volumePathNext:
            inc hl
            dec b
            jp nz,volumePathScan
            pop hl
            call blockArgument
            jp volumePathReturn
volumePathScanned:
            pop hl
            ld a,d
            or e
            jp z,volumePathLeaf
            ex de,hl
            ld (v_pathEnd),hl
            ex de,hl
            ; Корневой разделитель тоже должен пройти через directoryParse.
            call directoryParse
            jp c,volumePathReturn
            inc hl
volumePathLeaf:
            pop de
            pop bc
            call volumePrepareName
            ; Исторический разбор короткого имени не задавал CF.
            or a
            ret
volumePathReturn:
            pop de
            pop bc
            push af
            ld a,0
            ld (de),a
            pop af
            ret

; HL=адрес обработчика, исходный HL уже на стеке.
volumeDispatch:
    push af
    ld (volumeJump+1),hl
    xor a
    ld (v_dosError),a
    pop af
    pop hl
volumeJump:
    call 0
            push af
            jp c,volumeStatusError
            xor a
            ld (v_dosError),a
            jp volumeStatusDone
volumeStatusError:
            ld a,(v_dosError)
            or a
            call z,dosIoError
volumeStatusDone:
            pop af
            ret


; Явный mount инвалидирует выбранный диск после внешнего RAW-доступа.
mountVolume:
    ld a,(v_drive)
    call invalidateDriveCache
selectVolume:
    push hl
    push de
    push bc
    call selectCacheForDrive
    call volumeMountDispatch
    pop bc
    pop de
    pop hl
    ret
