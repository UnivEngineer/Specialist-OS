reportMode:         DB 0
reportOverflow:     DB 0
reportWriteFailed:  DB 0
reportDrive:        DB 0
reportPointer:      DW 0
reportSize:         DW 0
reportDirectory:    DW 0
reportMessage:      DW reportFailedText
reportName:         DB 'FONTBEN TXT'
reportHandle:       DS FILE_HANDLE,0
reportNote:         DB '1152 bytes; MAIN / BANK; milliseconds',13,10,0
reportSavedText:    DB 'Saved FONTBEN.TXT on RAM disk',13,10,0
reportFailedText:   DB 'Cannot save FONTBEN.TXT on RAM disk',13,10,0
