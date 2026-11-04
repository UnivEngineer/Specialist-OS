; Резидентные шлюзы модуля. Исходный AF передаётся реализации без изменений.
uiDraw:
            push af
            ld a,0
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
dosModuleDispatch:
            push hl
            push de
            ld hl,v_dosModuleReady
            inc (hl)
            dec (hl)
            jp z,dosModuleMissing
            ld hl,DOS_MODULE_JUMPS
            ld e,a
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
            push bc
            push de
            push hl
            ld a,(v_drive)
            push af
            xor a
            ld (v_dosModuleReady),a
            call fileChangeDrive
            jp c,dosModuleRestore
            ld hl,dosModuleName
            call imageOpen
            jp c,dosModuleRestore
            push hl
            ld hl,DOS_MODULE_ADDR
            call cmp_hl_de
            pop hl
            jp nz,dosModuleBad
            push hl
            ld h,b
            ld l,c
            ld de,DOS_MODULE_END-DOS_MODULE_ADDR
            call cmp_hl_de
            pop hl
            jp c,dosModuleLoadBody
            jp nz,dosModuleBad
dosModuleLoadBody:
            ld de,DOS_MODULE_ADDR
            call imageLoad
            jp c,dosModuleRestore
            ld hl,(DOS_MODULE_ADDR+3)
            ld de,04D44h
            call cmp_hl_de
            jp nz,dosModuleBad
            ld a,(DOS_MODULE_ADDR+5)
            dec a
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
    INCLUDE "moduleImports.inc"
