;----------------------------------------------------------------------------
; MXOS
;
; Начать последовательный обход записей корня или подкаталога FAT.
;
; вход:
;   v_directory = 0 для корня или номер кластера подкаталога. Геометрия находится в
;   v_diskInfo.
;
; выход:
;   При cf = 0 курсор установлен перед первой физической записью. При cf = 1 том
;   недоступен или кластер неверен. bc, de и hl сохранены.
;----------------------------------------------------------------------------

fatDirStart:
            call fatCheckMounted
            ret c
            push hl
            push de
            xor a
            ld (v_dirRemaining),a
            ld hl,(v_directory)
            ld a,h
            or l
            jp z,fatDirRootStart
            ex de,hl
            call fatCheckCluster
            jp c,fatDirStartReturn
            ex de,hl
            ld (v_dirCluster),hl
            ex de,hl
            ld hl,(v_diskInfo.dataClusters)
            dec hl
            ld (v_dirBudget),hl
            call fatDirClusterStart
            jp fatDirStartReturn
fatDirRootStart:
            ld hl,(v_diskInfo.rootDirMaxFiles)
            ld (v_dirRootRemaining),hl
            ld hl,(v_diskInfo.dirStartSector)
            dec hl
            ld (v_cachedSector),hl
            or a
fatDirStartReturn:
            pop de
            pop hl
            ret

;----------------------------------------------------------------------------
; MXOS
;
; Проверить доступность файловой системы на выбранном диске.
;
; вход:
;   v_diskInfo.isValid = состояние после монтирования.
;
; выход:
;   При DISK_VALID возвращает z = 1 и cf = 0. Иначе a и v_dosError содержат ошибку, cf
;   = 1. bc, de и hl сохранены.
;----------------------------------------------------------------------------

fatCheckMounted:
            ld a,(v_diskInfo.isValid)
            cp DISK_VALID
            ret z
            ; Значения DISK_* для отказов смещены от DOS_ERR_NO_DRIVER.
            add a,DOS_ERR_NO_DRIVER
            jp dosSetError
;----------------------------------------------------------------------------
; MXOS
;
; Получить следующую физическую запись каталога, включая свободные и удалённые.
;
; вход:
;   Курсор инициализирован fatDirStart. Текущий каталог и строки кэша не должны
;   меняться между шагами.
;
; выход:
;   При cf = 0 hl указывает в кэш на 32-байтную запись. При cf = 1 значение a = 0
;   означает конец, ненулевое a — ошибку чтения или цепочки. bc и de сохранены.
;----------------------------------------------------------------------------

fatDirNext:
            push de
            push bc
            ; Во время прохода каталога кэш не используется другими циклами.
            ; Сектор получаем один раз, остальные записи идут по указателю.
            ld a,(v_dirRemaining)
            or a
            jp z,fatDirLoadSector
            dec a
            ld (v_dirRemaining),a
            ld de,FILE_DESCR_SIZE
            ld hl,(v_cachedDescrOffset)
            add hl,de
            ld (v_cachedDescrOffset),hl
            ld hl,(v_cachedDescrPtr)
            add hl,de
            ld (v_cachedDescrPtr),hl
            or a
            jp fatDirReturn
fatDirLoadSector:
            ld hl,(v_directory)
            ld a,h
            or l
            jp z,fatDirCheckRoot
            ld hl,(v_dirSectorsLeft)
            ld a,h
            or l
            jp nz,fatDirUseSector
            ld hl,(v_dirCluster)
            ex de,hl
            call fatReadCluster
            jp c,fatDirReturn
            ld a,d
            cp 0FFh
            jp nz,fatDirFollow
            ld a,e
            cp 0F8h
            jp nc,fatDirEnd
fatDirFollow:
            call fatCheckCluster
            jp c,fatDirReturn
            ld hl,(v_dirBudget)
            ld a,h
            or l
            jp z,fatDirBadChain
            dec hl
            ld (v_dirBudget),hl
            ex de,hl
            ld (v_dirCluster),hl
            ex de,hl
            call fatDirClusterStart
            ld hl,(v_dirSectorsLeft)
fatDirUseSector:
            dec hl
            ld (v_dirSectorsLeft),hl
            jp fatDirReadSector
fatDirCheckRoot:
            ld hl,(v_dirRootRemaining)
            ld a,h
            or l
            jp z,fatDirEnd
