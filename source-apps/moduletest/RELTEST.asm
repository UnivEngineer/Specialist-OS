; Контроллер проверки двух MAIN DLL на исходном MX, без свободного EXEC.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 0
;----------------------------------------------------------------------------
; MXOS
; Загрузить обе DLL в MAIN и чередовать вызовы.
; вход: текущий каталог содержит RELOCA.DLL и RELOCB.DLL; параметры отсутствуют.
; выход: testResult=55AAh при успехе, DEADh при ошибке. В консоли выведен итог;
; ожидание клавиши сохраняет результат для отладчика. Регистры рабочие.
;----------------------------------------------------------------------------
entry:
    ld a,(bios_upper.koi7)
    push af
    xor a
    call bios_setFontEncoding
    ld hl,pathA
    call loadMain
    jp c,failed
    ld (handleA),hl
    ld hl,pathB
    call loadMain
    jp c,failed
    ld (handleB),hl
    ld hl,(handleA)
    ld bc,1101h
    call probe
    jp nz,failed
    ld hl,(handleB)
    ld bc,2201h
    call probe
    jp nz,failed
    ld hl,(handleA)
    ld bc,1102h
    call probe
    jp nz,failed
    ld hl,(handleB)
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    jp c,failed
    ld hl,(handleA)
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    jp c,failed
    ld hl,pathA
    call loadMain
    jp c,failed
    ld (handleA),hl
    ld bc,1103h
    call probe
    jp nz,failed
    ld a,(packet+4)
    dec a
    jp nz,failed
    ld hl,(handleA)
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    jp c,failed
    ld hl,055AAh
    ld de,successText
    jp finish
failed:
    ld hl,0DEADh
    ld de,errorText
finish:
    ld (testResult),hl
    ex de,hl
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING
testDone:
    call bios_getch
    pop af
    jp bios_setFontEncoding
;----------------------------------------------------------------------------
; MXOS
; Загрузить DLL из текущего каталога.
; вход: HL=имя ASCIIZ в MAIN. Выход: HL=handle при CF=0, A=ошибка при CF=1.
; BC/DE и прочие флаги рабочие.
;----------------------------------------------------------------------------
loadMain:
    xor a
    MXOS_CALL_REGS MXOS_DOS_MODULE_LOAD
    ret
;----------------------------------------------------------------------------
; MXOS
; Проверить результат MODULE_CALL и записанное в BSS значение.
; вход: HL=handle, BC=ожидаемый счётчик. Выход: Z=1 при успехе, регистры рабочие.
;----------------------------------------------------------------------------
probe:
    push bc
    xor a
    ld (packet+7),a
    ld de,packet
    MXOS_CALL_REGS MXOS_DOS_MODULE_CALL
    pop de
    jp c,probeFailed
    ld hl,(packet)
    call bios_cmp_hl_de
    ret nz
    ld hl,(packet+2)
    ld a,(hl)
    cp e
    ret nz
    inc hl
    ld a,(hl)
    cp d
    ret
probeFailed:
    ld a,1
    or a
    ret
handleA:     DW 0
handleB:     DW 0
testResult:  DW 0
packet:      DW 0,0,0,0
pathA:       DB "RELOCA.DLL",0
             DS MAX_PATH-($-pathA),0
pathB:       DB "RELOCB.DLL",0
             DS MAX_PATH-($-pathB),0
successText: DB "Relocatable MAIN DLLs PASS",13,10,0
errorText:   DB "Relocatable MAIN DLLs FAIL",13,10,0
    END
