; Исполнитель TEXT в MAIN. Окно остаётся источником на всё время печати.
    MODULE bankText
window:
    or a
    jp z,bios_printStringZ
    jp bios_printStringRawZ
;----------------------------------------------------------------------------
; MXOS
; Печать порции без копирования. До возврата движок не занимает окно.
; вход: HL = BANK_WINDOW, BC = длина 1..32, A = D = режим 0, 1, 2 или 8.
; Для Z и RAW_Z инициатор уже исключил завершающий NUL из длины.
; выход: HL сразу за выведенными байтами. Остальные регистры рабочие.
;----------------------------------------------------------------------------
printWindow:
    cp 2
    jp z,bios_printStringRawLim
    jp nc,bios_printStringRawPseudo
    ; Оставляем старым перехватчикам Z их собственный способ рисования.
    or a
    ld hl,(bios_printStringZ+1)
    ld de,FONT_ENGINE_Z+1
    jp z,windowVector
    ld hl,(bios_printStringRawZ+1)
    ld de,FONT_ENGINE_RAWZ+1
windowVector:
    ld b,a
    ld a,(de)
    cp l
    jp nz,windowLegacy
    inc de
    ld a,(de)
    cp h
    jp nz,windowLegacy
    ld a,b
    or a
    ld b,0
    ld hl,BANK_WINDOW
    jp nz,FONT_ENGINE_LIM
    ld hl,(FONT_ENGINE_TEXT_EXT)
    ld a,h
    cp FONT_ENGINE_TEXT_LIM >> 8
    jp nz,windowLegacy
    ld a,l
    cp FONT_ENGINE_TEXT_LIM & 0FFh
    ld hl,BANK_WINDOW
    jp z,FONT_ENGINE_TEXT_LIM
windowLegacy:
    ld d,b
    ld b,0
    ld hl,BANK_WINDOW
    ld a,c
    cp BANK_TEXT_BYTES
    ld a,d
    jp nz,window
    ; Прежний вектор принимает только ASCIIZ. Печатаем 31+1 через этот же
    ; вектор. Последний код сохранён в E, после первого вызова HL = FFDFh.
    ld a,(BANK_WINDOW+BANK_TEXT_BYTES-1)
    ld e,a
    xor a
    ld (BANK_WINDOW+BANK_TEXT_BYTES-1),a
    ld a,d
    call window
    dec hl
    ld (hl),e
    ld a,d
    call window
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
;----------------------------------------------------------------------------
; MXOS
;
; Скопировать строку из банка источника порциями в MAIN и вывести её.
;
; вход:
;   hl = NEAR-адрес TEXT_PRINT_REQUEST. request выбирает MAIN, общий вход
;   execute принимает FAR-код банка в a. Пакет и строка в одном банке.
;   count ограничивает чтение. При TEXT_PRINT_MAIN_BUFFER вызывающий выделяет
;   capacity байт в MAIN и ещё байт под NUL для режимов Z/RAW_Z.
;
; выход:
;   cf = 0, a = 0, hl указывает за выведенными байтами (на NUL для строк,
;   остановленных по терминатору). При отказе cf = 1, a = код ошибки.
;   Ранее выведенные порции при ошибке не отменяются.
;----------------------------------------------------------------------------

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
kind:           DB 0
method:         DB 0
source:         DW 0
remaining:      DW 0
destination:    DW 0
capacity:       DW 0
    ENDMODULE
