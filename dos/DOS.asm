;+---------------------------------------------------------------------------
; MXOS
;
; 2013-12-12 Disassembled by vinxru
;----------------------------------------------------------------------------

; Compilation options
BIG_MEM				 = 1		; Enable large additional memory (> 64 KB)
ARAM_MAX_PAGE		 = 0Fh		; Max number of RAM disk pages (up to 0Fh)
ARAM_PAGE_END		 = 0FFBBh	; End of RAM disk page (36 bytes of common RAM after this)
ROM_64K				 = 1		; Enable Specialist-MX2 ROM (64 KB)
DISABLE_COLOR_BUG	 = 1		; Enable color support
LOAD_FONT			 = 1		; Load font to RAM
FONT_ADDR			 = 0E900h	; Font address
RAMFOS_COMPATIBILITY = 1        ; Compatibility with RAMFOS (WIP)

; Memory map:
;   8FDF-8FFF - [  32  B] Variables
;   9000-BFFF - [12   KB] Screen
;   C000-CFFF - [4    KB] DOS.SYS (+ some free space after)
;   D000-E1FF - [4.5  KB] NC.COM (+ space for directory listing and some free space)
;   E200-E7FF - [1.5  KB] === 1536 bytes free ===
;   E900-F0FF - [2    KB] Font (can be disabled by running ROMFNT.COM or using option LOAD_FONT=0)
;   F100-F837 - [2.25 KB] Monitor-2
;   FA00-FAFF - [ 256  B] ROM disk Driver
;   FB00-FDFF - [ 768  B] Disk buffer
;   FF00-FF81 - [ 129  B] Command line, filled by fileExec
;   FF82-FFBF - [ 130  B] Stack
;   FFC0-FFEF - [  32  B] Part of the RAM disk driver
;   FFD0-FFFF - [  32  B] Hardware ports

fat				= 0FB00h
diskDirectory	= 0FC00h
diskDirectoryL	= 0FE00h
v_cmdLine		= 0FF00h
STACK_ADDR		= 0FFC0h

INIT_COLOR		= 0F0h	; Must match COLOR_CMDSCREEN in NC.asm for consistency

IO_KEYB_A		= 0FFE0h
IO_KEYB_B		= 0FFE1h
IO_KEYB_C		= 0FFE2h
IO_KEYB_MODE	= 0FFE3h
IO_PROG			= 0FFE4h
IO_TIMER		= 0FFECh
IO_COLOR		= 0FFF8h
IO_RAM			= 0FFFCh
IO_ARAM			= 0FFFDh
IO_ROM			= 0FFFEh
IO_PAGE_STD		= 0FFFFh

.org 08FDFh

vars:			.block 2
v_tapeError:	.block 2	; Tape load error jump address
v_tapeAddr:		.block 2	; Address of program loaded form tape
				.block 2
v_charGen:		.block 2	; Alternative font address / 8
v_cursorCfg:	.block 1	; Cursor shape (bits: 7 - visibility, 654 - position, 3210 - height)
v_koi8:			.block 1	; KOI mode: 0FFh (KOI-8), 0 (KOI-7)
v_escMode:		.block 1	; ESC sequence processing
v_keyLocks:		.block 1
				.block 2
v_lastLastKey:	.block 1
v_lastKey:		.block 1
v_beep:			.block 2	; Sound duration and frequency
v_tapeInverse:	.block 1
v_cursorDelay:	.block 1
byte_8FF5:		.block 1
v_oldSP:		.block 2	; Used to save SP by some functions
v_maxRamPage:	.block 1    ; Max detected ARAM page
v_flashPage:	.block 1    ; Current flash disk page
v_inverse:		.block 2	; Inverse font (0=normal, 0FFFFh=inverse)
v_cursorY:		.block 1	; Cursor Y position in pixels
v_cursorX:		.block 1	; Cursor X position in double pixels
v_writeDelay:	.block 1	; Tape write speed
v_readDelay:	.block 1	; Tape read speed

.org 0C000h

.include "jmps_c000.inc"
.include "reboot0.inc"
.db 0
.include "clearScreen.inc"
.include "printChar.inc"
.db 2Ah, 0FCh
.include "printChar5.inc" ; Continued in drawChar
.include "drawChar.inc"
.include "printChar3.inc"
.db 0FFh
.include "beep.inc"
.include "delay_l.inc"
.db 0C9h
.include "printChar4.inc" ; Continued in scrollUp
.include "scrollUp.inc"

; Buffer for character image from ROM
v_char:	.db 0FFh, 0FFh,	0FFh, 0FFh, 0FFh, 0FFh,	0FFh, 0FFh, 0FFh, 0FFh,	0FFh, 0FFh, 0FFh

.include "keyScan.inc"
.include "getch2.inc"
.include "calcCursorAddr.inc"
.include "getch.inc"
.include "calcCursorAddr2.inc"
.include "drawCursor.inc"
.include "tapeWriteDelay.inc"
.include "tapeRead.inc"
.include "tapeReadDelay.inc"
.include "tapeWrite.inc"
.db 0, 0, 0, 0
.include "tapeLoadInt.inc"
.include "cmp_hl_de.inc"
.include "memcpy_bc_hl.inc"
.include "printString1.inc"
.include "printChar6.inc"
.db 0, 0
.include "drawCursor2.inc"
.include "reboot1.inc"
.db 0
.include "tapeReadError.inc"

