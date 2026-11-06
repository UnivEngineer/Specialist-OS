; Алгоритм из docs/diskbench/ramd-002/stack.inc (31.10.2026).
; Буферный обмен: 14 порций по 18 байт и последняя 4 байта.
; Копии FF00h..FFAFh установлены в основной RAM и всех страницах диска.
; Вход FFC0h/FFD0h прежний: C=страница, DE=адрес сектора, HL=буфер.
; Разрушает AF/BC/DE/HL, флаги. SP и основная RAM восстановлены перед RET.
; Не реентерабелен; прерывания запрещены системным контрактом MXOS.
; POP/PUSH только BC/DE/HL: PSW не переносит произвольные данные на 8080.
; Полный C863h при 8080 без ожиданий: чтение 20584, запись 20548 тактов;
; худший допустимый буфер FE00h: 20600/20564. Установка сюда не входит.
; Код хвоста 159 байт. На сектор: 45 записей маппера и 1024 обращения
; к полезным данным (диск, общий буфер, основная RAM), без учёта параметров.

ramdBuffer = 0FFC3h
ramdBuffer2 = 0FFD3h
ramdSetSP  = 0FFDAh

driverBufferedStart:
            jp      ramdBufferedRead
            BLOCK   13, 0
            jp      ramdBufferedWrite
            BLOCK   7, 0
            ld      sp, 0
            jp      ramdTailFillSecond
driverBufferedEnd:
            ASSERT driverBufferedEnd-driverBufferedStart == 32

; Подготовка выполняется в основной RAM, даже пока SP указывает на данные.
; Между сохранением и восстановлением SP здесь нет CALL/PUSH/POP.
ramdBufferedRead:
            xor     a
            ex      de, hl
            jp      ramdBufferedInit
ramdBufferedWrite:
            ld      a, 1
ramdBufferedInit:
            ld      (v_ramdMode), a
            ld      (v_ramdSource), hl
            ex      de, hl
            ld      (v_ramdTarget), hl
            ld      a, c
            ld      (v_ramdPage), a
            ld      hl, 0
            add     hl, sp
            ld      (v_ramdOldSP), hl
            ld      a, 15
            ld      (v_ramdCount), a
ramdBufferedLoop:
            ld      hl, (v_ramdSource)
            ld      sp, hl
            ld      de, 6
            add     hl, de
            ld      (ramdSetSP+1), hl
            add     hl, de
            add     hl, de
            ld      (v_ramdSource), hl
            ld      hl, ramdTailFillSecond
            ld      (ramdSetSP+4), hl
            ld      a, (v_ramdMode)
            or      a
            jp nz,  ramdBufferedFillMain
            ld      a, (v_ramdCount)
            cp      1
            ld      a, (v_ramdPage)
            jp      ramdTailFillBank
ramdBufferedFillMain:
            ld      a, (v_ramdCount)
            cp      1
            jp      ramdTailFillFirst
ramdBufferedFilled:
            ld      hl, (v_ramdTarget)
            ld      de, 6
            ld      a, (v_ramdCount)
            cp      1
            jp nz,  ramdBufferedTargetSize
            ld      e, 4
ramdBufferedTargetSize:
            add     hl, de
            ld      (ramdSetSP+1), hl
            add     hl, de
            add     hl, de
            ld      (v_ramdTarget), hl
            ld      hl, ramdTailPushFirst
            ld      (ramdSetSP+4), hl
            ld      a, (v_ramdMode)
            or      a
            jp z,   ramdBufferedLoadMain
            ld      a, (v_ramdCount)
            cp      1
            ld      a, (v_ramdPage)
            jp      ramdTailLoadBank
ramdBufferedLoadMain:
            ld      a, (v_ramdCount)
            cp      1
            jp      ramdTailLoadFirst
ramdBufferedFinished:
            ld      a, (v_ramdCount)
            dec     a
            ld      (v_ramdCount), a
            jp nz,  ramdBufferedLoop
            ld      hl, (v_ramdOldSP)
            ld      sp, hl
            ret
v_ramdSource: DW 0
v_ramdTarget: DW 0
v_ramdOldSP:  DW 0
v_ramdPage:   DB 0
v_ramdCount:  DB 0
v_ramdMode:   DB 0

ramdTailImage:
    DISP 0FF00h
ramdTailFillBank:
            ld      (IO_PAGE_RAMD), a
ramdTailFillFirst:
            pop     bc
            pop     de
            jp z,   ramdTailFill4
            pop     hl
ramdTailFill4:
            ld      sp, ramdBuffer+6
            jp z,   ramdTailSave4
            push    hl
ramdTailSave4:
            push    de
            push    bc
            jp z,   ramdTailFilled
            jp      ramdSetSP
ramdTailFillSecond:
            pop     bc
            pop     de
            pop     hl
            ld      sp, ramdBuffer+12
            push    hl
            push    de
            push    bc
            ld      hl, (ramdSetSP+1)
            ld      de, 6
            add     hl, de
            ld      (ramdSetSP+1), hl
            ld      hl, ramdTailFillThird
            ld      (ramdSetSP+4), hl
            jp      ramdSetSP
ramdTailFillThird:
            pop     bc
            pop     de
            pop     hl
            ld      sp, ramdBuffer2+6
            push    hl
            push    de
            push    bc
ramdTailFilled:
            ld      (IO_PAGE_RAM), a
            jp      ramdBufferedFilled
ramdTailLoadBank:
            ld      (IO_PAGE_RAMD), a
ramdTailLoadFirst:
            ld      sp, ramdBuffer
            jp nz,  ramdTailLoad6
            ld      sp, ramdBuffer+2
ramdTailLoad6:
            pop     bc
            pop     de
            jp z,   ramdSetSP
            pop     hl
            jp      ramdSetSP
ramdTailPushFirst:
            jp z,   ramdTailPush4
            push    hl
ramdTailPush4:
            push    de
            push    bc
            jp z,   ramdTailFinished
            ld      hl, (ramdSetSP+1)
            ld      de, 6
            add     hl, de
            ld      (ramdSetSP+1), hl
            ld      hl, ramdTailPushSecond
            ld      (ramdSetSP+4), hl
            ld      sp, ramdBuffer+6
            pop     bc
            pop     de
            pop     hl
            jp      ramdSetSP
ramdTailPushSecond:
            push    hl
            push    de
            push    bc
            ld      hl, (ramdSetSP+1)
            ld      de, 6
            add     hl, de
            ld      (ramdSetSP+1), hl
            ld      hl, ramdTailPushThird
            ld      (ramdSetSP+4), hl
            ld      sp, ramdBuffer2
            pop     bc
            pop     de
            pop     hl
            jp      ramdSetSP
ramdTailPushThird:
            push    hl
            push    de
            push    bc
ramdTailFinished:
            ld      (IO_PAGE_RAM), a
            jp      ramdBufferedFinished
ramdTailEnd:
            ASSERT ramdTailEnd <= NC_STATE_ADDR
    ENT
ramdTailImageEnd:
