; Восстановлено из originals/Monitor2.mon; адреса и байты сохранены.


; Вход LAUNCH: HL лежит на стеке; POP BC; запись A в FFFFh; режим STD, 8 цветов.
monitor2_start:
    pop bc                                     ; C000h
    ld (0FFFFh),a                              ; C001h
    ld sp,07FFFh                               ; C004h
    ld a,003h                                  ; C007h
    ld (0F7FAh),a                              ; C009h
    push bc                                    ; C00Ch
    jp loader_init                             ; C00Dh

; Очистка экрана стековыми PUSH. Сохраняет HL/BC и SP; AF разрушается.
loader_clearScreen:
    push hl                                    ; C010h
    push bc                                    ; C011h
    ld hl,00000h                               ; C012h
    add hl,sp                                  ; C015h
    ld (08FF6h),hl                             ; C016h
    ld sp,monitor2_start                       ; C019h
    ld hl,(08FFAh)                             ; C01Ch
    ld bc,00300h                               ; C01Fh
LC022:
    push hl                                    ; C022h
    push hl                                    ; C023h
    push hl                                    ; C024h
    push hl                                    ; C025h
    push hl                                    ; C026h
    push hl                                    ; C027h
    push hl                                    ; C028h
    push hl                                    ; C029h
    dec bc                                     ; C02Ah
    ld a,b                                     ; C02Bh
    or c                                       ; C02Ch
    jp nz,LC022                                ; C02Dh
    ld hl,(08FF6h)                             ; C030h
    ld sp,hl                                   ; C033h
    pop bc                                     ; C034h
    pop hl                                     ; C035h
    ret                                        ; C036h

; Вывод символа C. Сохраняет AF/BC/DE/HL; использует подэкранные переменные.
loader_printChar:
    push hl                                    ; C037h
    push de                                    ; C038h
    push bc                                    ; C039h
    push af                                    ; C03Ah
    ld a,c                                     ; C03Bh
    cp 021h                                    ; C03Ch
    jp c,LC0D4                                 ; C03Eh
LC041:
    ld hl,(08FFCh)                             ; C041h
    ld a,h                                     ; C044h
    cp 0BEh                                    ; C045h
    jp nc,LC0B2                                ; C047h
LC04A:
    add a,003h                                 ; C04Ah
    ld (08FFDh),a                              ; C04Ch
    ex de,hl                                   ; C04Fh
LC050:
    ld a,c                                     ; C050h
    ld (08FE9h),a                              ; C051h
    sub 020h                                   ; C054h
    ld hl,(08FE7h)                             ; C056h
    add a,l                                    ; C059h
    ld l,a                                     ; C05Ah
    add hl,hl                                  ; C05Bh
    add hl,hl                                  ; C05Ch
    add hl,hl                                  ; C05Dh
    ex de,hl                                   ; C05Eh
    nop                                        ; C05Fh
    ld a,h                                     ; C060h
    and 003h                                   ; C061h
    ld c,a                                     ; C063h
    ld a,005h                                  ; C064h
    sub c                                      ; C066h
    ld c,a                                     ; C067h
    ld a,h                                     ; C068h
    and 0FCh                                   ; C069h
    rrca                                       ; C06Bh
    rrca                                       ; C06Ch
    add a,090h                                 ; C06Dh
    ld h,a                                     ; C06Fh
    ld (08FF8h),hl                             ; C070h
    ld b,008h                                  ; C073h
    nop                                        ; C075h
LC076:
    ld a,(de)                                  ; C076h
    ld l,a                                     ; C077h
    ld h,000h                                  ; C078h
    ld a,c                                     ; C07Ah
LC07B:
    add hl,hl                                  ; C07Bh
    add hl,hl                                  ; C07Ch
    dec a                                      ; C07Dh
    jp nz,LC07B                                ; C07Eh
    push hl                                    ; C081h
    inc de                                     ; C082h
    dec b                                      ; C083h
    jp nz,LC076                                ; C084h
    ld b,008h                                  ; C087h
    ld hl,(08FF8h)                             ; C089h
