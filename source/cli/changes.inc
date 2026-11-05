; Наблюдатель изменений: CLI_OP_OBSERVER, HL=адрес или 0 (отключить).
; Обработчик получает A=диск, DE=каталог; C=0 точный каталог, C=1 весь диск.
; Вызывать DOS/CLI из обработчика нельзя. Адрес принадлежит приложению;
; перед его выгрузкой обработчик обязательно отключается.
cliSetObserver:
    ld (cliChangeHook),hl
    xor a
    ret
; Перед попыткой записи: возможна частичная запись даже при последующем отказе.
; Все регистры/флаги сохранены. Нулевой обработчик ничего не делает.
cliChanged:
    push af
    push bc
    push de
    push hl
    ld c,0
    jp cliNotify
cliChangedDisk:
    push af
    push bc
    push de
    push hl
    ld c,1
cliNotify:
    ld hl,(cliChangeHook)
    ld a,h
    or l
    jp z,cliNotifyDone
    push hl
    ld hl,(v_directory)
    ex de,hl
    pop hl
    ld a,(v_drive)
    call cliNotifyJump
cliNotifyDone:
    pop hl
    pop de
    pop bc
    pop af
    ret
cliNotifyJump:
    jp (hl)
cliDeletePrepared:
    call cliChanged
    jp fileDelete
cliRenamePrepared:
    call fileFindAny
    ret c
    call cliChangedObject
    jp fileRename
cliChangeHook: DW 0

; Имя каталога меняет пути потомков; для него инвалидируется весь том.
; Обычный файл затрагивает только запись текущего каталога.
cliChangedObject:
    push af
    ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
    and 010h
    call nz,cliChangedDisk
    call z,cliChanged
    pop af
    ret
