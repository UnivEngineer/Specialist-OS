cliSavedDirectories: DS 16,0
cliOldDrive: DB 0
cliOldValid: DB 0
cliOldDirectory: DW 0
cliMoveFlag: DB 0
cliParent: DW 0
cliNewCluster: DW 0
cliRemaining: DW 0
cliCursor: DS FILE_STREAM_CURSOR,0
cliRequest: FILE_COPY_REQUEST
; Ассоциации и файловые команды выполняются последовательно.
cliIoBuffer:
cliAssociationBuffer: DS 512,0