LC08C:
    pop de                                     ; C08Ch
    ld a,d                                     ; C08Dh
    call LC163                                 ; C08Eh
    ld (hl),a                                  ; C091h
    inc h                                      ; C092h
    ld a,e                                     ; C093h
    call LC163                                 ; C094h
    ld (hl),a                                  ; C097h
    dec h                                      ; C098h
    dec l                                      ; C099h
    dec b                                      ; C09Ah
    jp nz,LC08C                                ; C09Bh
LC09E:
    pop af                                     ; C09Eh
    pop bc                                     ; C09Fh
    pop de                                     ; C0A0h
    pop hl                                     ; C0A1h
    ret                                        ; C0A2h
LC0A3:
    ld c,a                                     ; C0A3h
    ld a,(08FFAh)                              ; C0A4h
    or a                                       ; C0A7h
    jp nz,LC0AF                                ; C0A8h
    ld a,c                                     ; C0ABh
    cpl                                        ; C0ACh
    and (hl)                                   ; C0ADh
    ret                                        ; C0AEh
LC0AF:
    ld a,(hl)                                  ; C0AFh
    or c                                       ; C0B0h
    ret                                        ; C0B1h
LC0B2:
    ld a,l                                     ; C0B2h
    cp 0F5h                                    ; C0B3h
    jp nc,LC0C4                                ; C0B5h
    add a,00Ah                                 ; C0B8h
    ld l,a                                     ; C0BAh
    ld (08FFCh),a                              ; C0BBh
    ld h,000h                                  ; C0BEh
    ld a,h                                     ; C0C0h
    jp LC04A                                   ; C0C1h
LC0C4:
    call LC22D                                 ; C0C4h
    nop                                        ; C0C7h
    call loader_clearScreen                    ; C0C8h
    ld hl,00008h                               ; C0CBh
    ld (08FFCh),hl                             ; C0CEh
    jp LC041                                   ; C0D1h
LC0D4:
    ld hl,(08FFCh)                             ; C0D4h
    cp 020h                                    ; C0D7h
    jp z,LC107                                 ; C0D9h
    cp 00Ah                                    ; C0DCh
    jp z,LC113                                 ; C0DEh
    cp 00Dh                                    ; C0E1h
    jp z,LC128                                 ; C0E3h
    cp 018h                                    ; C0E6h
    jp z,LC107                                 ; C0E8h
    cp 008h                                    ; C0EBh
    jp z,LC12D                                 ; C0EDh
    cp 019h                                    ; C0F0h
    jp z,LC13B                                 ; C0F2h
    cp 01Ah                                    ; C0F5h
    jp z,LC147                                 ; C0F7h
    cp 00Ch                                    ; C0FAh
    jp z,LC15A                                 ; C0FCh
    cp 01Fh                                    ; C0FFh
    jp z,LC157                                 ; C101h
    jp LC15D                                   ; C104h
LC107:
    ld a,h                                     ; C107h
    cp 0BEh                                    ; C108h
    jp nc,LC113                                ; C10Ah
    add a,003h                                 ; C10Dh
    ld h,a                                     ; C10Fh
    jp LC15D                                   ; C110h
LC113:
    ld h,000h                                  ; C113h
    ld a,l                                     ; C115h
    cp 0F5h                                    ; C116h
    jp nc,LC121                                ; C118h
    add a,00Ah                                 ; C11Bh
    ld l,a                                     ; C11Dh
    jp LC15D                                   ; C11Eh
LC121:
    call LC22D                                 ; C121h
    nop                                        ; C124h
    jp LC157                                   ; C125h
LC128:
    ld h,000h                                  ; C128h
    jp LC15D                                   ; C12Ah
LC12D:
    ld a,h                                     ; C12Dh
    cp 002h                                    ; C12Eh
    jp c,LC139                                 ; C130h
    sub 003h                                   ; C133h
    ld h,a                                     ; C135h
    jp LC15D                                   ; C136h
LC139:
    ld h,0BDh                                  ; C139h
LC13B:
    ld a,l                                     ; C13Bh
    cp 011h                                    ; C13Ch
    jp c,LC15D                                 ; C13Eh
    sub 00Ah                                   ; C141h
    ld l,a                                     ; C143h
    jp LC15D                                   ; C144h
LC147:
    ld a,l                                     ; C147h
    cp 0F5h                                    ; C148h
    jp nc,LC15D                                ; C14Ah
    add a,00Ah                                 ; C14Dh
    ld l,a                                     ; C14Fh
    jp LC15D                                   ; C150h
    nop                                        ; C153h
    nop                                        ; C154h
    nop                                        ; C155h
    nop                                        ; C156h
LC157:
    call loader_clearScreen                    ; C157h
LC15A:
    ld hl,00008h                               ; C15Ah
LC15D:
    ld (08FFCh),hl                             ; C15Dh
    jp LC09E                                   ; C160h
LC163:
    ld c,a                                     ; C163h
    ld a,(08FE9h)                              ; C164h
    cp 07Fh                                    ; C167h
    ld a,c                                     ; C169h
    jp z,LC0A3                                 ; C16Ah
    xor (hl)                                   ; C16Dh
    ret                                        ; C16Eh
    nop                                        ; C16Fh
loader_beep:
    push af                                    ; C170h
    push bc                                    ; C171h
    push hl                                    ; C172h
    ld hl,(08FF1h)                             ; C173h
LC176:
    ld a,00Bh                                  ; C176h
    ld (0FF03h),a                              ; C178h
    call LC18F                                 ; C17Bh
    ld a,00Ah                                  ; C17Eh
    ld (0FF03h),a                              ; C180h
    call LC18F                                 ; C183h
    dec h                                      ; C186h
    jp nz,LC176                                ; C187h
    nop                                        ; C18Ah
    pop hl                                     ; C18Bh
    pop bc                                     ; C18Ch
    pop af                                     ; C18Dh
    ret                                        ; C18Eh
LC18F:
    ld b,l                                     ; C18Fh
loader_delay:
    dec b                                      ; C190h
    jp nz,loader_delay                         ; C191h
    ret                                        ; C194h
    ret                                        ; C195h
    push bc                                    ; C196h
    ld b,015h                                  ; C197h
    call loader_delay                          ; C199h
    pop bc                                     ; C19Ch
    jp LC287                                   ; C19Dh
LC1A0:
    push hl                                    ; C1A0h
    ld a,040h                                  ; C1A1h
LC1A3:
    ld (08FF1h),a                              ; C1A3h
    call loader_beep                           ; C1A6h
    pop hl                                     ; C1A9h
    ret                                        ; C1AAh
LC1AB:
    push hl                                    ; C1ABh
    ld a,05Fh                                  ; C1ACh
    jp LC1A3                                   ; C1AEh
LC1B1:
    push hl                                    ; C1B1h
    push bc                                    ; C1B2h
    ld b,0FFh                                  ; C1B3h
LC1B5:
    call LC283                                 ; C1B5h
    ld a,(08FF4h)                              ; C1B8h
    or a                                       ; C1BBh
    jp z,LC1E2                                 ; C1BCh
    call loader_keybMode82                     ; C1BFh
    ld a,(0FF01h)                              ; C1C2h
    and 002h                                   ; C1C5h
    jp z,LC24B                                 ; C1C7h
LC1CA:
    call loader_keybMode91                     ; C1CAh
    ld a,(0FF00h)                              ; C1CDh
    cp 0FFh                                    ; C1D0h
    jp nz,LC1FF                                ; C1D2h
    ld a,(0FF02h)                              ; C1D5h
    or 0F0h                                    ; C1D8h
    cp 0FFh                                    ; C1DAh
    jp nz,LC205                                ; C1DCh
    jp LC1B5                                   ; C1DFh
LC1E2:
    call loader_keybMode82                     ; C1E2h
    ld a,(0FF01h)                              ; C1E5h
    and 002h                                   ; C1E8h
    jp nz,LC1F2                                ; C1EAh
    ld b,0FFh                                  ; C1EDh
    jp LC1CA                                   ; C1EFh
LC1F2:
    dec b                                      ; C1F2h
    jp nz,LC1CA                                ; C1F3h
    ld a,(08FECh)                              ; C1F6h
    ld (08FF4h),a                              ; C1F9h
    jp LC1B5                                   ; C1FCh
LC1FF:
    ld l,a                                     ; C1FFh
    ld h,0FFh                                  ; C200h
    jp LC208                                   ; C202h
LC205:
    ld h,a                                     ; C205h
    ld l,0FFh                                  ; C206h
LC208:
    ld c,0FBh                                  ; C208h
