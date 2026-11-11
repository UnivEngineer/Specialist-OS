; Общий исполнитель TEXT: окна MEM и одна команда copy+print.
    MODULE bankText
dispatch:
    cp 5
    jp z,fromBank
    cp 6
    jp z,buffered
; A=0 Z, 1 RawZ, 2 RawLim, 3 Char, 4 CharRaw; HL=буфер, C=байты окна.
window:
    or a
    jp z,bios_printStringZ
    dec a
    jp z,bios_printStringRawZ
    dec a
    jp nz,one
    ld b,0
    jp bios_printStringRawLim
one:
    ; DEC задаёт Z для Char, NZ для Raw; оба публичных вызова сохраняют
    ; AF и HL. Поэтому вторая условная CALL видит тот же флаг, а окно
    ; не требует ещё одной пары PUSH/POP указателя.
    dec a
    ld c,(hl)
    ld a,c
    call z,bios_printChar
    call nz,bios_printCharRaw
    inc hl
    ret
; Принимающий адаптер уже сохранил пакет и установил main-стек.
fromBank:
    ld a,(io_vars.bbPage)
    ld hl,(io_vars.bbRegs)
    call execute
finish:
    ld (io_vars.bbRegs),hl
    push af
    pop hl
    ld (io_vars.bbRegs+6),hl
    jp 0F000h+6 ; Возврат через отдельный фиксированный вход bbFinish.
buffered:
    ld a,(io_vars.bbPage)
    ld (copy),a
    ld hl,(io_vars.bbRegs)
    ld (source),hl
    ex de,hl
    ld hl,0F800h
    call bios_sub_hl_de
    ld (remaining),hl
    ld a,(io_vars.bbFunction)
    cp MXOS_BIOS_PRINT_STRING_RAW_Z & 0FFh
    ld a,0
    jp nz,bufferedKind
    inc a
bufferedKind:
    ld (kind),a
    call shared
    jp finish
request:
    xor a ; Нативный MAIN: запрос и источник находятся в main.
execute:
    ld (copy),a
    ld (copy+1),hl
    ld hl,requestData
    ld (copy+4),hl
    ld hl,10
    ld (copy+6),hl
    ld hl,copy
    call bankCopyFar
    ret c
    ld a,(kind)
    cp 3
    jp nc,bad
    ld a,(method)
    dec a
    jp z,shared
    dec a
    jp nz,bad
    ld hl,(capacity)
    ld a,h
    or l
    jp z,bad
    ld hl,(destination)
    ld a,h
    or a
    jp z,bad
    push hl
    ld hl,(capacity)
    ex de,hl
    pop hl
    add hl,de
    jp c,bad
    ld de,08C00h
    call bios_cmp_hl_de
    jp nc,bad
    jp ready
shared:
    ld hl,BIOS_BANK_IO_BUFFER
    ld (destination),hl
    ld hl,256
    ld (capacity),hl
ready:
    ld hl,(source)
    ld (copy+1),hl
    ex de,hl
    ld hl,(remaining)
    add hl,de
    jp c,bad
    ld de,0F800h
    call bios_cmp_hl_de
    jp c,next
    jp nz,bad
next:
    ld hl,(remaining)
    ld a,h
    or l
    jp z,done
    ex de,hl
    ld hl,(capacity)
    call bios_cmp_hl_de
    jp c,chunk
    ex de,hl
chunk:
    ld (copy+6),hl
    ld hl,(destination)
    ld (copy+4),hl
    ld hl,copy
    call bankCopyFar
    ret c
    ld a,(kind)
    cp 2
    jp z,print
    ; NUL-ограничитель за порцией, без предварительного поиска терминатора.
    ld hl,(copy+6)
    ex de,hl
    ld hl,(destination)
    add hl,de
    ld (hl),0
print:
    ld hl,(copy+6)
    ld b,h
    ld c,l
    ld hl,(destination)
    ld a,(kind)
    or a
    jp z,printZ
    dec a
    jp z,printRawZ
    call bios_printStringRawLim
    jp printed
printZ:
    call bios_printStringZ
    jp printed
printRawZ:
    call bios_printStringRawZ
printed:
    push hl
    ld hl,(destination)
    ex de,hl
    pop hl
    call bios_sub_hl_de
    ld b,h
    ld c,l
    ld hl,(copy+1)
    add hl,bc
    ld (copy+1),hl
    ld hl,(copy+6)
    ld a,l
    sub c
    ld e,a
    ld a,h
    sbc a,b
    or e
    jp nz,done
    ld hl,(remaining)
    ld a,l
    sub c
    ld l,a
    ld a,h
    sbc a,b
    ld h,a
    ld (remaining),hl
    jp next
done:
    ld hl,(copy+1)
    xor a
    ret
bad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
copy: DB 0
    DW 0
    DB 0
    DW 0,0
requestData:
kind: DB 0
method: DB 0
source: DW 0
remaining: DW 0
destination: DW 0
capacity: DW 0
    ENDMODULE
