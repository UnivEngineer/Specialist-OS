;+---------------------------------------------------------------------------
; MXOS
; Запуск RKS файла
;
; 2021-01-27 Разработано SpaceEngineer
;----------------------------------------------------------------------------

; Адрес загрузки монитора
MON_ADDR = 0C000h

; Адрес временного буфера загрузки монитора
MON_ADDR_TEMP = 0D000h

; Функции системы
getch			= 0C803h ; Ожидание ввода с клавиатуры
printChar		= 0C809h ; Вывод символа на экран
printString 	= 0C818h ; Вывести строку на экран
fileLoad		= 0C848h ; Загрузить файл по адресу из заголовка этого файла
fileNamePrepare	= 0C85Ah ; Преобразовать имя файла во внутренний формат
fileLoad2		= 0C866h ; Загрузить файл по адресу DE

.org 0F100h

        ; В DE передаётся адрес строки аргументов
        push d
        lxi  h, aLoading
        call printString
		pop  h
        push h
        call printString
        pop  h

        ; Загрузка программы
        ; Подготовка имени файла и переключение накопителя
		lxi  d, nameBuffer
		call fileNamePrepare   ; HL = имя файла = строка аргументов
        xchg

		; Загружаем файл
		lxi  h, nameBuffer
		call fileLoad
		jc   fileNotFoundRet

		; Получаем адрес загрузки (= адрес запуска) файла в DE
		lxi	d, 10
		dad	d   ; HL += 10
		mov	e, m
		inx	h
		mov	d, m

        ; Помещаем в стек для передачи Монитору
        push d

        lxi  h, aLoading
        call printString
		lxi  h, aMonitorPath
        call printString

        ; Загрузка Монитора
        ; Подготовка имени файла A:MON2.SYS
		lxi  h, aMonitorPath
		lxi  d, nameBuffer
		call fileNamePrepare

        ; Загружаем файл MON2.SYS на 0D000h - он затрет NC.COM
		lxi  h, nameBuffer
		lxi  d, MON_ADDR_TEMP ; изменить адрес загрузки файла на DE
		call fileLoad2  ; нужна исправленная функция! BIOS 4.50 и старше 
		jc   popFileNotFoundRet

		; Получаем размер файла Монитора в DE
		lxi	d, 12
		dad	d   ; HL += 12
		mov	e, m
		inx	h
		mov	d, m

        ; HL = MON_ADDR_TEMP
        ; DE = MON_ADDR_TEMP + размер монитора
		lxi  h, MON_ADDR_TEMP
        xchg
		dad  d
        xchg

        ; Копируем Монитор на адрес C000h. При этом он затрёт BIOS, и дисковые
        ; функциии ОС станут недоступны. Поэтому и нужен был временный буфер.
		lxi  b, MON_ADDR
        call memcpy

        ; Запуск Монитора. Монитор сам переходит в режим STD, инициализируется,
        ; и запускает программу по адресу из вершины стека.
        jmp  MON_ADDR

memcpy: ; Копироваение из HL в BC с увеличением адресов, пока HL не равно DE
        mov  a, m
        stax b
        call cmp_hl_de
        rz
        inx  h
        inx  b
        jmp  memcpy

cmp_hl_de:
        mov	a, l
        cmp	e
        rnz
        mov	a, h
        cmp	d
        ret

popFileNotFoundRet:
        pop  d ; восстанавливаем стек для правильного аварийного выхода
fileNotFoundRet:
        lxi  h, aFileNotFound
        jmp  printString

        ; Esc + ) включает KOI-8 до конца выводимой строки
aLoading:
        .db 0Ah,1Bh,"(Loading ",0

aFileNotFound:
        .db 0Ah,1Bh,"(File not found",0

aMonitorPath:
        .db "A:MON2.SYS",0

nameBuffer:
        .block 10

.end
