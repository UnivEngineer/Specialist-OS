; Ассоциации и файловые команды выполняются последовательно.
    IFDEF BANKED_DOS_BUILD
cliIoBuffer = DOS_BANK_DATA_ADDR
cliAssociationBuffer = cliIoBuffer
    ELSE
cliIoBuffer:
cliAssociationBuffer: DS 512,0
    ENDIF

