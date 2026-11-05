SECTION code_crt_init
PUBLIC __Start
EXTERN _main
ORG 02000h
__Start:
 ld sp,08FDFh
 call _main
 ld (05E80h),hl
 jp 06100h