LC20A:
    inc c                                      ; C20Ah
    add hl,hl                                  ; C20Bh
    jp c,LC20A                                 ; C20Ch
    ld l,c                                     ; C20Fh
    ld b,0FFh                                  ; C210h
LC212:
    call loader_keybMode82                     ; C212h
    ld a,(0FF01h)                              ; C215h
    or 003h                                    ; C218h
    cp 0FFh                                    ; C21Ah
    jp nz,LC235                                ; C21Ch
    dec b                                      ; C21Fh
    jp nz,LC212                                ; C220h
    jp LC1B5                                   ; C223h
LC226:
    ld a,(0FF01h)                              ; C226h
    cpl                                        ; C229h
    and 0F7h                                   ; C22Ah
    ret                                        ; C22Ch
LC22D:
    call LC226                                 ; C22Dh
    call nz,LC260                              ; C230h
    ret                                        ; C233h
    nop                                        ; C234h
LC235:
    ld c,0FDh                                  ; C235h
LC237:
    inc c                                      ; C237h
    rrca                                       ; C238h
    jp c,LC237                                 ; C239h
    ld a,c                                     ; C23Ch
    rlca                                       ; C23Dh
    rlca                                       ; C23Eh
    rlca                                       ; C23Fh
    rlca                                       ; C240h
    add a,0A0h                                 ; C241h
    or l                                       ; C243h
    ld l,a                                     ; C244h
    ld h,0C4h                                  ; C245h
    ld a,(hl)                                  ; C247h
LC248:
    pop bc                                     ; C248h
    pop hl                                     ; C249h
    ret                                        ; C24Ah
LC24B:
    ld (08FF4h),a                              ; C24Bh
    call loader_beep                           ; C24Eh
    jp LC1B5                                   ; C251h

; ВВ55: режим 82h. Выход A=82h; флаги и BC/DE/HL сохраняются.
loader_keybMode82:
    ld a,082h                                  ; C254h
    ld (0FF03h),a                              ; C256h
    ret                                        ; C259h

; ВВ55: режим 91h. Выход A=91h; флаги и BC/DE/HL сохраняются.
loader_keybMode91:
    ld a,091h                                  ; C25Ah
    ld (0FF03h),a                              ; C25Ch
    ret                                        ; C25Fh
LC260:
    push bc                                    ; C260h
    ld a,(08FEFh)                              ; C261h
    cp 080h                                    ; C264h
    jp z,LC341                                 ; C266h
LC269:
    ld c,0FFh                                  ; C269h
LC26B:
    call loader_keybMode82                     ; C26Bh
    ld a,(0FF01h)                              ; C26Eh
    or 003h                                    ; C271h
    cp 0FFh                                    ; C273h
    jp nz,LC269                                ; C275h
    ld b,015h                                  ; C278h
    call loader_delay                          ; C27Ah
    dec c                                      ; C27Dh
    jp nz,LC26B                                ; C27Eh
    pop bc                                     ; C281h
    ret                                        ; C282h
LC283:
    ld hl,(08FEDh)                             ; C283h
    jp (hl)                                    ; C286h
LC287:
    push hl                                    ; C287h
    ld hl,08FEBh                               ; C288h
    dec (hl)                                   ; C28Bh
    call z,LC291                               ; C28Ch
    pop hl                                     ; C28Fh
    ret                                        ; C290h
LC291:
    push bc                                    ; C291h
    ld hl,08FEBh                               ; C292h
    ld (hl),0FFh                               ; C295h
    dec hl                                     ; C297h
    inc (hl)                                   ; C298h
    ld hl,(08FFCh)                             ; C299h
    inc hl                                     ; C29Ch
    inc hl                                     ; C29Dh
    ld (08FFCh),hl                             ; C29Eh
    ld c,05Fh                                  ; C2A1h
    call LC2BC                                 ; C2A3h
    ld hl,(08FFCh)                             ; C2A6h
    dec hl                                     ; C2A9h
    dec hl                                     ; C2AAh
    ld (08FFCh),hl                             ; C2ABh
    pop bc                                     ; C2AEh
    ret                                        ; C2AFh
LC2B0:
    push hl                                    ; C2B0h
    push af                                    ; C2B1h
    ld a,(08FEAh)                              ; C2B2h
    rrca                                       ; C2B5h
    call nc,LC291                              ; C2B6h
    pop af                                     ; C2B9h
    pop hl                                     ; C2BAh
    ret                                        ; C2BBh
