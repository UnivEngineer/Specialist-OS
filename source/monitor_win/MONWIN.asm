; Byte-exact recovery of originals/monitor_win.rom. Intel 8080.
    ORG 0C000h

;----------------------------------------------------------------------------
; Оконный монитор
;
; Выполнить холодный вход оконного монитора по C000h.
;
; вход:
;   В адресном пространстве STD установлен образ MONWIN.MON, доступна нижняя память для
;   стека. Исходные значения регистров не используются при установке SP.
;
; выход:
;   Установлен SP = 7FFFh, в порт 0FF03h записано 82h, исполнение продолжается по
;   C444h. Вызов не возвращает управление через RET.
;----------------------------------------------------------------------------

monitor_start:
    jp 0C003h                         ; C000h
    ld sp,07FFFh                      ; C003h
    ld a,082h                         ; C006h
    ld (0FF03h),a                     ; C008h
    jp 0C444h                         ; C00Bh
    nop                               ; C00Eh
    nop                               ; C00Fh
    push hl                           ; C010h
    push bc                           ; C011h
    ld hl,00000h                      ; C012h
    add hl,sp                         ; C015h
    ld (08FF6h),hl                    ; C016h
    ld sp,0C000h                      ; C019h
    ld hl,(08FFAh)                    ; C01Ch
    ld bc,00300h                      ; C01Fh
    push hl                           ; C022h
    push hl                           ; C023h
    push hl                           ; C024h
    push hl                           ; C025h
    push hl                           ; C026h
    push hl                           ; C027h
    push hl                           ; C028h
    push hl                           ; C029h
    dec bc                            ; C02Ah
    ld a,b                            ; C02Bh
    or c                              ; C02Ch
    jp nz,0C022h                      ; C02Dh
    ld hl,(08FF6h)                    ; C030h
    ld sp,hl                          ; C033h
    pop bc                            ; C034h
    pop hl                            ; C035h
    ret                               ; C036h
    push hl                           ; C037h
    push de                           ; C038h
    push bc                           ; C039h
    push af                           ; C03Ah
    ld a,c                            ; C03Bh
    cp 021h                           ; C03Ch
    jp c,0C0D4h                       ; C03Eh
    ld hl,(08FFCh)                    ; C041h
    ld a,h                            ; C044h
    cp 0BEh                           ; C045h
    jp nc,0C0B2h                      ; C047h
    add a,003h                        ; C04Ah
    ld (08FFDh),a                     ; C04Ch
    ex de,hl                          ; C04Fh
    ld a,c                            ; C050h
    ld (08FE9h),a                     ; C051h
    sub 020h                          ; C054h
    ld hl,(08FE7h)                    ; C056h
    add a,l                           ; C059h
    ld l,a                            ; C05Ah
    add hl,hl                         ; C05Bh
    add hl,hl                         ; C05Ch
    add hl,hl                         ; C05Dh
    ex de,hl                          ; C05Eh
    nop                               ; C05Fh
    ld a,h                            ; C060h
    and 003h                          ; C061h
    ld c,a                            ; C063h
    ld a,005h                         ; C064h
    sub c                             ; C066h
    ld c,a                            ; C067h
    ld a,h                            ; C068h
    and 0FCh                          ; C069h
    rrca                              ; C06Bh
    rrca                              ; C06Ch
    add a,090h                        ; C06Dh
    ld h,a                            ; C06Fh
    ld (08FF8h),hl                    ; C070h
    ld b,008h                         ; C073h
    nop                               ; C075h
    ld a,(de)                         ; C076h
    ld l,a                            ; C077h
    ld h,000h                         ; C078h
    ld a,c                            ; C07Ah
    add hl,hl                         ; C07Bh
    add hl,hl                         ; C07Ch
    dec a                             ; C07Dh
    jp nz,0C07Bh                      ; C07Eh
    push hl                           ; C081h
    inc de                            ; C082h
    dec b                             ; C083h
    jp nz,0C076h                      ; C084h
    ld b,008h                         ; C087h
    ld hl,(08FF8h)                    ; C089h
    pop de                            ; C08Ch
    ld a,d                            ; C08Dh
    call 0C163h                       ; C08Eh
    ld (hl),a                         ; C091h
    inc h                             ; C092h
    ld a,e                            ; C093h
    call 0C163h                       ; C094h
    ld (hl),a                         ; C097h
    dec h                             ; C098h
    dec l                             ; C099h
    dec b                             ; C09Ah
    jp nz,0C08Ch                      ; C09Bh
    pop af                            ; C09Eh
    pop bc                            ; C09Fh
    pop de                            ; C0A0h
    pop hl                            ; C0A1h
    ret                               ; C0A2h
    ld c,a                            ; C0A3h
    ld a,(08FFAh)                     ; C0A4h
    or a                              ; C0A7h
    jp nz,0C0AFh                      ; C0A8h
    ld a,c                            ; C0ABh
    cpl                               ; C0ACh
    and (hl)                          ; C0ADh
    ret                               ; C0AEh
    ld a,(hl)                         ; C0AFh
    or c                              ; C0B0h
    ret                               ; C0B1h
    ld a,l                            ; C0B2h
    cp 0F5h                           ; C0B3h
    jp nc,0C0C4h                      ; C0B5h
    add a,00Ah                        ; C0B8h
    ld l,a                            ; C0BAh
    ld (08FFCh),a                     ; C0BBh
    ld h,000h                         ; C0BEh
    ld a,h                            ; C0C0h
    jp 0C04Ah                         ; C0C1h
    call 0C22Dh                       ; C0C4h
    nop                               ; C0C7h
    call 0C010h                       ; C0C8h
    ld hl,00008h                      ; C0CBh
    ld (08FFCh),hl                    ; C0CEh
    jp 0C041h                         ; C0D1h
    ld hl,(08FFCh)                    ; C0D4h
    cp 020h                           ; C0D7h
    jp z,0C107h                       ; C0D9h
    cp 00Ah                           ; C0DCh
    jp z,0C113h                       ; C0DEh
    cp 00Dh                           ; C0E1h
    jp z,0C128h                       ; C0E3h
    cp 018h                           ; C0E6h
    jp z,0C107h                       ; C0E8h
    cp 008h                           ; C0EBh
    jp z,0C12Dh                       ; C0EDh
    cp 019h                           ; C0F0h
    jp z,0C13Bh                       ; C0F2h
    cp 01Ah                           ; C0F5h
    jp z,0C147h                       ; C0F7h
    cp 00Ch                           ; C0FAh
    jp z,0C15Ah                       ; C0FCh
    cp 01Fh                           ; C0FFh
    jp z,0C157h                       ; C101h
    jp 0C15Dh                         ; C104h
    ld a,h                            ; C107h
    cp 0BEh                           ; C108h
    jp nc,0C113h                      ; C10Ah
    add a,003h                        ; C10Dh
    ld h,a                            ; C10Fh
    jp 0C15Dh                         ; C110h
    ld h,000h                         ; C113h
    ld a,l                            ; C115h
    cp 0F5h                           ; C116h
    jp nc,0C121h                      ; C118h
    add a,00Ah                        ; C11Bh
    ld l,a                            ; C11Dh
    jp 0C15Dh                         ; C11Eh
    call 0C22Dh                       ; C121h
    nop                               ; C124h
    jp 0C157h                         ; C125h
    ld h,000h                         ; C128h
    jp 0C15Dh                         ; C12Ah
    ld a,h                            ; C12Dh
    cp 002h                           ; C12Eh
    jp c,0C139h                       ; C130h
    sub 003h                          ; C133h
    ld h,a                            ; C135h
    jp 0C15Dh                         ; C136h
    ld h,0BDh                         ; C139h
    ld a,l                            ; C13Bh
    cp 011h                           ; C13Ch
    jp c,0C15Dh                       ; C13Eh
    sub 00Ah                          ; C141h
    ld l,a                            ; C143h
    jp 0C15Dh                         ; C144h
    ld a,l                            ; C147h
    cp 0F5h                           ; C148h
    jp nc,0C15Dh                      ; C14Ah
    add a,00Ah                        ; C14Dh
    ld l,a                            ; C14Fh
    jp 0C15Dh                         ; C150h
    nop                               ; C153h
    nop                               ; C154h
    nop                               ; C155h
    nop                               ; C156h
    call 0C010h                       ; C157h
    ld hl,00008h                      ; C15Ah
    ld (08FFCh),hl                    ; C15Dh
    jp 0C09Eh                         ; C160h
    ld c,a                            ; C163h
    ld a,(08FE9h)                     ; C164h
    cp 07Fh                           ; C167h
    ld a,c                            ; C169h
    jp z,0C0A3h                       ; C16Ah
    xor (hl)                          ; C16Dh
    ret                               ; C16Eh
    nop                               ; C16Fh
    push af                           ; C170h
    push bc                           ; C171h
    push hl                           ; C172h
    ld hl,(08FF1h)                    ; C173h
    ld a,00Bh                         ; C176h
    ld (0FF03h),a                     ; C178h
    call 0C18Fh                       ; C17Bh
    ld a,00Ah                         ; C17Eh
    ld (0FF03h),a                     ; C180h
    call 0C18Fh                       ; C183h
    dec h                             ; C186h
    jp nz,0C176h                      ; C187h
    nop                               ; C18Ah
    pop hl                            ; C18Bh
    pop bc                            ; C18Ch
    pop af                            ; C18Dh
    ret                               ; C18Eh
    ld b,l                            ; C18Fh
    dec b                             ; C190h
    jp nz,0C190h                      ; C191h
    ret                               ; C194h
    ret                               ; C195h
    push bc                           ; C196h
    ld b,015h                         ; C197h
    call 0C190h                       ; C199h
    pop bc                            ; C19Ch
    jp 0C287h                         ; C19Dh
    push hl                           ; C1A0h
    ld a,040h                         ; C1A1h
    ld (08FF1h),a                     ; C1A3h
    call 0C170h                       ; C1A6h
    pop hl                            ; C1A9h
    ret                               ; C1AAh
    push hl                           ; C1ABh
    ld a,05Fh                         ; C1ACh
    jp 0C1A3h                         ; C1AEh
    push hl                           ; C1B1h
    push bc                           ; C1B2h
    ld b,0FFh                         ; C1B3h
    call 0C283h                       ; C1B5h
    ld a,(08FF4h)                     ; C1B8h
    or a                              ; C1BBh
    jp z,0C1E2h                       ; C1BCh
    call 0C254h                       ; C1BFh
    ld a,(0FF01h)                     ; C1C2h
    and 002h                          ; C1C5h
    jp z,0C24Bh                       ; C1C7h
    call 0C25Ah                       ; C1CAh
    ld a,(0FF00h)                     ; C1CDh
    cp 0FFh                           ; C1D0h
    jp nz,0C1FFh                      ; C1D2h
    ld a,(0FF02h)                     ; C1D5h
    or 0F0h                           ; C1D8h
    cp 0FFh                           ; C1DAh
    jp nz,0C205h                      ; C1DCh
    jp 0C1B5h                         ; C1DFh
    call 0C254h                       ; C1E2h
    ld a,(0FF01h)                     ; C1E5h
    and 002h                          ; C1E8h
    jp nz,0C1F2h                      ; C1EAh
    ld b,0FFh                         ; C1EDh
    jp 0C1CAh                         ; C1EFh
    dec b                             ; C1F2h
    jp nz,0C1CAh                      ; C1F3h
    ld a,(08FECh)                     ; C1F6h
    ld (08FF4h),a                     ; C1F9h
    jp 0C1B5h                         ; C1FCh
    ld l,a                            ; C1FFh
    ld h,0FFh                         ; C200h
    jp 0C208h                         ; C202h
    ld h,a                            ; C205h
    ld l,0FFh                         ; C206h
    ld c,0FBh                         ; C208h
    inc c                             ; C20Ah
    add hl,hl                         ; C20Bh
    jp c,0C20Ah                       ; C20Ch
    ld l,c                            ; C20Fh
    ld b,0FFh                         ; C210h
    call 0C254h                       ; C212h
    ld a,(0FF01h)                     ; C215h
    or 003h                           ; C218h
    cp 0FFh                           ; C21Ah
    jp nz,0C235h                      ; C21Ch
    dec b                             ; C21Fh
    jp nz,0C212h                      ; C220h
    jp 0C1B5h                         ; C223h
    ld a,(0FF01h)                     ; C226h
    cpl                               ; C229h
    and 0F7h                          ; C22Ah
    ret                               ; C22Ch
    call 0C226h                       ; C22Dh
    call nz,0C260h                    ; C230h
    ret                               ; C233h
    nop                               ; C234h
    ld c,0FDh                         ; C235h
    inc c                             ; C237h
    rrca                              ; C238h
    jp c,0C237h                       ; C239h
    ld a,c                            ; C23Ch
    rlca                              ; C23Dh
    rlca                              ; C23Eh
    rlca                              ; C23Fh
    rlca                              ; C240h
    add a,0A0h                        ; C241h
    or l                              ; C243h
    ld l,a                            ; C244h
    ld h,0C4h                         ; C245h
    ld a,(hl)                         ; C247h
    pop bc                            ; C248h
    pop hl                            ; C249h
    ret                               ; C24Ah
    ld (08FF4h),a                     ; C24Bh
    call 0C170h                       ; C24Eh
    jp 0C1B5h                         ; C251h
    ld a,082h                         ; C254h
    ld (0FF03h),a                     ; C256h
    ret                               ; C259h
    ld a,091h                         ; C25Ah
    ld (0FF03h),a                     ; C25Ch
    ret                               ; C25Fh
    push bc                           ; C260h
    ld a,(08FEFh)                     ; C261h
    cp 080h                           ; C264h
    jp z,0C341h                       ; C266h
    ld c,0FFh                         ; C269h
    call 0C254h                       ; C26Bh
    ld a,(0FF01h)                     ; C26Eh
    or 003h                           ; C271h
    cp 0FFh                           ; C273h
    jp nz,0C269h                      ; C275h
    ld b,015h                         ; C278h
    call 0C190h                       ; C27Ah
    dec c                             ; C27Dh
    jp nz,0C26Bh                      ; C27Eh
    pop bc                            ; C281h
    ret                               ; C282h
    ld hl,(08FEDh)                    ; C283h
    jp (hl)                           ; C286h
    push hl                           ; C287h
    ld hl,08FEBh                      ; C288h
    dec (hl)                          ; C28Bh
    call z,0C291h                     ; C28Ch
    pop hl                            ; C28Fh
    ret                               ; C290h
    push bc                           ; C291h
    ld hl,08FEBh                      ; C292h
    ld (hl),0FFh                      ; C295h
    dec hl                            ; C297h
    inc (hl)                          ; C298h
    ld hl,(08FFCh)                    ; C299h
    inc hl                            ; C29Ch
    inc hl                            ; C29Dh
    ld (08FFCh),hl                    ; C29Eh
    ld c,05Fh                         ; C2A1h
    call 0C2BCh                       ; C2A3h
    ld hl,(08FFCh)                    ; C2A6h
    dec hl                            ; C2A9h
    dec hl                            ; C2AAh
    ld (08FFCh),hl                    ; C2ABh
    pop bc                            ; C2AEh
    ret                               ; C2AFh
    push hl                           ; C2B0h
    push af                           ; C2B1h
    ld a,(08FEAh)                     ; C2B2h
    rrca                              ; C2B5h
    call nc,0C291h                    ; C2B6h
    pop af                            ; C2B9h
    pop hl                            ; C2BAh
    ret                               ; C2BBh
    push hl                           ; C2BCh
    push de                           ; C2BDh
    push bc                           ; C2BEh
    push af                           ; C2BFh
    ld a,c                            ; C2C0h
    ld hl,(08FFCh)                    ; C2C1h
    ex de,hl                          ; C2C4h
    jp 0C050h                         ; C2C5h
    ld hl,08FEBh                      ; C2C8h
    ld (hl),001h                      ; C2CBh
    call 0C283h                       ; C2CDh
    call 0C260h                       ; C2D0h
    xor a                             ; C2D3h
    ld (08FEAh),a                     ; C2D4h
    call 0C1B1h                       ; C2D7h
    call 0C2B0h                       ; C2DAh
    cp 080h                           ; C2DDh
    jp z,0C34Eh                       ; C2DFh
    call 0C170h                       ; C2E2h
    cp 081h                           ; C2E5h
    jp nc,0C30Ah                      ; C2E7h
    ld (08FF0h),a                     ; C2EAh
    ld (08FEFh),a                     ; C2EDh
    cp 021h                           ; C2F0h
    ret c                             ; C2F2h
    push af                           ; C2F3h
    ld a,(08FF4h)                     ; C2F4h
    or a                              ; C2F7h
    ld hl,(08FDFh)                    ; C2F8h
    jp (hl)                           ; C2FBh
    jp z,0C367h                       ; C2FCh
    ld a,(08FECh)                     ; C2FFh
    cp 03Ah                           ; C302h
    jp z,0C360h                       ; C304h
    pop af                            ; C307h
    ret                               ; C308h
    nop                               ; C309h
    jp nz,0C323h                      ; C30Ah
    ld a,(08FECh)                     ; C30Dh
    add a,080h                        ; C310h
    ld (08FECh),a                     ; C312h
    ld (08FF4h),a                     ; C315h
    cp 03Ah                           ; C318h
    jp z,0C331h                       ; C31Ah
    call 0C1A0h                       ; C31Dh
    jp 0C2C8h                         ; C320h
    cp 08Ch                           ; C323h
    jp nz,0C357h                      ; C325h
    ld hl,0FFFFh                      ; C328h
    ld (08FFAh),hl                    ; C32Bh
    jp 0C2C8h                         ; C32Eh
    call 0C1ABh                       ; C331h
    jp 0C2C8h                         ; C334h
    push bc                           ; C337h
    push de                           ; C338h
    push hl                           ; C339h
    call 0C2C8h                       ; C33Ah
    pop hl                            ; C33Dh
    pop de                            ; C33Eh
    pop bc                            ; C33Fh
    ret                               ; C340h
    ld c,010h                         ; C341h
    ld b,0FFh                         ; C343h
    call 0C190h                       ; C345h
    dec c                             ; C348h
    jp nz,0C343h                      ; C349h
    pop bc                            ; C34Ch
    ret                               ; C34Dh
    ld (08FEFh),a                     ; C34Eh
    ld a,(08FF0h)                     ; C351h
    jp 0C2F0h                         ; C354h
    cp 08Bh                           ; C357h
    ret nz                            ; C359h
    ld hl,00000h                      ; C35Ah
    jp 0C32Bh                         ; C35Dh
    pop af                            ; C360h
    cp 040h                           ; C361h
    ret c                             ; C363h
    sub 020h                          ; C364h
    ret                               ; C366h
    pop af                            ; C367h
    cp 040h                           ; C368h
    jp nc,0C2C8h                      ; C36Ah
    xor 010h                          ; C36Dh
    ret                               ; C36Fh
    nop                               ; C370h
    nop                               ; C371h
    nop                               ; C372h
    nop                               ; C373h
    nop                               ; C374h
    nop                               ; C375h
    nop                               ; C376h
    push bc                           ; C377h
    push de                           ; C378h
    ld c,000h                         ; C379h
    ld d,a                            ; C37Bh
    ld a,(0FF01h)                     ; C37Ch
    and 001h                          ; C37Fh
    ld e,a                            ; C381h
    ld a,c                            ; C382h
    and 07Fh                          ; C383h
    rlca                              ; C385h
    ld c,a                            ; C386h
    ld a,(0FF01h)                     ; C387h
    cp 080h                           ; C38Ah
    jp c,0C45Ah                       ; C38Ch
    and 001h                          ; C38Fh
    cp e                              ; C391h
    jp z,0C387h                       ; C392h
    or c                              ; C395h
    ld c,a                            ; C396h
    call 0C3C9h                       ; C397h
    ld a,(0FF01h)                     ; C39Ah
    and 001h                          ; C39Dh
    ld e,a                            ; C39Fh
    ld a,d                            ; C3A0h
    or a                              ; C3A1h
    jp p,0C3BEh                       ; C3A2h
    ld a,c                            ; C3A5h
    cp 0E6h                           ; C3A6h
    jp nz,0C3B2h                      ; C3A8h
    xor a                             ; C3ABh
    ld (08FF3h),a                     ; C3ACh
    jp 0C3BCh                         ; C3AFh
    cp 019h                           ; C3B2h
    jp nz,0C382h                      ; C3B4h
    ld a,0FFh                         ; C3B7h
    ld (08FF3h),a                     ; C3B9h
    ld d,009h                         ; C3BCh
    dec d                             ; C3BEh
    jp nz,0C382h                      ; C3BFh
    ld a,(08FF3h)                     ; C3C2h
    xor c                             ; C3C5h
    pop de                            ; C3C6h
    pop bc                            ; C3C7h
    ret                               ; C3C8h
    ld a,(08FFFh)                     ; C3C9h
    ld b,a                            ; C3CCh
    jp 0C190h                         ; C3CDh
    push bc                           ; C3D0h
    push de                           ; C3D1h
    push af                           ; C3D2h
    ld d,a                            ; C3D3h
    ld c,008h                         ; C3D4h
    ld a,d                            ; C3D6h
    rlca                              ; C3D7h
    ld d,a                            ; C3D8h
    and 001h                          ; C3D9h
    or 00Eh                           ; C3DBh
    ld (0FF03h),a                     ; C3DDh
    ld e,a                            ; C3E0h
    call 0C488h                       ; C3E1h
    ld a,e                            ; C3E4h
    xor 001h                          ; C3E5h
    ld (0FF03h),a                     ; C3E7h
    call 0C488h                       ; C3EAh
    dec c                             ; C3EDh
    jp nz,0C3D6h                      ; C3EEh
    pop af                            ; C3F1h
    pop de                            ; C3F2h
    pop bc                            ; C3F3h
    ret                               ; C3F4h
    nop                               ; C3F5h
    nop                               ; C3F6h
    nop                               ; C3F7h
    nop                               ; C3F8h
    ld a,0FFh                         ; C3F9h
    call 0C377h                       ; C3FBh
    ld l,a                            ; C3FEh
    ld a,008h                         ; C3FFh
    call 0C377h                       ; C401h
    ld h,a                            ; C404h
    ld (08FE3h),hl                    ; C405h
    ld a,008h                         ; C408h
    call 0C377h                       ; C40Ah
    ld e,a                            ; C40Dh
    ld a,008h                         ; C40Eh
    call 0C377h                       ; C410h
    ld d,a                            ; C413h
    ld a,008h                         ; C414h
    call 0C377h                       ; C416h
    ld (hl),a                         ; C419h
    call 0C427h                       ; C41Ah
    inc hl                            ; C41Dh
    jp nz,0C414h                      ; C41Eh
    ret                               ; C421h
    ld a,0FFh                         ; C422h
    jp 0C416h                         ; C424h
    ld a,h                            ; C427h
    cp d                              ; C428h
    ret nz                            ; C429h
    ld a,l                            ; C42Ah
    cp e                              ; C42Bh
    ret                               ; C42Ch
    ld a,(hl)                         ; C42Dh
    ld (bc),a                         ; C42Eh
    inc hl                            ; C42Fh
    inc bc                            ; C430h
    call 0C427h                       ; C431h
    jp nz,0C42Dh                      ; C434h
    ret                               ; C437h
    ld a,(hl)                         ; C438h
    ld c,a                            ; C439h
    cp 000h                           ; C43Ah
    ret z                             ; C43Ch
    call 0C037h                       ; C43Dh
    inc hl                            ; C440h
    jp 0C438h                         ; C441h
    ld hl,0C473h                      ; C444h
    ld de,0C494h                      ; C447h
    ld bc,08FDFh                      ; C44Ah
    call 0C42Dh                       ; C44Dh
    call 0C438h                       ; C450h
    nop                               ; C453h
    nop                               ; C454h
    nop                               ; C455h
    nop                               ; C456h
    nop                               ; C457h
    nop                               ; C458h
    nop                               ; C459h
    nop                               ; C45Ah
    nop                               ; C45Bh
    nop                               ; C45Ch
    nop                               ; C45Dh
    nop                               ; C45Eh
    jp 0C800h                         ; C45Fh
    cp 00Ah                           ; C462h
    jp z,0C46Fh                       ; C464h
    ld (08FFFh),a                     ; C467h
    ld a,0FFh                         ; C46Ah
    jp 0C379h                         ; C46Ch
    ld hl,(08FE1h)                    ; C46Fh
    jp (hl)                           ; C472h
    call m,000C2h                     ; C473h
    ret nz                            ; C476h
    nop                               ; C477h
    ret nz                            ; C478h
    ret z                             ; C479h
    jp nz,018A0h                      ; C47Ah
    nop                               ; C47Dh
    nop                               ; C47Eh
    nop                               ; C47Fh
    ld a,(0C196h)                     ; C480h
    DB 020h,020h                      ; C483h
    ld e,a                            ; C485h
    DB 020h,000h                      ; C486h
    ld a,(08FFEh)                     ; C488h
    jp 0C3CCh                         ; C48Bh
    nop                               ; C48Eh
    nop                               ; C48Fh
    nop                               ; C490h
    nop                               ; C491h
    DB 028h,03Ch                      ; C492h
    rra                               ; C494h
    DB 020h,050h                      ; C495h
    ld d,d                            ; C497h
    ld c,a                            ; C498h
    ld b,a                            ; C499h
    ld d,d                            ; C49Ah
    ld b,c                            ; C49Bh
    ld c,l                            ; C49Ch
    DB 020h,03Fh                      ; C49Dh
    nop                               ; C49Fh
    add a,c                           ; C4A0h
    inc c                             ; C4A1h
    add hl,de                         ; C4A2h
    ld a,(de)                         ; C4A3h
    add hl,bc                         ; C4A4h
    dec de                            ; C4A5h
    DB 020h,008h                      ; C4A6h
    add a,b                           ; C4A8h
    DB 018h,00Ah                      ; C4A9h
    dec c                             ; C4ABh
    nop                               ; C4ACh
    nop                               ; C4ADh
    nop                               ; C4AEh
    nop                               ; C4AFh
    ld (hl),c                         ; C4B0h
    ld a,(hl)                         ; C4B1h
    ld (hl),e                         ; C4B2h
    ld l,l                            ; C4B3h
    ld l,c                            ; C4B4h
    ld (hl),h                         ; C4B5h
    ld a,b                            ; C4B6h
    ld h,d                            ; C4B7h
    ld h,b                            ; C4B8h
    inc l                             ; C4B9h
    cpl                               ; C4BAh
    ld a,a                            ; C4BBh
    nop                               ; C4BCh
    nop                               ; C4BDh
    nop                               ; C4BEh
    nop                               ; C4BFh
    ld h,(hl)                         ; C4C0h
    ld a,c                            ; C4C1h
    ld (hl),a                         ; C4C2h
    ld h,c                            ; C4C3h
    ld (hl),b                         ; C4C4h
    ld (hl),d                         ; C4C5h
    ld l,a                            ; C4C6h
    ld l,h                            ; C4C7h
    ld h,h                            ; C4C8h
    halt                              ; C4C9h
    ld a,h                            ; C4CAh
    ld l,000h                         ; C4CBh
    nop                               ; C4CDh
    nop                               ; C4CEh
    nop                               ; C4CFh
    ld l,d                            ; C4D0h
    ld h,e                            ; C4D1h
    ld (hl),l                         ; C4D2h
    ld l,e                            ; C4D3h
    ld h,l                            ; C4D4h
    ld l,(hl)                         ; C4D5h
    ld h,a                            ; C4D6h
    ld a,e                            ; C4D7h
    ld a,l                            ; C4D8h
    ld a,d                            ; C4D9h
    ld l,b                            ; C4DAh
    ld a,(00000h)                     ; C4DBh
    nop                               ; C4DEh
    nop                               ; C4DFh
    dec sp                            ; C4E0h
    ld sp,03332h                      ; C4E1h
    inc (hl)                          ; C4E4h
    dec (hl)                          ; C4E5h
    ld (hl),037h                      ; C4E6h
    DB 038h,039h                      ; C4E8h
    DB 030h,02Dh                      ; C4EAh
    nop                               ; C4ECh
    nop                               ; C4EDh
    nop                               ; C4EEh
    nop                               ; C4EFh
    nop                               ; C4F0h
    ld bc,00302h                      ; C4F1h
    inc b                             ; C4F4h
    dec b                             ; C4F5h
    ld b,007h                         ; C4F6h
    adc a,d                           ; C4F8h
    adc a,e                           ; C4F9h
    adc a,h                           ; C4FAh
    rra                               ; C4FBh
    nop                               ; C4FCh
    nop                               ; C4FDh
    nop                               ; C4FEh
    nop                               ; C4FFh
    nop                               ; C500h
    nop                               ; C501h
    nop                               ; C502h
    nop                               ; C503h
    nop                               ; C504h
    nop                               ; C505h
    nop                               ; C506h
    nop                               ; C507h
    inc b                             ; C508h
    inc b                             ; C509h
    inc b                             ; C50Ah
    inc b                             ; C50Bh
    inc b                             ; C50Ch
    nop                               ; C50Dh
    inc b                             ; C50Eh
    nop                               ; C50Fh
    ld a,(bc)                         ; C510h
    ld a,(bc)                         ; C511h
    ld a,(bc)                         ; C512h
    nop                               ; C513h
    nop                               ; C514h
    nop                               ; C515h
    nop                               ; C516h
    nop                               ; C517h
    ld a,(bc)                         ; C518h
    ld a,(bc)                         ; C519h
    rra                               ; C51Ah
    ld a,(bc)                         ; C51Bh
    rra                               ; C51Ch
    ld a,(bc)                         ; C51Dh
    ld a,(bc)                         ; C51Eh
    nop                               ; C51Fh
    inc b                             ; C520h
    rrca                              ; C521h
    inc d                             ; C522h
    ld c,005h                         ; C523h
    ld e,004h                         ; C525h
    nop                               ; C527h
    DB 018h,019h                      ; C528h
    ld (bc),a                         ; C52Ah
    inc b                             ; C52Bh
    DB 008h                           ; C52Ch
    inc de                            ; C52Dh
    inc bc                            ; C52Eh
    nop                               ; C52Fh
    inc b                             ; C530h
    ld a,(bc)                         ; C531h
    ld a,(bc)                         ; C532h
    inc c                             ; C533h
    dec d                             ; C534h
    ld (de),a                         ; C535h
    dec c                             ; C536h
    nop                               ; C537h
    ld b,006h                         ; C538h
    ld (bc),a                         ; C53Ah
    inc b                             ; C53Bh
    nop                               ; C53Ch
    nop                               ; C53Dh
    nop                               ; C53Eh
    nop                               ; C53Fh
    ld (bc),a                         ; C540h
    inc b                             ; C541h
    DB 008h                           ; C542h
    DB 008h                           ; C543h
    DB 008h                           ; C544h
    inc b                             ; C545h
    ld (bc),a                         ; C546h
    nop                               ; C547h
    DB 008h                           ; C548h
    inc b                             ; C549h
    ld (bc),a                         ; C54Ah
    ld (bc),a                         ; C54Bh
    ld (bc),a                         ; C54Ch
    inc b                             ; C54Dh
    DB 008h                           ; C54Eh
    nop                               ; C54Fh
    nop                               ; C550h
    inc b                             ; C551h
    dec d                             ; C552h
    ld c,015h                         ; C553h
    inc b                             ; C555h
    nop                               ; C556h
    nop                               ; C557h
    nop                               ; C558h
    inc b                             ; C559h
    inc b                             ; C55Ah
    rra                               ; C55Bh
    inc b                             ; C55Ch
    inc b                             ; C55Dh
    nop                               ; C55Eh
    nop                               ; C55Fh
    nop                               ; C560h
    nop                               ; C561h
    nop                               ; C562h
    inc c                             ; C563h
    inc c                             ; C564h
    inc b                             ; C565h
    DB 008h                           ; C566h
    nop                               ; C567h
    nop                               ; C568h
    nop                               ; C569h
    nop                               ; C56Ah
    rra                               ; C56Bh
    nop                               ; C56Ch
    nop                               ; C56Dh
    nop                               ; C56Eh
    nop                               ; C56Fh
    nop                               ; C570h
    nop                               ; C571h
    nop                               ; C572h
    nop                               ; C573h
    nop                               ; C574h
    inc c                             ; C575h
    inc c                             ; C576h
    nop                               ; C577h
    nop                               ; C578h
    ld bc,00402h                      ; C579h
    DB 008h                           ; C57Ch
    DB 010h,000h                      ; C57Dh
    nop                               ; C57Fh
    ld c,011h                         ; C580h
    inc de                            ; C582h
    dec d                             ; C583h
    add hl,de                         ; C584h
    ld de,0000Eh                      ; C585h
    inc b                             ; C588h
    inc c                             ; C589h
    inc b                             ; C58Ah
    inc b                             ; C58Bh
    inc b                             ; C58Ch
    inc b                             ; C58Dh
    ld c,000h                         ; C58Eh
    ld c,011h                         ; C590h
    ld bc,00806h                      ; C592h
    DB 010h,01Fh                      ; C595h
    nop                               ; C597h
    rra                               ; C598h
    ld bc,00602h                      ; C599h
    ld bc,00E11h                      ; C59Ch
    nop                               ; C59Fh
    ld (bc),a                         ; C5A0h
    ld b,00Ah                         ; C5A1h
    ld (de),a                         ; C5A3h
    rra                               ; C5A4h
    ld (bc),a                         ; C5A5h
    ld (bc),a                         ; C5A6h
    nop                               ; C5A7h
    rra                               ; C5A8h
    DB 010h,01Eh                      ; C5A9h
    ld bc,01101h                      ; C5ABh
    ld c,000h                         ; C5AEh
    rlca                              ; C5B0h
    DB 008h                           ; C5B1h
    DB 010h,01Eh                      ; C5B2h
    ld de,00E11h                      ; C5B4h
    nop                               ; C5B7h
    rra                               ; C5B8h
    ld bc,00402h                      ; C5B9h
    DB 008h                           ; C5BCh
    DB 008h                           ; C5BDh
    DB 008h                           ; C5BEh
    nop                               ; C5BFh
    ld c,011h                         ; C5C0h
    ld de,0110Eh                      ; C5C2h
    ld de,0000Eh                      ; C5C5h
    ld c,011h                         ; C5C8h
    ld de,0010Fh                      ; C5CAh
    ld (bc),a                         ; C5CDh
    inc e                             ; C5CEh
    nop                               ; C5CFh
    nop                               ; C5D0h
    inc c                             ; C5D1h
    inc c                             ; C5D2h
    nop                               ; C5D3h
    nop                               ; C5D4h
    inc c                             ; C5D5h
    inc c                             ; C5D6h
    nop                               ; C5D7h
    inc c                             ; C5D8h
    inc c                             ; C5D9h
    nop                               ; C5DAh
    inc c                             ; C5DBh
    inc c                             ; C5DCh
    inc b                             ; C5DDh
    DB 008h                           ; C5DEh
    nop                               ; C5DFh
    ld (bc),a                         ; C5E0h
    inc b                             ; C5E1h
    DB 008h                           ; C5E2h
    DB 010h,008h                      ; C5E3h
    inc b                             ; C5E5h
    ld (bc),a                         ; C5E6h
    nop                               ; C5E7h
    nop                               ; C5E8h
    nop                               ; C5E9h
    rra                               ; C5EAh
    nop                               ; C5EBh
    rra                               ; C5ECh
    nop                               ; C5EDh
    nop                               ; C5EEh
    nop                               ; C5EFh
    DB 008h                           ; C5F0h
    inc b                             ; C5F1h
    ld (bc),a                         ; C5F2h
    ld bc,00402h                      ; C5F3h
    DB 008h                           ; C5F6h
    nop                               ; C5F7h
    ld c,011h                         ; C5F8h
    ld bc,00402h                      ; C5FAh
    nop                               ; C5FDh
    inc b                             ; C5FEh
    nop                               ; C5FFh
    ld c,011h                         ; C600h
    inc de                            ; C602h
    dec d                             ; C603h
    rla                               ; C604h
    DB 010h,00Eh                      ; C605h
    nop                               ; C607h
    inc b                             ; C608h
    ld a,(bc)                         ; C609h
    ld de,01F11h                      ; C60Ah
    ld de,00011h                      ; C60Dh
    ld e,011h                         ; C610h
    ld de,0111Eh                      ; C612h
    ld de,0001Eh                      ; C615h
    ld c,011h                         ; C618h
    DB 010h,010h                      ; C61Ah
    DB 010h,011h                      ; C61Ch
    ld c,000h                         ; C61Eh
    ld e,009h                         ; C620h
    add hl,bc                         ; C622h
    add hl,bc                         ; C623h
    add hl,bc                         ; C624h
    add hl,bc                         ; C625h
    ld e,000h                         ; C626h
    rra                               ; C628h
    DB 010h,010h                      ; C629h
    ld e,010h                         ; C62Bh
    DB 010h,01Fh                      ; C62Dh
    nop                               ; C62Fh
    rra                               ; C630h
    DB 010h,010h                      ; C631h
    ld e,010h                         ; C633h
    DB 010h,010h                      ; C635h
    nop                               ; C637h
    ld c,011h                         ; C638h
    DB 010h,010h                      ; C63Ah
    inc de                            ; C63Ch
    ld de,0000Fh                      ; C63Dh
    ld de,01111h                      ; C640h
    rra                               ; C643h
    ld de,01111h                      ; C644h
    nop                               ; C647h
    ld c,004h                         ; C648h
    inc b                             ; C64Ah
    inc b                             ; C64Bh
    inc b                             ; C64Ch
    inc b                             ; C64Dh
    ld c,000h                         ; C64Eh
    ld bc,00101h                      ; C650h
    ld bc,01111h                      ; C653h
    ld c,000h                         ; C656h
    ld de,01412h                      ; C658h
    DB 018h,014h                      ; C65Bh
    ld (de),a                         ; C65Dh
    ld de,01000h                      ; C65Eh
    DB 010h,010h                      ; C661h
    DB 010h,010h                      ; C663h
    ld de,0001Fh                      ; C665h
    ld de,0151Bh                      ; C668h
    dec d                             ; C66Bh
    ld de,01111h                      ; C66Ch
    nop                               ; C66Fh
    ld de,01911h                      ; C670h
    dec d                             ; C673h
    inc de                            ; C674h
    ld de,00011h                      ; C675h
    ld c,011h                         ; C678h
    ld de,01111h                      ; C67Ah
    ld de,0000Eh                      ; C67Dh
    ld e,011h                         ; C680h
    ld de,0101Eh                      ; C682h
    DB 010h,010h                      ; C685h
    nop                               ; C687h
    ld c,011h                         ; C688h
    ld de,01511h                      ; C68Ah
    ld (de),a                         ; C68Dh
    dec c                             ; C68Eh
    nop                               ; C68Fh
    ld e,011h                         ; C690h
    ld de,0141Eh                      ; C692h
    ld (de),a                         ; C695h
    ld de,00E00h                      ; C696h
    ld de,00E10h                      ; C699h
    ld bc,00E11h                      ; C69Ch
    nop                               ; C69Fh
    rra                               ; C6A0h
    inc b                             ; C6A1h
    inc b                             ; C6A2h
    inc b                             ; C6A3h
    inc b                             ; C6A4h
    inc b                             ; C6A5h
    inc b                             ; C6A6h
    nop                               ; C6A7h
    ld de,01111h                      ; C6A8h
    ld de,01111h                      ; C6ABh
    ld c,000h                         ; C6AEh
    ld de,01111h                      ; C6B0h
    ld a,(bc)                         ; C6B3h
    ld a,(bc)                         ; C6B4h
    inc b                             ; C6B5h
    inc b                             ; C6B6h
    nop                               ; C6B7h
    ld de,01111h                      ; C6B8h
    dec d                             ; C6BBh
    dec d                             ; C6BCh
    dec d                             ; C6BDh
    ld a,(bc)                         ; C6BEh
    nop                               ; C6BFh
    ld de,00A11h                      ; C6C0h
    inc b                             ; C6C3h
    ld a,(bc)                         ; C6C4h
    ld de,00011h                      ; C6C5h
    ld de,00A11h                      ; C6C8h
    inc b                             ; C6CBh
    inc b                             ; C6CCh
    inc b                             ; C6CDh
    inc b                             ; C6CEh
    nop                               ; C6CFh
    rra                               ; C6D0h
    ld bc,00E02h                      ; C6D1h
    DB 008h                           ; C6D4h
    DB 010h,01Fh                      ; C6D5h
    nop                               ; C6D7h
    ld c,008h                         ; C6D8h
    DB 008h                           ; C6DAh
    DB 008h                           ; C6DBh
    DB 008h                           ; C6DCh
    DB 008h                           ; C6DDh
    ld c,000h                         ; C6DEh
    nop                               ; C6E0h
    DB 010h,008h                      ; C6E1h
    inc b                             ; C6E3h
    ld (bc),a                         ; C6E4h
    ld bc,00000h                      ; C6E5h
    ld c,002h                         ; C6E8h
    ld (bc),a                         ; C6EAh
    ld (bc),a                         ; C6EBh
    ld (bc),a                         ; C6ECh
    ld (bc),a                         ; C6EDh
    ld c,000h                         ; C6EEh
    ld c,011h                         ; C6F0h
    nop                               ; C6F2h
    nop                               ; C6F3h
    nop                               ; C6F4h
    nop                               ; C6F5h
    nop                               ; C6F6h
    nop                               ; C6F7h
    nop                               ; C6F8h
    nop                               ; C6F9h
    nop                               ; C6FAh
    nop                               ; C6FBh
    nop                               ; C6FCh
    nop                               ; C6FDh
    nop                               ; C6FEh
    rra                               ; C6FFh
    ld (de),a                         ; C700h
    dec d                             ; C701h
    dec d                             ; C702h
    dec e                             ; C703h
    dec d                             ; C704h
    dec d                             ; C705h
    ld (de),a                         ; C706h
    nop                               ; C707h
    inc b                             ; C708h
    ld a,(bc)                         ; C709h
    ld de,01F11h                      ; C70Ah
    ld de,00011h                      ; C70Dh
    rra                               ; C710h
    DB 010h,010h                      ; C711h
    ld e,011h                         ; C713h
    ld de,0001Eh                      ; C715h
    ld (de),a                         ; C718h
    ld (de),a                         ; C719h
    ld (de),a                         ; C71Ah
    ld (de),a                         ; C71Bh
    ld (de),a                         ; C71Ch
    rra                               ; C71Dh
    ld bc,00600h                      ; C71Eh
    ld a,(bc)                         ; C721h
    ld a,(bc)                         ; C722h
    ld a,(bc)                         ; C723h
    ld a,(bc)                         ; C724h
    rra                               ; C725h
    ld de,01F00h                      ; C726h
    DB 010h,010h                      ; C729h
    ld e,010h                         ; C72Bh
    DB 010h,01Fh                      ; C72Dh
    nop                               ; C72Fh
    inc b                             ; C730h
    rra                               ; C731h
    dec d                             ; C732h
    dec d                             ; C733h
    rra                               ; C734h
    inc b                             ; C735h
    inc b                             ; C736h
    nop                               ; C737h
    rra                               ; C738h
    ld de,01010h                      ; C739h
    DB 010h,010h                      ; C73Ch
    DB 010h,000h                      ; C73Eh
    ld de,00A11h                      ; C740h
    inc b                             ; C743h
    ld a,(bc)                         ; C744h
    ld de,00011h                      ; C745h
    ld de,01311h                      ; C748h
    dec d                             ; C74Bh
    add hl,de                         ; C74Ch
    ld de,00011h                      ; C74Dh
    dec d                             ; C750h
    ld de,01513h                      ; C751h
    add hl,de                         ; C754h
    ld de,00011h                      ; C755h
    ld de,01412h                      ; C758h
    DB 018h,014h                      ; C75Bh
    ld (de),a                         ; C75Dh
    ld de,00700h                      ; C75Eh
    add hl,bc                         ; C761h
    add hl,bc                         ; C762h
    add hl,bc                         ; C763h
    add hl,bc                         ; C764h
    add hl,bc                         ; C765h
    add hl,de                         ; C766h
    nop                               ; C767h
    ld de,0151Bh                      ; C768h
    dec d                             ; C76Bh
    ld de,01111h                      ; C76Ch
    nop                               ; C76Fh
    ld de,01111h                      ; C770h
    rra                               ; C773h
    ld de,01111h                      ; C774h
    nop                               ; C777h
    ld c,011h                         ; C778h
    ld de,01111h                      ; C77Ah
    ld de,0000Eh                      ; C77Dh
    rra                               ; C780h
    ld de,01111h                      ; C781h
    ld de,01111h                      ; C784h
    nop                               ; C787h
    rrca                              ; C788h
    ld de,00F11h                      ; C789h
    dec b                             ; C78Ch
    add hl,bc                         ; C78Dh
    ld de,01E00h                      ; C78Eh
    ld de,01E11h                      ; C791h
    DB 010h,010h                      ; C794h
    DB 010h,000h                      ; C796h
    ld c,011h                         ; C798h
    DB 010h,010h                      ; C79Ah
    DB 010h,011h                      ; C79Ch
    ld c,000h                         ; C79Eh
    rra                               ; C7A0h
    inc b                             ; C7A1h
    inc b                             ; C7A2h
    inc b                             ; C7A3h
    inc b                             ; C7A4h
    inc b                             ; C7A5h
    inc b                             ; C7A6h
    nop                               ; C7A7h
    ld de,01111h                      ; C7A8h
    ld a,(bc)                         ; C7ABh
    inc b                             ; C7ACh
    DB 008h                           ; C7ADh
    DB 010h,000h                      ; C7AEh
    ld de,01515h                      ; C7B0h
    ld c,015h                         ; C7B3h
    dec d                             ; C7B5h
    ld de,01E00h                      ; C7B6h
    ld de,01E11h                      ; C7B9h
    ld de,01E11h                      ; C7BCh
    nop                               ; C7BFh
    DB 010h,010h                      ; C7C0h
    DB 010h,01Eh                      ; C7C2h
    ld de,01E11h                      ; C7C4h
    nop                               ; C7C7h
    ld de,01111h                      ; C7C8h
    add hl,de                         ; C7CBh
    dec d                             ; C7CCh
    dec d                             ; C7CDh
    add hl,de                         ; C7CEh
    nop                               ; C7CFh
    ld c,011h                         ; C7D0h
    ld bc,00106h                      ; C7D2h
    ld de,0000Eh                      ; C7D5h
    ld de,01515h                      ; C7D8h
    dec d                             ; C7DBh
    dec d                             ; C7DCh
    dec d                             ; C7DDh
    rra                               ; C7DEh
    nop                               ; C7DFh
    ld c,011h                         ; C7E0h
    ld bc,00107h                      ; C7E2h
    ld de,0000Eh                      ; C7E5h
    dec d                             ; C7E8h
    dec d                             ; C7E9h
    dec d                             ; C7EAh
    dec d                             ; C7EBh
    dec d                             ; C7ECh
    rra                               ; C7EDh
    ld bc,01100h                      ; C7EEh
    ld de,01F11h                      ; C7F1h
    ld bc,00101h                      ; C7F4h
    nop                               ; C7F7h
    ccf                               ; C7F8h
    ccf                               ; C7F9h
    ccf                               ; C7FAh
    ccf                               ; C7FBh
    ccf                               ; C7FCh
    ccf                               ; C7FDh
    ccf                               ; C7FEh
    ccf                               ; C7FFh
