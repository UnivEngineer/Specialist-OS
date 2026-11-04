; Экспериментальный ABI: A = страница возврата, B = функция, C = параметр.
; 0: символ C; 1: C байт из общего буфера; 2: пустой вызов.
; Прерывания запрещены, все регистры и флаги сохраняются.
BUFFER_GATE = 0FFC0h
BUFFER_RETURN = 0FFC7h
BUFFER_ADDR = 0FFCCh
BUFFER_SIZE = 20
BUFFER_DISPATCH = 0F880h
