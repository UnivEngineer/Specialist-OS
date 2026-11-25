; DEC номера страницы перед LD координат оставил минус только у MAIN.
memFixedWordReturn:
    ASSERT memFixedWordReturn == MEM_WORD_RETURN
    jp m,memFixedWordDone
    ld (BANK_IO_RAM),a
memFixedWordDone:
    pop af
    ret