;----------------------------------------------------------------------------
; Войти через первый элемент таблицы Монитора-2 в оконный монитор.
;
; вход:
;   Оконный монитор установлен в STD. Начальные регистры и стек для входа
;   по C800h не требуются.
;
; выход:
;   После записи 93h в FF03h установлен sp = 8F50h, инициализирован монитор
;   и начато исполнение его командного цикла. Регистрового результата нет.
;----------------------------------------------------------------------------

monitor_entry:
    jp 0C842h                         ; C800h
    jp 08FA5h                         ; C803h
    jp 0C377h                         ; C806h
    jp 08FA8h                         ; C809h
    jp 0C3D0h                         ; C80Ch
    jp 0CBC0h                         ; C80Fh
    jp 0CECAh                         ; C812h
    jp 0CA7Ch                         ; C815h
    jp 0CA4Dh                         ; C818h
    jp 0D0CBh                         ; C81Bh
    jp 0C964h                         ; C81Eh
    jp 0CB19h                         ; C821h
    jp 0C879h                         ; C824h
    jp 0C888h                         ; C827h
    jp 0CA05h                         ; C82Ah
    jp 0CD2Dh                         ; C82Dh
    jp 0C875h                         ; C830h
    jp 0C871h                         ; C833h
    jp 0D0E3h                         ; C836h
    jp 0CB9Bh                         ; C839h
    jp 0CB6Fh                         ; C83Ch
    jp 0C8AEh                         ; C83Fh
    ld a,093h                         ; C842h
    ld (0FF03h),a                     ; C844h
    ld sp,08F50h                      ; C847h
    ld hl,08F50h                      ; C84Ah
    ld (08F50h),hl                    ; C84Dh
    call 0D0FBh                       ; C850h
    call 0CE39h                       ; C853h
    ld hl,0CDCCh                      ; C856h
    call 0CA4Dh                       ; C859h
    jp 0CB6Fh                         ; C85Ch
    push hl                           ; C85Fh
    add hl,de                         ; C860h
    call 0CD2Ah                       ; C861h
    ld a,e                            ; C864h
    cpl                               ; C865h
    ld e,a                            ; C866h
    ld a,d                            ; C867h
    cpl                               ; C868h
    ld d,a                            ; C869h
    inc de                            ; C86Ah
    pop hl                            ; C86Bh
    add hl,de                         ; C86Ch
    call 0CD2Dh                       ; C86Dh
    ret                               ; C870h
    ld (08F50h),hl                    ; C871h
    ret                               ; C874h
    ld hl,(08F50h)                    ; C875h
    ret                               ; C878h
    ld a,l                            ; C879h
    or h                              ; C87Ah
    or d                              ; C87Bh
    or e                              ; C87Ch
    jp z,0C902h                       ; C87Dh
    ld a,0FFh                         ; C880h
    call 0C416h                       ; C882h
    jp 0C90Ah                         ; C885h
    push hl                           ; C888h
    push de                           ; C889h
    push bc                           ; C88Ah
    call 0CA05h                       ; C88Bh
    ld h,b                            ; C88Eh
    ld l,c                            ; C88Fh
    ld (08F52h),hl                    ; C890h
    pop bc                            ; C893h
    ld a,c                            ; C894h
    or a                              ; C895h
    jp z,0C9B6h                       ; C896h
    ld (08FFEh),a                     ; C899h
    jp 0C9B6h                         ; C89Ch
    ld hl,0CA66h                      ; C89Fh
    call 0CA4Dh                       ; C8A2h
    ld de,08F58h                      ; C8A5h
    ld a,014h                         ; C8A8h
    ld (de),a                         ; C8AAh
    call 0CBC0h                       ; C8ABh
    ld a,092h                         ; C8AEh
    ld (0FF03h),a                     ; C8B0h
    xor a                             ; C8B3h
    ld (08FF3h),a                     ; C8B4h
    ld a,0FFh                         ; C8B7h
    call 0C377h                       ; C8B9h
    cp 0D9h                           ; C8BCh
    jp nz,0C8B3h                      ; C8BEh
    ld c,002h                         ; C8C1h
    ld a,008h                         ; C8C3h
    call 0C377h                       ; C8C5h
    cp 0D9h                           ; C8C8h
    jp nz,0C8B3h                      ; C8CAh
    dec c                             ; C8CDh
    jp nz,0C8C3h                      ; C8CEh
    ld hl,08F6Dh                      ; C8D1h
    ld de,08F81h                      ; C8D4h
    call 0C414h                       ; C8D7h
    ld hl,0CA66h                      ; C8DAh
    call 0CA4Dh                       ; C8DDh
    ld hl,08F6Dh                      ; C8E0h
    call 0CA4Dh                       ; C8E3h
    ld a,(08F59h)                     ; C8E6h
    dec a                             ; C8E9h
    jp z,0C902h                       ; C8EAh
    ld hl,08F5Ah                      ; C8EDh
    ld de,08F6Dh                      ; C8F0h
    ld c,(hl)                         ; C8F3h
    ld a,(de)                         ; C8F4h
    cp c                              ; C8F5h
    jp nz,0C8B3h                      ; C8F6h
    or a                              ; C8F9h
    jp z,0C902h                       ; C8FAh
    inc hl                            ; C8FDh
    inc de                            ; C8FEh
    jp 0C8F3h                         ; C8FFh
    ld a,092h                         ; C902h
    ld (0FF03h),a                     ; C904h
    call 0C3F9h                       ; C907h
    ld a,008h                         ; C90Ah
    call 0C377h                       ; C90Ch
    ld l,a                            ; C90Fh
    ld a,008h                         ; C910h
    call 0C377h                       ; C912h
    ld h,a                            ; C915h
    ld (08F54h),hl                    ; C916h
    ex de,hl                          ; C919h
    ld (08F56h),hl                    ; C91Ah
    ex de,hl                          ; C91Dh
    ld hl,(08FE3h)                    ; C91Eh
    call 0CA05h                       ; C921h
    ld hl,(08F54h)                    ; C924h
    ld e,c                            ; C927h
    ld d,b                            ; C928h
    call 0C427h                       ; C929h
    jp nz,0C960h                      ; C92Ch
    call 0CD1Dh                       ; C92Fh
    ld hl,0CA5Fh                      ; C932h
    call 0CA4Dh                       ; C935h
    ld hl,(08F54h)                    ; C938h
    call 0CD2Dh                       ; C93Bh
    ld hl,0CA6Eh                      ; C93Eh
    call 0CA4Dh                       ; C941h
    ld hl,(08FE3h)                    ; C944h
    call 0CD2Dh                       ; C947h
    ld hl,0CA75h                      ; C94Ah
    call 0CA4Dh                       ; C94Dh
    ld hl,(08F56h)                    ; C950h
    call 0CD2Dh                       ; C953h
    ld a,(08FDEh)                     ; C956h
    cp 041h                           ; C959h
    ret nz                            ; C95Bh
    ld hl,(08FE3h)                    ; C95Ch
    jp (hl)                           ; C95Fh
    call 0CC31h                       ; C960h
    ret                               ; C963h
    ld hl,(08F82h)                    ; C964h
    ret                               ; C967h
    push hl                           ; C968h
    push de                           ; C969h
    push bc                           ; C96Ah
    call 0CA05h                       ; C96Bh
    ld h,b                            ; C96Eh
    ld l,c                            ; C96Fh
    ld (08F52h),hl                    ; C970h
    ld hl,0CA66h                      ; C973h
    call 0CA4Dh                       ; C976h
    ld de,08F58h                      ; C979h
    ld a,014h                         ; C97Ch
    ld (de),a                         ; C97Eh
    call 0CBC0h                       ; C97Fh
    pop bc                            ; C982h
    ld a,c                            ; C983h
    or a                              ; C984h
    jp z,0C98Bh                       ; C985h
    ld (08FFEh),a                     ; C988h
    call 0C9EAh                       ; C98Bh
    call 0C9EAh                       ; C98Eh
    ld a,0E6h                         ; C991h
    call 0C3D0h                       ; C993h
    ld c,003h                         ; C996h
    ld a,0D9h                         ; C998h
    call 0C3D0h                       ; C99Ah
    dec c                             ; C99Dh
    jp nz,0C998h                      ; C99Eh
    ld hl,08F5Ah                      ; C9A1h
    ld a,(hl)                         ; C9A4h
    or a                              ; C9A5h
    jp z,0C9B0h                       ; C9A6h
    call 0C3D0h                       ; C9A9h
    inc hl                            ; C9ACh
    jp 0C9A4h                         ; C9ADh
    call 0C9EAh                       ; C9B0h
    call 0C9EAh                       ; C9B3h
    call 0C9EAh                       ; C9B6h
    ld a,0E6h                         ; C9B9h
    call 0C3D0h                       ; C9BBh
    pop de                            ; C9BEh
    pop hl                            ; C9BFh
    ld a,l                            ; C9C0h
    call 0C3D0h                       ; C9C1h
    ld a,h                            ; C9C4h
    call 0C3D0h                       ; C9C5h
    ld a,e                            ; C9C8h
    call 0C3D0h                       ; C9C9h
    ld a,d                            ; C9CCh
    call 0C3D0h                       ; C9CDh
    ld a,(hl)                         ; C9D0h
    call 0C3D0h                       ; C9D1h
    call 0C427h                       ; C9D4h
    jp z,0C9DEh                       ; C9D7h
    inc hl                            ; C9DAh
    jp 0C9D0h                         ; C9DBh
    ld hl,(08F52h)                    ; C9DEh
    ld a,l                            ; C9E1h
    call 0C3D0h                       ; C9E2h
    ld a,h                            ; C9E5h
    call 0C3D0h                       ; C9E6h
    ret                               ; C9E9h
    ld c,0FFh                         ; C9EAh
    ld a,000h                         ; C9ECh
    call 0C3D0h                       ; C9EEh
    dec c                             ; C9F1h
    jp nz,0C9ECh                      ; C9F2h
    ret                               ; C9F5h
    call 0CA05h                       ; C9F6h
    push bc                           ; C9F9h
    ld hl,0CA5Fh                      ; C9FAh
    call 0CA4Dh                       ; C9FDh
    pop hl                            ; CA00h
    call 0CD2Dh                       ; CA01h
    ret                               ; CA04h
    ld bc,00000h                      ; CA05h
    ld a,(hl)                         ; CA08h
    add a,c                           ; CA09h
    ld c,a                            ; CA0Ah
    push af                           ; CA0Bh
    call 0C427h                       ; CA0Ch
    jp z,0CA27h                       ; CA0Fh
    pop af                            ; CA12h
    ld a,b                            ; CA13h
    adc a,(hl)                        ; CA14h
    ld b,a                            ; CA15h
    call 0CA1Ch                       ; CA16h
    jp 0CA08h                         ; CA19h
    call 0C427h                       ; CA1Ch
    jp nz,0CA25h                      ; CA1Fh
    inc sp                            ; CA22h
    inc sp                            ; CA23h
    ret                               ; CA24h
    inc hl                            ; CA25h
    ret                               ; CA26h
    pop af                            ; CA27h
    ret                               ; CA28h
    call 0CD2Ah                       ; CA29h
    ld a,(hl)                         ; CA2Ch
    call 0CA7Ch                       ; CA2Dh
    call 0CC07h                       ; CA30h
    call 0CB19h                       ; CA33h
    jp nc,0CA44h                      ; CA36h
    cp 01Ah                           ; CA39h
    jp z,0CA45h                       ; CA3Bh
    cp 008h                           ; CA3Eh
    jp z,0CA49h                       ; CA40h
    ret                               ; CA43h
    ld (hl),a                         ; CA44h
    inc hl                            ; CA45h
    jp 0CA29h                         ; CA46h
    dec hl                            ; CA49h
    jp 0CA29h                         ; CA4Ah
    push bc                           ; CA4Dh
    push af                           ; CA4Eh
    ld a,(hl)                         ; CA4Fh
    ld c,a                            ; CA50h
    or a                              ; CA51h
    jp z,0CA5Ch                       ; CA52h
    call 0C809h                       ; CA55h
    inc hl                            ; CA58h
    jp 0CA4Fh                         ; CA59h
    pop af                            ; CA5Ch
    pop bc                            ; CA5Dh
    ret                               ; CA5Eh
    ld a,(bc)                         ; CA5Fh
    dec c                             ; CA60h
    ld b,e                            ; CA61h
    ld d,e                            ; CA62h
    DB 020h,03Dh                      ; CA63h
    nop                               ; CA65h
    ld a,(bc)                         ; CA66h
    dec c                             ; CA67h
    ld b,(hl)                         ; CA68h
    ld c,c                            ; CA69h
    ld c,h                            ; CA6Ah
    ld b,l                            ; CA6Bh
    ld a,(00A00h)                     ; CA6Ch
    dec c                             ; CA6Fh
    ld b,d                            ; CA70h
    ld b,l                            ; CA71h
    ld b,a                            ; CA72h
    dec a                             ; CA73h
    nop                               ; CA74h
    ld a,(bc)                         ; CA75h
    dec c                             ; CA76h
    ld b,l                            ; CA77h
    ld c,(hl)                         ; CA78h
    ld b,h                            ; CA79h
    dec a                             ; CA7Ah
    nop                               ; CA7Bh
    push de                           ; CA7Ch
    push bc                           ; CA7Dh
    call 0CA8Ch                       ; CA7Eh
    ld c,d                            ; CA81h
    call 0C809h                       ; CA82h
    ld c,e                            ; CA85h
    call 0C809h                       ; CA86h
    pop bc                            ; CA89h
    pop de                            ; CA8Ah
    ret                               ; CA8Bh
    ld e,a                            ; CA8Ch
    rrca                              ; CA8Dh
    rrca                              ; CA8Eh
    rrca                              ; CA8Fh
    rrca                              ; CA90h
    call 0CA9Bh                       ; CA91h
    ld d,a                            ; CA94h
    ld a,e                            ; CA95h
    call 0CA9Bh                       ; CA96h
    ld e,a                            ; CA99h
    ret                               ; CA9Ah
    and 00Fh                          ; CA9Bh
    cp 00Ah                           ; CA9Dh
    jp c,0CAA4h                       ; CA9Fh
    add a,007h                        ; CAA2h
    add a,030h                        ; CAA4h
    ret                               ; CAA6h
    call 0CD1Dh                       ; CAA7h
    push hl                           ; CAAAh
    call 0CD2Dh                       ; CAABh
    call 0CC07h                       ; CAAEh
    ld b,008h                         ; CAB1h
    ld a,(hl)                         ; CAB3h
    call 0CA7Ch                       ; CAB4h
    call 0CC07h                       ; CAB7h
    dec b                             ; CABAh
    inc hl                            ; CABBh
    jp nz,0CAB3h                      ; CABCh
    call 0CC07h                       ; CABFh
    pop hl                            ; CAC2h
    ld b,008h                         ; CAC3h
    ld a,(hl)                         ; CAC5h
    call 0CAE4h                       ; CAC6h
    ld c,a                            ; CAC9h
    call 0C809h                       ; CACAh
    call 0C427h                       ; CACDh
    ret z                             ; CAD0h
    dec b                             ; CAD1h
    inc hl                            ; CAD2h
    jp nz,0CAC5h                      ; CAD3h
    call 0D0E3h                       ; CAD6h
    cp 01Fh                           ; CAD9h
    call z,0C803h                     ; CADBh
    cp 01Fh                           ; CADEh
    ret z                             ; CAE0h
    jp 0CAA7h                         ; CAE1h
    cp 020h                           ; CAE4h
    jp c,0CAECh                       ; CAE6h
    cp 080h                           ; CAE9h
    ret c                             ; CAEBh
    ld a,02Eh                         ; CAECh
    ret                               ; CAEEh
    call 0C803h                       ; CAEFh
    cp 020h                           ; CAF2h
    jp c,0CB17h                       ; CAF4h
    cp 041h                           ; CAF7h
    jp nc,0CB0Ah                      ; CAF9h
    or 010h                           ; CAFCh
    cp 03Ah                           ; CAFEh
    jp nc,0CAEFh                      ; CB00h
    ld c,a                            ; CB03h
    sub 030h                          ; CB04h
    call 0C809h                       ; CB06h
    ret                               ; CB09h
    and 05Fh                          ; CB0Ah
    cp 047h                           ; CB0Ch
    jp nc,0CAEFh                      ; CB0Eh
    ld c,a                            ; CB11h
    sub 037h                          ; CB12h
    jp 0CB06h                         ; CB14h
    scf                               ; CB17h
    ret                               ; CB18h
    push bc                           ; CB19h
    call 0CAEFh                       ; CB1Ah
    jp c,0CB2Ch                       ; CB1Dh
    rlca                              ; CB20h
    rlca                              ; CB21h
    rlca                              ; CB22h
    rlca                              ; CB23h
    ld b,a                            ; CB24h
    call 0CAEFh                       ; CB25h
    jp c,0CB2Ch                       ; CB28h
    or b                              ; CB2Bh
    pop bc                            ; CB2Ch
    ret                               ; CB2Dh
    ld de,00000h                      ; CB2Eh
    ld a,(hl)                         ; CB31h
    cp 020h                           ; CB32h
    jp nz,0CB3Bh                      ; CB34h
    inc hl                            ; CB37h
    jp 0CB31h                         ; CB38h
    cp 02Ch                           ; CB3Bh
    ret z                             ; CB3Dh
    or a                              ; CB3Eh
    ret z                             ; CB3Fh
    cp 00Dh                           ; CB40h
    ret z                             ; CB42h
    cp 020h                           ; CB43h
    jp nc,0CB4Ah                      ; CB45h
    scf                               ; CB48h
    ret                               ; CB49h
    cp 041h                           ; CB4Ah
    jp nc,0CB63h                      ; CB4Ch
    or 010h                           ; CB4Fh
    cp 03Ah                           ; CB51h
    jp nc,0CB48h                      ; CB53h
    sub 030h                          ; CB56h
    ex de,hl                          ; CB58h
    add hl,hl                         ; CB59h
    add hl,hl                         ; CB5Ah
    add hl,hl                         ; CB5Bh
    add hl,hl                         ; CB5Ch
    ex de,hl                          ; CB5Dh
    or e                              ; CB5Eh
    ld e,a                            ; CB5Fh
    jp 0CB37h                         ; CB60h
    and 05Fh                          ; CB63h
    cp 047h                           ; CB65h
    jp nc,0CB48h                      ; CB67h
    sub 037h                          ; CB6Ah
    jp 0CB58h                         ; CB6Ch
    call 0C875h                       ; CB6Fh
    ld sp,hl                          ; CB72h
    ld hl,0CC18h                      ; CB73h
    call 0CA4Dh                       ; CB76h
    ld c,000h                         ; CB79h
    ld hl,08F58h                      ; CB7Bh
    ld de,08F81h                      ; CB7Eh
    call 0CC0Fh                       ; CB81h
    ld de,08F58h                      ; CB84h
    ld a,028h                         ; CB87h
    ld (de),a                         ; CB89h
    call 0CBC0h                       ; CB8Ah
    inc de                            ; CB8Dh
    ld a,(de)                         ; CB8Eh
    inc de                            ; CB8Fh
    dec a                             ; CB90h
    jp z,0CB6Fh                       ; CB91h
    ex de,hl                          ; CB94h
    call 0CB9Bh                       ; CB95h
    jp 0CB6Fh                         ; CB98h
    ld a,(hl)                         ; CB9Bh
    ld (08FDEh),a                     ; CB9Ch
    cp 041h                           ; CB9Fh
    jp nz,0CBA5h                      ; CBA1h
    inc hl                            ; CBA4h
    ld (08F56h),hl                    ; CBA5h
    ld b,003h                         ; CBA8h
    inc hl                            ; CBAAh
    call 0CB2Eh                       ; CBABh
    call c,0CC31h                     ; CBAEh
    jp c,0CB6Fh                       ; CBB1h
    push de                           ; CBB4h
    dec b                             ; CBB5h
    jp nz,0CBAAh                      ; CBB6h
    jp 0CC3Ah                         ; CBB9h
    DB 008h                           ; CBBCh
    DB 020h,008h                      ; CBBDh
    nop                               ; CBBFh
    push hl                           ; CBC0h
    push bc                           ; CBC1h
    push af                           ; CBC2h
    push de                           ; CBC3h
    ld b,000h                         ; CBC4h
    ex de,hl                          ; CBC6h
    ld d,(hl)                         ; CBC7h
    inc hl                            ; CBC8h
    inc hl                            ; CBC9h
    call 0C803h                       ; CBCAh
    ld c,a                            ; CBCDh
    cp 008h                           ; CBCEh
    jp z,0CBF4h                       ; CBD0h
    cp 07Fh                           ; CBD3h
    jp z,0CBF4h                       ; CBD5h
    call 0C809h                       ; CBD8h
    dec d                             ; CBDBh
    jp nz,0CBE1h                      ; CBDCh
    ld a,00Dh                         ; CBDFh
    ld (hl),a                         ; CBE1h
    inc hl                            ; CBE2h
    inc b                             ; CBE3h
    cp 00Dh                           ; CBE4h
    jp nz,0CBCAh                      ; CBE6h
    ld (hl),000h                      ; CBE9h
    pop hl                            ; CBEBh
    push hl                           ; CBECh
    inc hl                            ; CBEDh
    ld (hl),b                         ; CBEEh
    pop de                            ; CBEFh
    pop af                            ; CBF0h
    pop bc                            ; CBF1h
    pop hl                            ; CBF2h
    ret                               ; CBF3h
    ld a,b                            ; CBF4h
    or a                              ; CBF5h
    jp z,0CBCAh                       ; CBF6h
    inc d                             ; CBF9h
    dec b                             ; CBFAh
    dec hl                            ; CBFBh
    push hl                           ; CBFCh
    ld hl,0CBBCh                      ; CBFDh
    call 0CA4Dh                       ; CC00h
    pop hl                            ; CC03h
    jp 0CBCAh                         ; CC04h
    push bc                           ; CC07h
    ld c,020h                         ; CC08h
    call 0C809h                       ; CC0Ah
    pop bc                            ; CC0Dh
    ret                               ; CC0Eh
    ld (hl),c                         ; CC0Fh
    call 0C427h                       ; CC10h
    ret z                             ; CC13h
    inc hl                            ; CC14h
    jp 0CC0Fh                         ; CC15h
    ld a,(bc)                         ; CC18h
    dec c                             ; CC19h
    DB 020h,043h                      ; CC1Ah
    ld c,h                            ; CC1Ch
    ld c,c                            ; CC1Dh
    ld a,020h                         ; CC1Eh
    nop                               ; CC20h
    ld a,(bc)                         ; CC21h
    dec c                             ; CC22h
    DB 020h,043h                      ; CC23h
    ld c,h                            ; CC25h
    ld c,c                            ; CC26h
    DB 020h,02Dh                      ; CC27h
    DB 020h,045h                      ; CC29h
    ld d,d                            ; CC2Bh
    ld d,d                            ; CC2Ch
    ld c,a                            ; CC2Dh
    ld d,d                            ; CC2Eh
    rlca                              ; CC2Fh
    nop                               ; CC30h
    push af                           ; CC31h
    ld hl,0CC21h                      ; CC32h
    call 0CA4Dh                       ; CC35h
    pop af                            ; CC38h
    ret                               ; CC39h
    ld hl,(08F56h)                    ; CC3Ah
    ld a,(hl)                         ; CC3Dh
    and 05Fh                          ; CC3Eh
    ld c,a                            ; CC40h
    ld hl,0CDA0h                      ; CC41h
    ld a,(hl)                         ; CC44h
    and (hl)                          ; CC45h
    call z,0CC31h                     ; CC46h
    jp z,0CB6Fh                       ; CC49h
    cp c                              ; CC4Ch
    inc hl                            ; CC4Dh
    jp z,0CC56h                       ; CC4Eh
    inc hl                            ; CC51h
    inc hl                            ; CC52h
    jp 0CC44h                         ; CC53h
    ld e,(hl)                         ; CC56h
    inc hl                            ; CC57h
    ld d,(hl)                         ; CC58h
    pop bc                            ; CC59h
    ex de,hl                          ; CC5Ah
    ex (sp),hl                        ; CC5Bh
    ex de,hl                          ; CC5Ch
    pop hl                            ; CC5Dh
    ex (sp),hl                        ; CC5Eh
    ret                               ; CC5Fh
    push hl                           ; CC60h
    push de                           ; CC61h
    ld c,000h                         ; CC62h
    ld hl,08F5Ah                      ; CC64h
    call 0CC9Eh                       ; CC67h
    call 0CC9Eh                       ; CC6Ah
    call 0CC9Eh                       ; CC6Dh
    jp z,0CCA6h                       ; CC70h
    inc c                             ; CC73h
    ld (08F54h),a                     ; CC74h
    call 0CC9Eh                       ; CC77h
    jp z,0CC8Bh                       ; CC7Ah
    inc c                             ; CC7Dh
    ld (08F55h),a                     ; CC7Eh
    call 0CC9Eh                       ; CC81h
    jp z,0CC8Bh                       ; CC84h
    inc c                             ; CC87h
    ld (08F56h),a                     ; CC88h
    pop de                            ; CC8Bh
    pop hl                            ; CC8Ch
    ld b,c                            ; CC8Dh
    ld c,b                            ; CC8Eh
    call 0C427h                       ; CC8Fh
    ret z                             ; CC92h
    ld a,(08F54h)                     ; CC93h
    cp (hl)                           ; CC96h
    jp z,0CCA9h                       ; CC97h
    inc hl                            ; CC9Ah
    jp 0CC8Eh                         ; CC9Bh
    inc hl                            ; CC9Eh
    call 0CB2Eh                       ; CC9Fh
    ld a,(hl)                         ; CCA2h
    or a                              ; CCA3h
    ld a,e                            ; CCA4h
    ret                               ; CCA5h
    pop de                            ; CCA6h
    pop hl                            ; CCA7h
    ret                               ; CCA8h
    dec c                             ; CCA9h
    jp z,0CCCDh                       ; CCAAh
    inc hl                            ; CCADh
    call 0C427h                       ; CCAEh
    ret z                             ; CCB1h
    ld a,(08F55h)                     ; CCB2h
    cp (hl)                           ; CCB5h
    jp nz,0CC8Eh                      ; CCB6h
    dec c                             ; CCB9h
    jp z,0CCCDh                       ; CCBAh
    inc hl                            ; CCBDh
    call 0C427h                       ; CCBEh
    ret z                             ; CCC1h
    ld a,(08F56h)                     ; CCC2h
    cp (hl)                           ; CCC5h
    jp nz,0CC8Eh                      ; CCC6h
    dec c                             ; CCC9h
    jp z,0CCCDh                       ; CCCAh
    ld c,b                            ; CCCDh
    dec hl                            ; CCCEh
    dec c                             ; CCCFh
    jp nz,0CCCEh                      ; CCD0h
    inc hl                            ; CCD3h
    call 0CCE6h                       ; CCD4h
    call 0D0E3h                       ; CCD7h
    cp 01Fh                           ; CCDAh
    call z,0C803h                     ; CCDCh
    cp 01Fh                           ; CCDFh
    ret z                             ; CCE1h
    inc hl                            ; CCE2h
    jp 0CC8Eh                         ; CCE3h
    push bc                           ; CCE6h
    call 0CD1Dh                       ; CCE7h
    call 0CD2Dh                       ; CCEAh
    ld a,b                            ; CCEDh
    add a,004h                        ; CCEEh
    ld b,a                            ; CCF0h
    push hl                           ; CCF1h
    dec hl                            ; CCF2h
    dec hl                            ; CCF3h
    ld a,(hl)                         ; CCF4h
    call 0CA7Ch                       ; CCF5h
    call 0CC07h                       ; CCF8h
    inc hl                            ; CCFBh
    dec b                             ; CCFCh
    jp nz,0CCF4h                      ; CCFDh
    call 0CC07h                       ; CD00h
    pop hl                            ; CD03h
    pop bc                            ; CD04h
    push bc                           ; CD05h
    push hl                           ; CD06h
    dec hl                            ; CD07h
    dec hl                            ; CD08h
    ld a,b                            ; CD09h
    add a,004h                        ; CD0Ah
    ld b,a                            ; CD0Ch
    ld a,(hl)                         ; CD0Dh
    call 0CAE4h                       ; CD0Eh
    ld c,a                            ; CD11h
    call 0C809h                       ; CD12h
    inc hl                            ; CD15h
    dec b                             ; CD16h
    jp nz,0CD0Dh                      ; CD17h
    pop hl                            ; CD1Ah
    pop bc                            ; CD1Bh
    ret                               ; CD1Ch
    push bc                           ; CD1Dh
    ld c,00Ah                         ; CD1Eh
    call 0C809h                       ; CD20h
    ld c,00Dh                         ; CD23h
    call 0C809h                       ; CD25h
    pop bc                            ; CD28h
    ret                               ; CD29h
    call 0CD1Dh                       ; CD2Ah
    ld a,h                            ; CD2Dh
    call 0CA7Ch                       ; CD2Eh
    ld a,l                            ; CD31h
    call 0CA7Ch                       ; CD32h
    call 0CC07h                       ; CD35h
    ret                               ; CD38h
    jp (hl)                           ; CD39h
    ld a,(bc)                         ; CD3Ah
    cp (hl)                           ; CD3Bh
    jp z,0CD54h                       ; CD3Ch
    push af                           ; CD3Fh
    call nz,0CD2Ah                    ; CD40h
    pop af                            ; CD43h
    push af                           ; CD44h
    ld a,(hl)                         ; CD45h
    call nz,0CA7Ch                    ; CD46h
    push bc                           ; CD49h
    ld c,020h                         ; CD4Ah
    call 0C809h                       ; CD4Ch
    pop bc                            ; CD4Fh
    pop af                            ; CD50h
    call nz,0CA7Ch                    ; CD51h
    call 0C427h                       ; CD54h
    ret z                             ; CD57h
    inc hl                            ; CD58h
    inc bc                            ; CD59h
    call 0D0E3h                       ; CD5Ah
    cp 01Fh                           ; CD5Dh
    call z,0C803h                     ; CD5Fh
    cp 01Fh                           ; CD62h
    ret z                             ; CD64h
    jp 0CD3Ah                         ; CD65h
    ld a,h                            ; CD68h
    cp b                              ; CD69h
    jp c,0CD92h                       ; CD6Ah
    ld a,l                            ; CD6Dh
    cp c                              ; CD6Eh
    jp c,0CD92h                       ; CD6Fh
    ld a,(hl)                         ; CD72h
    ld (bc),a                         ; CD73h
    call 0C427h                       ; CD74h
    ret z                             ; CD77h
    inc hl                            ; CD78h
    inc bc                            ; CD79h
    jp 0CD72h                         ; CD7Ah
    push hl                           ; CD7Dh
    push bc                           ; CD7Eh
    or a                              ; CD7Fh
    ld a,e                            ; CD80h
    sbc a,l                           ; CD81h
    ld c,a                            ; CD82h
    ld a,d                            ; CD83h
    sbc a,h                           ; CD84h
    jp c,0CD8Fh                       ; CD85h
    ld b,a                            ; CD88h
    pop hl                            ; CD89h
    add hl,bc                         ; CD8Ah
    push hl                           ; CD8Bh
    pop bc                            ; CD8Ch
    pop hl                            ; CD8Dh
    ret                               ; CD8Eh
    pop bc                            ; CD8Fh
    pop hl                            ; CD90h
    ret                               ; CD91h
    call 0CD7Dh                       ; CD92h
    ld a,(de)                         ; CD95h
    ld (bc),a                         ; CD96h
    call 0C427h                       ; CD97h
    ret z                             ; CD9Ah
    dec de                            ; CD9Bh
    dec bc                            ; CD9Ch
    jp 0CD95h                         ; CD9Dh
    ld b,h                            ; CDA0h
    and a                             ; CDA1h
    jp z,0294Dh                       ; CDA2h
    jp z,00F46h                       ; CDA5h
    call z,06053h                     ; CDA8h
    call z,03947h                     ; CDABh
    call 03A43h                       ; CDAEh
    call 06854h                       ; CDB1h
    call 0684Fh                       ; CDB4h
    ret                               ; CDB7h
    ld c,c                            ; CDB8h
    sbc a,a                           ; CDB9h
    ret z                             ; CDBAh
    ld c,b                            ; CDBBh
    ld e,a                            ; CDBCh
    ret z                             ; CDBDh
    ld d,a                            ; CDBEh
    adc a,b                           ; CDBFh
    ret z                             ; CDC0h
    ld d,d                            ; CDC1h
    ld a,c                            ; CDC2h
    ret z                             ; CDC3h
    ld c,e                            ; CDC4h
    or 0C9h                           ; CDC5h
    ld d,(hl)                         ; CDC7h
    and 0CDh                          ; CDC8h
    nop                               ; CDCAh
    nop                               ; CDCBh
    add hl,bc                         ; CDCCh
    dec de                            ; CDCDh
    ld e,h                            ; CDCEh
    DB 020h,028h                      ; CDCFh
    ld (hl),e                         ; CDD1h
    add hl,hl                         ; CDD2h
    DB 020h,073h                      ; CDD3h
    ld d,b                            ; CDD5h
    ld b,l                            ; CDD6h
    ld b,e                            ; CDD7h
    ld c,c                            ; CDD8h
    ld b,c                            ; CDD9h
    ld c,h                            ; CDDAh
    ld c,c                            ; CDDBh
    ld d,e                            ; CDDCh
    ld d,h                            ; CDDDh
    dec de                            ; CDDEh
    ld e,l                            ; CDDFh
    DB 020h,056h                      ; CDE0h
    dec (hl)                          ; CDE2h
    ld l,037h                         ; CDE3h
    nop                               ; CDE5h
    ld sp,08F50h                      ; CDE6h
    ld hl,0CE06h                      ; CDE9h
    push hl                           ; CDECh
    ld c,03Fh                         ; CDEDh
    ld hl,0CE0Ah                      ; CDEFh
    push hl                           ; CDF2h
    dec c                             ; CDF3h
    jp nz,0CDF2h                      ; CDF4h
    ld de,0FF01h                      ; CDF7h
    ld hl,0FFFBh                      ; CDFAh
    ld bc,000AEh                      ; CDFDh
    nop                               ; CE00h
    nop                               ; CE01h
    nop                               ; CE02h
    nop                               ; CE03h
    nop                               ; CE04h
    nop                               ; CE05h
    ld sp,08ED0h                      ; CE06h
    inc hl                            ; CE09h
    ld a,(de)                         ; CE0Ah
    cp b                              ; CE0Bh
    ret z                             ; CE0Ch
    ld b,a                            ; CE0Dh
    dec c                             ; CE0Eh
    ret nz                            ; CE0Fh
    add hl,hl                         ; CE10h
    add hl,hl                         ; CE11h
    add hl,hl                         ; CE12h
    add hl,hl                         ; CE13h
    add hl,hl                         ; CE14h
    add hl,hl                         ; CE15h
    ld a,h                            ; CE16h
    add hl,hl                         ; CE17h
    add a,h                           ; CE18h
    ld d,a                            ; CE19h
    ld hl,0CE2Ah                      ; CE1Ah
    call 0CA4Dh                       ; CE1Dh
    ld a,d                            ; CE20h
    ld (08FFFh),a                     ; CE21h
    call 0CA7Ch                       ; CE24h
    jp 0CB6Fh                         ; CE27h
    ld a,(bc)                         ; CE2Ah
    ld l,e                            ; CE2Bh
    ld a,(hl)                         ; CE2Ch
    dec a                             ; CE2Dh
    nop                               ; CE2Eh
    push hl                           ; CE2Fh
    push de                           ; CE30h
    push bc                           ; CE31h
    xor a                             ; CE32h
    ld (08FDBh),a                     ; CE33h
    jp 08FD0h                         ; CE36h
    ld hl,08FD0h                      ; CE39h
    ld de,0CEBFh                      ; CE3Ch
    ld b,00Bh                         ; CE3Fh
    call 0D3CBh                       ; CE41h
    ld a,093h                         ; CE44h
    ld (0FF03h),a                     ; CE46h
    ld a,(0FF01h)                     ; CE49h
    or 003h                           ; CE4Ch
    ld h,a                            ; CE4Eh
    ld a,(0FF00h)                     ; CE4Fh
    cp h                              ; CE52h
    jp nz,0CE5Ch                      ; CE53h
    ld hl,0CE73h                      ; CE56h
    jp 0CE5Fh                         ; CE59h
    ld hl,0D09Dh                      ; CE5Ch
    ld (08FD1h),hl                    ; CE5Fh
    ret                               ; CE62h
    call 0D071h                       ; CE63h
    call 0D091h                       ; CE66h
    ld a,(08FDDh)                     ; CE69h
    ld (08FDCh),a                     ; CE6Ch
    pop bc                            ; CE6Fh
    pop de                            ; CE70h
    pop hl                            ; CE71h
    ret                               ; CE72h
    call 0CEF1h                       ; CE73h
    call 0CF72h                       ; CE76h
    jp nc,0CE89h                      ; CE79h
    ld (08FDCh),a                     ; CE7Ch
    xor a                             ; CE7Fh
    ld (08FD3h),a                     ; CE80h
    ld (08FD4h),a                     ; CE83h
    jp 0CE73h                         ; CE86h
    ld c,a                            ; CE89h
    ld a,(08FDCh)                     ; CE8Ah
    cp c                              ; CE8Dh
    jp z,0CE97h                       ; CE8Eh
    call 0D063h                       ; CE91h
    jp 0CE63h                         ; CE94h
    ld a,(08FD3h)                     ; CE97h
    or a                              ; CE9Ah
    jp z,0CEA7h                       ; CE9Bh
    ld hl,00A00h                      ; CE9Eh
    call 0D066h                       ; CEA1h
    jp 0CE66h                         ; CEA4h
    ld a,(08FD6h)                     ; CEA7h
    ld h,a                            ; CEAAh
    call 0CEF1h                       ; CEABh
    call 0CF72h                       ; CEAEh
    jp c,0CE7Ch                       ; CEB1h
    dec h                             ; CEB4h
    jp nz,0CEABh                      ; CEB5h
    cpl                               ; CEB8h
    ld (08FD3h),a                     ; CEB9h
    jp 0CE91h                         ; CEBCh
    jp 0CE73h                         ; CEBFh
    nop                               ; CEC2h
    nop                               ; CEC3h
    nop                               ; CEC4h
    ld d,l                            ; CEC5h
    ret p                             ; CEC6h
    nop                               ; CEC7h
    ld d,0CFh                         ; CEC8h
    ld a,(08FD1h)                     ; CECAh
    cp 073h                           ; CECDh
    jp z,0CEE2h                       ; CECFh
    ld a,092h                         ; CED2h
    ld (0FF03h),a                     ; CED4h
    ld a,(0FF01h)                     ; CED7h
    or 07Fh                           ; CEDAh
    xor 0FFh                          ; CEDCh
    ret z                             ; CEDEh
    ld a,0FFh                         ; CEDFh
    ret                               ; CEE1h
    ld a,082h                         ; CEE2h
    ld (0FF03h),a                     ; CEE4h
    ld a,(0FF01h)                     ; CEE7h
    or 003h                           ; CEEAh
    xor 0FFh                          ; CEECh
    jp 0CEDEh                         ; CEEEh
    push hl                           ; CEF1h
    push af                           ; CEF2h
    ld a,(08F8Ah)                     ; CEF3h
    rrca                              ; CEF6h
    rrca                              ; CEF7h
    jp c,0CF13h                       ; CEF8h
    ld hl,08FDBh                      ; CEFBh
    ld a,(hl)                         ; CEFEh
    cpl                               ; CEFFh
    ld (hl),a                         ; CF00h
    ld hl,(08F84h)                    ; CF01h
    ld a,(hl)                         ; CF04h
    cpl                               ; CF05h
    ld (hl),a                         ; CF06h
    ld a,(08F8Bh)                     ; CF07h
    cp 008h                           ; CF0Ah
    jp z,0CF13h                       ; CF0Ch
    inc l                             ; CF0Fh
    ld a,(hl)                         ; CF10h
    cpl                               ; CF11h
    ld (hl),a                         ; CF12h
    pop af                            ; CF13h
    pop hl                            ; CF14h
    ret                               ; CF15h
    nop                               ; CF16h
    ld bc,00302h                      ; CF17h
    inc b                             ; CF1Ah
    dec b                             ; CF1Bh
    ld b,007h                         ; CF1Ch
    DB 008h                           ; CF1Eh
    add hl,bc                         ; CF1Fh
    call m,0001Fh                     ; CF20h
    nop                               ; CF23h
    nop                               ; CF24h
    nop                               ; CF25h
    dec sp                            ; CF26h
    ld sp,03332h                      ; CF27h
    inc (hl)                          ; CF2Ah
    dec (hl)                          ; CF2Bh
    ld (hl),037h                      ; CF2Ch
    DB 038h,039h                      ; CF2Eh
    DB 030h,02Dh                      ; CF30h
    nop                               ; CF32h
    nop                               ; CF33h
    nop                               ; CF34h
    nop                               ; CF35h
    ld c,d                            ; CF36h
    ld b,e                            ; CF37h
    ld d,l                            ; CF38h
    ld c,e                            ; CF39h
    ld b,l                            ; CF3Ah
    ld c,(hl)                         ; CF3Bh
    ld b,a                            ; CF3Ch
    ld e,e                            ; CF3Dh
    ld e,l                            ; CF3Eh
    ld e,d                            ; CF3Fh
    ld c,b                            ; CF40h
    ld a,(00000h)                     ; CF41h
    nop                               ; CF44h
    nop                               ; CF45h
    ld b,(hl)                         ; CF46h
    ld e,c                            ; CF47h
    ld d,a                            ; CF48h
    ld b,c                            ; CF49h
    ld d,b                            ; CF4Ah
    ld d,d                            ; CF4Bh
    ld c,a                            ; CF4Ch
    ld c,h                            ; CF4Dh
    ld b,h                            ; CF4Eh
    ld d,(hl)                         ; CF4Fh
    ld e,h                            ; CF50h
    ld l,000h                         ; CF51h
    nop                               ; CF53h
    nop                               ; CF54h
    nop                               ; CF55h
    ld d,c                            ; CF56h
    ld e,(hl)                         ; CF57h
    ld d,e                            ; CF58h
    ld c,l                            ; CF59h
    ld c,c                            ; CF5Ah
    ld d,h                            ; CF5Bh
    ld e,b                            ; CF5Ch
    ld b,d                            ; CF5Dh
    ld b,b                            ; CF5Eh
    inc l                             ; CF5Fh
    cpl                               ; CF60h
    ld e,a                            ; CF61h
    nop                               ; CF62h
    nop                               ; CF63h
    nop                               ; CF64h
    nop                               ; CF65h
    cp 00Ch                           ; CF66h
    add hl,de                         ; CF68h
    ld a,(de)                         ; CF69h
    add hl,bc                         ; CF6Ah
    dec de                            ; CF6Bh
    DB 020h,008h                      ; CF6Ch
    DB 0FDh                           ; CF6Eh
    DB 018h,00Ah                      ; CF6Fh
    dec c                             ; CF71h
    push hl                           ; CF72h
    push de                           ; CF73h
    push bc                           ; CF74h
    ld a,082h                         ; CF75h
    ld (0FF03h),a                     ; CF77h
    ld bc,08000h                      ; CF7Ah
    ld hl,00001h                      ; CF7Dh
    ld a,l                            ; CF80h
    cpl                               ; CF81h
    ld (0FF00h),a                     ; CF82h
    ld a,h                            ; CF85h
    cpl                               ; CF86h
    and 00Fh                          ; CF87h
    ld (0FF02h),a                     ; CF89h
    ld a,(0FF01h)                     ; CF8Ch
    or 003h                           ; CF8Fh
    cpl                               ; CF91h
    and b                             ; CF92h
    jp z,0CF99h                       ; CF93h
    push hl                           ; CF96h
    push bc                           ; CF97h
    inc c                             ; CF98h
    add hl,hl                         ; CF99h
    jp nc,0CF80h                      ; CF9Ah
    ld a,b                            ; CF9Dh
    rrca                              ; CF9Eh
    ld b,a                            ; CF9Fh
    cp 002h                           ; CFA0h
    jp nz,0CF7Dh                      ; CFA2h
    ld a,c                            ; CFA5h
    or a                              ; CFA6h
    jp z,0D032h                       ; CFA7h
    cp 003h                           ; CFAAh
    jp c,0CFB8h                       ; CFACh
    pop de                            ; CFAFh
    pop hl                            ; CFB0h
    dec c                             ; CFB1h
    jp nz,0CFAFh                      ; CFB2h
    jp 0D032h                         ; CFB5h
    ld b,0FFh                         ; CFB8h
    pop hl                            ; CFBAh
    pop de                            ; CFBBh
    call 0D038h                       ; CFBCh
    ld hl,08FD4h                      ; CFBFh
    cp 0FEh                           ; CFC2h
    jp nz,0CFCCh                      ; CFC4h
    ld (hl),010h                      ; CFC7h
    jp 0CFF5h                         ; CFC9h
    cp 0FDh                           ; CFCCh
    jp nz,0CFD6h                      ; CFCEh
    ld (hl),01Fh                      ; CFD1h
    jp 0CFF5h                         ; CFD3h
    cp 0FCh                           ; CFD6h
    jp nz,0CFECh                      ; CFD8h
    inc hl                            ; CFDBh
    ld a,020h                         ; CFDCh
    xor (hl)                          ; CFDEh
    ld (hl),a                         ; CFDFh
    call 0D071h                       ; CFE0h
    call 0CECAh                       ; CFE3h
    jp nz,0CFE3h                      ; CFE6h
    jp 0CFF5h                         ; CFE9h
    cp 0FBh                           ; CFECh
    jp nz,0CFF4h                      ; CFEEh
    ld a,(08FDDh)                     ; CFF1h
    ld b,a                            ; CFF4h
    dec c                             ; CFF5h
    jp nz,0CFBAh                      ; CFF6h
    ld a,b                            ; CFF9h
    cp 0FFh                           ; CFFAh
    jp z,0D032h                       ; CFFCh
    ld (08FDDh),a                     ; CFFFh
    cp 040h                           ; D002h
    jp c,0D00Ch                       ; D004h
    ld a,(08FD5h)                     ; D007h
    or b                              ; D00Ah
    ld b,a                            ; D00Bh
    cp 040h                           ; D00Ch
    jp nc,0D020h                      ; D00Eh
    cp 020h                           ; D011h
    jp c,0D020h                       ; D013h
    ld a,(08FD4h)                     ; D016h
    cp 010h                           ; D019h
    jp nz,0D020h                      ; D01Bh
    xor b                             ; D01Eh
    ld b,a                            ; D01Fh
    ld a,(08FD4h)                     ; D020h
    cp 01Fh                           ; D023h
    jp nz,0D02Ah                      ; D025h
    and b                             ; D028h
    ld b,a                            ; D029h
    ld a,b                            ; D02Ah
    ld (08FDDh),a                     ; D02Bh
    or a                              ; D02Eh
    jp 0CE6Fh                         ; D02Fh
    ld a,0FFh                         ; D032h
    scf                               ; D034h
    jp 0CE6Fh                         ; D035h
    push bc                           ; D038h
    ld a,h                            ; D039h
    ld bc,00000h                      ; D03Ah
    rlca                              ; D03Dh
    jp c,0D045h                       ; D03Eh
    inc b                             ; D041h
    jp 0D03Dh                         ; D042h
    ex de,hl                          ; D045h
    add hl,hl                         ; D046h
    add hl,hl                         ; D047h
    add hl,hl                         ; D048h
    add hl,hl                         ; D049h
    add hl,hl                         ; D04Ah
    jp c,0D052h                       ; D04Bh
    inc c                             ; D04Eh
    jp 0D04Ah                         ; D04Fh
    ex de,hl                          ; D052h
    ld a,b                            ; D053h
    rlca                              ; D054h
    rlca                              ; D055h
    rlca                              ; D056h
    rlca                              ; D057h
    or c                              ; D058h
    ld c,a                            ; D059h
    ld b,000h                         ; D05Ah
    ld hl,(08FD9h)                    ; D05Ch
    add hl,bc                         ; D05Fh
    ld a,(hl)                         ; D060h
    pop bc                            ; D061h
    ret                               ; D062h
    ld a,(08FD7h)                     ; D063h
    dec a                             ; D066h
    jp nz,0D066h                      ; D067h
    ret                               ; D06Ah
    ld a,050h                         ; D06Bh
    ret                               ; D06Dh
    ld a,040h                         ; D06Eh
    ret                               ; D070h
    push af                           ; D071h
    ld a,(08F8Ah)                     ; D072h
    add a,a                           ; D075h
    jp m,0D08Fh                       ; D076h
    ld a,(08FD5h)                     ; D079h
    and 020h                          ; D07Ch
    call nz,0D06Eh                    ; D07Eh
    call z,0D06Bh                     ; D081h
    ld (08FF1h),a                     ; D084h
    ld a,040h                         ; D087h
    ld (08FF2h),a                     ; D089h
    call 0C170h                       ; D08Ch
    pop af                            ; D08Fh
    ret                               ; D090h
    ld hl,(08F84h)                    ; D091h
    ld a,(08FDBh)                     ; D094h
    or a                              ; D097h
    ret z                             ; D098h
    call 0CEF1h                       ; D099h
    ret                               ; D09Ch
    call 0CEF1h                       ; D09Dh
    ld a,092h                         ; D0A0h
    ld (0FF03h),a                     ; D0A2h
    ld a,(0FF01h)                     ; D0A5h
    or a                              ; D0A8h
    jp m,0D09Dh                       ; D0A9h
    call 0D0B5h                       ; D0ACh
    ld (08FDDh),a                     ; D0AFh
    jp 0CE63h                         ; D0B2h
    push hl                           ; D0B5h
    ld a,092h                         ; D0B6h
    ld (0FF03h),a                     ; D0B8h
    ld a,(0FF00h)                     ; D0BBh
    push af                           ; D0BEh
    ld a,001h                         ; D0BFh
    ld (0FF02h),a                     ; D0C1h
    xor a                             ; D0C4h
    ld (0FF02h),a                     ; D0C5h
    pop af                            ; D0C8h
    pop hl                            ; D0C9h
    ret                               ; D0CAh
    call 0CECAh                       ; D0CBh
    cpl                               ; D0CEh
    ret z                             ; D0CFh
    ld (08FD4h),a                     ; D0D0h
    ld a,(08FD1h)                     ; D0D3h
    cp 073h                           ; D0D6h
    jp nz,0D0DFh                      ; D0D8h
    call 0CF72h                       ; D0DBh
    ret                               ; D0DEh
    call 0D0B5h                       ; D0DFh
    ret                               ; D0E2h
    call 0D0CBh                       ; D0E3h
    ret z                             ; D0E6h
    push af                           ; D0E7h
    call 0CECAh                       ; D0E8h
    jp nz,0D0E8h                      ; D0EBh
    call 0D0CBh                       ; D0EEh
    pop af                            ; D0F1h
    ret                               ; D0F2h
    push hl                           ; D0F3h
    push de                           ; D0F4h
    push bc                           ; D0F5h
    push af                           ; D0F6h
    ld a,c                            ; D0F7h
    jp 08FA2h                         ; D0F8h
    push hl                           ; D0FBh
    push de                           ; D0FCh
    push bc                           ; D0FDh
    push af                           ; D0FEh
    ld hl,08F82h                      ; D0FFh
    ld de,0D384h                      ; D102h
    ld b,030h                         ; D105h
    call 0D3CBh                       ; D107h
    call 0C010h                       ; D10Ah
    jp 0D4E9h                         ; D10Dh
    call 0D3B4h                       ; D110h
    ld hl,0D16Dh                      ; D113h
    ld (08FA3h),hl                    ; D116h
    xor a                             ; D119h
    ld (08FABh),a                     ; D11Ah
    pop af                            ; D11Dh
    pop bc                            ; D11Eh
    pop de                            ; D11Fh
    pop hl                            ; D120h
    ret                               ; D121h
    cp 001h                           ; D122h
    jp nz,0D12Dh                      ; D124h
    ld hl,0D172h                      ; D127h
    jp 0D116h                         ; D12Ah
    cp 01Bh                           ; D12Dh
    jp nz,0D138h                      ; D12Fh
    ld hl,0D1ABh                      ; D132h
    jp 0D116h                         ; D135h
    cp 007h                           ; D138h
    jp z,0D4E3h                       ; D13Ah
    cp 008h                           ; D13Dh
    jp z,0D26Eh                       ; D13Fh
    cp 009h                           ; D142h
    jp z,0D2B2h                       ; D144h
    cp 00Ah                           ; D147h
    jp z,0D2D6h                       ; D149h
    cp 00Ch                           ; D14Ch
    jp z,0D2C4h                       ; D14Eh
    cp 00Dh                           ; D151h
    jp z,0D2CDh                       ; D153h
    cp 018h                           ; D156h
    jp z,0D28Ah                       ; D158h
    cp 019h                           ; D15Bh
    jp z,0D228h                       ; D15Dh
    cp 01Ah                           ; D160h
    jp z,0D247h                       ; D162h
    cp 01Fh                           ; D165h
    jp z,0D4D7h                       ; D167h
    jp 0D11Dh                         ; D16Ah
    cp 020h                           ; D16Dh
    jp c,0D122h                       ; D16Fh
    ld hl,08F8Dh                      ; D172h
    rrca                              ; D175h
    rrca                              ; D176h
    rrca                              ; D177h
    rrca                              ; D178h
    and 00Eh                          ; D179h
    add a,l                           ; D17Bh
    ld l,a                            ; D17Ch
    ld a,000h                         ; D17Dh
    adc a,h                           ; D17Fh
    ld h,a                            ; D180h
    ld e,(hl)                         ; D181h
    inc hl                            ; D182h
    ld d,(hl)                         ; D183h
    ld a,c                            ; D184h
    and 01Fh                          ; D185h
    ld l,a                            ; D187h
    ld h,000h                         ; D188h
    add hl,hl                         ; D18Ah
    add hl,hl                         ; D18Bh
    add hl,hl                         ; D18Ch
    add hl,de                         ; D18Dh
    ex de,hl                          ; D18Eh
    ld hl,(08F84h)                    ; D18Fh
    ld a,l                            ; D192h
    sub 007h                          ; D193h
    ld l,a                            ; D195h
    ld c,008h                         ; D196h
    ld a,(de)                         ; D198h
    call 08F9Dh                       ; D199h
    ld (hl),a                         ; D19Ch
    inc l                             ; D19Dh
    inc e                             ; D19Eh
    dec c                             ; D19Fh
    jp nz,0D198h                      ; D1A0h
    jp 0D28Ah                         ; D1A3h
    cpl                               ; D1A6h
    xor (hl)                          ; D1A7h
    ret                               ; D1A8h
    cpl                               ; D1A9h
    ret                               ; D1AAh
    cp 020h                           ; D1ABh
    jp c,0D113h                       ; D1ADh
    cp 060h                           ; D1B0h
    jp nc,0D113h                      ; D1B2h
    and 01Fh                          ; D1B5h
    rlca                              ; D1B7h
    ld hl,0D1C5h                      ; D1B8h
    ld bc,00000h                      ; D1BBh
    ld c,a                            ; D1BEh
    add hl,bc                         ; D1BFh
    ld e,(hl)                         ; D1C0h
    inc hl                            ; D1C1h
    ld d,(hl)                         ; D1C2h
    ex de,hl                          ; D1C3h
    jp (hl)                           ; D1C4h
    jp (hl)                           ; D1C5h
    push de                           ; D1C6h
    DB 028h,0D2h                      ; D1C7h
    ld b,a                            ; D1C9h
    jp nc,0D28Ah                      ; D1CAh
    ld l,(hl)                         ; D1CDh
    jp nc,0D0FFh                      ; D1CEh
    jp nz,0CDD3h                      ; D1D1h
    jp nc,0D2C4h                      ; D1D4h
    ld d,0D6h                         ; D1D7h
    sub e                             ; D1D9h
    call nc,0D464h                    ; D1DAh
    DB 0EDh,0D2h                      ; D1DDh
    ld bc,0E9D3h                      ; D1DFh
    call nc,0D501h                    ; D1E2h
    rlca                              ; D1E5h
    out (00Fh),a                      ; D1E6h
    out (02Dh),a                      ; D1E8h
    sub 0E0h                          ; D1EAh
    out (009h),a                      ; D1ECh
    call nc,0D638h                    ; D1EEh
    rla                               ; D1F1h
    out (027h),a                      ; D1F2h
    out (005h),a                      ; D1F4h
    jp nc,0D50Ah                      ; D1F6h
    DB 010h,0D5h                      ; D1F9h
    adc a,c                           ; D1FBh
    push de                           ; D1FCh
    adc a,a                           ; D1FDh
    push de                           ; D1FEh
    sub l                             ; D1FFh
    push de                           ; D200h
    sbc a,e                           ; D201h
    push de                           ; D202h
    ld l,h                            ; D203h
    push de                           ; D204h
    ld hl,0D20Bh                      ; D205h
    jp 0D116h                         ; D208h
    ld (08F8Ah),a                     ; D20Bh
    call 0D217h                       ; D20Eh
    ld (08F9Eh),hl                    ; D211h
    jp 0D110h                         ; D214h
    ld hl,0D1A6h                      ; D217h
    and 0A0h                          ; D21Ah
    cp 0A0h                           ; D21Ch
    ret z                             ; D21Eh
    inc hl                            ; D21Fh
    cp 020h                           ; D220h
    ret z                             ; D222h
    inc hl                            ; D223h
    or a                              ; D224h
    ret z                             ; D225h
    inc hl                            ; D226h
    ret                               ; D227h
    ld a,(08F87h)                     ; D228h
    ld l,a                            ; D22Bh
    ld a,(08F8Bh)                     ; D22Ch
    ld c,a                            ; D22Fh
    ld a,(08F83h)                     ; D230h
    sub c                             ; D233h
    jp c,0D23Eh                       ; D234h
    ld (08F83h),a                     ; D237h
    cp l                              ; D23Ah
    jp nc,0D110h                      ; D23Bh
    ld a,(08F89h)                     ; D23Eh
    ld (08F83h),a                     ; D241h
    jp 0D110h                         ; D244h
    ld a,(08F83h)                     ; D247h
    ld c,a                            ; D24Ah
    ld a,(08F8Bh)                     ; D24Bh
    add a,c                           ; D24Eh
    jp c,0D25Dh                       ; D24Fh
    ld l,a                            ; D252h
    ld (08F83h),a                     ; D253h
    ld a,(08F89h)                     ; D256h
    cp l                              ; D259h
    jp nc,0D110h                      ; D25Ah
    ld a,(08F89h)                     ; D25Dh
    ld (08F83h),a                     ; D260h
    ld a,h                            ; D263h
    or a                              ; D264h
    jp z,0D3D9h                       ; D265h
    ld a,(08F87h)                     ; D268h
    jp 0D241h                         ; D26Bh
    ld a,(08F86h)                     ; D26Eh
    ld l,a                            ; D271h
    ld a,(08F82h)                     ; D272h
    sub 001h                          ; D275h
    jp c,0D281h                       ; D277h
    ld (08F82h),a                     ; D27Ah
    cp l                              ; D27Dh
    jp nc,0D110h                      ; D27Eh
    ld a,(08F88h)                     ; D281h
    ld (08F82h),a                     ; D284h
    jp 0D228h                         ; D287h
    ld a,(08F82h)                     ; D28Ah
    inc a                             ; D28Dh
    ld l,a                            ; D28Eh
    ld a,(08F88h)                     ; D28Fh
    cp l                              ; D292h
    jp c,0D29Dh                       ; D293h
    ld a,l                            ; D296h
    ld (08F82h),a                     ; D297h
    jp 0D110h                         ; D29Ah
    ld a,(08F8Ah)                     ; D29Dh
    and 004h                          ; D2A0h
    ld a,(08F88h)                     ; D2A2h
    jp nz,0D297h                      ; D2A5h
    ld a,(08F86h)                     ; D2A8h
    ld (08F82h),a                     ; D2ABh
    ld h,c                            ; D2AEh
    jp 0D247h                         ; D2AFh
    ld a,(08F82h)                     ; D2B2h
    add a,004h                        ; D2B5h
    and 03Ch                          ; D2B7h
    ld l,a                            ; D2B9h
    ld a,(08F88h)                     ; D2BAh
    cp l                              ; D2BDh
    jp c,0D297h                       ; D2BEh
    jp 0D296h                         ; D2C1h
    ld hl,(08F86h)                    ; D2C4h
    ld (08F82h),hl                    ; D2C7h
    jp 0D110h                         ; D2CAh
    ld a,(08F86h)                     ; D2CDh
    ld (08F82h),a                     ; D2D0h
    jp 0D110h                         ; D2D3h
    ld a,(08F83h)                     ; D2D6h
    ld c,a                            ; D2D9h
    ld a,(08F8Bh)                     ; D2DAh
    add a,c                           ; D2DDh
    ld l,a                            ; D2DEh
    ld a,(08F89h)                     ; D2DFh
    jp c,0D3D9h                       ; D2E2h
    cp l                              ; D2E5h
    jp c,0D3D9h                       ; D2E6h
    ld a,l                            ; D2E9h
    jp 0D241h                         ; D2EAh
    ld l,000h                         ; D2EDh
    ld a,(08F8Bh)                     ; D2EFh
    dec a                             ; D2F2h
    ld h,a                            ; D2F3h
    ld (08F86h),hl                    ; D2F4h
    ld hl,0FF2Fh                      ; D2F7h
    ld (08F88h),hl                    ; D2FAh
    ld a,h                            ; D2FDh
    jp 0D35Bh                         ; D2FEh
    ld hl,0D63Dh                      ; D301h
    jp 0D116h                         ; D304h
    ld a,(08F8Ah)                     ; D307h
    and 07Fh                          ; D30Ah
    jp 0D20Bh                         ; D30Ch
    ld a,(08F8Ah)                     ; D30Fh
    or 080h                           ; D312h
    jp 0D20Bh                         ; D314h
    ld hl,0D31Dh                      ; D317h
    jp 0D116h                         ; D31Ah
    ld hl,(08FA0h)                    ; D31Dh
    ld (hl),a                         ; D320h
    ld (08F8Ch),a                     ; D321h
    jp 0D113h                         ; D324h
    ld hl,0D32Dh                      ; D327h
    jp 0D116h                         ; D32Ah
    cp 02Eh                           ; D32Dh
    jp c,0D334h                       ; D32Fh
    ld a,02Eh                         ; D332h
    ld (08F86h),a                     ; D334h
    ld hl,0D33Dh                      ; D337h
    jp 0D116h                         ; D33Ah
    ld a,(08F8Bh)                     ; D33Dh
    dec a                             ; D340h
    add a,c                           ; D341h
    ld (08F87h),a                     ; D342h
    ld hl,0D34Bh                      ; D345h
    jp 0D116h                         ; D348h
    cp 02Fh                           ; D34Bh
    jp c,0D352h                       ; D34Dh
    ld a,02Fh                         ; D350h
    ld (08F88h),a                     ; D352h
    ld hl,0D35Bh                      ; D355h
    jp 0D116h                         ; D358h
    ld (08F89h),a                     ; D35Bh
    ld b,a                            ; D35Eh
    ld a,(08F8Bh)                     ; D35Fh
    ld c,a                            ; D362h
    ld a,(08F87h)                     ; D363h
    cp b                              ; D366h
    jp nc,0D501h                      ; D367h
    cp b                              ; D36Ah
    jp z,0D2C4h                       ; D36Bh
    jp nc,0D379h                      ; D36Eh
    add a,c                           ; D371h
    jp nc,0D36Ah                      ; D372h
    sub c                             ; D375h
    jp 0D35Bh                         ; D376h
    ld e,a                            ; D379h
    ld a,c                            ; D37Ah
    cp 008h                           ; D37Bh
    ld a,e                            ; D37Dh
    jp z,0D35Bh                       ; D37Eh
    jp 0D375h                         ; D381h
    inc b                             ; D384h
    daa                               ; D385h
    nop                               ; D386h
    sub b                             ; D387h
    inc b                             ; D388h
    daa                               ; D389h
    inc l                             ; D38Ah
    push hl                           ; D38Bh
    nop                               ; D38Ch
    ld a,(bc)                         ; D38Dh
    rrca                              ; D38Eh
    nop                               ; D38Fh
    ret c                             ; D390h
    nop                               ; D391h
    DB 0D9h                           ; D392h
    nop                               ; D393h
    jp c,0DF00h                       ; D394h
    nop                               ; D397h
    call c,0D900h                     ; D398h
    nop                               ; D39Bh
    in a,(000h)                       ; D39Ch
    sbc a,0C3h                        ; D39Eh
    xor b                             ; D3A0h
    pop de                            ; D3A1h
    nop                               ; D3A2h
    ret p                             ; D3A3h
    jp 0D16Dh                         ; D3A4h
    jp 0D76Ah                         ; D3A7h
    jp 0D775h                         ; D3AAh
    nop                               ; D3ADh
    DB 010h,001h                      ; D3AEh
    nop                               ; D3B0h
    nop                               ; D3B1h
    nop                               ; D3B2h
    nop                               ; D3B3h
    push hl                           ; D3B4h
    ld hl,(08F82h)                    ; D3B5h
    ld a,090h                         ; D3B8h
    add a,l                           ; D3BAh
    ld l,h                            ; D3BBh
    ld h,a                            ; D3BCh
    ld (08F84h),hl                    ; D3BDh
    pop hl                            ; D3C0h
    ret                               ; D3C1h
    ld a,(08F86h)                     ; D3C2h
    ld (08F82h),a                     ; D3C5h
    jp 0D2D6h                         ; D3C8h
    push hl                           ; D3CBh
    push de                           ; D3CCh
    push bc                           ; D3CDh
    ld a,(de)                         ; D3CEh
    ld (hl),a                         ; D3CFh
    inc de                            ; D3D0h
    inc hl                            ; D3D1h
    dec b                             ; D3D2h
    jp nz,0D3CEh                      ; D3D3h
    jp 0D11Eh                         ; D3D6h
    ld a,(08F8Ah)                     ; D3D9h
    rrca                              ; D3DCh
    jp c,0D2C4h                       ; D3DDh
    call 0D42Ah                       ; D3E0h
    call 0D3CBh                       ; D3E3h
    inc h                             ; D3E6h
    inc d                             ; D3E7h
    dec c                             ; D3E8h
    jp nz,0D3E3h                      ; D3E9h
    ld hl,(08F82h)                    ; D3ECh
    push hl                           ; D3EFh
    ld hl,(08F88h)                    ; D3F0h
    ld (08F82h),hl                    ; D3F3h
    ld a,(08F86h)                     ; D3F6h
    ld (08F82h),a                     ; D3F9h
    call 0D3B4h                       ; D3FCh
    call 0D46Ah                       ; D3FFh
    pop hl                            ; D402h
    ld (08F82h),hl                    ; D403h
    jp 0D110h                         ; D406h
    call 0D42Ah                       ; D409h
    ld a,l                            ; D40Ch
    add a,b                           ; D40Dh
    dec a                             ; D40Eh
    ld l,a                            ; D40Fh
    ld a,e                            ; D410h
    add a,b                           ; D411h
    dec a                             ; D412h
    ld e,a                            ; D413h
    call 0D456h                       ; D414h
    inc h                             ; D417h
    inc d                             ; D418h
    dec c                             ; D419h
    jp nz,0D414h                      ; D41Ah
    ld hl,(08F82h)                    ; D41Dh
    push hl                           ; D420h
    ld hl,(08F86h)                    ; D421h
    ld (08F82h),hl                    ; D424h
    jp 0D3FCh                         ; D427h
    ld a,(08F8Bh)                     ; D42Ah
    ld c,a                            ; D42Dh
    ld a,(08F87h)                     ; D42Eh
    inc a                             ; D431h
    ld e,a                            ; D432h
    sub c                             ; D433h
    jp nc,0D43Fh                      ; D434h
    ld a,c                            ; D437h
    dec a                             ; D438h
    ld (08F87h),a                     ; D439h
    jp 0D42Ah                         ; D43Ch
    ld l,a                            ; D43Fh
    ld a,(08F89h)                     ; D440h
    sub e                             ; D443h
    inc a                             ; D444h
    ld b,a                            ; D445h
    ld a,(08F86h)                     ; D446h
    ld h,a                            ; D449h
    ld a,(08F88h)                     ; D44Ah
    sub h                             ; D44Dh
    inc a                             ; D44Eh
    ld c,a                            ; D44Fh
    ld a,h                            ; D450h
    add a,090h                        ; D451h
    ld h,a                            ; D453h
    ld d,h                            ; D454h
    ret                               ; D455h
    push hl                           ; D456h
    push de                           ; D457h
    push bc                           ; D458h
    ld a,(hl)                         ; D459h
    ld (de),a                         ; D45Ah
    dec l                             ; D45Bh
    dec e                             ; D45Ch
    dec b                             ; D45Dh
    jp nz,0D459h                      ; D45Eh
    jp 0D11Eh                         ; D461h
    call 0D46Ah                       ; D464h
    jp 0D110h                         ; D467h
    ld hl,(08F84h)                    ; D46Ah
    ld a,(08F88h)                     ; D46Dh
    add a,090h                        ; D470h
    sub h                             ; D472h
    inc a                             ; D473h
    ld b,a                            ; D474h
    ld e,b                            ; D475h
    ld d,h                            ; D476h
    ld a,(08F8Bh)                     ; D477h
    ld c,a                            ; D47Ah
    ld a,(08F8Ah)                     ; D47Bh
    or a                              ; D47Eh
    ld a,0FFh                         ; D47Fh
    jp m,0D485h                       ; D481h
    cpl                               ; D484h
    ld (hl),a                         ; D485h
    inc h                             ; D486h
    dec b                             ; D487h
    jp nz,0D485h                      ; D488h
    ld b,e                            ; D48Bh
    ld h,d                            ; D48Ch
    dec l                             ; D48Dh
    dec c                             ; D48Eh
    jp nz,0D485h                      ; D48Fh
    ret                               ; D492h
    call 0D499h                       ; D493h
    jp 0D110h                         ; D496h
    call 0D3B4h                       ; D499h
    call 0D46Ah                       ; D49Ch
    ld hl,(08F84h)                    ; D49Fh
    ld a,(08F86h)                     ; D4A2h
    add a,090h                        ; D4A5h
    ld h,a                            ; D4A7h
    inc l                             ; D4A8h
    ld a,(08F89h)                     ; D4A9h
    ld c,a                            ; D4ACh
    ld a,(08F88h)                     ; D4ADh
    add a,090h                        ; D4B0h
    ld d,a                            ; D4B2h
    ld a,(08F8Ah)                     ; D4B3h
    or a                              ; D4B6h
    ld a,0FFh                         ; D4B7h
    jp m,0D4BDh                       ; D4B9h
    cpl                               ; D4BCh
    ld b,a                            ; D4BDh
    call 0D4C8h                       ; D4BEh
    ld a,h                            ; D4C1h
    cp d                              ; D4C2h
    ret z                             ; D4C3h
    inc h                             ; D4C4h
    jp 0D4BEh                         ; D4C5h
    push hl                           ; D4C8h
    push bc                           ; D4C9h
    ld a,c                            ; D4CAh
    ld (hl),b                         ; D4CBh
    inc l                             ; D4CCh
    jp z,0D4D4h                       ; D4CDh
    cp l                              ; D4D0h
    jp nc,0D4CBh                      ; D4D1h
    pop bc                            ; D4D4h
    pop hl                            ; D4D5h
    ret                               ; D4D6h
    ld hl,(08F86h)                    ; D4D7h
    ld (08F82h),hl                    ; D4DAh
    call 0D499h                       ; D4DDh
    jp 0D110h                         ; D4E0h
    call 0C170h                       ; D4E3h
    jp 0D113h                         ; D4E6h
    ld de,08F82h                      ; D4E9h
    ld hl,08FB2h                      ; D4ECh
    ld b,01Eh                         ; D4EFh
    call 0D3CBh                       ; D4F1h
    ld hl,(08FA0h)                    ; D4F4h
    ld a,(08F8Ch)                     ; D4F7h
    ld (hl),a                         ; D4FAh
    ld a,(08F8Ah)                     ; D4FBh
    jp 0D20Bh                         ; D4FEh
    ld de,08FB2h                      ; D501h
    ld hl,08F82h                      ; D504h
    jp 0D4EFh                         ; D507h
    ld hl,0D51Fh                      ; D50Ah
    jp 0D116h                         ; D50Dh
    ld hl,0D53Eh                      ; D510h
    jp 0D116h                         ; D513h
    sub 020h                          ; D516h
    ld c,a                            ; D518h
    call 0D547h                       ; D519h
    jp 0D110h                         ; D51Ch
    ld hl,0D516h                      ; D51Fh
    sub 020h                          ; D522h
    rlca                              ; D524h
    ld b,a                            ; D525h
    rlca                              ; D526h
    rlca                              ; D527h
    add a,b                           ; D528h
    ld c,a                            ; D529h
    ld a,(08F8Bh)                     ; D52Ah
    cp 00Ah                           ; D52Dh
    jp z,0D541h                       ; D52Fh
    ld a,c                            ; D532h
    sub b                             ; D533h
    ld c,a                            ; D534h
    jp 0D541h                         ; D535h
    call 0D547h                       ; D538h
    jp 0D110h                         ; D53Bh
    ld hl,0D538h                      ; D53Eh
    call 0D558h                       ; D541h
    jp 0D116h                         ; D544h
    ld a,(08F86h)                     ; D547h
    add a,c                           ; D54Ah
    ld c,a                            ; D54Bh
    ld a,(08F88h)                     ; D54Ch
    cp c                              ; D54Fh
    jp c,0D554h                       ; D550h
    ld a,c                            ; D553h
    ld (08F82h),a                     ; D554h
    ret                               ; D557h
    ld a,(08F87h)                     ; D558h
    add a,c                           ; D55Bh
    ld c,a                            ; D55Ch
    ld a,(08F89h)                     ; D55Dh
    jp c,0D568h                       ; D560h
    cp c                              ; D563h
    jp c,0D568h                       ; D564h
    ld a,c                            ; D567h
    ld (08F83h),a                     ; D568h
    ret                               ; D56Bh
    ld hl,0D572h                      ; D56Ch
    jp 0D116h                         ; D56Fh
    ld a,(08FABh)                     ; D572h
    ld hl,08F8Dh                      ; D575h
    ld e,a                            ; D578h
    inc a                             ; D579h
    ld d,000h                         ; D57Ah
    add hl,de                         ; D57Ch
    ld (hl),c                         ; D57Dh
    cp 010h                           ; D57Eh
    jp z,0D113h                       ; D580h
    ld (08FABh),a                     ; D583h
    jp 0D11Dh                         ; D586h
    ld de,0D5A9h                      ; D589h
    jp 0D59Eh                         ; D58Ch
    ld de,0D5B9h                      ; D58Fh
    jp 0D59Eh                         ; D592h
    ld de,0D5C9h                      ; D595h
    jp 0D59Eh                         ; D598h
    ld de,0D5D9h                      ; D59Bh
    ld hl,08F8Dh                      ; D59Eh
    ld b,010h                         ; D5A1h
    call 0D3CBh                       ; D5A3h
    jp 0D113h                         ; D5A6h
    nop                               ; D5A9h
    ret c                             ; D5AAh
    nop                               ; D5ABh
    DB 0D9h                           ; D5ACh
    nop                               ; D5ADh
    jp c,0DB00h                       ; D5AEh
    nop                               ; D5B1h
    call c,0D900h                     ; D5B2h
    nop                               ; D5B5h
    sbc a,000h                        ; D5B6h
    rst 018h                          ; D5B8h
    nop                               ; D5B9h
    ret c                             ; D5BAh
    nop                               ; D5BBh
    DB 0D9h                           ; D5BCh
    nop                               ; D5BDh
    sbc a,000h                        ; D5BEh
    rst 018h                          ; D5C0h
    nop                               ; D5C1h
    call c,0D900h                     ; D5C2h
    nop                               ; D5C5h
    jp c,0DB00h                       ; D5C6h
    nop                               ; D5C9h
    ret c                             ; D5CAh
    nop                               ; D5CBh
    DB 0D9h                           ; D5CCh
    nop                               ; D5CDh
    jp c,0DF00h                       ; D5CEh
    nop                               ; D5D1h
    call c,0D900h                     ; D5D2h
    nop                               ; D5D5h
    in a,(000h)                       ; D5D6h
    sbc a,000h                        ; D5D8h
    ret c                             ; D5DAh
    nop                               ; D5DBh
    DB 0D9h                           ; D5DCh
    nop                               ; D5DDh
    jp c,0DB00h                       ; D5DEh
    nop                               ; D5E1h
    call c,0DD00h                     ; D5E2h
    nop                               ; D5E5h
    sbc a,000h                        ; D5E6h
    rst 018h                          ; D5E8h
    call 0D42Ah                       ; D5E9h
    dec c                             ; D5ECh
    ld a,c                            ; D5EDh
    add a,h                           ; D5EEh
    ld h,a                            ; D5EFh
    ld e,l                            ; D5F0h
    ld d,h                            ; D5F1h
    dec d                             ; D5F2h
    ld a,(08F8Bh)                     ; D5F3h
    add a,b                           ; D5F6h
    ld b,a                            ; D5F7h
    call 0D3CBh                       ; D5F8h
    dec d                             ; D5FBh
    dec h                             ; D5FCh
    dec c                             ; D5FDh
    jp nz,0D5F8h                      ; D5FEh
    ld a,(08F89h)                     ; D601h
    ld c,a                            ; D604h
    ld a,(08F8Ah)                     ; D605h
    or a                              ; D608h
    ld a,0FFh                         ; D609h
    jp m,0D60Fh                       ; D60Bh
    cpl                               ; D60Eh
    ld b,a                            ; D60Fh
    call 0D4C8h                       ; D610h
    jp 0D110h                         ; D613h
    call 0D42Ah                       ; D616h
    dec c                             ; D619h
    ld a,(08F8Bh)                     ; D61Ah
    add a,b                           ; D61Dh
    ld b,a                            ; D61Eh
    ld e,l                            ; D61Fh
    inc d                             ; D620h
    call 0D3CBh                       ; D621h
    inc h                             ; D624h
    inc d                             ; D625h
    dec c                             ; D626h
    jp nz,0D621h                      ; D627h
    jp 0D601h                         ; D62Ah
    ld a,00Ah                         ; D62Dh
    ld (08F8Bh),a                     ; D62Fh
    ld a,(08F89h)                     ; D632h
    jp 0D35Bh                         ; D635h
    ld a,008h                         ; D638h
    jp 0D62Fh                         ; D63Ah
    cp 02Bh                           ; D63Dh
    jp c,0D113h                       ; D63Fh
    cp 041h                           ; D642h
    jp nz,0D64Ch                      ; D644h
    ld a,048h                         ; D647h
    jp 0D706h                         ; D649h
    cp 048h                           ; D64Ch
    jp nz,0D656h                      ; D64Eh
    ld a,049h                         ; D651h
    jp 0D706h                         ; D653h
    cp 043h                           ; D656h
    jp nc,0D706h                      ; D658h
    cp 030h                           ; D65Bh
    jp nc,0D730h                      ; D65Dh
    cp 02Eh                           ; D660h
    jp z,0D67Ch                       ; D662h
    cp 02Dh                           ; D665h
    jp z,0D677h                       ; D667h
    cp 02Bh                           ; D66Ah
    jp nz,0D11Dh                      ; D66Ch
    ld a,001h                         ; D66Fh
    ld (08FAEh),a                     ; D671h
    jp 0D11Dh                         ; D674h
    ld a,0FFh                         ; D677h
    jp 0D671h                         ; D679h
    ld a,(08FACh)                     ; D67Ch
    ld c,a                            ; D67Fh
    rrca                              ; D680h
    add a,c                           ; D681h
    ld (08FACh),a                     ; D682h
    jp 0D11Dh                         ; D685h
    rlca                              ; D688h
    ld c,a                            ; D689h
    ld a,(08FAEh)                     ; D68Ah
    add a,c                           ; D68Dh
    ld c,a                            ; D68Eh
    ld b,000h                         ; D68Fh
    ld hl,0D75Ch                      ; D691h
    add hl,bc                         ; D694h
    ld e,(hl)                         ; D695h
    ld d,000h                         ; D696h
    ex de,hl                          ; D698h
    ld a,(08FADh)                     ; D699h
    dec a                             ; D69Ch
    jp z,0D6A4h                       ; D69Dh
    add hl,hl                         ; D6A0h
    jp 0D69Ch                         ; D6A1h
    ex de,hl                          ; D6A4h
    call 0D6AFh                       ; D6A5h
    xor a                             ; D6A8h
    ld (08FAEh),a                     ; D6A9h
    jp 0D11Dh                         ; D6ACh
    ld hl,(08FAFh)                    ; D6AFh
    ld a,(08FACh)                     ; D6B2h
    ld b,a                            ; D6B5h
    ld c,000h                         ; D6B6h
    add hl,de                         ; D6B8h
    ld a,h                            ; D6B9h
    and 020h                          ; D6BAh
    ld (0FF02h),a                     ; D6BCh
    dec bc                            ; D6BFh
    ld a,b                            ; D6C0h
    or c                              ; D6C1h
    jp nz,0D6B8h                      ; D6C2h
    ld (08FAFh),hl                    ; D6C5h
    ret                               ; D6C8h
    rlca                              ; D6C9h
    ld c,a                            ; D6CAh
    ld b,000h                         ; D6CBh
    ld hl,0D000h                      ; D6CDh
    add hl,bc                         ; D6D0h
    ld e,(hl)                         ; D6D1h
    inc hl                            ; D6D2h
    ld d,(hl)                         ; D6D3h
    ex de,hl                          ; D6D4h
    call 0D6DBh                       ; D6D5h
    jp 0D11Dh                         ; D6D8h
    ld a,(08FACh)                     ; D6DBh
    ld d,a                            ; D6DEh
    ld e,000h                         ; D6DFh
    dec hl                            ; D6E1h
    ld b,001h                         ; D6E2h
    dec b                             ; D6E4h
    jp nz,0D6FBh                      ; D6E5h
    inc hl                            ; D6E8h
    ld b,008h                         ; D6E9h
    ld c,(hl)                         ; D6EBh
    ld a,c                            ; D6ECh
    rlca                              ; D6EDh
    ld c,a                            ; D6EEh
    and 020h                          ; D6EFh
    ld (0FF02h),a                     ; D6F1h
    dec de                            ; D6F4h
    ld a,d                            ; D6F5h
    or e                              ; D6F6h
    jp nz,0D6E4h                      ; D6F7h
    ret                               ; D6FAh
    ld a,a                            ; D6FBh
    nop                               ; D6FCh
    jp 0D6ECh                         ; D6FDh
    ld de,00000h                      ; D700h
    jp 0D6A5h                         ; D703h
    and 05Fh                          ; D706h
    cp 054h                           ; D708h
    jp z,0D72Bh                       ; D70Ah
    cp 052h                           ; D70Dh
    jp z,0D700h                       ; D70Fh
    cp 04Fh                           ; D712h
    jp z,0D723h                       ; D714h
    sub 043h                          ; D717h
    cp 007h                           ; D719h
    jp c,0D688h                       ; D71Bh
    sub 007h                          ; D71Eh
    jp 0D6C9h                         ; D720h
    ld a,001h                         ; D723h
    ld (08FB1h),a                     ; D725h
    jp 0D11Dh                         ; D728h
    ld a,080h                         ; D72Bh
    jp 0D725h                         ; D72Dh
    and 00Fh                          ; D730h
    ld c,a                            ; D732h
    ld a,(08FB1h)                     ; D733h
    or a                              ; D736h
    jp nz,0D74Ch                      ; D737h
    ld a,(08FACh)                     ; D73Ah
    rlca                              ; D73Dh
    rlca                              ; D73Eh
    rlca                              ; D73Fh
    rlca                              ; D740h
    or c                              ; D741h
    ld (08FACh),a                     ; D742h
    xor a                             ; D745h
    ld (08FB1h),a                     ; D746h
    jp 0D11Dh                         ; D749h
    ld a,c                            ; D74Ch
    jp m,0D756h                       ; D74Dh
    ld (08FADh),a                     ; D750h
    jp 0D745h                         ; D753h
    ld (08FACh),a                     ; D756h
    jp 0D745h                         ; D759h
    add a,b                           ; D75Ch
    add a,a                           ; D75Dh
    sub b                             ; D75Eh
    sbc a,b                           ; D75Fh
    and c                             ; D760h
    and c                             ; D761h
    xor e                             ; D762h
    or l                              ; D763h
    cp a                              ; D764h
    DB 0CBh,0D7h                      ; D765h
    call po,0FFF2h                    ; D767h
    ld a,(08F8Ah)                     ; D76Ah
    and 010h                          ; D76Dh
    jp z,0CE2Fh                       ; D76Fh
    jp 0C337h                         ; D772h
    push af                           ; D775h
    ld a,(08F8Ah)                     ; D776h
    and 010h                          ; D779h
    jp nz,0D782h                      ; D77Bh
    pop af                            ; D77Eh
    jp 0D0F3h                         ; D77Fh
    ld a,c                            ; D782h
    cp 007h                           ; D783h
    call z,0C170h                     ; D785h
    cp 020h                           ; D788h
    jp c,0D799h                       ; D78Ah
    push bc                           ; D78Dh
    ld c,07Fh                         ; D78Eh
    call 0D79Eh                       ; D790h
    ld c,008h                         ; D793h
    call 0D79Eh                       ; D795h
    pop bc                            ; D798h
    call 0D79Eh                       ; D799h
    pop af                            ; D79Ch
    ret                               ; D79Dh
    ld a,(08FFCh)                     ; D79Eh
    cp 0F8h                           ; D7A1h
    call nc,0D7AAh                    ; D7A3h
    call 0C037h                       ; D7A6h
    ret                               ; D7A9h
    ld a,c                            ; D7AAh
    cp 00Ah                           ; D7ABh
    jp z,0D7BBh                       ; D7ADh
    cp 01Ah                           ; D7B0h
    jp z,0D7BBh                       ; D7B2h
    ld a,(08FFDh)                     ; D7B5h
    cp 0BEh                           ; D7B8h
    ret c                             ; D7BAh
    push hl                           ; D7BBh
    push de                           ; D7BCh
    push bc                           ; D7BDh
    ld hl,09000h                      ; D7BEh
    ld de,0900Ah                      ; D7C1h
    ld b,030h                         ; D7C4h
    ld c,078h                         ; D7C6h
    ld a,(de)                         ; D7C8h
    ld (hl),a                         ; D7C9h
    inc hl                            ; D7CAh
    inc de                            ; D7CBh
    ld a,(de)                         ; D7CCh
    ld (hl),a                         ; D7CDh
    inc hl                            ; D7CEh
    inc de                            ; D7CFh
    dec c                             ; D7D0h
    jp nz,0D7C8h                      ; D7D1h
    ld a,(08FFAh)                     ; D7D4h
    ld c,010h                         ; D7D7h
    ld (hl),a                         ; D7D9h
    inc hl                            ; D7DAh
    inc de                            ; D7DBh
    dec c                             ; D7DCh
    jp nz,0D7D9h                      ; D7DDh
    dec b                             ; D7E0h
    jp nz,0D7C6h                      ; D7E1h
    ld hl,000F8h                      ; D7E4h
    ld (08FFCh),hl                    ; D7E7h
    pop bc                            ; D7EAh
    pop de                            ; D7EBh
    pop hl                            ; D7ECh
    ld a,c                            ; D7EDh
    cp 00Ah                           ; D7EEh
    ret nz                            ; D7F0h
    ld c,01Ah                         ; D7F1h
    ret                               ; D7F3h
    rst 038h                          ; D7F4h
    rst 038h                          ; D7F5h
    rst 038h                          ; D7F6h
    rst 038h                          ; D7F7h
    rst 038h                          ; D7F8h
    rst 038h                          ; D7F9h
    rst 038h                          ; D7FAh
    rst 038h                          ; D7FBh
    rst 038h                          ; D7FCh
    rst 038h                          ; D7FDh
    rst 038h                          ; D7FEh
    rst 038h                          ; D7FFh
