fontCorpus:
    INCBIN "corpus.koi8",0,FONT_CORPUS_SIZE
    ASSERT $-fontCorpus == FONT_CORPUS_SIZE