; ---------------------------------------------------------------------------

initVars:	.dw -1
		.dw 0C800h		; v_tapeError
		.dw -1			; v_tapeAddr
		.dw -1
#if LOAD_FONT
		.dw FONT_ADDR/8	; v_charGen
#else
		.dw -1			; v_charGen
#endif
		.db 0A9h		; v_cursorCfg
		.db -1			; v_koi8
		.db -1			; v_escMode
		.db 03Ah		; v_keyLocks
		.dw -1
		.db -1			; v_lastLastKey
		.db -1			; v_lastKey
		.db 05Fh, 20h	; v_beep
		.db 0FFh		; v_tapeInverse
		.db 020h		; v_cursorDelay
		.db 0E0h		; byte_8FF5
		.dw -1			; v_oldSP
		.db ARAM_MAX_PAGE; v_maxRamPage
		.db 0           ; v_flashPage
		.dw 0			; v_inverse
		.db -1			; v_cursorY
		.db -1			; v_cursorX
		.db 28h			; v_writeDelay
		.db 3Ch			; v_readDelay
initVarsEnd:	.db 00h

; Keyboard

v_keybTbl:
		.db 81h, 0Ch, 19h, 1Ah, 09h, 1Bh, 20h,  8,  80h, 18h, 0Ah, 0Dh, 0, 0, 0, 0
		.db 71h, 7Eh, 73h, 6Dh, 69h, 74h, 78h, 62h, 60h, 2Ch, 2Fh, 7Fh, 0, 0, 0, 0
		.db 66h, 79h, 77h, 61h, 70h, 72h, 6Fh, 6Ch, 64h, 76h, 7Ch, 2Eh, 0, 0, 0, 0
		.db 6Ah, 63h, 75h, 6Bh, 65h, 6Eh, 67h, 7Bh, 7Dh, 7Ah, 68h, 3Ah, 0, 0, 0, 0
		.db 3Bh, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 38h, 39h, 30h, 2Dh, 0, 0, 0, 0
		.db 00h, 01h, 02h, 03h, 04h, 05h, 06h, 07h, 8Ah, 8Bh, 8Ch, 1Fh, 0, 0, 0, 0

.include "printer.inc"
.include "printString.inc"
.include "reboot2.inc"
.include "getch3.inc"
.include "printChar2.inc"	; Continued in scrollDown
.include "scrollDown.inc"
.include "scrollUp2.inc"
.include "checkARAM.inc"

; Not used (free space?)

;		.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
;		.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
;		.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
;		.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
;		.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
;		.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
;		.db 0FFh

.org 0C800h

.include "jmps_c800.inc"
.include "setGetCursorPos.inc"
.include "setGetMemTop.inc"
.include "printHex.inc"
.include "input.inc"
.include "cmp_hl_de_2.inc"
.include "sbb_de_hl_to_hl.inc"
.include "memmove_bc_hl.inc"
.include "calcCS.inc"
.include "tapeSave.inc"
.include "tapeWriteWord.inc"
.include "tapeLoad.inc"
.include "reboot3.inc"
.include "fileExecBat.inc"
.include "fileExec.inc"
.include "fileCmpExt.inc"
.include "driver_FFC0.inc"
.include "driver.inc"
.include "installDriver.inc"
.include "fileGetSetDrive.inc"
.include "loadSaveFatDir.inc"
.include "fileFindCluster.inc"
.include "fileCreate.inc"
.include "fileFind.inc"
.include "fileLoad.inc"
.include "fileDelete.inc"
.include "fileRename.inc"
.include "fileGetSetAttr.inc"
.include "fileGetSetAddr.inc"
.include "fileGetInfoAddr.inc"
.include "fileList.inc"
.include "fileNamePrepare.inc"
.include "memset_de_20_b.inc"

#if RAMFOS_COMPATIBILITY
.include "strToHex.inc"
#endif

; ---------------------------------------------------------------------------

aBadCommandOrFi:	.db 0Ah,"BAD COMMAND OR FILE NAME",0
v_drives:			.dw diskDriver, diskDriver, diskDriver, diskDriver  ; Addresses of drivers for 8 disks
					.dw diskDriver, diskDriver, diskDriver, diskDriver  ; (initial value = diskDriver = C863h)
v_findCluster:		.dw 0
v_drive:			.db 1
v_input_start:		.dw 0
v_createdFile:		.dw 0
v_foundedFile:		.dw 0
v_input_end:		.dw 0
v_batPtr:			.dw 0               ; aaddress of the buffer for BAT file content
v_memTop:			.dw 0FAFFh          ; top memory address

aANc_com:			.db "A:NC.COM",0
aAAutoex_bat:		.db "A:AUTOEX.BAT",0
aBat:				.db "BAT"
aCom:				.db "COM"
aExe:				.db "EXE"

v_fileName:			.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
v_fileName_ext:		.db 0FFh, 0FFh, 0FFh
					.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
v_fileName_end:		.db 0FFh ; Copied including this unused byte
v_batFileName:		.db 0FFh, 0FFh,	0FFh
					.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
v_batFileN_end:		.db 0FFh ; Copied including this unused byte

#if LOAD_FONT
.include "font.inc"
#else
notused:	.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
			.db 0FFh, 0FFh,	0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
			.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
			.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
			.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
			.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
			.db 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
#endif

.end