LC2BC:
    push hl                                    ; C2BCh
    push de                                    ; C2BDh
    push bc                                    ; C2BEh
    push af                                    ; C2BFh
    ld a,c                                     ; C2C0h
    ld hl,(08FFCh)                             ; C2C1h
    ex de,hl                                   ; C2C4h
    jp LC050                                   ; C2C5h
LC2C8:
    ld hl,08FEBh                               ; C2C8h
    ld (hl),001h                               ; C2CBh
    call LC283                                 ; C2CDh
    call LC260                                 ; C2D0h
    xor a                                      ; C2D3h
    ld (08FEAh),a                              ; C2D4h
    call LC1B1                                 ; C2D7h
    call LC2B0                                 ; C2DAh
    cp 080h                                    ; C2DDh
    jp z,LC34E                                 ; C2DFh
    call loader_beep                           ; C2E2h
    cp 081h                                    ; C2E5h
    jp nc,LC30A                                ; C2E7h
    ld (08FF0h),a                              ; C2EAh
    ld (08FEFh),a                              ; C2EDh
LC2F0:
    cp 021h                                    ; C2F0h
    ret c                                      ; C2F2h
    push af                                    ; C2F3h
    ld a,(08FF4h)                              ; C2F4h
    or a                                       ; C2F7h
    ld hl,(08FDFh)                             ; C2F8h
    jp (hl)                                    ; C2FBh
    jp z,LC367                                 ; C2FCh
    ld a,(08FECh)                              ; C2FFh
    cp 03Ah                                    ; C302h
    jp z,LC360                                 ; C304h
    pop af                                     ; C307h
    ret                                        ; C308h
    nop                                        ; C309h
LC30A:
    jp nz,LC323                                ; C30Ah
    ld a,(08FECh)                              ; C30Dh
    add a,080h                                 ; C310h
    ld (08FECh),a                              ; C312h
    ld (08FF4h),a                              ; C315h
    cp 03Ah                                    ; C318h
    jp z,LC331                                 ; C31Ah
    call LC1A0                                 ; C31Dh
    jp LC2C8                                   ; C320h
LC323:
    cp 08Ch                                    ; C323h
    jp nz,LC357                                ; C325h
    ld hl,0FFFFh                               ; C328h
LC32B:
    ld (08FFAh),hl                             ; C32Bh
    jp LC2C8                                   ; C32Eh
LC331:
    call LC1AB                                 ; C331h
    jp LC2C8                                   ; C334h

; Ожидание клавиши. Выход A=код; BC/DE/HL сохраняются, флаги меняются.
loader_getch:
    push bc                                    ; C337h
    push de                                    ; C338h
    push hl                                    ; C339h
    call LC2C8                                 ; C33Ah
    pop hl                                     ; C33Dh
    pop de                                     ; C33Eh
    pop bc                                     ; C33Fh
    ret                                        ; C340h
LC341:
    ld c,010h                                  ; C341h
LC343:
    ld b,0FFh                                  ; C343h
    call loader_delay                          ; C345h
    dec c                                      ; C348h
    jp nz,LC343                                ; C349h
    pop bc                                     ; C34Ch
    ret                                        ; C34Dh
LC34E:
    ld (08FEFh),a                              ; C34Eh
    ld a,(08FF0h)                              ; C351h
    jp LC2F0                                   ; C354h
LC357:
    cp 08Bh                                    ; C357h
    ret nz                                     ; C359h
    ld hl,00000h                               ; C35Ah
    jp LC32B                                   ; C35Dh
LC360:
    pop af                                     ; C360h
    cp 040h                                    ; C361h
    ret c                                      ; C363h
    sub 020h                                   ; C364h
    ret                                        ; C366h
LC367:
    pop af                                     ; C367h
    cp 040h                                    ; C368h
    jp nc,LC2C8                                ; C36Ah
    xor 010h                                   ; C36Dh
    ret                                        ; C36Fh
    nop                                        ; C370h
    nop                                        ; C371h
    nop                                        ; C372h
    nop                                        ; C373h
    nop                                        ; C374h
    nop                                        ; C375h
    nop                                        ; C376h
