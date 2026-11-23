; Все службы бывшего DOSMOD связаны с DOS непосредственно.
; Графика находится в BIOS, внутренние импорты между двумя DOS не нужны.
    IFDEF BANKED_DOS_BUILD
v_imageHeader = DOS_IMAGE_HEADER_ADDR
    ELSE
v_imageHeader: DS IMAGE_HEADER_SIZE,0
    ENDIF

;----------------------------------------------------------------------------
; MXOS
;
; Установить режим оболочки либо запросить режим или запомненный диск.
;
; вход:
;   a = 0 или 1 для установки v_shell, 2 для чтения диска оболочки, 3 для чтения
;   режима.
;
; выход:
;   cf = 0 при успехе. Установка возвращает a = 0, при режиме 1 запоминается текущий
;   диск. Запрос возвращает значение в a. При неизвестной операции cf = 1. bc, de и hl
;   сохранены.
;----------------------------------------------------------------------------

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
    IFNDEF BANKED_DOS_BUILD
pathCliCom: DB "C:",05Ch,"DOS",05Ch,"CLI.COM",0
    ENDIF

    IFNDEF BANKED_DOS_BUILD
; В переходной основной сборке служебные банковые операции недоступны.
dosRawIO:
    jp j_diskDriver
dosMemoryService:
timerService:
fontConfigService:
moduleScope:
dosCommandBatch:
moduleLoad:
moduleCall:
moduleUnload:
fileExecNative:
residentService:
enumService:
fileopsService:
dosDriverInstall:
dosDriverRefresh:
    jp dosUnsupported
    ENDIF

getShellMode:
    ld a,(v_shell)
    or a
    ret
