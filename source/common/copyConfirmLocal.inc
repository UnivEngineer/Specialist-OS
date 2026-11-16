; Переходник приложения: A=FAR-банк DOS, HL=FILE_DESCRIPTOR назначения.
; COPY_CONFIRM_BODY получает локальную копию дескриптора на стеке.
; Ответ A/CF сохраняется; остальные регистры разрушены. DOS не вызывается.
    push af
    push hl
    call mx_memInfo
    ld c,a
    pop de
    pop af
    ld hl,-(FILE_DESCRIPTOR+MEM_COPY_REQUEST)
    add hl,sp
    ld sp,hl
    push hl
    ld (hl),a
    inc hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (hl),c
    inc hl
    push hl
    ld hl,MEM_COPY_REQUEST+4
    add hl,sp
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (hl),FILE_DESCRIPTOR
    inc hl
    ld (hl),0
    pop hl
    call mx_memcpyFar
    jp c,copyConfirmLocalDone
    ld hl,MEM_COPY_REQUEST
    add hl,sp
    call COPY_CONFIRM_BODY
copyConfirmLocalDone:
    push af
    pop bc
    ld hl,FILE_DESCRIPTOR+MEM_COPY_REQUEST
    add hl,sp
    ld sp,hl
    push bc
    pop af
    ret
