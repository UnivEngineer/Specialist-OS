; Проверка предков назначения до изменения каталога: нельзя переместить
; каталог внутрь себя или потомка. Число переходов ограничено геометрией.
; AF/BC/DE/HL разрушены, CF/A=ошибка; запрос cliRequest сохраняется.

;----------------------------------------------------------------------------
; MXOS
;
; Проверить родительскую цепочку перед перемещением каталога.
;
; вход:
;   cliRequest содержит исходный дескриптор и кластер каталога назначения. Геометрия
;   текущего тома задаёт максимальное число переходов.
;
; выход:
;   cf = 0 для допустимого назначения или обычного файла. При отказе cf = 1, a = код
;   DOS. Запрос сохранён, v_directory используется при обходе; af, bc, de и hl рабочие.
;----------------------------------------------------------------------------

cliMoveCheckParent:
    ld a,(cliRequest.source+FILE_DESCRIPTOR.attrib)
    and 010h
    ret z
    ld hl,(v_diskInfo.dataClusters)
    ld (cliMoveBudget),hl
    ld hl,(cliRequest.destinationDirectory)
cliMoveParentLoop:
    ld a,h
    or l
    ret z
    ld (v_directory),hl
    ex de,hl
    ld hl,(cliRequest.source+FILE_DESCRIPTOR.firstCluster)
    call cmp_hl_de
    jp z,cliMoveDenied
    ld hl,(cliMoveBudget)
    dec hl
    ld (cliMoveBudget),hl
    ld a,h
    or l
    jp z,dosCorrupt
    ld hl,cwdDots
    call fileFindAny
    ret c
    ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
    and 010h
    jp z,dosCorrupt
    ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstCluster)
    jp cliMoveParentLoop
; Новая запись уже сохранена; обновить .. перед удалением прежней записи.
cliMoveFixParent:
    ld a,(cliRequest.source+FILE_DESCRIPTOR.attrib)
    and 010h
    ret z
    ld hl,(cliRequest.source+FILE_DESCRIPTOR.firstCluster)
    ld (v_directory),hl
    ld hl,cwdDots
    call fileFindAny
    ret c
    call markDirSectorAsModified
    ret c
    ld de,FILE_DESCRIPTOR.firstCluster
    add hl,de
    ex de,hl
    ld hl,(cliRequest.destinationDirectory)
    ex de,hl
    ld (hl),e
    inc hl
    ld (hl),d
    call saveSectorCache
    ret c
    jp fatFinishWrite