fatDirReadSector:
            ld hl,(v_cachedSector)
            inc hl
            ld (v_cachedSector),hl
            ex de,hl
            xor a
            call getSectorAddressInCache
            jp c,fatDirReturn
            ld (v_cachedDescrPtr),hl
            ld hl,0
            ld (v_cachedDescrOffset),hl
            ; Корневой лимит списывается пачкой при загрузке сектора.
            ; Поэтому обычный шаг по записи не проверяет тип каталога.
            ld a,(v_diskInfo.descrPerSector)
            ld b,a
            ld hl,(v_directory)
            ld a,h
            or l
            jp nz,fatDirCountReady
            ld hl,(v_dirRootRemaining)
            ld a,h
            or a
            jp nz,fatDirReserveRoot
            ld a,l
            cp b
            jp nc,fatDirReserveRoot
            ld b,a
fatDirReserveRoot:
            ld e,b
            ld d,0
            call sub_hl_de
            ld (v_dirRootRemaining),hl
fatDirCountReady:
            ld a,b
            dec a
            ld (v_dirRemaining),a
            ld hl,(v_cachedDescrPtr)
            or a
fatDirReturn:
            pop bc
            pop de
            ret
fatDirEnd:
            xor a
            scf
            jp fatDirReturn
fatDirBadChain:
            call dosCorrupt
            jp fatDirReturn
;----------------------------------------------------------------------------
; MXOS
;
; Подготовить секторный курсор к переходу на кластер подкаталога.
;
; вход:
;   de = проверенный кластер данных текущего тома.
;
; выход:
;   v_cachedSector указывает на сектор перед началом кластера, v_dirSectorsLeft = число
;   секторов в кластере. cf = 0, de и bc сохранены, hl и af рабочие.
;----------------------------------------------------------------------------

fatDirClusterStart:
            push de
            call fatClusterToSector
            ex de,hl
            dec hl
            ld (v_cachedSector),hl
            ld hl,(v_diskInfo.sectorsPerCluster)
            ld (v_dirSectorsLeft),hl
            pop de
            or a
            ret
;----------------------------------------------------------------------------
; MXOS
;
; Найти следующую обычную файловую запись.
;
; вход:
;   Курсор FAT установлен на текущий каталог.
;
; выход:
;   При cf = 0 hl указывает на запись файла в кэше. Удалённые записи, каталоги, метки
;   тома и LFN пропускаются. При cf = 1 значение a = 0 означает конец, иначе a =
;   ошибка. bc и de сохранены.
;----------------------------------------------------------------------------

fatDirFile:
            ld a,018h
            jp fatDirFiltered

;----------------------------------------------------------------------------
; MXOS
;
; Найти следующий файл или подкаталог.
;
; вход:
;   Курсор FAT установлен на текущий каталог.
;
; выход:
;   При cf = 0 hl указывает на живую запись файла или каталога. Метки тома и LFN
;   пропускаются. При cf = 1 значение a = 0 означает конец, иначе a = ошибка. bc и de
;   сохранены.
;----------------------------------------------------------------------------

fatDirItem:
            ld a,008h

;----------------------------------------------------------------------------
; MXOS
;
; Получить следующую живую запись, исключив указанные атрибуты.
;
; вход:
;   a = маска запрещённых битов атрибута, курсор подготовлен fatDirStart.
;
; выход:
;   При cf = 0 hl указывает в кэш на запись с подходящим атрибутом. При cf = 1 значение
;   a = 0 означает конец, иначе a = ошибка. bc и de сохранены.
;----------------------------------------------------------------------------

fatDirFiltered:
            push bc
            push de
            ld c,a
            xor a
            jp fatDirScanStart
;----------------------------------------------------------------------------
; MXOS
;
; Найти короткое имя в оставшейся части каталога.
;
; вход:
;   a = маска запрещённых атрибутов, v_foundDescrPtr указывает на искомое имя. Операнд
;   fatDirFindFirst содержит его первый символ в верхнем регистре.
;
; выход:
;   При cf = 0 hl = адрес совпавшей записи в кэше. При cf = 1 значение a = 0 означает
;   конец без совпадения, ненулевое a — ошибку. bc и de сохранены.
;----------------------------------------------------------------------------

fatDirFind:
            push bc
            push de
            ld c,a
            ld a,1
fatDirScanStart:
            ld (fatDirScanMode+1),a
fatDirScanSector:
            call fatDirNext
            jp c,fatDirScanReturn
            ld a,(v_dirRemaining)
            ld b,a
