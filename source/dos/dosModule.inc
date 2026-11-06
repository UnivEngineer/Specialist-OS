; Резидентные шлюзы модуля. Исходный AF передаётся реализации без изменений.
uiDraw:
            push af
            xor a
            jp dosModuleDispatch
uiInverseRect:
            push af
            ld a,3
            jp dosModuleDispatch
uiToolbar:
            push af
            ld a,6
            jp dosModuleDispatch
uiCopyRect:
            push af
            ld a,9
            jp dosModuleDispatch
fileAssociation:
            push af
            ld a,12
            jp dosModuleDispatch
fileCopy:
            push af
            ld a,15
            jp dosModuleDispatch
fileHandle:
            push af
            ld a,18
            jp dosModuleDispatch
directoryGetCwd:
            push af
            ld a,21
            jp dosModuleDispatch
cliService:
            push af
            ld a,27
            jp dosModuleDispatch
cliResolveExternal:
            push af
            ld a,36
            jp dosModuleDispatch
directoryCreate:
            push af
            ld a,30
            jp dosModuleDispatch
directoryRemove:
            push af
            ld a,33
            jp dosModuleDispatch
directoryExtendGate:
            push af
            ld a,39
            jp dosModuleDispatch

directoryCommand:
            push af
            ld a,24
dosModuleDispatch:
            push hl
            push de
            ld e,a
            ld a,(v_dosModuleReady)
            or a
            jp z,dosModuleMissing
            ld hl,DOS_MODULE_JUMPS
            ld d,0
            add hl,de
            ld (dosModuleJump+1),hl
            pop de
            pop hl
            pop af
dosModuleJump:
            jp 0
dosModuleMissing:
            pop de
            pop hl
            pop af
            jp dosUnsupported

; Загрузка: BC/DE/HL сохранены, CF/A=код DOS. Программа соблюдает memtop.
; Повторная загрузка восстанавливает модуль после классической программы.
; Контейнер и сумма проверяются общим загрузчиком; активный диск сохранён.
loadDosModule:
            ; Обычные приложения соблюдают memtop. Повторно читать модуль
            ; нужно только после явного releaseDosModule или холодного старта.
            ld a,(v_dosModuleReady)
            or a
            jp z,dosModuleLoad
            xor a
            ld (v_dosError),a
            ret
dosModuleLoad:
            push bc
            push de
            push hl
            ld a,(v_drive)
            push af
            xor a
            ld (v_dosModuleReady),a
            call fileChangeDrive
            jp c,dosModuleRestore
            ld hl,0
            ld (v_directory),hl
            ld hl,dosModuleName
            call imageOpen
            jp c,dosModuleRestore
            push hl
            ld hl,DOS_MODULE_LOAD_ADDR
            call cmp_hl_de
            pop hl
            jp nz,dosModuleBad
            push hl
            ld h,b
            ld l,c
            ld de,DOS_MODULE_END-DOS_MODULE_LOAD_ADDR
            call cmp_hl_de
            pop hl
            jp c,dosModuleLoadBody
            jp nz,dosModuleBad
dosModuleLoadBody:
            ld de,DOS_MODULE_LOAD_ADDR
            call imageLoad
            jp c,dosModuleRestore
            ld hl,(DOS_MODULE_ADDR+3)
            ld de,04D44h
            call cmp_hl_de
            jp nz,dosModuleBad
            ld a,(DOS_MODULE_ADDR+5)
            cp DOS_MODULE_VERSION
            jp nz,dosModuleBad
            ; Сумма таблицы импортов проверяет соответствие модулю ядра.
            ld hl,moduleImports
            ld de,moduleImportsEnd-1
            call calcCS
            ld hl,(DOS_MODULE_ADDR+6)
            ld d,b
            ld e,c
            call cmp_hl_de
            jp nz,dosModuleBad
            ld hl,(v_memTop)
            ld de,APP_MEMORY_END-1
            call cmp_hl_de
            jp c,dosModuleLimitReady
            ex de,hl
dosModuleLimitReady:
            call setMemTop
            ld a,1
            ld (v_dosModuleReady),a
            xor a
            ld (v_dosError),a
            jp dosModuleRestore
dosModuleBad:
            ld a,DOS_ERR_BAD_IMAGE
            call dosSetError
dosModuleRestore:
            call c,dosIoError
            ld a,(v_dosError)
            ld b,a
            pop af
            push bc
            call fileSelectDrive
            pop bc
            jp nc,dosModuleRestoreOk
            call dosIoError
            ld b,a
dosModuleRestoreOk:
            ld a,b
            ld (v_dosError),a
            pop hl
            pop de
            pop bc
            jp dosGetError
v_dosModuleReady: DB 0
; Без расширенных служб программа может явно получить всё нижнее ОЗУ.
; BC/DE/HL сохранены, A=0/CF=0. Вызывать только вне модуля, закрыв его файлы.
releaseDosModule:
            xor a
            ld (v_dosModuleReady),a
            ld (v_dosError),a
            push hl
            push de
            ld hl,(v_memTop)
            ld de,APP_MEMORY_END-1
            call cmp_hl_de
            jp c,dosModuleReleased
            ld hl,LEGACY_APP_MEMORY_END-1
            call setMemTop
dosModuleReleased:
            pop de
            pop hl
            xor a
            ret
dosModuleName: DB "DOSMOD  COM"
            DS FILE_DESCR_SIZE-11,0
; После fileLoadInfo imageOpen использует v_tmpFileDescr, а не хвост имени.
; Поэтому 16-байтный заголовок контейнера не требует отдельного буфера.
v_imageHeader = dosModuleName+FILE_DESCR_SIZE-16
            ASSERT v_imageHeader >= dosModuleName+FILE_NAME_LENGTH+3
    INCLUDE "moduleImports.inc"

; Выбранная оболочка переживает загрузку обычной программы.
setShell:
    cp 3
    jp z,getShellMode
            cp 2
            jp z,getShellDrive
            jp nc,blockArgument
            ld (v_shell),a
            or a
            ret z
            ld a,(v_drive)
            ld (v_shellDrive),a
            xor a
            ret
getShellDrive:
            ld a,(v_shellDrive)
            or a
            ret
v_shell: DB 0
v_shellDrive: DB 0
pathCliCom: DB "A:",05Ch,"CLI.COM",0

getShellMode:
    ld a,(v_shell)
    or a
    ret
