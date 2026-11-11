; Старт sccz80/8080 для MAIN и BANK. Стек предоставляет загрузчик MXOS.
; BSS обнуляется, локальная куча занимает остаток до границы профиля.
    defc __crt_model = 0
    defc __crt_org_bss = 0
    defc __crt_org_bss_fardata_start = 0
    defc __crt_org_bss_compiler_start = 0
    defc __crt_org_graphics = 0
    INCLUDE "crt/classic/crt_section_standard.asm"
    INCLUDE "../src/mxos_syscall_ids.inc"
    SECTION code_crt_init
    ORG 00100h
    PUBLIC __Start
    EXTERN _main, _mxos_heap_init, _mxos_default_heap
    EXTERN __BSS_head, __BSS_END_tail
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
    call 0F812h
    ld (_mxos_home_page),a
    or a
    ld hl,0F000h
    jp nz,crtLimit
    call 0F806h
    defw MXOS_BIOS_GET_MEM_TOP
    inc hl
crtLimit:
    ld de,__BSS_END_tail
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc a,d
    ld h,a
    jp c,crtNoHeap
    push hl
    ld hl,_mxos_default_heap
    pop bc
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
crtNoHeap:
    ld hl,0FFFFh
    scf
    ret
    SECTION bss_crt
    PUBLIC _mxos_args, _mxos_home_page
_mxos_args: defw 0
_mxos_home_page: defb 0