loader_tapeRead:
    push bc                                    ; C377h
    push de                                    ; C378h
LC379:
    ld c,000h                                  ; C379h
    ld d,a                                     ; C37Bh
    ld a,(0FF01h)                              ; C37Ch
    and 001h                                   ; C37Fh
    ld e,a                                     ; C381h
LC382:
    ld a,c                                     ; C382h
    and 07Fh                                   ; C383h
    rlca                                       ; C385h
    ld c,a                                     ; C386h
LC387:
    ld a,(0FF01h)                              ; C387h
    cp 080h                                    ; C38Ah
    jp c,LC45A                                 ; C38Ch
    and 001h                                   ; C38Fh
    cp e                                       ; C391h
    jp z,LC387                                 ; C392h
    or c                                       ; C395h
    ld c,a                                     ; C396h
    call LC3C9                                 ; C397h
    ld a,(0FF01h)                              ; C39Ah
    and 001h                                   ; C39Dh
    ld e,a                                     ; C39Fh
    ld a,d                                     ; C3A0h
    or a                                       ; C3A1h
    jp p,LC3BE                                 ; C3A2h
    ld a,c                                     ; C3A5h
    cp 0E6h                                    ; C3A6h
    jp nz,LC3B2                                ; C3A8h
    xor a                                      ; C3ABh
    ld (08FF3h),a                              ; C3ACh
    jp LC3BC                                   ; C3AFh
LC3B2:
    cp 019h                                    ; C3B2h
    jp nz,LC382                                ; C3B4h
    ld a,0FFh                                  ; C3B7h
    ld (08FF3h),a                              ; C3B9h
LC3BC:
    ld d,009h                                  ; C3BCh
LC3BE:
    dec d                                      ; C3BEh
    jp nz,LC382                                ; C3BFh
    ld a,(08FF3h)                              ; C3C2h
    xor c                                      ; C3C5h
    pop de                                     ; C3C6h
    pop bc                                     ; C3C7h
    ret                                        ; C3C8h
LC3C9:
    ld a,(08FFFh)                              ; C3C9h
LC3CC:
    ld b,a                                     ; C3CCh
    jp loader_delay                            ; C3CDh
loader_tapeWrite:
    push bc                                    ; C3D0h
    push de                                    ; C3D1h
    push af                                    ; C3D2h
    ld d,a                                     ; C3D3h
    ld c,008h                                  ; C3D4h
LC3D6:
    ld a,d                                     ; C3D6h
    rlca                                       ; C3D7h
    ld d,a                                     ; C3D8h
    and 001h                                   ; C3D9h
    or 00Eh                                    ; C3DBh
    ld (0FF03h),a                              ; C3DDh
    ld e,a                                     ; C3E0h
    call loader_writeDelay                     ; C3E1h
    ld a,e                                     ; C3E4h
    xor 001h                                   ; C3E5h
    ld (0FF03h),a                              ; C3E7h
    call loader_writeDelay                     ; C3EAh
    dec c                                      ; C3EDh
    jp nz,LC3D6                                ; C3EEh
    pop af                                     ; C3F1h
    pop de                                     ; C3F2h
    pop bc                                     ; C3F3h
    ret                                        ; C3F4h
    nop                                        ; C3F5h
    nop                                        ; C3F6h
    nop                                        ; C3F7h
    nop                                        ; C3F8h
LC3F9:
    ld a,0FFh                                  ; C3F9h
    call loader_tapeRead                       ; C3FBh
    ld l,a                                     ; C3FEh
    ld a,008h                                  ; C3FFh
    call loader_tapeRead                       ; C401h
    ld h,a                                     ; C404h
    ld (08FE3h),hl                             ; C405h
    ld a,008h                                  ; C408h
    call loader_tapeRead                       ; C40Ah
    ld e,a                                     ; C40Dh
    ld a,008h                                  ; C40Eh
    call loader_tapeRead                       ; C410h
    ld d,a                                     ; C413h
LC414:
    ld a,008h                                  ; C414h
LC416:
    call loader_tapeRead                       ; C416h
    ld (hl),a                                  ; C419h
    call loader_compareHLDE                    ; C41Ah
    inc hl                                     ; C41Dh
    jp nz,LC414                                ; C41Eh
    ret                                        ; C421h
    ld a,0FFh                                  ; C422h
    jp LC416                                   ; C424h

