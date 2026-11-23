; Наблюдатель изменений: CLI_OP_OBSERVER, HL=адрес или 0 (отключить).
; Обработчик получает A=диск, DE=каталог; C=0 точный каталог, C=1 весь диск.
; Вызывать DOS/CLI из обработчика нельзя. Адрес принадлежит приложению;
; перед его выгрузкой обработчик обязательно отключается.

;----------------------------------------------------------------------------
; MXOS
;
; Зарегистрировать наблюдателя изменений файловой системы или отключить его.
;
; вход:
;   hl = адрес обработчика или 0. В обработчик приходят a = диск, de = каталог, c = 0
;   для каталога или 1 для всего диска. До выгрузки приложения привязку следует снять;
;   DOS из обработчика не вызывают.
;
; выход:
;   cliChangeHook = hl, a = 0, cf = 0, z = 1. bc, de и hl сохранены.
;----------------------------------------------------------------------------

cliSetObserver:
    ld (cliChangeHook),hl
    xor a
    ret
; Перед попыткой записи: возможна частичная запись даже при последующем отказе.
; Все регистры/флаги сохранены. Нулевой обработчик ничего не делает.

;----------------------------------------------------------------------------
; MXOS
;
; Уведомить об изменении текущего каталога перед попыткой записи.
;
; вход:
;   v_drive и v_directory = диск и каталог операции. cliChangeHook содержит адрес
;   обработчика или ноль.
;
; выход:
;   Обновлены общий и дисковый счётчики изменений. Установленный обработчик вызван с c
;   = 0. Все регистры и флаги вызывающего сохранены.
;----------------------------------------------------------------------------

cliChanged:
    push af
    push bc
    push de
    push hl
    ld c,0
    jp cliNotify

;----------------------------------------------------------------------------
; MXOS
;
; Уведомить об изменении, затрагивающем каталоги всего диска.
;
; вход:
;   v_drive = диск изменения, v_directory = текущий каталог.
;
; выход:
;   Увеличены оба счётчика изменений. При наличии наблюдателя он вызван с c = 1. Все
;   регистры и флаги сохранены.
;----------------------------------------------------------------------------

cliChangedDisk:
    push af
    push bc
    push de
    push hl
    ld c,1
cliNotify:
    ; Generation survives disconnecting the transient UI observer.
    ld hl,cliChangeEpoch
    call cliIncrementEpoch
    ld a,(v_drive)
    add a,a
    add a,a
    ld e,a
    ld d,0
    ld hl,cliDriveEpochs
    add hl,de
    call cliIncrementEpoch
    jp cliEpochReady

;----------------------------------------------------------------------------
; MXOS
;
; Увеличить 32-битный счётчик изменений на единицу.
;
; вход:
;   hl = адрес младшего байта счётчика.
;
; выход:
;   Обновлены байты по цепочке переноса. hl указывает на последний изменённый байт, z =
;   1 при переполнении всего слова из четырёх байтов. a, bc, de и cf сохранены.
;----------------------------------------------------------------------------

cliIncrementEpoch:
    inc (hl)
    ret nz
    inc hl
    inc (hl)
    ret nz
    inc hl
    inc (hl)
    ret nz
    inc hl
    inc (hl)
    ret
; Unknown MAIN/upper launchers may write disks outside DOS notifications.
cliInvalidateEpochs:
    push bc
    ld hl,cliChangeEpoch
    call cliIncrementEpoch
    ld hl,cliDriveEpochs
    ld b,8
cliInvalidateDriveEpoch:
    push hl
    call cliIncrementEpoch
    pop hl
    ld de,4
    add hl,de
    dec b
    jp nz,cliInvalidateDriveEpoch
    pop bc
    ret
cliEpochReady:
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
    call fileFindAny
    ret c
    ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
    and 10h
    jp z,cliDeleteFile
    ld a,DOS_ERR_ACCESS
    jp dosSetError
cliDeleteFile:
    call cliChanged
    jp fileDelete
cliRenamePrepared:
    call fileFindAny
    ret c
    call cliChangedObject
    jp fileRename

; Имя каталога меняет пути потомков; для него инвалидируется весь том.
; Обычный файл затрагивает только запись текущего каталога.

;----------------------------------------------------------------------------
; MXOS
;
; Выбрать область уведомления по атрибуту найденного объекта.
;
; вход:
;   v_tmpFileDescr содержит актуальный дескриптор, v_drive и v_directory соответствуют
;   найденной записи.
;
; выход:
;   Для каталога наблюдателю сообщается изменение всего диска, для файла — текущего
;   каталога. Регистры и флаги сохранены.
;----------------------------------------------------------------------------

cliChangedObject:
    push af
    ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
    and 010h
    call nz,cliChangedDisk
    call z,cliChanged
    pop af
    ret
