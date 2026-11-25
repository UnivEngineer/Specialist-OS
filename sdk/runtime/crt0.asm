; Старт sccz80/8080 для MAIN и BANK. Стек предоставляет загрузчик MXOS.
; BSS обнуляется, локальная куча занимает остаток до границы профиля.
    defc __crt_model                  = 0
    defc __crt_org_bss                = 0
    defc __crt_org_bss_fardata_start  = 0
    defc __crt_org_bss_compiler_start = 0
    defc __crt_org_graphics           = 0
    INCLUDE "crt/classic/crt_section_standard.asm"
    INCLUDE "../src/mxos_syscall_ids.inc"
    SECTION code_crt_init
    ORG 00100h
    PUBLIC __Start
    EXTERN _main, _mxos_heap_init, _mxos_default_heap
    EXTERN __BSS_head, __BSS_END_tail

;----------------------------------------------------------------------------
; MXOS
;
; Подготовить окружение sccz80 и вызвать main.
;
; вход:
;   de = указатель аргументов запуска. Стек и текущую страницу установил загрузчик
;   MXOS.
;
; выход:
;   BSS обнулена, сохранены адрес аргументов и номер страницы. При успехе
;   инициализирована куча и возвращён результат main в hl. Если куча недоступна, hl =
;   0FFFFh, cf = 1.
;----------------------------------------------------------------------------

__Start:
    push de
    ld hl,__BSS_head
    ld bc,__BSS_END_tail-__BSS_head
crtClear:
    ld a,b
    or c
    jp z,crtCleared
    ld (hl),0
    inc hl
    dec bc
    jp crtClear
crtCleared:
    pop de
    ex de,hl
    ld (_mxos_args),hl
    call mx_memInfo
    ld (_mxos_home_page),a
    or a
    ld hl,0EE00h
    jp nz,crtLimit
    call mx_getMemTop
    inc hl
crtLimit:
    defc crt_heap_start = (__BSS_END_tail+1)&0FFFEh
    ld de,crt_heap_start
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    jp c,crtNoHeap
    push hl
    ld a,(_mxos_home_page)
    or a
    jp nz,crtLocalHeap
    ; Reserve one MAIN heap block; malloc/free stay entirely local thereafter.
    ex de,hl
    ld b,0
    ld a,9                   ; MEM_OP_ARENA
    call mx_callRegs
    defw MXOS_DOS_MEMORY
    jp c,crtHeapRefused
    pop hl
    push hl
    ld b,0
    ld a,6                   ; MEM_OP_ALLOC
    call mx_callRegs
    defw MXOS_DOS_MEMORY
    jp c,crtHeapRefused
    ex de,hl
crtLocalHeap:
    pop bc
    ld hl,_mxos_default_heap
    push hl
    push de
    push bc
    call _mxos_heap_init
    pop bc
    pop bc
    pop bc
    ld a,h
    or l
    jp z,crtNoHeap
    call _main
    ret
crtHeapRefused:
    pop hl
crtNoHeap:
    ld hl,0FFFFh
    scf
    ret
    SECTION bss_crt
    PUBLIC _mxos_args, _mxos_home_page
_mxos_args:         defw 0
_mxos_home_page:    defb 0
