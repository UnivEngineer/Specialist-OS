; Все службы бывшего DOSMOD связаны с DOS непосредственно.
; Графика находится в BIOS, внутренние импорты между двумя DOS не нужны.
directoryExtendGate:
    jp directoryExtend
v_imageHeader: DS 16,0
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

    IFNDEF BANKED_DOS_BUILD
; В переходной основной сборке служебные банковые операции недоступны.
dosRawIO:
    jp j_diskDriver
dosMemoryService:
moduleLoad:
moduleCall:
moduleUnload:
fileExecNative:
residentService:
dosDriverSelect:
dosDriverInstall:
dosDriverRefresh:
    jp dosUnsupported
    ENDIF

getShellMode:
    ld a,(v_shell)
    or a
    ret