fatDirScanRecord:
            ld a,(hl)
            or a
            jp z,fatDirScanEnd
            cp 0E5h
            jp z,fatDirScanAdvance
            push hl
            ld a,l
            add a,FILE_DESCRIPTOR.attrib ; 32-байтная запись не пересекает страницу.
            ld l,a
            ld a,(hl)
            and c
            pop hl
            jp nz,fatDirScanAdvance
fatDirScanMode:
            ld a,0
            or a
            jp z,fatDirScanFound
            ld a,(hl)
            cp 5
            jp nz,fatDirScanFirst
            ld a,0E5h
fatDirScanFirst:
            call upperCase
fatDirFindFirst:
            cp 0
            jp nz,fatDirScanAdvance
            push hl
            push bc
            ex de,hl
            ld hl,(v_foundDescrPtr)
            ld c,FILE_NAME_LENGTH+3
            ld a,(de)
            cp 5
            jp nz,fatDirScanCompare
            inc hl
            inc de
            dec c
fatDirScanCompare:
            call stricmp
            pop bc
            pop hl
            jp nz,fatDirScanAdvance
fatDirScanFound:
            call fatDirScanCommit
            or a
fatDirScanReturn:
            pop de
            pop bc
            ret
fatDirScanEnd:
            call fatDirScanCommit
            xor a
            scf
            jp fatDirScanReturn
fatDirScanAdvance:
            ld a,b
            or a
            jp z,fatDirScanReload
            ld de,FILE_DESCR_SIZE
            add hl,de
            dec b
            jp fatDirScanRecord
fatDirScanReload:
            call fatDirScanCommit
            jp fatDirScanSector
; Между выдачами запись и счётчик живут в регистрах. Фиксируем курсор
; только при выдаче/смене сектора, чтобы поиск имени не сохранял его 128 раз.

;----------------------------------------------------------------------------
; MXOS
;
; Перенести позицию внутреннего цикла поиска в курсор каталога.
;
; вход:
;   hl указывает на текущую запись в секторе, b = число записей, оставшихся в этом
;   секторе после неё.
;
; выход:
;   Обновлены указатель, смещение и счётчик курсора по числу пройденных записей. При
;   неизменной позиции запись в память пропускается. bc и hl сохранены, af и de
;   рабочие.
;----------------------------------------------------------------------------

fatDirScanCommit:
            ld a,(v_dirRemaining)
            sub b
            ret z
            ld (v_cachedDescrPtr),hl
            push hl
            push bc
            ld l,a
            ld h,0
            add hl,hl
            add hl,hl
            add hl,hl
            add hl,hl
            add hl,hl
            ex de,hl
            ld hl,(v_cachedDescrOffset)
            add hl,de
            ld (v_cachedDescrOffset),hl
            ld a,b
            ld (v_dirRemaining),a
            pop bc
            pop hl
            ret

;----------------------------------------------------------------------------
; MXOS
;
; Подготовить свободную запись каталога к публикации нового имени.
;
; вход:
;   hl указывает на найденную свободную запись текущего каталога.
;
; выход:
;   При cf = 0 hl = актуальный адрес выбранной записи. При необходимости следующий
;   дескриптор помечен как конец каталога. При cf = 1 возникла ошибка. bc и de
;   сохранены, прежний курсор обхода больше не используется.
;----------------------------------------------------------------------------

fatDirPrepareEntry:
            ld a,(hl)
            or a
            ret nz
            push bc
            push de
            ld hl,(v_cachedSector)
            push hl
            ld hl,(v_cachedDescrOffset)
            push hl
            call fatDirNext
            jp nc,fatDirMoveEnd
            ; A=0 означает последнюю запись корня: продолжения не нужно.
            or a
            jp z,fatDirRestoreEntry
            scf
            jp fatDirRestoreEntry
fatDirMoveEnd:
            call markDirSectorAsModified
            jp c,fatDirRestoreEntry
            ld (hl),0
fatDirRestoreEntry:
            pop hl
            ld (v_cachedDescrOffset),hl
            pop hl
            ld (v_cachedSector),hl
            pop de
            pop bc
            ret c
            ; Загрузка соседнего сектора могла вытеснить выбранную запись.
            jp markDirSectorAsModified


; HL=запись каталога. E5h в первой букве имени хранится как 05h.
; HL/BC/DE сохранены, AF разрушается.
fatEncodeName:
            ld a,(hl)
            cp 0E5h
            ret nz
            ld (hl),5
            ret