; Сравнение HL и DE: Z при равенстве; A и флаги разрушаются.
loader_compareHLDE:
    ld a,h                                     ; C427h
    cp d                                       ; C428h
    ret nz                                     ; C429h
    ld a,l                                     ; C42Ah
    cp e                                       ; C42Bh
    ret                                        ; C42Ch

; Копирование HL..DE исключительно в BC. Разрушает A/BC/HL и флаги.
loader_copy:
    ld a,(hl)                                  ; C42Dh
    ld (bc),a                                  ; C42Eh
    inc hl                                     ; C42Fh
    inc bc                                     ; C430h
    call loader_compareHLDE                    ; C431h
    jp nz,loader_copy                          ; C434h
    ret                                        ; C437h

; Строка HL с нулевым завершением. Разрушает A/C/HL и флаги.
loader_printString:
    ld a,(hl)                                  ; C438h
    ld c,a                                     ; C439h
    cp 000h                                    ; C43Ah
    ret z                                      ; C43Ch
    call loader_printChar                      ; C43Dh
    inc hl                                     ; C440h
    jp loader_printString                      ; C441h
loader_init:
    ld hl,loader_initialState                  ; C444h
    ld de,loader_prompt                        ; C447h
    ld bc,08FDFh                               ; C44Ah
    call loader_copy                           ; C44Dh
    call loader_printString                    ; C450h
    ret                                        ; C453h

; Остаточные байты исторического варианта, без установленного входа.
    DB 0F9h,0C3h,02Ah,0E3h,08Fh,0E9h           ; C454h
LC45A:
    call loader_getch                          ; C45Ah
    cp 00Dh                                    ; C45Dh
    jp z,monitor2_vectors                      ; C45Fh
    cp 00Ah                                    ; C462h
    jp z,LC46F                                 ; C464h
    ld (08FFFh),a                              ; C467h
    ld a,0FFh                                  ; C46Ah
    jp LC379                                   ; C46Ch
LC46F:
    ld hl,(08FE1h)                             ; C46Fh
    jp (hl)                                    ; C472h

; Шаблон C473h..C493h копируется в 8FDFh..8FFFh; внутри также код C488h.
loader_initialState:
    DB 0FCh,0C2h,000h,0C0h,000h,0C0h,0C8h,0C2h ; C473h
    DB 0A0h,018h,000h,000h,000h,03Ah,096h,0C1h ; C47Bh
    DB 020h,020h,05Fh,020h,000h                ; C483h
loader_writeDelay:
    ld a,(08FFEh)                              ; C488h
    jp LC3CC                                   ; C48Bh
    DB 000h,000h,000h,000h,028h,03Ch           ; C48Eh

; Приглашение загрузчика, затем таблица клавиатуры C4A0h..C4FFh.
loader_prompt:
    DB 01Fh,020h,050h,052h,04Fh,047h,052h,041h ; C494h
    DB 04Dh,020h,03Fh,000h                     ; C49Ch
loader_keyTable:
    DB 081h,00Ch,019h,01Ah,009h,01Bh,020h,008h ; C4A0h
    DB 080h,018h,00Ah,00Dh,000h,000h,000h,000h ; C4A8h
    DB 071h,07Eh,073h,06Dh,069h,074h,078h,062h ; C4B0h
    DB 060h,02Ch,02Fh,07Fh,000h,000h,000h,000h ; C4B8h
    DB 066h,079h,077h,061h,070h,072h,06Fh,06Ch ; C4C0h
    DB 064h,076h,07Ch,02Eh,000h,000h,000h,000h ; C4C8h
    DB 06Ah,063h,075h,06Bh,065h,06Eh,067h,07Bh ; C4D0h
    DB 07Dh,07Ah,068h,03Ah,000h,000h,000h,000h ; C4D8h
    DB 03Bh,031h,032h,033h,034h,035h,036h,037h ; C4E0h
    DB 038h,039h,030h,02Dh,000h,000h,000h,000h ; C4E8h
    DB 000h,001h,002h,003h,004h,005h,006h,007h ; C4F0h
    DB 08Ah,08Bh,08Ch,01Fh,000h,000h,000h,000h ; C4F8h
