;+---------------------------------------------------------------------------
; MXOS
; FLASH.COM - драйвер флеш-диска ВВ55
;
; Драйвер имеет три функции (номер передаётся в регистре Е):
; 1 - записать сектор (256 байт, номер сектора в регистре D, адрес буфера в HL);
; 2 - считать сектор  (256 байт, номер сектора в регистре D, адрес буфера в HL);
; 3 - выдать размер диска (в секторах, в регистре А).
;
; 2022-01-24 Дизассемблировано и доработано SpaceEngineer
;
;----------------------------------------------------------------------------

; Функции DOS
fileGetSetDrive	= 0C842h	; Получить/установить активное устройство
installDriver   = 0C860h	; Установить драйвер накопителя

; Порты
IO_KEYB_MODE = 0FFE3h
IO_PROG_A    = 0FFE4h
IO_PROG_B    = 0FFE5h
IO_PROG_C    = 0FFE6h
IO_PROG_MODE = 0FFE7h

.ORG 0FA00H

    ; Установить драйвер для накопителья 7 ("H")
	MVI  A, 7
	LXI  H, Driver
	JMP	 installDriver

    ; Выбрать накопитель 7 ("H")
	MVI	 E, 1
	MVI	 A, 7
	JMP	 fileGetSetDrive

Driver:
    MOV	 A, E
	CPI	 1
	RZ          ; Запись не поддерживается
	PUSH H
	PUSH D
	PUSH B

    ; Настройка порта ВВ55
	MVI	A, 90H
	STA	IO_PROG_MODE
	MVI	A, 0DH ; ??? порт клавиатуры
	STA	IO_KEYB_MODE

	MOV	A ,E
	CPI	3
	JZ	FuncGetSize
	CPI	2
	JNZ	Exit

    ; Чтение блока
    ; вход:
    ; D  - номер блока
    ; HL - адрес буфера в памяти
FuncRead:
	XRA	 A
	MOV	 E, A
ReadLoop:
    CALL Read
	MOV	 M, A
	INX	 H
	INR	 E
	JZ   Exit
	JMP  ReadLoop

    ; Определение обьема накопителя
    ; выход:
    ; A - количество секторов
FuncGetSize:
    XRA	A
	MOV	B, A
	MOV	D, A
	MVI	E, 4 ; начальный байт в секторе
LBL3:
    CALL Read
	CPI  0FFH
	JNZ  LBL4 ; прыгнуть, если не конец диска
	INR  B    ; увеличить счетчик байтов
LBL4:
    INR  E    ; следующий байт в секторе
	MOV  A, E
	CPI  0C0H
	JNZ  LBL3
	MVI  A, 0C0H
	SUB  B

    ; Восстановление портов и выход
Exit:
    PUSH PSW
	MVI  A, 0CH
	STA  IO_KEYB_MODE
	MVI  A, 9BH
	STA  IO_PROG_MODE
	POP  PSW
	POP  B
	POP  D
	POP  H
	RET

    ; Чтение данных
    ; вход:
    ; DE - адрес в диске
    ; выход:
    ; A - данные
Read:
    XCHG
	SHLD IO_PROG_B
	LDA IO_PROG_A
	XCHG
	RET

.END
