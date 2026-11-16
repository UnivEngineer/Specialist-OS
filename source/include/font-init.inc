; Disposable font initializer ABI v1. MAIN-only; all registers volatile.
; Native MAIN DLL loaded with its 16-byte header immediately below entry.
; Entry: JMP implementation, then DB "FIN1",1 at +3.
; A=FONT_INIT_IMPORT or FONT_INIT_GENERATE; HL -> FONT_INIT_REQUEST.
; CF=0 success, CF=1 A=DOS error. No allocation, printing or file I/O.
; Resident engine descriptor +46 points to companion ASCIZ filename (8.3).

FONT_INIT_ADDR     = 05000h
FONT_INIT_END      = 05800h
FONT_IMPORT_ADDR   = 04000h
FONT_INIT_IMPORT   = 0
FONT_INIT_GENERATE = 1

    STRUCT FONT_INIT_REQUEST
engine DW 0 ; Staged engine BODY, not its native executable header.
flat   DW 0 ; Read-only flat input: 256 glyphs * 8 bytes.
lower  DW 0 ; Writable reserved planes; ignored during IMPORT.
    ENDS