font8:
    DB 000h                           ; D800h
    DB 000h                           ; D801h
    DB 000h                           ; D802h
    DB 000h                           ; D803h
    DB 000h                           ; D804h
    DB 000h                           ; D805h
    DB 000h                           ; D806h
    DB 000h                           ; D807h
    DB 07Eh                           ; D808h
    DB 081h                           ; D809h
    DB 0A5h                           ; D80Ah
    DB 081h                           ; D80Bh
    DB 0BDh                           ; D80Ch
    DB 099h                           ; D80Dh
    DB 081h                           ; D80Eh
    DB 07Eh                           ; D80Fh
    DB 07Eh                           ; D810h
    DB 0FFh                           ; D811h
    DB 0DBh,0FFh                      ; D812h
    DB 0C3h,0E7h,0FFh                 ; D814h
    DB 07Eh                           ; D817h
    DB 06Ch                           ; D818h
    DB 0FEh,0FEh                      ; D819h
    DB 0FEh,07Ch                      ; D81Bh
    DB 038h,010h                      ; D81Dh
    DB 000h                           ; D81Fh
    DB 010h,038h                      ; D820h
    DB 07Ch                           ; D822h
    DB 0FEh,07Ch                      ; D823h
    DB 038h,010h                      ; D825h
    DB 000h                           ; D827h
    DB 038h,07Ch                      ; D828h
    DB 038h,0FEh                      ; D82Ah
    DB 0FEh,0D6h                      ; D82Ch
    DB 010h,038h                      ; D82Eh
    DB 010h,010h                      ; D830h
    DB 038h,07Ch                      ; D832h
    DB 0FEh,07Ch                      ; D834h
    DB 010h,038h                      ; D836h
    DB 000h                           ; D838h
    DB 000h                           ; D839h
    DB 018h,03Ch                      ; D83Ah
    DB 03Ch                           ; D83Ch
    DB 018h,000h                      ; D83Dh
    DB 000h                           ; D83Fh
    DB 0FFh                           ; D840h
    DB 0FFh                           ; D841h
    DB 0E7h                           ; D842h
    DB 0C3h,0C3h,0E7h                 ; D843h
    DB 0FFh                           ; D846h
    DB 0FFh                           ; D847h
    DB 000h                           ; D848h
    DB 03Ch                           ; D849h
    DB 066h                           ; D84Ah
    DB 042h                           ; D84Bh
    DB 042h                           ; D84Ch
    DB 066h                           ; D84Dh
    DB 03Ch                           ; D84Eh
    DB 000h                           ; D84Fh
    DB 0FFh                           ; D850h
    DB 0C3h,099h,0BDh                 ; D851h
    DB 0BDh                           ; D854h
    DB 099h                           ; D855h
    DB 0C3h,0FFh,00Fh                 ; D856h
    DB 007h                           ; D859h
    DB 00Fh                           ; D85Ah
    DB 07Dh                           ; D85Bh
    DB 0CCh,0CCh,0CCh                 ; D85Ch
    DB 078h                           ; D85Fh
    DB 03Ch                           ; D860h
    DB 066h                           ; D861h
    DB 066h                           ; D862h
    DB 066h                           ; D863h
    DB 03Ch                           ; D864h
    DB 018h,07Eh                      ; D865h
    DB 018h,03Fh                      ; D867h
    DB 033h                           ; D869h
    DB 03Fh                           ; D86Ah
    DB 030h,030h                      ; D86Bh
    DB 070h                           ; D86Dh
    DB 0F0h                           ; D86Eh
    DB 0E0h                           ; D86Fh
    DB 07Fh                           ; D870h
    DB 063h                           ; D871h
    DB 07Fh                           ; D872h
    DB 063h                           ; D873h
    DB 063h                           ; D874h
    DB 067h                           ; D875h
    DB 0E6h,0C0h                      ; D876h
    DB 018h,0DBh                      ; D878h
    DB 03Ch                           ; D87Ah
    DB 0E7h                           ; D87Bh
    DB 0E7h                           ; D87Ch
    DB 03Ch                           ; D87Dh
    DB 0DBh,018h                      ; D87Eh
    DB 080h                           ; D880h
    DB 0E0h                           ; D881h
    DB 0F8h                           ; D882h
    DB 0FEh,0F8h                      ; D883h
    DB 0E0h                           ; D885h
    DB 080h                           ; D886h
    DB 000h                           ; D887h
    DB 002h                           ; D888h
    DB 00Eh,03Eh                      ; D889h
    DB 0FEh,03Eh                      ; D88Bh
    DB 00Eh,002h                      ; D88Dh
    DB 000h                           ; D88Fh
    DB 018h,03Ch                      ; D890h
    DB 07Eh                           ; D892h
    DB 018h,018h                      ; D893h
    DB 07Eh                           ; D895h
    DB 03Ch                           ; D896h
    DB 018h,066h                      ; D897h
    DB 066h                           ; D899h
    DB 066h                           ; D89Ah
    DB 066h                           ; D89Bh
    DB 066h                           ; D89Ch
    DB 000h                           ; D89Dh
    DB 066h                           ; D89Eh
    DB 000h                           ; D89Fh
    DB 07Fh                           ; D8A0h
    DB 0DBh,0DBh                      ; D8A1h
    DB 07Bh                           ; D8A3h
    DB 01Bh                           ; D8A4h
    DB 01Bh                           ; D8A5h
    DB 01Bh                           ; D8A6h
    DB 000h                           ; D8A7h
    DB 03Eh,063h                      ; D8A8h
    DB 038h,06Ch                      ; D8AAh
    DB 06Ch                           ; D8ACh
    DB 038h,0CCh                      ; D8ADh
    DB 078h                           ; D8AFh
    DB 000h                           ; D8B0h
    DB 000h                           ; D8B1h
    DB 000h                           ; D8B2h
    DB 000h                           ; D8B3h
    DB 07Eh                           ; D8B4h
    DB 07Eh                           ; D8B5h
    DB 07Eh                           ; D8B6h
    DB 000h                           ; D8B7h
    DB 018h,03Ch                      ; D8B8h
    DB 07Eh                           ; D8BAh
    DB 018h,07Eh                      ; D8BBh
    DB 03Ch                           ; D8BDh
    DB 018h,0FFh                      ; D8BEh
    DB 018h,03Ch                      ; D8C0h
    DB 07Eh                           ; D8C2h
    DB 018h,018h                      ; D8C3h
    DB 018h,018h                      ; D8C5h
    DB 000h                           ; D8C7h
    DB 018h,018h                      ; D8C8h
    DB 018h,018h                      ; D8CAh
    DB 07Eh                           ; D8CCh
    DB 03Ch                           ; D8CDh
    DB 018h,000h                      ; D8CEh
    DB 000h                           ; D8D0h
    DB 018h,00Ch                      ; D8D1h
    DB 0FEh,00Ch                      ; D8D3h
    DB 018h,000h                      ; D8D5h
    DB 000h                           ; D8D7h
    DB 000h                           ; D8D8h
    DB 030h,060h                      ; D8D9h
    DB 0FEh,060h                      ; D8DBh
    DB 030h,000h                      ; D8DDh
    DB 000h                           ; D8DFh
    DB 000h                           ; D8E0h
    DB 000h                           ; D8E1h
    DB 0C0h                           ; D8E2h
    DB 0C0h                           ; D8E3h
    DB 0C0h                           ; D8E4h
    DB 0FEh,000h                      ; D8E5h
    DB 000h                           ; D8E7h
    DB 000h                           ; D8E8h
    DB 024h                           ; D8E9h
    DB 066h                           ; D8EAh
    DB 0FFh                           ; D8EBh
    DB 066h                           ; D8ECh
    DB 024h                           ; D8EDh
    DB 000h                           ; D8EEh
    DB 000h                           ; D8EFh
    DB 000h                           ; D8F0h
    DB 018h,03Ch                      ; D8F1h
    DB 07Eh                           ; D8F3h
    DB 0FFh                           ; D8F4h
    DB 0FFh                           ; D8F5h
    DB 000h                           ; D8F6h
    DB 000h                           ; D8F7h
    DB 000h                           ; D8F8h
    DB 0FFh                           ; D8F9h
    DB 0FFh                           ; D8FAh
    DB 07Eh                           ; D8FBh
    DB 03Ch                           ; D8FCh
    DB 018h,000h                      ; D8FDh
    DB 000h                           ; D8FFh
    DB 000h                           ; D900h
    DB 000h                           ; D901h
    DB 000h                           ; D902h
    DB 000h                           ; D903h
    DB 000h                           ; D904h
    DB 000h                           ; D905h
    DB 000h                           ; D906h
    DB 000h                           ; D907h
    DB 030h,078h                      ; D908h
    DB 078h                           ; D90Ah
    DB 030h,030h                      ; D90Bh
    DB 000h                           ; D90Dh
    DB 030h,000h                      ; D90Eh
    DB 06Ch                           ; D910h
    DB 06Ch                           ; D911h
    DB 06Ch                           ; D912h
    DB 000h                           ; D913h
    DB 000h                           ; D914h
    DB 000h                           ; D915h
    DB 000h                           ; D916h
    DB 000h                           ; D917h
    DB 06Ch                           ; D918h
    DB 06Ch                           ; D919h
    DB 0FEh,06Ch                      ; D91Ah
    DB 0FEh,06Ch                      ; D91Ch
    DB 06Ch                           ; D91Eh
    DB 000h                           ; D91Fh
    DB 030h,07Ch                      ; D920h
    DB 0C0h                           ; D922h
    DB 078h                           ; D923h
    DB 00Ch                           ; D924h
    DB 0F8h                           ; D925h
    DB 030h,000h                      ; D926h
    DB 000h                           ; D928h
    DB 0C6h,0CCh                      ; D929h
    DB 018h,030h                      ; D92Bh
    DB 066h                           ; D92Dh
    DB 0C6h,000h                      ; D92Eh
    DB 038h,06Ch                      ; D930h
    DB 038h,076h                      ; D932h
    DB 0DCh,0CCh,076h                 ; D934h
    DB 000h                           ; D937h
    DB 060h                           ; D938h
    DB 060h                           ; D939h
    DB 0C0h                           ; D93Ah
    DB 000h                           ; D93Bh
    DB 000h                           ; D93Ch
    DB 000h                           ; D93Dh
    DB 000h                           ; D93Eh
    DB 000h                           ; D93Fh
    DB 018h,030h                      ; D940h
    DB 060h                           ; D942h
    DB 060h                           ; D943h
    DB 060h                           ; D944h
    DB 030h,018h                      ; D945h
    DB 000h                           ; D947h
    DB 060h                           ; D948h
    DB 030h,018h                      ; D949h
    DB 018h,018h                      ; D94Bh
    DB 030h,060h                      ; D94Dh
    DB 000h                           ; D94Fh
    DB 000h                           ; D950h
    DB 066h                           ; D951h
    DB 03Ch                           ; D952h
    DB 0FFh                           ; D953h
    DB 03Ch                           ; D954h
    DB 066h                           ; D955h
    DB 000h                           ; D956h
    DB 000h                           ; D957h
    DB 000h                           ; D958h
    DB 030h,030h                      ; D959h
    DB 0FCh,030h,030h                 ; D95Bh
    DB 000h                           ; D95Eh
    DB 000h                           ; D95Fh
    DB 000h                           ; D960h
    DB 000h                           ; D961h
    DB 000h                           ; D962h
    DB 000h                           ; D963h
    DB 000h                           ; D964h
    DB 030h,030h                      ; D965h
    DB 060h                           ; D967h
    DB 000h                           ; D968h
    DB 000h                           ; D969h
    DB 000h                           ; D96Ah
    DB 0FCh,000h,000h                 ; D96Bh
    DB 000h                           ; D96Eh
    DB 000h                           ; D96Fh
    DB 000h                           ; D970h
    DB 000h                           ; D971h
    DB 000h                           ; D972h
    DB 000h                           ; D973h
    DB 000h                           ; D974h
    DB 030h,030h                      ; D975h
    DB 000h                           ; D977h
    DB 006h,00Ch                      ; D978h
    DB 018h,030h                      ; D97Ah
    DB 060h                           ; D97Ch
    DB 0C0h                           ; D97Dh
    DB 080h                           ; D97Eh
    DB 000h                           ; D97Fh
    DB 07Ch                           ; D980h
    DB 0C6h,0CEh                      ; D981h
    DB 0DEh,0F6h                      ; D983h
    DB 0E6h,07Ch                      ; D985h
    DB 000h                           ; D987h
    DB 030h,070h                      ; D988h
    DB 030h,030h                      ; D98Ah
    DB 030h,030h                      ; D98Ch
    DB 0FCh,000h,078h                 ; D98Eh
    DB 0CCh,00Ch,038h                 ; D991h
    DB 060h                           ; D994h
    DB 0CCh,0FCh,000h                 ; D995h
    DB 078h                           ; D998h
    DB 0CCh,00Ch,038h                 ; D999h
    DB 00Ch                           ; D99Ch
    DB 0CCh,078h,000h                 ; D99Dh
    DB 01Ch                           ; D9A0h
    DB 03Ch                           ; D9A1h
    DB 06Ch                           ; D9A2h
    DB 0CCh,0FEh,00Ch                 ; D9A3h
    DB 01Eh,000h                      ; D9A6h
    DB 0FCh,0C0h,0F8h                 ; D9A8h
    DB 00Ch                           ; D9ABh
    DB 00Ch                           ; D9ACh
    DB 0CCh,078h,000h                 ; D9ADh
    DB 038h,060h                      ; D9B0h
    DB 0C0h                           ; D9B2h
    DB 0F8h                           ; D9B3h
    DB 0CCh,0CCh,078h                 ; D9B4h
    DB 000h                           ; D9B7h
    DB 0FCh,0CCh,00Ch                 ; D9B8h
    DB 018h,030h                      ; D9BBh
    DB 030h,030h                      ; D9BDh
    DB 000h                           ; D9BFh
    DB 078h                           ; D9C0h
    DB 0CCh,0CCh,078h                 ; D9C1h
    DB 0CCh,0CCh,078h                 ; D9C4h
    DB 000h                           ; D9C7h
    DB 078h                           ; D9C8h
    DB 0CCh,0CCh,07Ch                 ; D9C9h
    DB 00Ch                           ; D9CCh
    DB 018h,070h                      ; D9CDh
    DB 000h                           ; D9CFh
    DB 000h                           ; D9D0h
    DB 030h,030h                      ; D9D1h
    DB 000h                           ; D9D3h
    DB 000h                           ; D9D4h
    DB 030h,030h                      ; D9D5h
    DB 000h                           ; D9D7h
    DB 000h                           ; D9D8h
    DB 030h,030h                      ; D9D9h
    DB 000h                           ; D9DBh
    DB 000h                           ; D9DCh
    DB 030h,030h                      ; D9DDh
    DB 060h                           ; D9DFh
    DB 018h,030h                      ; D9E0h
    DB 060h                           ; D9E2h
    DB 0C0h                           ; D9E3h
    DB 060h                           ; D9E4h
    DB 030h,018h                      ; D9E5h
    DB 000h                           ; D9E7h
    DB 000h                           ; D9E8h
    DB 000h                           ; D9E9h
    DB 0FCh,000h,0FCh                 ; D9EAh
    DB 000h                           ; D9EDh
    DB 000h                           ; D9EEh
    DB 000h                           ; D9EFh
    DB 060h                           ; D9F0h
    DB 030h,018h                      ; D9F1h
    DB 00Ch                           ; D9F3h
    DB 018h,030h                      ; D9F4h
    DB 060h                           ; D9F6h
    DB 000h                           ; D9F7h
    DB 078h                           ; D9F8h
    DB 0CCh,00Ch,018h                 ; D9F9h
    DB 030h,000h                      ; D9FCh
    DB 030h,000h                      ; D9FEh
    DB 07Ch                           ; DA00h
    DB 0C6h,0DEh                      ; DA01h
    DB 0DEh,0DEh                      ; DA03h
    DB 0C0h                           ; DA05h
    DB 078h                           ; DA06h
    DB 000h                           ; DA07h
    DB 030h,078h                      ; DA08h
    DB 0CCh,0CCh,0FCh                 ; DA0Ah
    DB 0CCh,0CCh,000h                 ; DA0Dh
    DB 0FCh,066h,066h                 ; DA10h
    DB 07Ch                           ; DA13h
    DB 066h                           ; DA14h
    DB 066h                           ; DA15h
    DB 0FCh,000h,03Ch                 ; DA16h
    DB 066h                           ; DA19h
    DB 0C0h                           ; DA1Ah
    DB 0C0h                           ; DA1Bh
    DB 0C0h                           ; DA1Ch
    DB 066h                           ; DA1Dh
    DB 03Ch                           ; DA1Eh
    DB 000h                           ; DA1Fh
    DB 0F8h                           ; DA20h
    DB 06Ch                           ; DA21h
    DB 066h                           ; DA22h
    DB 066h                           ; DA23h
    DB 066h                           ; DA24h
    DB 06Ch                           ; DA25h
    DB 0F8h                           ; DA26h
    DB 000h                           ; DA27h
    DB 0FEh,062h                      ; DA28h
    DB 068h                           ; DA2Ah
    DB 078h                           ; DA2Bh
    DB 068h                           ; DA2Ch
    DB 062h                           ; DA2Dh
    DB 0FEh,000h                      ; DA2Eh
    DB 0FEh,062h                      ; DA30h
    DB 068h                           ; DA32h
    DB 078h                           ; DA33h
    DB 068h                           ; DA34h
    DB 060h                           ; DA35h
    DB 0F0h                           ; DA36h
    DB 000h                           ; DA37h
    DB 03Ch                           ; DA38h
    DB 066h                           ; DA39h
    DB 0C0h                           ; DA3Ah
    DB 0C0h                           ; DA3Bh
    DB 0CEh,066h                      ; DA3Ch
    DB 03Eh,000h                      ; DA3Eh
    DB 0CCh,0CCh,0CCh                 ; DA40h
    DB 0FCh,0CCh,0CCh                 ; DA43h
    DB 0CCh,000h,078h                 ; DA46h
    DB 030h,030h                      ; DA49h
    DB 030h,030h                      ; DA4Bh
    DB 030h,078h                      ; DA4Dh
    DB 000h                           ; DA4Fh
    DB 01Eh,00Ch                      ; DA50h
    DB 00Ch                           ; DA52h
    DB 00Ch                           ; DA53h
    DB 0CCh,0CCh,078h                 ; DA54h
    DB 000h                           ; DA57h
    DB 0E6h,066h                      ; DA58h
    DB 06Ch                           ; DA5Ah
    DB 078h                           ; DA5Bh
    DB 06Ch                           ; DA5Ch
    DB 066h                           ; DA5Dh
    DB 0E6h,000h                      ; DA5Eh
    DB 0F0h                           ; DA60h
    DB 060h                           ; DA61h
    DB 060h                           ; DA62h
    DB 060h                           ; DA63h
    DB 062h                           ; DA64h
    DB 066h                           ; DA65h
    DB 0FEh,000h                      ; DA66h
    DB 0C6h,0EEh                      ; DA68h
    DB 0FEh,0FEh                      ; DA6Ah
    DB 0D6h,0C6h                      ; DA6Ch
    DB 0C6h,000h                      ; DA6Eh
    DB 0C6h,0E6h                      ; DA70h
    DB 0F6h,0DEh                      ; DA72h
    DB 0CEh,0C6h                      ; DA74h
    DB 0C6h,000h                      ; DA76h
    DB 038h,06Ch                      ; DA78h
    DB 0C6h,0C6h                      ; DA7Ah
    DB 0C6h,06Ch                      ; DA7Ch
    DB 038h,000h                      ; DA7Eh
    DB 0FCh,066h,066h                 ; DA80h
    DB 07Ch                           ; DA83h
    DB 060h                           ; DA84h
    DB 060h                           ; DA85h
    DB 0F0h                           ; DA86h
    DB 000h                           ; DA87h
    DB 078h                           ; DA88h
    DB 0CCh,0CCh,0CCh                 ; DA89h
    DB 0DCh,078h,01Ch                 ; DA8Ch
    DB 000h                           ; DA8Fh
    DB 0FCh,066h,066h                 ; DA90h
    DB 07Ch                           ; DA93h
    DB 06Ch                           ; DA94h
    DB 066h                           ; DA95h
    DB 0E6h,000h                      ; DA96h
    DB 078h                           ; DA98h
    DB 0CCh,060h,030h                 ; DA99h
    DB 018h,0CCh                      ; DA9Ch
    DB 078h                           ; DA9Eh
    DB 000h                           ; DA9Fh
    DB 0FCh,0B4h,030h                 ; DAA0h
    DB 030h,030h                      ; DAA3h
    DB 030h,078h                      ; DAA5h
    DB 000h                           ; DAA7h
    DB 0CCh,0CCh,0CCh                 ; DAA8h
    DB 0CCh,0CCh,0CCh                 ; DAABh
    DB 078h                           ; DAAEh
    DB 000h                           ; DAAFh
    DB 0CCh,0CCh,0CCh                 ; DAB0h
    DB 0CCh,0CCh,078h                 ; DAB3h
    DB 030h,000h                      ; DAB6h
    DB 0C6h,0C6h                      ; DAB8h
    DB 0C6h,0D6h                      ; DABAh
    DB 0FEh,0EEh                      ; DABCh
    DB 0C6h,000h                      ; DABEh
    DB 0C6h,0C6h                      ; DAC0h
    DB 06Ch                           ; DAC2h
    DB 038h,038h                      ; DAC3h
    DB 06Ch                           ; DAC5h
    DB 0C6h,000h                      ; DAC6h
    DB 0CCh,0CCh,0CCh                 ; DAC8h
    DB 078h                           ; DACBh
    DB 030h,030h                      ; DACCh
    DB 078h                           ; DACEh
    DB 000h                           ; DACFh
    DB 0FEh,0C6h                      ; DAD0h
    DB 08Ch                           ; DAD2h
    DB 018h,032h                      ; DAD3h
    DB 066h                           ; DAD5h
    DB 0FEh,000h                      ; DAD6h
    DB 078h                           ; DAD8h
    DB 060h                           ; DAD9h
    DB 060h                           ; DADAh
    DB 060h                           ; DADBh
    DB 060h                           ; DADCh
    DB 060h                           ; DADDh
    DB 078h                           ; DADEh
    DB 000h                           ; DADFh
    DB 0C0h                           ; DAE0h
    DB 060h                           ; DAE1h
    DB 030h,018h                      ; DAE2h
    DB 00Ch                           ; DAE4h
    DB 006h,002h                      ; DAE5h
    DB 000h                           ; DAE7h
    DB 078h                           ; DAE8h
    DB 018h,018h                      ; DAE9h
    DB 018h,018h                      ; DAEBh
    DB 018h,078h                      ; DAEDh
    DB 000h                           ; DAEFh
    DB 010h,038h                      ; DAF0h
    DB 06Ch                           ; DAF2h
    DB 0C6h,000h                      ; DAF3h
    DB 000h                           ; DAF5h
    DB 000h                           ; DAF6h
    DB 000h                           ; DAF7h
    DB 000h                           ; DAF8h
    DB 000h                           ; DAF9h
    DB 000h                           ; DAFAh
    DB 000h                           ; DAFBh
    DB 000h                           ; DAFCh
    DB 000h                           ; DAFDh
    DB 000h                           ; DAFEh
    DB 0FFh                           ; DAFFh
    DB 030h,030h                      ; DB00h
    DB 018h,000h                      ; DB02h
    DB 000h                           ; DB04h
    DB 000h                           ; DB05h
    DB 000h                           ; DB06h
    DB 000h                           ; DB07h
    DB 000h                           ; DB08h
    DB 000h                           ; DB09h
    DB 078h                           ; DB0Ah
    DB 00Ch                           ; DB0Bh
    DB 07Ch                           ; DB0Ch
    DB 0CCh,076h,000h                 ; DB0Dh
    DB 0E0h                           ; DB10h
    DB 060h                           ; DB11h
    DB 060h                           ; DB12h
    DB 07Ch                           ; DB13h
    DB 066h                           ; DB14h
    DB 066h                           ; DB15h
    DB 0DCh,000h,000h                 ; DB16h
    DB 000h                           ; DB19h
    DB 078h                           ; DB1Ah
    DB 0CCh,0C0h,0CCh                 ; DB1Bh
    DB 078h                           ; DB1Eh
    DB 000h                           ; DB1Fh
    DB 01Ch                           ; DB20h
    DB 00Ch                           ; DB21h
    DB 00Ch                           ; DB22h
    DB 07Ch                           ; DB23h
    DB 0CCh,0CCh,076h                 ; DB24h
    DB 000h                           ; DB27h
    DB 000h                           ; DB28h
    DB 000h                           ; DB29h
    DB 078h                           ; DB2Ah
    DB 0CCh,0FCh,0C0h                 ; DB2Bh
    DB 078h                           ; DB2Eh
    DB 000h                           ; DB2Fh
    DB 038h,06Ch                      ; DB30h
    DB 060h                           ; DB32h
    DB 0F0h                           ; DB33h
    DB 060h                           ; DB34h
    DB 060h                           ; DB35h
    DB 0F0h                           ; DB36h
    DB 000h                           ; DB37h
    DB 000h                           ; DB38h
    DB 000h                           ; DB39h
    DB 076h                           ; DB3Ah
    DB 0CCh,0CCh,07Ch                 ; DB3Bh
    DB 00Ch                           ; DB3Eh
    DB 0F8h                           ; DB3Fh
    DB 0E0h                           ; DB40h
    DB 060h                           ; DB41h
    DB 06Ch                           ; DB42h
    DB 076h                           ; DB43h
    DB 066h                           ; DB44h
    DB 066h                           ; DB45h
    DB 0E6h,000h                      ; DB46h
    DB 030h,000h                      ; DB48h
    DB 070h                           ; DB4Ah
    DB 030h,030h                      ; DB4Bh
    DB 030h,078h                      ; DB4Dh
    DB 000h                           ; DB4Fh
    DB 00Ch                           ; DB50h
    DB 000h                           ; DB51h
    DB 00Ch                           ; DB52h
    DB 00Ch                           ; DB53h
    DB 00Ch                           ; DB54h
    DB 0CCh,0CCh,078h                 ; DB55h
    DB 0E0h                           ; DB58h
    DB 060h                           ; DB59h
    DB 066h                           ; DB5Ah
    DB 06Ch                           ; DB5Bh
    DB 078h                           ; DB5Ch
    DB 06Ch                           ; DB5Dh
    DB 0E6h,000h                      ; DB5Eh
    DB 070h                           ; DB60h
    DB 030h,030h                      ; DB61h
    DB 030h,030h                      ; DB63h
    DB 030h,078h                      ; DB65h
    DB 000h                           ; DB67h
    DB 000h                           ; DB68h
    DB 000h                           ; DB69h
    DB 0CCh,0FEh,0FEh                 ; DB6Ah
    DB 0D6h,0C6h                      ; DB6Dh
    DB 000h                           ; DB6Fh
    DB 000h                           ; DB70h
    DB 000h                           ; DB71h
    DB 0F8h                           ; DB72h
    DB 0CCh,0CCh,0CCh                 ; DB73h
    DB 0CCh,000h,000h                 ; DB76h
    DB 000h                           ; DB79h
    DB 078h                           ; DB7Ah
    DB 0CCh,0CCh,0CCh                 ; DB7Bh
    DB 078h                           ; DB7Eh
    DB 000h                           ; DB7Fh
    DB 000h                           ; DB80h
    DB 000h                           ; DB81h
    DB 0DCh,066h,066h                 ; DB82h
    DB 07Ch                           ; DB85h
    DB 060h                           ; DB86h
    DB 0F0h                           ; DB87h
    DB 000h                           ; DB88h
    DB 000h                           ; DB89h
    DB 076h                           ; DB8Ah
    DB 0CCh,0CCh,07Ch                 ; DB8Bh
    DB 00Ch                           ; DB8Eh
    DB 01Eh,000h                      ; DB8Fh
    DB 000h                           ; DB91h
    DB 0DCh,076h,066h                 ; DB92h
    DB 060h                           ; DB95h
    DB 0F0h                           ; DB96h
    DB 000h                           ; DB97h
    DB 000h                           ; DB98h
    DB 000h                           ; DB99h
    DB 07Ch                           ; DB9Ah
    DB 0C0h                           ; DB9Bh
    DB 078h                           ; DB9Ch
    DB 00Ch                           ; DB9Dh
    DB 0F8h                           ; DB9Eh
    DB 000h                           ; DB9Fh
    DB 010h,030h                      ; DBA0h
    DB 07Ch                           ; DBA2h
    DB 030h,030h                      ; DBA3h
    DB 034h                           ; DBA5h
    DB 018h,000h                      ; DBA6h
    DB 000h                           ; DBA8h
    DB 000h                           ; DBA9h
    DB 0CCh,0CCh,0CCh                 ; DBAAh
    DB 0CCh,076h,000h                 ; DBADh
    DB 000h                           ; DBB0h
    DB 000h                           ; DBB1h
    DB 0CCh,0CCh,0CCh                 ; DBB2h
    DB 078h                           ; DBB5h
    DB 030h,000h                      ; DBB6h
    DB 000h                           ; DBB8h
    DB 000h                           ; DBB9h
    DB 0C6h,0D6h                      ; DBBAh
    DB 0FEh,0FEh                      ; DBBCh
    DB 06Ch                           ; DBBEh
    DB 000h                           ; DBBFh
    DB 000h                           ; DBC0h
    DB 000h                           ; DBC1h
    DB 0C6h,06Ch                      ; DBC2h
    DB 038h,06Ch                      ; DBC4h
    DB 0C6h,000h                      ; DBC6h
    DB 000h                           ; DBC8h
    DB 000h                           ; DBC9h
    DB 0CCh,0CCh,0CCh                 ; DBCAh
    DB 07Ch                           ; DBCDh
    DB 00Ch                           ; DBCEh
    DB 0F8h                           ; DBCFh
    DB 000h                           ; DBD0h
    DB 000h                           ; DBD1h
    DB 0FCh,098h,030h                 ; DBD2h
    DB 064h                           ; DBD5h
    DB 0FCh,000h,01Ch                 ; DBD6h
    DB 030h,030h                      ; DBD9h
    DB 0E0h                           ; DBDBh
    DB 030h,030h                      ; DBDCh
    DB 01Ch                           ; DBDEh
    DB 000h                           ; DBDFh
    DB 018h,018h                      ; DBE0h
    DB 018h,000h                      ; DBE2h
    DB 018h,018h                      ; DBE4h
    DB 018h,000h                      ; DBE6h
    DB 0E0h                           ; DBE8h
    DB 030h,030h                      ; DBE9h
    DB 01Ch                           ; DBEBh
    DB 030h,030h                      ; DBECh
    DB 0E0h                           ; DBEEh
    DB 000h                           ; DBEFh
    DB 076h                           ; DBF0h
    DB 0DCh,000h,000h                 ; DBF1h
    DB 000h                           ; DBF4h
    DB 000h                           ; DBF5h
    DB 000h                           ; DBF6h
    DB 000h                           ; DBF7h
    DB 000h                           ; DBF8h
    DB 010h,038h                      ; DBF9h
    DB 06Ch                           ; DBFBh
    DB 0C6h,0C6h                      ; DBFCh
    DB 0FEh,000h                      ; DBFEh
    DB 022h,088h,022h                 ; DC00h
    DB 088h                           ; DC03h
    DB 022h,088h,022h                 ; DC04h
    DB 088h                           ; DC07h
    DB 055h                           ; DC08h
    DB 0AAh                           ; DC09h
    DB 055h                           ; DC0Ah
    DB 0AAh                           ; DC0Bh
    DB 055h                           ; DC0Ch
    DB 0AAh                           ; DC0Dh
    DB 055h                           ; DC0Eh
    DB 0AAh                           ; DC0Fh
    DB 0EEh,0BBh                      ; DC10h
    DB 0EEh,0BBh                      ; DC12h
    DB 0EEh,0BBh                      ; DC14h
    DB 0EEh,0BBh                      ; DC16h
    DB 018h,018h                      ; DC18h
    DB 018h,018h                      ; DC1Ah
    DB 018h,018h                      ; DC1Ch
    DB 018h,018h                      ; DC1Eh
    DB 018h,018h                      ; DC20h
    DB 018h,018h                      ; DC22h
    DB 0F8h                           ; DC24h
    DB 018h,018h                      ; DC25h
    DB 018h,018h                      ; DC27h
    DB 018h,0F8h                      ; DC29h
    DB 018h,0F8h                      ; DC2Bh
    DB 018h,018h                      ; DC2Dh
    DB 018h,036h                      ; DC2Fh
    DB 036h,036h                      ; DC31h
    DB 036h,0F6h                      ; DC33h
    DB 036h,036h                      ; DC35h
    DB 036h,000h                      ; DC37h
    DB 000h                           ; DC39h
    DB 000h                           ; DC3Ah
    DB 000h                           ; DC3Bh
    DB 0FEh,036h                      ; DC3Ch
    DB 036h,036h                      ; DC3Eh
    DB 000h                           ; DC40h
    DB 000h                           ; DC41h
    DB 0F8h                           ; DC42h
    DB 018h,0F8h                      ; DC43h
    DB 018h,018h                      ; DC45h
    DB 018h,036h                      ; DC47h
    DB 036h,0F6h                      ; DC49h
    DB 006h,0F6h                      ; DC4Bh
    DB 036h,036h                      ; DC4Dh
    DB 036h,036h                      ; DC4Fh
    DB 036h,036h                      ; DC51h
    DB 036h,036h                      ; DC53h
    DB 036h,036h                      ; DC55h
    DB 036h,000h                      ; DC57h
    DB 000h                           ; DC59h
    DB 0FEh,006h                      ; DC5Ah
    DB 0F6h,036h                      ; DC5Ch
    DB 036h,036h                      ; DC5Eh
    DB 036h,036h                      ; DC60h
    DB 0F6h,006h                      ; DC62h
    DB 0FEh,000h                      ; DC64h
    DB 000h                           ; DC66h
    DB 000h                           ; DC67h
    DB 036h,036h                      ; DC68h
    DB 036h,036h                      ; DC6Ah
    DB 0FEh,000h                      ; DC6Ch
    DB 000h                           ; DC6Eh
    DB 000h                           ; DC6Fh
    DB 018h,018h                      ; DC70h
    DB 0F8h                           ; DC72h
    DB 018h,0F8h                      ; DC73h
    DB 000h                           ; DC75h
    DB 000h                           ; DC76h
    DB 000h                           ; DC77h
    DB 000h                           ; DC78h
    DB 000h                           ; DC79h
    DB 000h                           ; DC7Ah
    DB 000h                           ; DC7Bh
    DB 0F8h                           ; DC7Ch
    DB 018h,018h                      ; DC7Dh
    DB 018h,018h                      ; DC7Fh
    DB 018h,018h                      ; DC81h
    DB 018h,01Fh                      ; DC83h
    DB 000h                           ; DC85h
    DB 000h                           ; DC86h
    DB 000h                           ; DC87h
    DB 018h,018h                      ; DC88h
    DB 018h,018h                      ; DC8Ah
    DB 0FFh                           ; DC8Ch
    DB 000h                           ; DC8Dh
    DB 000h                           ; DC8Eh
    DB 000h                           ; DC8Fh
    DB 000h                           ; DC90h
    DB 000h                           ; DC91h
    DB 000h                           ; DC92h
    DB 000h                           ; DC93h
    DB 0FFh                           ; DC94h
    DB 018h,018h                      ; DC95h
    DB 018h,018h                      ; DC97h
    DB 018h,018h                      ; DC99h
    DB 018h,01Fh                      ; DC9Bh
    DB 018h,018h                      ; DC9Dh
    DB 018h,000h                      ; DC9Fh
    DB 000h                           ; DCA1h
    DB 000h                           ; DCA2h
    DB 000h                           ; DCA3h
    DB 0FFh                           ; DCA4h
    DB 000h                           ; DCA5h
    DB 000h                           ; DCA6h
    DB 000h                           ; DCA7h
    DB 018h,018h                      ; DCA8h
    DB 018h,018h                      ; DCAAh
    DB 0FFh                           ; DCACh
    DB 018h,018h                      ; DCADh
    DB 018h,018h                      ; DCAFh
    DB 018h,01Fh                      ; DCB1h
    DB 018h,01Fh                      ; DCB3h
    DB 018h,018h                      ; DCB5h
    DB 018h,036h                      ; DCB7h
    DB 036h,036h                      ; DCB9h
    DB 036h,037h                      ; DCBBh
    DB 036h,036h                      ; DCBDh
    DB 036h,036h                      ; DCBFh
    DB 036h,037h                      ; DCC1h
    DB 030h,03Fh                      ; DCC3h
    DB 000h                           ; DCC5h
    DB 000h                           ; DCC6h
    DB 000h                           ; DCC7h
    DB 000h                           ; DCC8h
    DB 000h                           ; DCC9h
    DB 03Fh                           ; DCCAh
    DB 030h,037h                      ; DCCBh
    DB 036h,036h                      ; DCCDh
    DB 036h,036h                      ; DCCFh
    DB 036h,0F7h                      ; DCD1h
    DB 000h                           ; DCD3h
    DB 0FFh                           ; DCD4h
    DB 000h                           ; DCD5h
    DB 000h                           ; DCD6h
    DB 000h                           ; DCD7h
    DB 000h                           ; DCD8h
    DB 000h                           ; DCD9h
    DB 0FFh                           ; DCDAh
    DB 000h                           ; DCDBh
    DB 0F7h                           ; DCDCh
    DB 036h,036h                      ; DCDDh
    DB 036h,036h                      ; DCDFh
    DB 036h,037h                      ; DCE1h
    DB 030h,037h                      ; DCE3h
    DB 036h,036h                      ; DCE5h
    DB 036h,000h                      ; DCE7h
    DB 000h                           ; DCE9h
    DB 0FFh                           ; DCEAh
    DB 000h                           ; DCEBh
    DB 0FFh                           ; DCECh
    DB 000h                           ; DCEDh
    DB 000h                           ; DCEEh
    DB 000h                           ; DCEFh
    DB 036h,036h                      ; DCF0h
    DB 0F7h                           ; DCF2h
    DB 000h                           ; DCF3h
    DB 0F7h                           ; DCF4h
    DB 036h,036h                      ; DCF5h
    DB 036h,018h                      ; DCF7h
    DB 018h,0FFh                      ; DCF9h
    DB 000h                           ; DCFBh
    DB 0FFh                           ; DCFCh
    DB 000h                           ; DCFDh
    DB 000h                           ; DCFEh
    DB 000h                           ; DCFFh
    DB 000h                           ; DD00h
    DB 000h                           ; DD01h
    DB 076h                           ; DD02h
    DB 0DCh,0C8h,0DCh                 ; DD03h
    DB 076h                           ; DD06h
    DB 000h                           ; DD07h
    DB 000h                           ; DD08h
    DB 078h                           ; DD09h
    DB 0CCh,0F8h,0CCh                 ; DD0Ah
    DB 0F8h                           ; DD0Dh
    DB 0C0h                           ; DD0Eh
    DB 0C0h                           ; DD0Fh
    DB 000h                           ; DD10h
    DB 0FCh,0CCh,0C0h                 ; DD11h
    DB 0C0h                           ; DD14h
    DB 0C0h                           ; DD15h
    DB 0C0h                           ; DD16h
    DB 000h                           ; DD17h
    DB 000h                           ; DD18h
    DB 0FEh,06Ch                      ; DD19h
    DB 06Ch                           ; DD1Bh
    DB 06Ch                           ; DD1Ch
    DB 06Ch                           ; DD1Dh
    DB 06Ch                           ; DD1Eh
    DB 000h                           ; DD1Fh
    DB 0FCh,0CCh,060h                 ; DD20h
    DB 030h,060h                      ; DD23h
    DB 0CCh,0FCh,000h                 ; DD25h
    DB 000h                           ; DD28h
    DB 000h                           ; DD29h
    DB 07Eh                           ; DD2Ah
    DB 0D8h                           ; DD2Bh
    DB 0D8h                           ; DD2Ch
    DB 0D8h                           ; DD2Dh
    DB 070h                           ; DD2Eh
    DB 000h                           ; DD2Fh
    DB 000h                           ; DD30h
    DB 066h                           ; DD31h
    DB 066h                           ; DD32h
    DB 066h                           ; DD33h
    DB 066h                           ; DD34h
    DB 07Ch                           ; DD35h
    DB 060h                           ; DD36h
    DB 0C0h                           ; DD37h
    DB 000h                           ; DD38h
    DB 076h                           ; DD39h
    DB 0DCh,018h,018h                 ; DD3Ah
    DB 018h,018h                      ; DD3Dh
    DB 000h                           ; DD3Fh
    DB 0FCh,030h,078h                 ; DD40h
    DB 0CCh,0CCh,078h                 ; DD43h
    DB 030h,0FCh                      ; DD46h
    DB 038h,06Ch                      ; DD48h
    DB 0C6h,0FEh                      ; DD4Ah
    DB 0C6h,06Ch                      ; DD4Ch
    DB 038h,000h                      ; DD4Eh
    DB 038h,06Ch                      ; DD50h
    DB 0C6h,0C6h                      ; DD52h
    DB 06Ch                           ; DD54h
    DB 06Ch                           ; DD55h
    DB 0EEh,000h                      ; DD56h
    DB 01Ch                           ; DD58h
    DB 030h,018h                      ; DD59h
    DB 07Ch                           ; DD5Bh
    DB 0CCh,0CCh,078h                 ; DD5Ch
    DB 000h                           ; DD5Fh
    DB 000h                           ; DD60h
    DB 000h                           ; DD61h
    DB 07Eh                           ; DD62h
    DB 0DBh,0DBh                      ; DD63h
    DB 07Eh                           ; DD65h
    DB 000h                           ; DD66h
    DB 000h                           ; DD67h
    DB 006h,00Ch                      ; DD68h
    DB 07Eh                           ; DD6Ah
    DB 0DBh,0DBh                      ; DD6Bh
    DB 07Eh                           ; DD6Dh
    DB 060h                           ; DD6Eh
    DB 0C0h                           ; DD6Fh
    DB 038h,060h                      ; DD70h
    DB 0C0h                           ; DD72h
    DB 0F8h                           ; DD73h
    DB 0C0h                           ; DD74h
    DB 060h                           ; DD75h
    DB 038h,000h                      ; DD76h
    DB 078h                           ; DD78h
    DB 0CCh,0CCh,0CCh                 ; DD79h
    DB 0CCh,0CCh,0CCh                 ; DD7Ch
    DB 000h                           ; DD7Fh
    DB 000h                           ; DD80h
    DB 000h                           ; DD81h
    DB 000h                           ; DD82h
    DB 008h                           ; DD83h
    DB 018h,038h                      ; DD84h
    DB 018h,018h                      ; DD86h
    DB 000h                           ; DD88h
    DB 000h                           ; DD89h
    DB 000h                           ; DD8Ah
    DB 038h,06Ch                      ; DD8Bh
    DB 018h,030h                      ; DD8Dh
    DB 07Ch                           ; DD8Fh
    DB 000h                           ; DD90h
    DB 000h                           ; DD91h
    DB 000h                           ; DD92h
    DB 038h,06Ch                      ; DD93h
    DB 018h,06Ch                      ; DD95h
    DB 038h,000h                      ; DD97h
    DB 000h                           ; DD99h
    DB 000h                           ; DD9Ah
    DB 06Ch                           ; DD9Bh
    DB 06Ch                           ; DD9Ch
    DB 07Ch                           ; DD9Dh
    DB 00Ch                           ; DD9Eh
    DB 00Ch                           ; DD9Fh
    DB 000h                           ; DDA0h
    DB 000h                           ; DDA1h
    DB 000h                           ; DDA2h
    DB 07Ch                           ; DDA3h
    DB 060h                           ; DDA4h
    DB 078h                           ; DDA5h
    DB 00Ch                           ; DDA6h
    DB 078h                           ; DDA7h
    DB 000h                           ; DDA8h
    DB 000h                           ; DDA9h
    DB 000h                           ; DDAAh
    DB 038h,060h                      ; DDABh
    DB 078h                           ; DDADh
    DB 06Ch                           ; DDAEh
    DB 038h,000h                      ; DDAFh
    DB 000h                           ; DDB1h
    DB 000h                           ; DDB2h
    DB 07Ch                           ; DDB3h
    DB 00Ch                           ; DDB4h
    DB 00Ch                           ; DDB5h
    DB 018h,030h                      ; DDB6h
    DB 000h                           ; DDB8h
    DB 000h                           ; DDB9h
    DB 000h                           ; DDBAh
    DB 038h,06Ch                      ; DDBBh
    DB 038h,06Ch                      ; DDBDh
    DB 038h,000h                      ; DDBFh
    DB 000h                           ; DDC1h
    DB 000h                           ; DDC2h
    DB 038h,06Ch                      ; DDC3h
    DB 03Ch                           ; DDC5h
    DB 00Ch                           ; DDC6h
    DB 038h,000h                      ; DDC7h
    DB 000h                           ; DDC9h
    DB 000h                           ; DDCAh
    DB 038h,06Ch                      ; DDCBh
    DB 06Ch                           ; DDCDh
    DB 06Ch                           ; DDCEh
    DB 038h,038h                      ; DDCFh
    DB 06Ch                           ; DDD1h
    DB 06Ch                           ; DDD2h
    DB 038h,000h                      ; DDD3h
    DB 000h                           ; DDD5h
    DB 000h                           ; DDD6h
    DB 000h                           ; DDD7h
    DB 038h,06Ch                      ; DDD8h
    DB 06Ch                           ; DDDAh
    DB 06Ch                           ; DDDBh
    DB 038h,000h                      ; DDDCh
    DB 000h                           ; DDDEh
    DB 000h                           ; DDDFh
    DB 008h                           ; DDE0h
    DB 018h,038h                      ; DDE1h
    DB 018h,018h                      ; DDE3h
    DB 000h                           ; DDE5h
    DB 000h                           ; DDE6h
    DB 000h                           ; DDE7h
    DB 038h,06Ch                      ; DDE8h
    DB 018h,030h                      ; DDEAh
    DB 07Ch                           ; DDECh
    DB 000h                           ; DDEDh
    DB 000h                           ; DDEEh
    DB 000h                           ; DDEFh
    DB 038h,06Ch                      ; DDF0h
    DB 018h,06Ch                      ; DDF2h
    DB 038h,000h                      ; DDF4h
    DB 000h                           ; DDF6h
    DB 000h                           ; DDF7h
    DB 06Ch                           ; DDF8h
    DB 06Ch                           ; DDF9h
    DB 07Ch                           ; DDFAh
    DB 00Ch                           ; DDFBh
    DB 00Ch                           ; DDFCh
    DB 000h                           ; DDFDh
    DB 000h                           ; DDFEh
    DB 000h                           ; DDFFh
    DB 000h                           ; DE00h
    DB 000h                           ; DE01h
    DB 0CCh,0D6h,0F6h                 ; DE02h
    DB 0D6h,0CCh                      ; DE05h
    DB 000h                           ; DE07h
    DB 000h                           ; DE08h
    DB 000h                           ; DE09h
    DB 078h                           ; DE0Ah
    DB 00Ch                           ; DE0Bh
    DB 07Ch                           ; DE0Ch
    DB 0CCh,07Eh,000h                 ; DE0Dh
    DB 000h                           ; DE10h
    DB 000h                           ; DE11h
    DB 0FCh,0C0h,0FCh                 ; DE12h
    DB 0C6h,0FCh                      ; DE15h
    DB 000h                           ; DE17h
    DB 000h                           ; DE18h
    DB 000h                           ; DE19h
    DB 0CCh,0CCh,0CCh                 ; DE1Ah
    DB 0CCh,0FEh,006h                 ; DE1Dh
    DB 000h                           ; DE20h
    DB 000h                           ; DE21h
    DB 03Ch                           ; DE22h
    DB 06Ch                           ; DE23h
    DB 06Ch                           ; DE24h
    DB 06Ch                           ; DE25h
    DB 0FEh,0C6h                      ; DE26h
    DB 000h                           ; DE28h
    DB 000h                           ; DE29h
    DB 078h                           ; DE2Ah
    DB 0CCh,0FCh,0C0h                 ; DE2Bh
    DB 078h                           ; DE2Eh
    DB 000h                           ; DE2Fh
    DB 000h                           ; DE30h
    DB 000h                           ; DE31h
    DB 010h,07Eh                      ; DE32h
    DB 0DBh,07Eh                      ; DE34h
    DB 018h,018h                      ; DE36h
    DB 000h                           ; DE38h
    DB 000h                           ; DE39h
    DB 0FCh,0C0h,0C0h                 ; DE3Ah
    DB 0C0h                           ; DE3Dh
    DB 0C0h                           ; DE3Eh
    DB 000h                           ; DE3Fh
    DB 000h                           ; DE40h
    DB 000h                           ; DE41h
    DB 0C6h,06Ch                      ; DE42h
    DB 038h,06Ch                      ; DE44h
    DB 0C6h,000h                      ; DE46h
    DB 000h                           ; DE48h
    DB 000h                           ; DE49h
    DB 0C6h,0C6h                      ; DE4Ah
    DB 0CEh,0FEh                      ; DE4Ch
    DB 066h                           ; DE4Eh
    DB 000h                           ; DE4Fh
    DB 00Ch                           ; DE50h
    DB 018h,0C6h                      ; DE51h
    DB 0C6h,0CEh                      ; DE53h
    DB 0FEh,066h                      ; DE55h
    DB 000h                           ; DE57h
    DB 000h                           ; DE58h
    DB 000h                           ; DE59h
    DB 0CCh,0D8h,0F0h                 ; DE5Ah
    DB 0D8h                           ; DE5Dh
    DB 0CEh,000h                      ; DE5Eh
    DB 000h                           ; DE60h
    DB 000h                           ; DE61h
    DB 01Eh,036h                      ; DE62h
    DB 066h                           ; DE64h
    DB 066h                           ; DE65h
    DB 0E6h,000h                      ; DE66h
    DB 000h                           ; DE68h
    DB 000h                           ; DE69h
    DB 0C6h,0EEh                      ; DE6Ah
    DB 0FEh,0D6h                      ; DE6Ch
    DB 0C6h,000h                      ; DE6Eh
    DB 000h                           ; DE70h
    DB 000h                           ; DE71h
    DB 0C6h,0C6h                      ; DE72h
    DB 0FEh,0C6h                      ; DE74h
    DB 0C6h,000h                      ; DE76h
    DB 000h                           ; DE78h
    DB 000h                           ; DE79h
    DB 07Ch                           ; DE7Ah
    DB 0C6h,0C6h                      ; DE7Bh
    DB 0C6h,07Ch                      ; DE7Dh
    DB 000h                           ; DE7Fh
    DB 000h                           ; DE80h
    DB 000h                           ; DE81h
    DB 0FEh,0C6h                      ; DE82h
    DB 0C6h,0C6h                      ; DE84h
    DB 0C6h,000h                      ; DE86h
    DB 000h                           ; DE88h
    DB 000h                           ; DE89h
    DB 07Eh                           ; DE8Ah
    DB 0C6h,07Eh                      ; DE8Bh
    DB 066h                           ; DE8Dh
    DB 0C6h,000h                      ; DE8Eh
    DB 000h                           ; DE90h
    DB 000h                           ; DE91h
    DB 0FCh,0C6h,0C6h                 ; DE92h
    DB 0FCh,0C0h,0C0h                 ; DE95h
    DB 000h                           ; DE98h
    DB 000h                           ; DE99h
    DB 07Ch                           ; DE9Ah
    DB 0C6h,0C0h                      ; DE9Bh
    DB 0C6h,07Ch                      ; DE9Dh
    DB 000h                           ; DE9Fh
    DB 000h                           ; DEA0h
    DB 000h                           ; DEA1h
    DB 0FCh,030h,030h                 ; DEA2h
    DB 030h,030h                      ; DEA5h
    DB 000h                           ; DEA7h
    DB 000h                           ; DEA8h
    DB 000h                           ; DEA9h
    DB 0C6h,066h                      ; DEAAh
    DB 03Eh,006h                      ; DEACh
    DB 07Ch                           ; DEAEh
    DB 000h                           ; DEAFh
    DB 000h                           ; DEB0h
    DB 000h                           ; DEB1h
    DB 0D6h,0D6h                      ; DEB2h
    DB 07Ch                           ; DEB4h
    DB 0D6h,0D6h                      ; DEB5h
    DB 000h                           ; DEB7h
    DB 000h                           ; DEB8h
    DB 000h                           ; DEB9h
    DB 0F8h                           ; DEBAh
    DB 0CCh,0F8h,0CCh                 ; DEBBh
    DB 0F8h                           ; DEBEh
    DB 000h                           ; DEBFh
    DB 000h                           ; DEC0h
    DB 000h                           ; DEC1h
    DB 0C0h                           ; DEC2h
    DB 0C0h                           ; DEC3h
    DB 0FCh,0C6h,0FCh                 ; DEC4h
    DB 000h                           ; DEC7h
    DB 000h                           ; DEC8h
    DB 000h                           ; DEC9h
    DB 0C6h,0C6h                      ; DECAh
    DB 0F6h,0DAh                      ; DECCh
    DB 0F6h,000h                      ; DECEh
    DB 000h                           ; DED0h
    DB 000h                           ; DED1h
    DB 078h                           ; DED2h
    DB 0CCh,018h,0CCh                 ; DED3h
    DB 078h                           ; DED6h
    DB 000h                           ; DED7h
    DB 000h                           ; DED8h
    DB 000h                           ; DED9h
    DB 0D6h,0D6h                      ; DEDAh
    DB 0D6h,0D6h                      ; DEDCh
    DB 0FEh,000h                      ; DEDEh
    DB 000h                           ; DEE0h
    DB 000h                           ; DEE1h
    DB 0FCh,006h,01Eh                 ; DEE2h
    DB 006h,0FCh                      ; DEE5h
    DB 000h                           ; DEE7h
    DB 000h                           ; DEE8h
    DB 000h                           ; DEE9h
    DB 0D6h,0D6h                      ; DEEAh
    DB 0D6h,0D6h                      ; DEECh
    DB 0FEh,006h                      ; DEEEh
    DB 000h                           ; DEF0h
    DB 000h                           ; DEF1h
    DB 0CCh,0CCh,07Ch                 ; DEF2h
    DB 00Ch                           ; DEF5h
    DB 00Ch                           ; DEF6h
    DB 000h                           ; DEF7h
    DB 000h                           ; DEF8h
    DB 000h                           ; DEF9h
    DB 0E0h                           ; DEFAh
    DB 060h                           ; DEFBh
    DB 07Ch                           ; DEFCh
    DB 066h                           ; DEFDh
    DB 07Ch                           ; DEFEh
    DB 000h                           ; DEFFh
    DB 0CCh,0D6h,0D6h                 ; DF00h
    DB 0F6h,0D6h                      ; DF03h
    DB 0D6h,0CCh                      ; DF05h
    DB 000h                           ; DF07h
    DB 030h,078h                      ; DF08h
    DB 0CCh,0CCh,0FCh                 ; DF0Ah
    DB 0CCh,0CCh,000h                 ; DF0Dh
    DB 0FEh,060h                      ; DF10h
    DB 060h                           ; DF12h
    DB 07Ch                           ; DF13h
    DB 066h                           ; DF14h
    DB 066h                           ; DF15h
    DB 0FCh,000h,0CCh                 ; DF16h
    DB 0CCh,0CCh,0CCh                 ; DF19h
    DB 0CCh,0CCh,0FEh                 ; DF1Ch
    DB 006h,03Ch                      ; DF1Fh
    DB 06Ch                           ; DF21h
    DB 06Ch                           ; DF22h
    DB 06Ch                           ; DF23h
    DB 06Ch                           ; DF24h
    DB 06Ch                           ; DF25h
    DB 0FEh,0C6h                      ; DF26h
    DB 0FEh,062h                      ; DF28h
    DB 068h                           ; DF2Ah
    DB 078h                           ; DF2Bh
    DB 068h                           ; DF2Ch
    DB 062h                           ; DF2Dh
    DB 0FEh,000h                      ; DF2Eh
    DB 018h,07Eh                      ; DF30h
    DB 0DBh,0DBh                      ; DF32h
    DB 07Eh                           ; DF34h
    DB 018h,03Ch                      ; DF35h
    DB 000h                           ; DF37h
    DB 0FEh,066h                      ; DF38h
    DB 060h                           ; DF3Ah
    DB 060h                           ; DF3Bh
    DB 060h                           ; DF3Ch
    DB 060h                           ; DF3Dh
    DB 0F0h                           ; DF3Eh
    DB 000h                           ; DF3Fh
    DB 0C6h,06Ch                      ; DF40h
    DB 038h,038h                      ; DF42h
    DB 06Ch                           ; DF44h
    DB 0C6h,0C6h                      ; DF45h
    DB 000h                           ; DF47h
    DB 0C6h,0C6h                      ; DF48h
    DB 0CEh,0DEh                      ; DF4Ah
    DB 0F6h,0E6h                      ; DF4Ch
    DB 0C6h,000h                      ; DF4Eh
    DB 038h,0C6h                      ; DF50h
    DB 0CEh,0DEh                      ; DF52h
    DB 0F6h,0E6h                      ; DF54h
    DB 0C6h,000h                      ; DF56h
    DB 0C6h,0CCh                      ; DF58h
    DB 0D8h                           ; DF5Ah
    DB 0F0h                           ; DF5Bh
    DB 0D8h                           ; DF5Ch
    DB 0CCh,0C6h,000h                 ; DF5Dh
    DB 01Eh,036h                      ; DF60h
    DB 066h                           ; DF62h
    DB 066h                           ; DF63h
    DB 066h                           ; DF64h
    DB 066h                           ; DF65h
    DB 0C6h,000h                      ; DF66h
    DB 0C6h,0EEh                      ; DF68h
    DB 0FEh,0FEh                      ; DF6Ah
    DB 0D6h,0C6h                      ; DF6Ch
    DB 0C6h,000h                      ; DF6Eh
    DB 0CCh,0CCh,0CCh                 ; DF70h
    DB 0FCh,0CCh,0CCh                 ; DF73h
    DB 0CCh,000h,07Ch                 ; DF76h
    DB 0C6h,0C6h                      ; DF79h
    DB 0C6h,0C6h                      ; DF7Bh
    DB 0C6h,07Ch                      ; DF7Dh
    DB 000h                           ; DF7Fh
    DB 0FEh,0C6h                      ; DF80h
    DB 0C6h,0C6h                      ; DF82h
    DB 0C6h,0C6h                      ; DF84h
    DB 0C6h,000h                      ; DF86h
    DB 07Eh                           ; DF88h
    DB 0C6h,0C6h                      ; DF89h
    DB 07Eh                           ; DF8Bh
    DB 036h,066h                      ; DF8Ch
    DB 0C6h,000h                      ; DF8Eh
    DB 0FCh,066h,066h                 ; DF90h
    DB 07Ch                           ; DF93h
    DB 060h                           ; DF94h
    DB 060h                           ; DF95h
    DB 0F0h                           ; DF96h
    DB 000h                           ; DF97h
    DB 03Ch                           ; DF98h
    DB 066h                           ; DF99h
    DB 0C0h                           ; DF9Ah
    DB 0C0h                           ; DF9Bh
    DB 0C0h                           ; DF9Ch
    DB 066h                           ; DF9Dh
    DB 03Ch                           ; DF9Eh
    DB 000h                           ; DF9Fh
    DB 0FCh,0B4h,030h                 ; DFA0h
    DB 030h,030h                      ; DFA3h
    DB 030h,078h                      ; DFA5h
    DB 000h                           ; DFA7h
    DB 0C6h,0C6h                      ; DFA8h
    DB 0C6h,07Eh                      ; DFAAh
    DB 006h,00Ch                      ; DFACh
    DB 078h                           ; DFAEh
    DB 000h                           ; DFAFh
    DB 0D6h,0D6h                      ; DFB0h
    DB 07Ch                           ; DFB2h
    DB 038h,07Ch                      ; DFB3h
    DB 0D6h,0D6h                      ; DFB5h
    DB 000h                           ; DFB7h
    DB 0FCh,066h,066h                 ; DFB8h
    DB 07Ch                           ; DFBBh
    DB 066h                           ; DFBCh
    DB 066h                           ; DFBDh
    DB 0FCh,000h,0C0h                 ; DFBEh
    DB 0C0h                           ; DFC1h
    DB 0C0h                           ; DFC2h
    DB 0FCh,0C6h,0C6h                 ; DFC3h
    DB 0FCh,000h,0C6h                 ; DFC6h
    DB 0C6h,0C6h                      ; DFC9h
    DB 0F6h,0DAh                      ; DFCBh
    DB 0DAh,0F6h,000h                 ; DFCDh
    DB 07Ch                           ; DFD0h
    DB 0C6h,006h                      ; DFD1h
    DB 01Ch                           ; DFD3h
    DB 006h,0C6h                      ; DFD4h
    DB 07Ch                           ; DFD6h
    DB 000h                           ; DFD7h
    DB 0C6h,0D6h                      ; DFD8h
    DB 0D6h,0D6h                      ; DFDAh
    DB 0D6h,0D6h                      ; DFDCh
    DB 0FEh,000h                      ; DFDEh
    DB 07Ch                           ; DFE0h
    DB 0C6h,006h                      ; DFE1h
    DB 03Eh,006h                      ; DFE3h
    DB 0C6h,07Ch                      ; DFE5h
    DB 000h                           ; DFE7h
    DB 0C6h,0D6h                      ; DFE8h
    DB 0D6h,0D6h                      ; DFEAh
    DB 0D6h,0D6h                      ; DFECh
    DB 0FEh,006h                      ; DFEEh
    DB 0C6h,0C6h                      ; DFF0h
    DB 0C6h,07Eh                      ; DFF2h
    DB 006h,006h                      ; DFF4h
    DB 006h,000h                      ; DFF6h
    DB 0FFh                           ; DFF8h
    DB 0FFh                           ; DFF9h
    DB 0FFh                           ; DFFAh
    DB 0FFh                           ; DFFBh
    DB 0FFh                           ; DFFCh
    DB 0FFh                           ; DFFDh
    DB 0FFh                           ; DFFEh
    DB 0FFh                           ; DFFFh
    ASSERT $ == 0E000h
    END
