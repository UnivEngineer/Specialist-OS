; Внутренний опрос A: для реестра DOS: B=3 -> BC/DE, B=FFh -> BC.
; DISK_INFO и строки принадлежат DOS; указатель MAIN наружу не передаётся.
mainIoQuery:
    ld b,a
    xor a
    call bios_blockIO
    ld hl,0
    ret
mainIoRequest = io_vars.mainIoRequest
mainIoCopy = io_vars.mainIoCopy
