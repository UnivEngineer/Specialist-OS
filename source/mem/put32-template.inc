; Шаблон PUT32 копируется в аппаратное окно перед исполнением.
memPutDwordTemplate:
    ld hl,0
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld hl,0
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (hl),c
    inc hl
    ld (hl),b
    ld hl,0
    ld a,h
    ld h,0FFh
    ld (hl),a
    jp 0 ; адрес возврата устанавливает dword.inc
memPutDwordTemplateEnd:
    ASSERT memPutDwordTemplateEnd-memPutDwordTemplate == 27
