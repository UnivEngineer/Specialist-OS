; Матрица клавиатуры: индексирование младшего байта требует одной страницы.
v_keybTbl:
                DB 81h, 0Ch, 19h, 1Ah, 09h, 1Bh, 20h, 08h, 80h, 18h, 0Ah, 0Dh, 0, 0, 0, 0
                DB 71h, 7Eh, 73h, 6Dh, 69h, 74h, 78h, 62h, 60h, 2Ch, 2Fh, 7Fh, 0, 0, 0, 0
                DB 66h, 79h, 77h, 61h, 70h, 72h, 6Fh, 6Ch, 64h, 76h, 7Ch, 2Eh, 0, 0, 0, 0
                DB 6Ah, 63h, 75h, 6Bh, 65h, 6Eh, 67h, 7Bh, 7Dh, 7Ah, 68h, 3Ah, 0, 0, 0, 0
                DB 3Bh, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 38h, 39h, 30h, 2Dh, 0, 0, 0, 0
                DB 00h, 01h, 02h, 03h, 04h, 05h, 06h, 07h, 8Ah, 8Bh, 8Ch, 1Fh, 0, 0, 0, 0

    ASSERT (v_keybTbl >> 8) == ((v_keybTbl + 95) >> 8)
