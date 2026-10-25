; Исторический магнитофонный формат и проверка записи
; Порядок включения задан главным .asm; см. README.md.

keyHandler_Esc_O:
        LD    HL,txtTapePrompt             ; 53370
        CALL  bios_printString
        CALL  promptTapeName
        JP    NZ,synchronizeTopAndRedraw
        LD    HL,(v_textBeginLo)
        CALL  sumTextUntilEnd
        PUSH  BC
        PUSH  DE
        CALL  writeTapeLeader
        EX    (SP),HL
        EX    DE,HL
        LD    HL,startupBannerAndLineBuffer
        LD    A,0E6H
        LD    B,05H
writeTapeNameSync:  CALL  writeTapeByte
        DEC   B
        JP    NZ,writeTapeNameSync
writeTapeNameLoop:  LD    A,(HL)
        CALL  writeTapeByte
        OR    A
        JP    Z,writeTapePayload
        INC   HL
        JP    writeTapeNameLoop
writeTapePayload:  CALL  writeTapeGap
        POP   DE
        LD    A,(v_textBeginLo)
        cpl
        INC   A
        LD    L,A
        LD    A,(v_textBeginHi)
        cpl
        INC   A
        LD    H,A
        ADD   HL,DE
        LD    A,0E6H
        CALL  writeTapeByte
        LD    A,L
        cpl
        CALL  writeTapeByte
        LD    A,H
        cpl
        LD    HL,(v_textBeginLo)
        CALL  writeTapeRange
        POP   BC
        LD    A,C
        CALL  writeTapeByte
        LD    A,B
        CALL  writeTapeByte
        JP    synchronizeTopAndRedraw
writeTapeLeader:  LD    D,04H         ; 4
        XOR   A
writeLeaderBlock:  LD    E,40H         ; 64 '@'
        XOR   55H           ; 85 'U'
        CALL  writeTapeRepeat
        DEC   D
        JP    NZ,writeLeaderBlock
        RET
writeTapeGap:  CALL  writeTapeZeroBlock
writeTapeZeroBlock:  XOR   A
        LD    E,A
writeTapeRepeat:  CALL  writeTapeByte         ; [2]
        DEC   E
        JP    NZ,writeTapeRepeat
        RET
writeTapeRange:  CALL  writeTapeByte         ; [2]
        CALL  compareHLDE
        LD    A,(HL)
        INC   HL
        JP    NZ,writeTapeRange
        JP    writeTapeByte
writeTapeByte:  LD    C,A           ; [9]
        JP    bios_tapeWrite

keyHandler_Esc_I:
        LD    B,00H

; Вход: B = 0 загрузка / 1 добавление / 0FFh проверка.
; Выход: перерисовка редактора либо сообщение об ошибке; регистры не сохраняются.
; При проверке совпавшие байты всё равно записываются обратно в память.
readOrVerifyTape:  LD    A,B           ; [2]
        LD    (v_tapeMode),A
        LD    HL,0000H
        LD    (v_selectionBegin),HL
        LD    HL,txtTapePrompt             ; 53370
        CALL  bios_printString
        CALL  promptTapeName
        JP    NZ,synchronizeTopAndRedraw
        CALL  readTapeHeader
        PUSH  HL
readTapeTextLoop:  CALL  readTapeByte
        LD    B,A
        LD    A,(v_tapeMode)
        INC   A
        JP    NZ,storeTapeTextByte
        LD    A,B
        CP    (HL)
        JP    NZ,tapeReadMismatch
storeTapeTextByte:  LD    (HL),B
        INC   HL
        INC   B
        JP    NZ,readTapeTextLoop
        LD    A,08H
        CALL  readTapeWord
        POP   HL
        PUSH  BC
        CALL  sumTextUntilEnd
        EX    (SP),HL
        LD    D,B
        LD    E,C
        CALL  compareHLDE
        JP    NZ,tapeReadMismatch
        POP   HL
        LD    (v_textEnd),HL
        JP    keyHandler_Esc_Home

tapeReadMismatch:  POP   HL
        LD    HL,txtTapeError
        CALL  bios_printString
        CALL  Beep4
        CALL  bios_getch
        LD    HL,(v_textEnd)
        LD    (HL),0FFH
        JP    keyHandler_Esc_Home

; САМОМОДИФИКАЦИЯ: имя с ленты пишется поверх операнда и стартового кода.
; Вход: имя запроса в startupBannerAndLineBuffer. Выход: Z при совпадении.
; Разрушает AF, BC, DE, HL, флаги. Длина входящего имени не ограничена.
readAndCompareTapeName:  LD    HL,smc_sessionTarget+1 ; начало буфера имени с ленты
        CALL  findTapeNameSync
readTapeNameLoop:  CALL  readTapeByte
        LD    (HL),A
        OR    A
        JP    Z,showTapeName
        INC   HL
        JP    readTapeNameLoop

showTapeName:  LD    HL,txtTapeFile
        CALL  bios_printString
        LD    HL,smc_sessionTarget+1 ; тот же буфер имени поверх кода
        PUSH  HL
        CALL  bios_printString
        CALL  tapeNamePause
        POP   HL
        LD    DE,startupBannerAndLineBuffer
compareTapeNameLoop:  LD    A,(DE)
        OR    A
        RET   Z
        CP    (HL)
        INC   HL
        INC   DE
        JP    Z,compareTapeNameLoop
        RET
tapeNamePause:  LD    HL,0000H
        LD    E,L
        LD    D,H
tapePauseLoop:  DEC   HL
        CALL  compareHLDE
        RET   Z
        JP    tapePauseLoop
readTapeWordSync:  LD    A,0FFH
readTapeWord:  CALL  bios_tapeRead
        LD    C,A
        CALL  readTapeByte
        LD    B,A
        RET
resetTapePolarity:  XOR   A
        LD    (bios_vars.tapeInverse),A

findTapeNameSync:  LD    B,04H
        LD    A,0FFH
findTapeSyncLoop:  CALL  bios_tapeRead
        CP    0E6H
        JP    NZ,resetTapePolarity
        DEC   B
        LD    A,08H
        JP    NZ,findTapeSyncLoop
        RET

readTapeByte:  LD    A,08H
        JP    bios_tapeRead

readTapeHeader:  CALL  readAndCompareTapeName
        JP    NZ,readTapeHeader
        CALL  readTapeWordSync
        LD    HL,(v_textBeginLo)
        LD    A,(v_tapeMode)
        DEC   A
        JP    M,checkTapeCapacity
        LD    HL,(v_textEnd)
checkTapeCapacity:  LD    A,B
        cpl
        LD    B,A
        LD    A,C
        cpl
        LD    C,A
        PUSH  HL
        EX    DE,HL
        LD    HL,(v_textLimitLo)
        EX    DE,HL
        ADD   HL,BC
        CALL  compareHLDE
        POP   HL
        RET   C
        JP    documentOutOfMemory

keyHandler_Esc_V:
        LD    B,0FFH                ; 255
        JP    readOrVerifyTape

keyHandler_Esc_G:
        LD    B,01H         ; 1
        JP    readOrVerifyTape

sumTextUntilEnd:  LD    BC,0000H    ; [2]
sumTextByte:  LD    A,(HL)
        CP    0FFH          ; 255
        RET   Z
        ADD   A,C
        LD    C,A
        LD    A,00H
        ADC   A,B
        LD    B,A
        INC   HL
        JP    sumTextByte
promptTapeName:  LD    B,30H         ; 48 '0'; [2]
        LD    HL,startupBannerAndLineBuffer             ; 53258
        LD    E,L
        LD    D,H
promptTextKey:  CALL  bios_getch          ; [5]
promptTextInput:  CP    20H           ; 32 ' '
        JP    C,promptControlKey
        DEC   B
        JP    NZ,storePromptCharacter
        INC   B
        JP    promptTextKey
storePromptCharacter:  LD    C,A
        CALL  bios_printChar
        LD    (HL),A
        INC   HL
        JP    promptTextKey
        LD    BC,2A20H    ; 10784
        CALL  keyHandler_Home
        JP    TypeCharCtimesB
promptControlKey:  CP    1FH           ; 31
        JP    NZ,promptBackspace
        OR    A
        RET
promptBackspace:  CP    08H           ; 8
        JP    NZ,promptEnter
        CALL  compareHLDE
        JP    Z,promptTextKey
        PUSH  HL
        LD    HL,txtEraseChar    ; 53380
        CALL  bios_printString
        POP   HL
        DEC   HL
        INC   B
        JP    promptTextKey
promptEnter:  CP    0DH           ; 13
        JP    NZ,promptTextKey
        XOR   A
        LD    (HL),A
        RET

