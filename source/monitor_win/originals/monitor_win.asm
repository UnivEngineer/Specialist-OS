                           .data:0000c000 c3 03 c0                         jp 0xc003
                           .data:0000c003 31 ff 7f                         ld sp,0x7fff
                           .data:0000c006 3e 82                            ld a,0x82
                           .data:0000c008 32 03 ff                         ld (0xff03),a
                           .data:0000c00b c3 44 c4                         jp 0xc444
                           .data:0000c00e 00                               nop
                           .data:0000c00f 00                               nop
                           .data:0000c010 e5                               push hl
                           .data:0000c011 c5                               push bc
                           .data:0000c012 21 00 00                         ld hl,0x0000
                           .data:0000c015 39                               add hl,sp
                           .data:0000c016 22 f6 8f                         ld (0x8ff6),hl
                           .data:0000c019 31 00 c0                         ld sp,0xc000
                           .data:0000c01c 2a fa 8f                         ld hl,(0x8ffa)
                           .data:0000c01f 01 00 03                         ld bc,0x0300
                           .data:0000c022 e5                               push hl
                           .data:0000c023 e5                               push hl
                           .data:0000c024 e5                               push hl
                           .data:0000c025 e5                               push hl
                           .data:0000c026 e5                               push hl
                           .data:0000c027 e5                               push hl
                           .data:0000c028 e5                               push hl
                           .data:0000c029 e5                               push hl
                           .data:0000c02a 0b                               dec bc
                           .data:0000c02b 78                               ld a,b
                           .data:0000c02c b1                               or c
                           .data:0000c02d c2 22 c0                         jp nz,0xc022
                           .data:0000c030 2a f6 8f                         ld hl,(0x8ff6)
                           .data:0000c033 f9                               ld sp,hl
                           .data:0000c034 c1                               pop bc
                           .data:0000c035 e1                               pop hl
                           .data:0000c036 c9                               ret
                           .data:0000c037 e5                               push hl
                           .data:0000c038 d5                               push de
                           .data:0000c039 c5                               push bc
                           .data:0000c03a f5                               push af
                           .data:0000c03b 79                               ld a,c
                           .data:0000c03c fe 21                            cp 0x21
                           .data:0000c03e da d4 c0                         jp c,0xc0d4
                           .data:0000c041 2a fc 8f                         ld hl,(0x8ffc)
                           .data:0000c044 7c                               ld a,h
                           .data:0000c045 fe be                            cp 0xbe
                           .data:0000c047 d2 b2 c0                         jp nc,0xc0b2
                           .data:0000c04a c6 03                            add a,0x03
                           .data:0000c04c 32 fd 8f                         ld (0x8ffd),a
                           .data:0000c04f eb                               ex de,hl
                           .data:0000c050 79                               ld a,c
                           .data:0000c051 32 e9 8f                         ld (0x8fe9),a
                           .data:0000c054 d6 20                            sub 0x20
                           .data:0000c056 2a e7 8f                         ld hl,(0x8fe7)
                           .data:0000c059 85                               add a,l
                           .data:0000c05a 6f                               ld l,a
                           .data:0000c05b 29                               add hl,hl
                           .data:0000c05c 29                               add hl,hl
                           .data:0000c05d 29                               add hl,hl
                           .data:0000c05e eb                               ex de,hl
                           .data:0000c05f 00                               nop
                           .data:0000c060 7c                               ld a,h
                           .data:0000c061 e6 03                            and 0x03
                           .data:0000c063 4f                               ld c,a
                           .data:0000c064 3e 05                            ld a,0x05
                           .data:0000c066 91                               sub c
                           .data:0000c067 4f                               ld c,a
                           .data:0000c068 7c                               ld a,h
                           .data:0000c069 e6 fc                            and 0xfc
                           .data:0000c06b 0f                               rrca
                           .data:0000c06c 0f                               rrca
                           .data:0000c06d c6 90                            add a,0x90
                           .data:0000c06f 67                               ld h,a
                           .data:0000c070 22 f8 8f                         ld (0x8ff8),hl
                           .data:0000c073 06 08                            ld b,0x08
                           .data:0000c075 00                               nop
                           .data:0000c076 1a                               ld a,(de)
                           .data:0000c077 6f                               ld l,a
                           .data:0000c078 26 00                            ld h,0x00
                           .data:0000c07a 79                               ld a,c
                           .data:0000c07b 29                               add hl,hl
                           .data:0000c07c 29                               add hl,hl
                           .data:0000c07d 3d                               dec a
                           .data:0000c07e c2 7b c0                         jp nz,0xc07b
                           .data:0000c081 e5                               push hl
                           .data:0000c082 13                               inc de
                           .data:0000c083 05                               dec b
                           .data:0000c084 c2 76 c0                         jp nz,0xc076
                           .data:0000c087 06 08                            ld b,0x08
                           .data:0000c089 2a f8 8f                         ld hl,(0x8ff8)
                           .data:0000c08c d1                               pop de
                           .data:0000c08d 7a                               ld a,d
                           .data:0000c08e cd 63 c1                         call 0xc163
                           .data:0000c091 77                               ld (hl),a
                           .data:0000c092 24                               inc h
                           .data:0000c093 7b                               ld a,e
                           .data:0000c094 cd 63 c1                         call 0xc163
                           .data:0000c097 77                               ld (hl),a
                           .data:0000c098 25                               dec h
                           .data:0000c099 2d                               dec l
                           .data:0000c09a 05                               dec b
                           .data:0000c09b c2 8c c0                         jp nz,0xc08c
                           .data:0000c09e f1                               pop af
                           .data:0000c09f c1                               pop bc
                           .data:0000c0a0 d1                               pop de
                           .data:0000c0a1 e1                               pop hl
                           .data:0000c0a2 c9                               ret
                           .data:0000c0a3 4f                               ld c,a
                           .data:0000c0a4 3a fa 8f                         ld a,(0x8ffa)
                           .data:0000c0a7 b7                               or a
                           .data:0000c0a8 c2 af c0                         jp nz,0xc0af
                           .data:0000c0ab 79                               ld a,c
                           .data:0000c0ac 2f                               cpl
                           .data:0000c0ad a6                               and (hl)
                           .data:0000c0ae c9                               ret
                           .data:0000c0af 7e                               ld a,(hl)
                           .data:0000c0b0 b1                               or c
                           .data:0000c0b1 c9                               ret
                           .data:0000c0b2 7d                               ld a,l
                           .data:0000c0b3 fe f5                            cp 0xf5
                           .data:0000c0b5 d2 c4 c0                         jp nc,0xc0c4
                           .data:0000c0b8 c6 0a                            add a,0x0a
                           .data:0000c0ba 6f                               ld l,a
                           .data:0000c0bb 32 fc 8f                         ld (0x8ffc),a
                           .data:0000c0be 26 00                            ld h,0x00
                           .data:0000c0c0 7c                               ld a,h
                           .data:0000c0c1 c3 4a c0                         jp 0xc04a
                           .data:0000c0c4 cd 2d c2                         call 0xc22d
                           .data:0000c0c7 00                               nop
                           .data:0000c0c8 cd 10 c0                         call 0xc010
                           .data:0000c0cb 21 08 00                         ld hl,0x0008
                           .data:0000c0ce 22 fc 8f                         ld (0x8ffc),hl
                           .data:0000c0d1 c3 41 c0                         jp 0xc041
                           .data:0000c0d4 2a fc 8f                         ld hl,(0x8ffc)
                           .data:0000c0d7 fe 20                            cp 0x20
                           .data:0000c0d9 ca 07 c1                         jp z,0xc107
                           .data:0000c0dc fe 0a                            cp 0x0a
                           .data:0000c0de ca 13 c1                         jp z,0xc113
                           .data:0000c0e1 fe 0d                            cp 0x0d
                           .data:0000c0e3 ca 28 c1                         jp z,0xc128
                           .data:0000c0e6 fe 18                            cp 0x18
                           .data:0000c0e8 ca 07 c1                         jp z,0xc107
                           .data:0000c0eb fe 08                            cp 0x08
                           .data:0000c0ed ca 2d c1                         jp z,0xc12d
                           .data:0000c0f0 fe 19                            cp 0x19
                           .data:0000c0f2 ca 3b c1                         jp z,0xc13b
                           .data:0000c0f5 fe 1a                            cp 0x1a
                           .data:0000c0f7 ca 47 c1                         jp z,0xc147
                           .data:0000c0fa fe 0c                            cp 0x0c
                           .data:0000c0fc ca 5a c1                         jp z,0xc15a
                           .data:0000c0ff fe 1f                            cp 0x1f
                           .data:0000c101 ca 57 c1                         jp z,0xc157
                           .data:0000c104 c3 5d c1                         jp 0xc15d
                           .data:0000c107 7c                               ld a,h
                           .data:0000c108 fe be                            cp 0xbe
                           .data:0000c10a d2 13 c1                         jp nc,0xc113
                           .data:0000c10d c6 03                            add a,0x03
                           .data:0000c10f 67                               ld h,a
                           .data:0000c110 c3 5d c1                         jp 0xc15d
                           .data:0000c113 26 00                            ld h,0x00
                           .data:0000c115 7d                               ld a,l
                           .data:0000c116 fe f5                            cp 0xf5
                           .data:0000c118 d2 21 c1                         jp nc,0xc121
                           .data:0000c11b c6 0a                            add a,0x0a
                           .data:0000c11d 6f                               ld l,a
                           .data:0000c11e c3 5d c1                         jp 0xc15d
                           .data:0000c121 cd 2d c2                         call 0xc22d
                           .data:0000c124 00                               nop
                           .data:0000c125 c3 57 c1                         jp 0xc157
                           .data:0000c128 26 00                            ld h,0x00
                           .data:0000c12a c3 5d c1                         jp 0xc15d
                           .data:0000c12d 7c                               ld a,h
                           .data:0000c12e fe 02                            cp 0x02
                           .data:0000c130 da 39 c1                         jp c,0xc139
                           .data:0000c133 d6 03                            sub 0x03
                           .data:0000c135 67                               ld h,a
                           .data:0000c136 c3 5d c1                         jp 0xc15d
                           .data:0000c139 26 bd                            ld h,0xbd
                           .data:0000c13b 7d                               ld a,l
                           .data:0000c13c fe 11                            cp 0x11
                           .data:0000c13e da 5d c1                         jp c,0xc15d
                           .data:0000c141 d6 0a                            sub 0x0a
                           .data:0000c143 6f                               ld l,a
                           .data:0000c144 c3 5d c1                         jp 0xc15d
                           .data:0000c147 7d                               ld a,l
                           .data:0000c148 fe f5                            cp 0xf5
                           .data:0000c14a d2 5d c1                         jp nc,0xc15d
                           .data:0000c14d c6 0a                            add a,0x0a
                           .data:0000c14f 6f                               ld l,a
                           .data:0000c150 c3 5d c1                         jp 0xc15d
                           .data:0000c153 00                               nop
                           .data:0000c154 00                               nop
                           .data:0000c155 00                               nop
                           .data:0000c156 00                               nop
                           .data:0000c157 cd 10 c0                         call 0xc010
                           .data:0000c15a 21 08 00                         ld hl,0x0008
                           .data:0000c15d 22 fc 8f                         ld (0x8ffc),hl
                           .data:0000c160 c3 9e c0                         jp 0xc09e
                           .data:0000c163 4f                               ld c,a
                           .data:0000c164 3a e9 8f                         ld a,(0x8fe9)
                           .data:0000c167 fe 7f                            cp 0x7f
                           .data:0000c169 79                               ld a,c
                           .data:0000c16a ca a3 c0                         jp z,0xc0a3
                           .data:0000c16d ae                               xor (hl)
                           .data:0000c16e c9                               ret
                           .data:0000c16f 00                               nop
                           .data:0000c170 f5                               push af
                           .data:0000c171 c5                               push bc
                           .data:0000c172 e5                               push hl
                           .data:0000c173 2a f1 8f                         ld hl,(0x8ff1)
                           .data:0000c176 3e 0b                            ld a,0x0b
                           .data:0000c178 32 03 ff                         ld (0xff03),a
                           .data:0000c17b cd 8f c1                         call 0xc18f
                           .data:0000c17e 3e 0a                            ld a,0x0a
                           .data:0000c180 32 03 ff                         ld (0xff03),a
                           .data:0000c183 cd 8f c1                         call 0xc18f
                           .data:0000c186 25                               dec h
                           .data:0000c187 c2 76 c1                         jp nz,0xc176
                           .data:0000c18a 00                               nop
                           .data:0000c18b e1                               pop hl
                           .data:0000c18c c1                               pop bc
                           .data:0000c18d f1                               pop af
                           .data:0000c18e c9                               ret
                           .data:0000c18f 45                               ld b,l
                           .data:0000c190 05                               dec b
                           .data:0000c191 c2 90 c1                         jp nz,0xc190
                           .data:0000c194 c9                               ret
                           .data:0000c195 c9                               ret
                           .data:0000c196 c5                               push bc
                           .data:0000c197 06 15                            ld b,0x15
                           .data:0000c199 cd 90 c1                         call 0xc190
                           .data:0000c19c c1                               pop bc
                           .data:0000c19d c3 87 c2                         jp 0xc287
                           .data:0000c1a0 e5                               push hl
                           .data:0000c1a1 3e 40                            ld a,0x40
                           .data:0000c1a3 32 f1 8f                         ld (0x8ff1),a
                           .data:0000c1a6 cd 70 c1                         call 0xc170
                           .data:0000c1a9 e1                               pop hl
                           .data:0000c1aa c9                               ret
                           .data:0000c1ab e5                               push hl
                           .data:0000c1ac 3e 5f                            ld a,0x5f
                           .data:0000c1ae c3 a3 c1                         jp 0xc1a3
                           .data:0000c1b1 e5                               push hl
                           .data:0000c1b2 c5                               push bc
                           .data:0000c1b3 06 ff                            ld b,0xff
                           .data:0000c1b5 cd 83 c2                         call 0xc283
                           .data:0000c1b8 3a f4 8f                         ld a,(0x8ff4)
                           .data:0000c1bb b7                               or a
                           .data:0000c1bc ca e2 c1                         jp z,0xc1e2
                           .data:0000c1bf cd 54 c2                         call 0xc254
                           .data:0000c1c2 3a 01 ff                         ld a,(0xff01)
                           .data:0000c1c5 e6 02                            and 0x02
                           .data:0000c1c7 ca 4b c2                         jp z,0xc24b
                           .data:0000c1ca cd 5a c2                         call 0xc25a
                           .data:0000c1cd 3a 00 ff                         ld a,(0xff00)
                           .data:0000c1d0 fe ff                            cp 0xff
                           .data:0000c1d2 c2 ff c1                         jp nz,0xc1ff
                           .data:0000c1d5 3a 02 ff                         ld a,(0xff02)
                           .data:0000c1d8 f6 f0                            or 0xf0
                           .data:0000c1da fe ff                            cp 0xff
                           .data:0000c1dc c2 05 c2                         jp nz,0xc205
                           .data:0000c1df c3 b5 c1                         jp 0xc1b5
                           .data:0000c1e2 cd 54 c2                         call 0xc254
                           .data:0000c1e5 3a 01 ff                         ld a,(0xff01)
                           .data:0000c1e8 e6 02                            and 0x02
                           .data:0000c1ea c2 f2 c1                         jp nz,0xc1f2
                           .data:0000c1ed 06 ff                            ld b,0xff
                           .data:0000c1ef c3 ca c1                         jp 0xc1ca
                           .data:0000c1f2 05                               dec b
                           .data:0000c1f3 c2 ca c1                         jp nz,0xc1ca
                           .data:0000c1f6 3a ec 8f                         ld a,(0x8fec)
                           .data:0000c1f9 32 f4 8f                         ld (0x8ff4),a
                           .data:0000c1fc c3 b5 c1                         jp 0xc1b5
                           .data:0000c1ff 6f                               ld l,a
                           .data:0000c200 26 ff                            ld h,0xff
                           .data:0000c202 c3 08 c2                         jp 0xc208
                           .data:0000c205 67                               ld h,a
                           .data:0000c206 2e ff                            ld l,0xff
                           .data:0000c208 0e fb                            ld c,0xfb
                           .data:0000c20a 0c                               inc c
                           .data:0000c20b 29                               add hl,hl
                           .data:0000c20c da 0a c2                         jp c,0xc20a
                           .data:0000c20f 69                               ld l,c
                           .data:0000c210 06 ff                            ld b,0xff
                           .data:0000c212 cd 54 c2                         call 0xc254
                           .data:0000c215 3a 01 ff                         ld a,(0xff01)
                           .data:0000c218 f6 03                            or 0x03
                           .data:0000c21a fe ff                            cp 0xff
                           .data:0000c21c c2 35 c2                         jp nz,0xc235
                           .data:0000c21f 05                               dec b
                           .data:0000c220 c2 12 c2                         jp nz,0xc212
                           .data:0000c223 c3 b5 c1                         jp 0xc1b5
                           .data:0000c226 3a 01 ff                         ld a,(0xff01)
                           .data:0000c229 2f                               cpl
                           .data:0000c22a e6 f7                            and 0xf7
                           .data:0000c22c c9                               ret
                           .data:0000c22d cd 26 c2                         call 0xc226
                           .data:0000c230 c4 60 c2                         call nz,0xc260
                           .data:0000c233 c9                               ret
                           .data:0000c234 00                               nop
                           .data:0000c235 0e fd                            ld c,0xfd
                           .data:0000c237 0c                               inc c
                           .data:0000c238 0f                               rrca
                           .data:0000c239 da 37 c2                         jp c,0xc237
                           .data:0000c23c 79                               ld a,c
                           .data:0000c23d 07                               rlca
                           .data:0000c23e 07                               rlca
                           .data:0000c23f 07                               rlca
                           .data:0000c240 07                               rlca
                           .data:0000c241 c6 a0                            add a,0xa0
                           .data:0000c243 b5                               or l
                           .data:0000c244 6f                               ld l,a
                           .data:0000c245 26 c4                            ld h,0xc4
                           .data:0000c247 7e                               ld a,(hl)
                           .data:0000c248 c1                               pop bc
                           .data:0000c249 e1                               pop hl
                           .data:0000c24a c9                               ret
                           .data:0000c24b 32 f4 8f                         ld (0x8ff4),a
                           .data:0000c24e cd 70 c1                         call 0xc170
                           .data:0000c251 c3 b5 c1                         jp 0xc1b5
                           .data:0000c254 3e 82                            ld a,0x82
                           .data:0000c256 32 03 ff                         ld (0xff03),a
                           .data:0000c259 c9                               ret
                           .data:0000c25a 3e 91                            ld a,0x91
                           .data:0000c25c 32 03 ff                         ld (0xff03),a
                           .data:0000c25f c9                               ret
                           .data:0000c260 c5                               push bc
                           .data:0000c261 3a ef 8f                         ld a,(0x8fef)
                           .data:0000c264 fe 80                            cp 0x80
                           .data:0000c266 ca 41 c3                         jp z,0xc341
                           .data:0000c269 0e ff                            ld c,0xff
                           .data:0000c26b cd 54 c2                         call 0xc254
                           .data:0000c26e 3a 01 ff                         ld a,(0xff01)
                           .data:0000c271 f6 03                            or 0x03
                           .data:0000c273 fe ff                            cp 0xff
                           .data:0000c275 c2 69 c2                         jp nz,0xc269
                           .data:0000c278 06 15                            ld b,0x15
                           .data:0000c27a cd 90 c1                         call 0xc190
                           .data:0000c27d 0d                               dec c
                           .data:0000c27e c2 6b c2                         jp nz,0xc26b
                           .data:0000c281 c1                               pop bc
                           .data:0000c282 c9                               ret
                           .data:0000c283 2a ed 8f                         ld hl,(0x8fed)
                           .data:0000c286 e9                               jp (hl)
                           .data:0000c287 e5                               push hl
                           .data:0000c288 21 eb 8f                         ld hl,0x8feb
                           .data:0000c28b 35                               dec (hl)
                           .data:0000c28c cc 91 c2                         call z,0xc291
                           .data:0000c28f e1                               pop hl
                           .data:0000c290 c9                               ret
                           .data:0000c291 c5                               push bc
                           .data:0000c292 21 eb 8f                         ld hl,0x8feb
                           .data:0000c295 36 ff                            ld (hl),0xff
                           .data:0000c297 2b                               dec hl
                           .data:0000c298 34                               inc (hl)
                           .data:0000c299 2a fc 8f                         ld hl,(0x8ffc)
                           .data:0000c29c 23                               inc hl
                           .data:0000c29d 23                               inc hl
                           .data:0000c29e 22 fc 8f                         ld (0x8ffc),hl
                           .data:0000c2a1 0e 5f                            ld c,0x5f
                           .data:0000c2a3 cd bc c2                         call 0xc2bc
                           .data:0000c2a6 2a fc 8f                         ld hl,(0x8ffc)
                           .data:0000c2a9 2b                               dec hl
                           .data:0000c2aa 2b                               dec hl
                           .data:0000c2ab 22 fc 8f                         ld (0x8ffc),hl
                           .data:0000c2ae c1                               pop bc
                           .data:0000c2af c9                               ret
                           .data:0000c2b0 e5                               push hl
                           .data:0000c2b1 f5                               push af
                           .data:0000c2b2 3a ea 8f                         ld a,(0x8fea)
                           .data:0000c2b5 0f                               rrca
                           .data:0000c2b6 d4 91 c2                         call nc,0xc291
                           .data:0000c2b9 f1                               pop af
                           .data:0000c2ba e1                               pop hl
                           .data:0000c2bb c9                               ret
                           .data:0000c2bc e5                               push hl
                           .data:0000c2bd d5                               push de
                           .data:0000c2be c5                               push bc
                           .data:0000c2bf f5                               push af
                           .data:0000c2c0 79                               ld a,c
                           .data:0000c2c1 2a fc 8f                         ld hl,(0x8ffc)
                           .data:0000c2c4 eb                               ex de,hl
                           .data:0000c2c5 c3 50 c0                         jp 0xc050
                           .data:0000c2c8 21 eb 8f                         ld hl,0x8feb
                           .data:0000c2cb 36 01                            ld (hl),0x01
                           .data:0000c2cd cd 83 c2                         call 0xc283
                           .data:0000c2d0 cd 60 c2                         call 0xc260
                           .data:0000c2d3 af                               xor a
                           .data:0000c2d4 32 ea 8f                         ld (0x8fea),a
                           .data:0000c2d7 cd b1 c1                         call 0xc1b1
                           .data:0000c2da cd b0 c2                         call 0xc2b0
                           .data:0000c2dd fe 80                            cp 0x80
                           .data:0000c2df ca 4e c3                         jp z,0xc34e
                           .data:0000c2e2 cd 70 c1                         call 0xc170
                           .data:0000c2e5 fe 81                            cp 0x81
                           .data:0000c2e7 d2 0a c3                         jp nc,0xc30a
                           .data:0000c2ea 32 f0 8f                         ld (0x8ff0),a
                           .data:0000c2ed 32 ef 8f                         ld (0x8fef),a
                           .data:0000c2f0 fe 21                            cp 0x21
                           .data:0000c2f2 d8                               ret c
                           .data:0000c2f3 f5                               push af
                           .data:0000c2f4 3a f4 8f                         ld a,(0x8ff4)
                           .data:0000c2f7 b7                               or a
                           .data:0000c2f8 2a df 8f                         ld hl,(0x8fdf)
                           .data:0000c2fb e9                               jp (hl)
                           .data:0000c2fc ca 67 c3                         jp z,0xc367
                           .data:0000c2ff 3a ec 8f                         ld a,(0x8fec)
                           .data:0000c302 fe 3a                            cp 0x3a
                           .data:0000c304 ca 60 c3                         jp z,0xc360
                           .data:0000c307 f1                               pop af
                           .data:0000c308 c9                               ret
                           .data:0000c309 00                               nop
                           .data:0000c30a c2 23 c3                         jp nz,0xc323
                           .data:0000c30d 3a ec 8f                         ld a,(0x8fec)
                           .data:0000c310 c6 80                            add a,0x80
                           .data:0000c312 32 ec 8f                         ld (0x8fec),a
                           .data:0000c315 32 f4 8f                         ld (0x8ff4),a
                           .data:0000c318 fe 3a                            cp 0x3a
                           .data:0000c31a ca 31 c3                         jp z,0xc331
                           .data:0000c31d cd a0 c1                         call 0xc1a0
                           .data:0000c320 c3 c8 c2                         jp 0xc2c8
                           .data:0000c323 fe 8c                            cp 0x8c
                           .data:0000c325 c2 57 c3                         jp nz,0xc357
                           .data:0000c328 21 ff ff                         ld hl,0xffff
                           .data:0000c32b 22 fa 8f                         ld (0x8ffa),hl
                           .data:0000c32e c3 c8 c2                         jp 0xc2c8
                           .data:0000c331 cd ab c1                         call 0xc1ab
                           .data:0000c334 c3 c8 c2                         jp 0xc2c8
                           .data:0000c337 c5                               push bc
                           .data:0000c338 d5                               push de
                           .data:0000c339 e5                               push hl
                           .data:0000c33a cd c8 c2                         call 0xc2c8
                           .data:0000c33d e1                               pop hl
                           .data:0000c33e d1                               pop de
                           .data:0000c33f c1                               pop bc
                           .data:0000c340 c9                               ret
                           .data:0000c341 0e 10                            ld c,0x10
                           .data:0000c343 06 ff                            ld b,0xff
                           .data:0000c345 cd 90 c1                         call 0xc190
                           .data:0000c348 0d                               dec c
                           .data:0000c349 c2 43 c3                         jp nz,0xc343
                           .data:0000c34c c1                               pop bc
                           .data:0000c34d c9                               ret
                           .data:0000c34e 32 ef 8f                         ld (0x8fef),a
                           .data:0000c351 3a f0 8f                         ld a,(0x8ff0)
                           .data:0000c354 c3 f0 c2                         jp 0xc2f0
                           .data:0000c357 fe 8b                            cp 0x8b
                           .data:0000c359 c0                               ret nz
                           .data:0000c35a 21 00 00                         ld hl,0x0000
                           .data:0000c35d c3 2b c3                         jp 0xc32b
                           .data:0000c360 f1                               pop af
                           .data:0000c361 fe 40                            cp 0x40
                           .data:0000c363 d8                               ret c
                           .data:0000c364 d6 20                            sub 0x20
                           .data:0000c366 c9                               ret
                           .data:0000c367 f1                               pop af
                           .data:0000c368 fe 40                            cp 0x40
                           .data:0000c36a d2 c8 c2                         jp nc,0xc2c8
                           .data:0000c36d ee 10                            xor 0x10
                           .data:0000c36f c9                               ret
                           .data:0000c370 00                               nop
                           .data:0000c371 00                               nop
                           .data:0000c372 00                               nop
                           .data:0000c373 00                               nop
                           .data:0000c374 00                               nop
                           .data:0000c375 00                               nop
                           .data:0000c376 00                               nop
                           .data:0000c377 c5                               push bc
                           .data:0000c378 d5                               push de
                           .data:0000c379 0e 00                            ld c,0x00
                           .data:0000c37b 57                               ld d,a
                           .data:0000c37c 3a 01 ff                         ld a,(0xff01)
                           .data:0000c37f e6 01                            and 0x01
                           .data:0000c381 5f                               ld e,a
                           .data:0000c382 79                               ld a,c
                           .data:0000c383 e6 7f                            and 0x7f
                           .data:0000c385 07                               rlca
                           .data:0000c386 4f                               ld c,a
                           .data:0000c387 3a 01 ff                         ld a,(0xff01)
                           .data:0000c38a fe 80                            cp 0x80
                           .data:0000c38c da 5a c4                         jp c,0xc45a
                           .data:0000c38f e6 01                            and 0x01
                           .data:0000c391 bb                               cp e
                           .data:0000c392 ca 87 c3                         jp z,0xc387
                           .data:0000c395 b1                               or c
                           .data:0000c396 4f                               ld c,a
                           .data:0000c397 cd c9 c3                         call 0xc3c9
                           .data:0000c39a 3a 01 ff                         ld a,(0xff01)
                           .data:0000c39d e6 01                            and 0x01
                           .data:0000c39f 5f                               ld e,a
                           .data:0000c3a0 7a                               ld a,d
                           .data:0000c3a1 b7                               or a
                           .data:0000c3a2 f2 be c3                         jp p,0xc3be
                           .data:0000c3a5 79                               ld a,c
                           .data:0000c3a6 fe e6                            cp 0xe6
                           .data:0000c3a8 c2 b2 c3                         jp nz,0xc3b2
                           .data:0000c3ab af                               xor a
                           .data:0000c3ac 32 f3 8f                         ld (0x8ff3),a
                           .data:0000c3af c3 bc c3                         jp 0xc3bc
                           .data:0000c3b2 fe 19                            cp 0x19
                           .data:0000c3b4 c2 82 c3                         jp nz,0xc382
                           .data:0000c3b7 3e ff                            ld a,0xff
                           .data:0000c3b9 32 f3 8f                         ld (0x8ff3),a
                           .data:0000c3bc 16 09                            ld d,0x09
                           .data:0000c3be 15                               dec d
                           .data:0000c3bf c2 82 c3                         jp nz,0xc382
                           .data:0000c3c2 3a f3 8f                         ld a,(0x8ff3)
                           .data:0000c3c5 a9                               xor c
                           .data:0000c3c6 d1                               pop de
                           .data:0000c3c7 c1                               pop bc
                           .data:0000c3c8 c9                               ret
                           .data:0000c3c9 3a ff 8f                         ld a,(0x8fff)
                           .data:0000c3cc 47                               ld b,a
                           .data:0000c3cd c3 90 c1                         jp 0xc190
                           .data:0000c3d0 c5                               push bc
                           .data:0000c3d1 d5                               push de
                           .data:0000c3d2 f5                               push af
                           .data:0000c3d3 57                               ld d,a
                           .data:0000c3d4 0e 08                            ld c,0x08
                           .data:0000c3d6 7a                               ld a,d
                           .data:0000c3d7 07                               rlca
                           .data:0000c3d8 57                               ld d,a
                           .data:0000c3d9 e6 01                            and 0x01
                           .data:0000c3db f6 0e                            or 0x0e
                           .data:0000c3dd 32 03 ff                         ld (0xff03),a
                           .data:0000c3e0 5f                               ld e,a
                           .data:0000c3e1 cd 88 c4                         call 0xc488
                           .data:0000c3e4 7b                               ld a,e
                           .data:0000c3e5 ee 01                            xor 0x01
                           .data:0000c3e7 32 03 ff                         ld (0xff03),a
                           .data:0000c3ea cd 88 c4                         call 0xc488
                           .data:0000c3ed 0d                               dec c
                           .data:0000c3ee c2 d6 c3                         jp nz,0xc3d6
                           .data:0000c3f1 f1                               pop af
                           .data:0000c3f2 d1                               pop de
                           .data:0000c3f3 c1                               pop bc
                           .data:0000c3f4 c9                               ret
                           .data:0000c3f5 00                               nop
                           .data:0000c3f6 00                               nop
                           .data:0000c3f7 00                               nop
                           .data:0000c3f8 00                               nop
                           .data:0000c3f9 3e ff                            ld a,0xff
                           .data:0000c3fb cd 77 c3                         call 0xc377
                           .data:0000c3fe 6f                               ld l,a
                           .data:0000c3ff 3e 08                            ld a,0x08
                           .data:0000c401 cd 77 c3                         call 0xc377
                           .data:0000c404 67                               ld h,a
                           .data:0000c405 22 e3 8f                         ld (0x8fe3),hl
                           .data:0000c408 3e 08                            ld a,0x08
                           .data:0000c40a cd 77 c3                         call 0xc377
                           .data:0000c40d 5f                               ld e,a
                           .data:0000c40e 3e 08                            ld a,0x08
                           .data:0000c410 cd 77 c3                         call 0xc377
                           .data:0000c413 57                               ld d,a
                           .data:0000c414 3e 08                            ld a,0x08
                           .data:0000c416 cd 77 c3                         call 0xc377
                           .data:0000c419 77                               ld (hl),a
                           .data:0000c41a cd 27 c4                         call 0xc427
                           .data:0000c41d 23                               inc hl
                           .data:0000c41e c2 14 c4                         jp nz,0xc414
                           .data:0000c421 c9                               ret
                           .data:0000c422 3e ff                            ld a,0xff
                           .data:0000c424 c3 16 c4                         jp 0xc416
                           .data:0000c427 7c                               ld a,h
                           .data:0000c428 ba                               cp d
                           .data:0000c429 c0                               ret nz
                           .data:0000c42a 7d                               ld a,l
                           .data:0000c42b bb                               cp e
                           .data:0000c42c c9                               ret
                           .data:0000c42d 7e                               ld a,(hl)
                           .data:0000c42e 02                               ld (bc),a
                           .data:0000c42f 23                               inc hl
                           .data:0000c430 03                               inc bc
                           .data:0000c431 cd 27 c4                         call 0xc427
                           .data:0000c434 c2 2d c4                         jp nz,0xc42d
                           .data:0000c437 c9                               ret
                           .data:0000c438 7e                               ld a,(hl)
                           .data:0000c439 4f                               ld c,a
                           .data:0000c43a fe 00                            cp 0x00
                           .data:0000c43c c8                               ret z
                           .data:0000c43d cd 37 c0                         call 0xc037
                           .data:0000c440 23                               inc hl
                           .data:0000c441 c3 38 c4                         jp 0xc438
                           .data:0000c444 21 73 c4                         ld hl,0xc473
                           .data:0000c447 11 94 c4                         ld de,0xc494
                           .data:0000c44a 01 df 8f                         ld bc,0x8fdf
                           .data:0000c44d cd 2d c4                         call 0xc42d
                           .data:0000c450 cd 38 c4                         call 0xc438
                           .data:0000c453 00                               nop
                           .data:0000c454 00                               nop
                           .data:0000c455 00                               nop
                           .data:0000c456 00                               nop
                           .data:0000c457 00                               nop
                           .data:0000c458 00                               nop
                           .data:0000c459 00                               nop
                           .data:0000c45a 00                               nop
                           .data:0000c45b 00                               nop
                           .data:0000c45c 00                               nop
                           .data:0000c45d 00                               nop
                           .data:0000c45e 00                               nop
                           .data:0000c45f c3 00 c8                         jp 0xc800
                           .data:0000c462 fe 0a                            cp 0x0a
                           .data:0000c464 ca 6f c4                         jp z,0xc46f
                           .data:0000c467 32 ff 8f                         ld (0x8fff),a
                           .data:0000c46a 3e ff                            ld a,0xff
                           .data:0000c46c c3 79 c3                         jp 0xc379
                           .data:0000c46f 2a e1 8f                         ld hl,(0x8fe1)
                           .data:0000c472 e9                               jp (hl)
                           .data:0000c473 fc c2 00                         call m,0x00c2
                           .data:0000c476 c0                               ret nz
                           .data:0000c477 00                               nop
                           .data:0000c478 c0                               ret nz
                           .data:0000c479 c8                               ret z
                           .data:0000c47a c2 a0 18                         jp nz,0x18a0
                           .data:0000c47d 00                               nop
                           .data:0000c47e 00                               nop
                           .data:0000c47f 00                               nop
                           .data:0000c480 3a 96 c1                         ld a,(0xc196)
                           .data:0000c483 20 20                            jr nz,0xc4a5
                           .data:0000c485 5f                               ld e,a
                           .data:0000c486 20 00                            jr nz,0xc488
                           .data:0000c488 3a fe 8f                         ld a,(0x8ffe)
                           .data:0000c48b c3 cc c3                         jp 0xc3cc
                           .data:0000c48e 00                               nop
                           .data:0000c48f 00                               nop
                           .data:0000c490 00                               nop
                           .data:0000c491 00                               nop
                           .data:0000c492 28 3c                            jr z,0xc4d0
                           .data:0000c494 1f                               rra
                           .data:0000c495 20 50                            jr nz,0xc4e7
                           .data:0000c497 52                               ld d,d
                           .data:0000c498 4f                               ld c,a
                           .data:0000c499 47                               ld b,a
                           .data:0000c49a 52                               ld d,d
                           .data:0000c49b 41                               ld b,c
                           .data:0000c49c 4d                               ld c,l
                           .data:0000c49d 20 3f                            jr nz,0xc4de
                           .data:0000c49f 00                               nop
                           .data:0000c4a0 81                               add a,c
                           .data:0000c4a1 0c                               inc c
                           .data:0000c4a2 19                               add hl,de
                           .data:0000c4a3 1a                               ld a,(de)
                           .data:0000c4a4 09                               add hl,bc
                           .data:0000c4a5 1b                               dec de
                           .data:0000c4a6 20 08                            jr nz,0xc4b0
                           .data:0000c4a8 80                               add a,b
                           .data:0000c4a9 18 0a                            jr 0xc4b5
                           .data:0000c4ab 0d                               dec c
                           .data:0000c4ac 00                               nop
                           .data:0000c4ad 00                               nop
                           .data:0000c4ae 00                               nop
                           .data:0000c4af 00                               nop
                           .data:0000c4b0 71                               ld (hl),c
                           .data:0000c4b1 7e                               ld a,(hl)
                           .data:0000c4b2 73                               ld (hl),e
                           .data:0000c4b3 6d                               ld l,l
                           .data:0000c4b4 69                               ld l,c
                           .data:0000c4b5 74                               ld (hl),h
                           .data:0000c4b6 78                               ld a,b
                           .data:0000c4b7 62                               ld h,d
                           .data:0000c4b8 60                               ld h,b
                           .data:0000c4b9 2c                               inc l
                           .data:0000c4ba 2f                               cpl
                           .data:0000c4bb 7f                               ld a,a
                           .data:0000c4bc 00                               nop
                           .data:0000c4bd 00                               nop
                           .data:0000c4be 00                               nop
                           .data:0000c4bf 00                               nop
                           .data:0000c4c0 66                               ld h,(hl)
                           .data:0000c4c1 79                               ld a,c
                           .data:0000c4c2 77                               ld (hl),a
                           .data:0000c4c3 61                               ld h,c
                           .data:0000c4c4 70                               ld (hl),b
                           .data:0000c4c5 72                               ld (hl),d
                           .data:0000c4c6 6f                               ld l,a
                           .data:0000c4c7 6c                               ld l,h
                           .data:0000c4c8 64                               ld h,h
                           .data:0000c4c9 76                               halt
                           .data:0000c4ca 7c                               ld a,h
                           .data:0000c4cb 2e 00                            ld l,0x00
                           .data:0000c4cd 00                               nop
                           .data:0000c4ce 00                               nop
                           .data:0000c4cf 00                               nop
                           .data:0000c4d0 6a                               ld l,d
                           .data:0000c4d1 63                               ld h,e
                           .data:0000c4d2 75                               ld (hl),l
                           .data:0000c4d3 6b                               ld l,e
                           .data:0000c4d4 65                               ld h,l
                           .data:0000c4d5 6e                               ld l,(hl)
                           .data:0000c4d6 67                               ld h,a
                           .data:0000c4d7 7b                               ld a,e
                           .data:0000c4d8 7d                               ld a,l
                           .data:0000c4d9 7a                               ld a,d
                           .data:0000c4da 68                               ld l,b
                           .data:0000c4db 3a 00 00                         ld a,(0x0000)
                           .data:0000c4de 00                               nop
                           .data:0000c4df 00                               nop
                           .data:0000c4e0 3b                               dec sp
                           .data:0000c4e1 31 32 33                         ld sp,0x3332
                           .data:0000c4e4 34                               inc (hl)
                           .data:0000c4e5 35                               dec (hl)
                           .data:0000c4e6 36 37                            ld (hl),0x37
                           .data:0000c4e8 38 39                            jr c,0xc523
                           .data:0000c4ea 30 2d                            jr nc,0xc519
                           .data:0000c4ec 00                               nop
                           .data:0000c4ed 00                               nop
                           .data:0000c4ee 00                               nop
                           .data:0000c4ef 00                               nop
                           .data:0000c4f0 00                               nop
                           .data:0000c4f1 01 02 03                         ld bc,0x0302
                           .data:0000c4f4 04                               inc b
                           .data:0000c4f5 05                               dec b
                           .data:0000c4f6 06 07                            ld b,0x07
                           .data:0000c4f8 8a                               adc a,d
                           .data:0000c4f9 8b                               adc a,e
                           .data:0000c4fa 8c                               adc a,h
                           .data:0000c4fb 1f                               rra
                           .data:0000c4fc 00                               nop
                           .data:0000c4fd 00                               nop
                           .data:0000c4fe 00                               nop
                           .data:0000c4ff 00                               nop
                           .data:0000c500 00                               nop
                           .data:0000c501 00                               nop
                           .data:0000c502 00                               nop
                           .data:0000c503 00                               nop
                           .data:0000c504 00                               nop
                           .data:0000c505 00                               nop
                           .data:0000c506 00                               nop
                           .data:0000c507 00                               nop
                           .data:0000c508 04                               inc b
                           .data:0000c509 04                               inc b
                           .data:0000c50a 04                               inc b
                           .data:0000c50b 04                               inc b
                           .data:0000c50c 04                               inc b
                           .data:0000c50d 00                               nop
                           .data:0000c50e 04                               inc b
                           .data:0000c50f 00                               nop
                           .data:0000c510 0a                               ld a,(bc)
                           .data:0000c511 0a                               ld a,(bc)
                           .data:0000c512 0a                               ld a,(bc)
                           .data:0000c513 00                               nop
                           .data:0000c514 00                               nop
                           .data:0000c515 00                               nop
                           .data:0000c516 00                               nop
                           .data:0000c517 00                               nop
                           .data:0000c518 0a                               ld a,(bc)
                           .data:0000c519 0a                               ld a,(bc)
                           .data:0000c51a 1f                               rra
                           .data:0000c51b 0a                               ld a,(bc)
                           .data:0000c51c 1f                               rra
                           .data:0000c51d 0a                               ld a,(bc)
                           .data:0000c51e 0a                               ld a,(bc)
                           .data:0000c51f 00                               nop
                           .data:0000c520 04                               inc b
                           .data:0000c521 0f                               rrca
                           .data:0000c522 14                               inc d
                           .data:0000c523 0e 05                            ld c,0x05
                           .data:0000c525 1e 04                            ld e,0x04
                           .data:0000c527 00                               nop
                           .data:0000c528 18 19                            jr 0xc543
                           .data:0000c52a 02                               ld (bc),a
                           .data:0000c52b 04                               inc b
                           .data:0000c52c 08                               ex af,af'
                           .data:0000c52d 13                               inc de
                           .data:0000c52e 03                               inc bc
                           .data:0000c52f 00                               nop
                           .data:0000c530 04                               inc b
                           .data:0000c531 0a                               ld a,(bc)
                           .data:0000c532 0a                               ld a,(bc)
                           .data:0000c533 0c                               inc c
                           .data:0000c534 15                               dec d
                           .data:0000c535 12                               ld (de),a
                           .data:0000c536 0d                               dec c
                           .data:0000c537 00                               nop
                           .data:0000c538 06 06                            ld b,0x06
                           .data:0000c53a 02                               ld (bc),a
                           .data:0000c53b 04                               inc b
                           .data:0000c53c 00                               nop
                           .data:0000c53d 00                               nop
                           .data:0000c53e 00                               nop
                           .data:0000c53f 00                               nop
                           .data:0000c540 02                               ld (bc),a
                           .data:0000c541 04                               inc b
                           .data:0000c542 08                               ex af,af'
                           .data:0000c543 08                               ex af,af'
                           .data:0000c544 08                               ex af,af'
                           .data:0000c545 04                               inc b
                           .data:0000c546 02                               ld (bc),a
                           .data:0000c547 00                               nop
                           .data:0000c548 08                               ex af,af'
                           .data:0000c549 04                               inc b
                           .data:0000c54a 02                               ld (bc),a
                           .data:0000c54b 02                               ld (bc),a
                           .data:0000c54c 02                               ld (bc),a
                           .data:0000c54d 04                               inc b
                           .data:0000c54e 08                               ex af,af'
                           .data:0000c54f 00                               nop
                           .data:0000c550 00                               nop
                           .data:0000c551 04                               inc b
                           .data:0000c552 15                               dec d
                           .data:0000c553 0e 15                            ld c,0x15
                           .data:0000c555 04                               inc b
                           .data:0000c556 00                               nop
                           .data:0000c557 00                               nop
                           .data:0000c558 00                               nop
                           .data:0000c559 04                               inc b
                           .data:0000c55a 04                               inc b
                           .data:0000c55b 1f                               rra
                           .data:0000c55c 04                               inc b
                           .data:0000c55d 04                               inc b
                           .data:0000c55e 00                               nop
                           .data:0000c55f 00                               nop
                           .data:0000c560 00                               nop
                           .data:0000c561 00                               nop
                           .data:0000c562 00                               nop
                           .data:0000c563 0c                               inc c
                           .data:0000c564 0c                               inc c
                           .data:0000c565 04                               inc b
                           .data:0000c566 08                               ex af,af'
                           .data:0000c567 00                               nop
                           .data:0000c568 00                               nop
                           .data:0000c569 00                               nop
                           .data:0000c56a 00                               nop
                           .data:0000c56b 1f                               rra
                           .data:0000c56c 00                               nop
                           .data:0000c56d 00                               nop
                           .data:0000c56e 00                               nop
                           .data:0000c56f 00                               nop
                           .data:0000c570 00                               nop
                           .data:0000c571 00                               nop
                           .data:0000c572 00                               nop
                           .data:0000c573 00                               nop
                           .data:0000c574 00                               nop
                           .data:0000c575 0c                               inc c
                           .data:0000c576 0c                               inc c
                           .data:0000c577 00                               nop
                           .data:0000c578 00                               nop
                           .data:0000c579 01 02 04                         ld bc,0x0402
                           .data:0000c57c 08                               ex af,af'
                           .data:0000c57d 10 00                            djnz 0xc57f
                           .data:0000c57f 00                               nop
                           .data:0000c580 0e 11                            ld c,0x11
                           .data:0000c582 13                               inc de
                           .data:0000c583 15                               dec d
                           .data:0000c584 19                               add hl,de
                           .data:0000c585 11 0e 00                         ld de,0x000e
                           .data:0000c588 04                               inc b
                           .data:0000c589 0c                               inc c
                           .data:0000c58a 04                               inc b
                           .data:0000c58b 04                               inc b
                           .data:0000c58c 04                               inc b
                           .data:0000c58d 04                               inc b
                           .data:0000c58e 0e 00                            ld c,0x00
                           .data:0000c590 0e 11                            ld c,0x11
                           .data:0000c592 01 06 08                         ld bc,0x0806
                           .data:0000c595 10 1f                            djnz 0xc5b6
                           .data:0000c597 00                               nop
                           .data:0000c598 1f                               rra
                           .data:0000c599 01 02 06                         ld bc,0x0602
                           .data:0000c59c 01 11 0e                         ld bc,0x0e11
                           .data:0000c59f 00                               nop
                           .data:0000c5a0 02                               ld (bc),a
                           .data:0000c5a1 06 0a                            ld b,0x0a
                           .data:0000c5a3 12                               ld (de),a
                           .data:0000c5a4 1f                               rra
                           .data:0000c5a5 02                               ld (bc),a
                           .data:0000c5a6 02                               ld (bc),a
                           .data:0000c5a7 00                               nop
                           .data:0000c5a8 1f                               rra
                           .data:0000c5a9 10 1e                            djnz 0xc5c9
                           .data:0000c5ab 01 01 11                         ld bc,0x1101
                           .data:0000c5ae 0e 00                            ld c,0x00
                           .data:0000c5b0 07                               rlca
                           .data:0000c5b1 08                               ex af,af'
                           .data:0000c5b2 10 1e                            djnz 0xc5d2
                           .data:0000c5b4 11 11 0e                         ld de,0x0e11
                           .data:0000c5b7 00                               nop
                           .data:0000c5b8 1f                               rra
                           .data:0000c5b9 01 02 04                         ld bc,0x0402
                           .data:0000c5bc 08                               ex af,af'
                           .data:0000c5bd 08                               ex af,af'
                           .data:0000c5be 08                               ex af,af'
                           .data:0000c5bf 00                               nop
                           .data:0000c5c0 0e 11                            ld c,0x11
                           .data:0000c5c2 11 0e 11                         ld de,0x110e
                           .data:0000c5c5 11 0e 00                         ld de,0x000e
                           .data:0000c5c8 0e 11                            ld c,0x11
                           .data:0000c5ca 11 0f 01                         ld de,0x010f
                           .data:0000c5cd 02                               ld (bc),a
                           .data:0000c5ce 1c                               inc e
                           .data:0000c5cf 00                               nop
                           .data:0000c5d0 00                               nop
                           .data:0000c5d1 0c                               inc c
                           .data:0000c5d2 0c                               inc c
                           .data:0000c5d3 00                               nop
                           .data:0000c5d4 00                               nop
                           .data:0000c5d5 0c                               inc c
                           .data:0000c5d6 0c                               inc c
                           .data:0000c5d7 00                               nop
                           .data:0000c5d8 0c                               inc c
                           .data:0000c5d9 0c                               inc c
                           .data:0000c5da 00                               nop
                           .data:0000c5db 0c                               inc c
                           .data:0000c5dc 0c                               inc c
                           .data:0000c5dd 04                               inc b
                           .data:0000c5de 08                               ex af,af'
                           .data:0000c5df 00                               nop
                           .data:0000c5e0 02                               ld (bc),a
                           .data:0000c5e1 04                               inc b
                           .data:0000c5e2 08                               ex af,af'
                           .data:0000c5e3 10 08                            djnz 0xc5ed
                           .data:0000c5e5 04                               inc b
                           .data:0000c5e6 02                               ld (bc),a
                           .data:0000c5e7 00                               nop
                           .data:0000c5e8 00                               nop
                           .data:0000c5e9 00                               nop
                           .data:0000c5ea 1f                               rra
                           .data:0000c5eb 00                               nop
                           .data:0000c5ec 1f                               rra
                           .data:0000c5ed 00                               nop
                           .data:0000c5ee 00                               nop
                           .data:0000c5ef 00                               nop
                           .data:0000c5f0 08                               ex af,af'
                           .data:0000c5f1 04                               inc b
                           .data:0000c5f2 02                               ld (bc),a
                           .data:0000c5f3 01 02 04                         ld bc,0x0402
                           .data:0000c5f6 08                               ex af,af'
                           .data:0000c5f7 00                               nop
                           .data:0000c5f8 0e 11                            ld c,0x11
                           .data:0000c5fa 01 02 04                         ld bc,0x0402
                           .data:0000c5fd 00                               nop
                           .data:0000c5fe 04                               inc b
                           .data:0000c5ff 00                               nop
                           .data:0000c600 0e 11                            ld c,0x11
                           .data:0000c602 13                               inc de
                           .data:0000c603 15                               dec d
                           .data:0000c604 17                               rla
                           .data:0000c605 10 0e                            djnz 0xc615
                           .data:0000c607 00                               nop
                           .data:0000c608 04                               inc b
                           .data:0000c609 0a                               ld a,(bc)
                           .data:0000c60a 11 11 1f                         ld de,0x1f11
                           .data:0000c60d 11 11 00                         ld de,0x0011
                           .data:0000c610 1e 11                            ld e,0x11
                           .data:0000c612 11 1e 11                         ld de,0x111e
                           .data:0000c615 11 1e 00                         ld de,0x001e
                           .data:0000c618 0e 11                            ld c,0x11
                           .data:0000c61a 10 10                            djnz 0xc62c
                           .data:0000c61c 10 11                            djnz 0xc62f
                           .data:0000c61e 0e 00                            ld c,0x00
                           .data:0000c620 1e 09                            ld e,0x09
                           .data:0000c622 09                               add hl,bc
                           .data:0000c623 09                               add hl,bc
                           .data:0000c624 09                               add hl,bc
                           .data:0000c625 09                               add hl,bc
                           .data:0000c626 1e 00                            ld e,0x00
                           .data:0000c628 1f                               rra
                           .data:0000c629 10 10                            djnz 0xc63b
                           .data:0000c62b 1e 10                            ld e,0x10
                           .data:0000c62d 10 1f                            djnz 0xc64e
                           .data:0000c62f 00                               nop
                           .data:0000c630 1f                               rra
                           .data:0000c631 10 10                            djnz 0xc643
                           .data:0000c633 1e 10                            ld e,0x10
                           .data:0000c635 10 10                            djnz 0xc647
                           .data:0000c637 00                               nop
                           .data:0000c638 0e 11                            ld c,0x11
                           .data:0000c63a 10 10                            djnz 0xc64c
                           .data:0000c63c 13                               inc de
                           .data:0000c63d 11 0f 00                         ld de,0x000f
                           .data:0000c640 11 11 11                         ld de,0x1111
                           .data:0000c643 1f                               rra
                           .data:0000c644 11 11 11                         ld de,0x1111
                           .data:0000c647 00                               nop
                           .data:0000c648 0e 04                            ld c,0x04
                           .data:0000c64a 04                               inc b
                           .data:0000c64b 04                               inc b
                           .data:0000c64c 04                               inc b
                           .data:0000c64d 04                               inc b
                           .data:0000c64e 0e 00                            ld c,0x00
                           .data:0000c650 01 01 01                         ld bc,0x0101
                           .data:0000c653 01 11 11                         ld bc,0x1111
                           .data:0000c656 0e 00                            ld c,0x00
                           .data:0000c658 11 12 14                         ld de,0x1412
                           .data:0000c65b 18 14                            jr 0xc671
                           .data:0000c65d 12                               ld (de),a
                           .data:0000c65e 11 00 10                         ld de,0x1000
                           .data:0000c661 10 10                            djnz 0xc673
                           .data:0000c663 10 10                            djnz 0xc675
                           .data:0000c665 11 1f 00                         ld de,0x001f
                           .data:0000c668 11 1b 15                         ld de,0x151b
                           .data:0000c66b 15                               dec d
                           .data:0000c66c 11 11 11                         ld de,0x1111
                           .data:0000c66f 00                               nop
                           .data:0000c670 11 11 19                         ld de,0x1911
                           .data:0000c673 15                               dec d
                           .data:0000c674 13                               inc de
                           .data:0000c675 11 11 00                         ld de,0x0011
                           .data:0000c678 0e 11                            ld c,0x11
                           .data:0000c67a 11 11 11                         ld de,0x1111
                           .data:0000c67d 11 0e 00                         ld de,0x000e
                           .data:0000c680 1e 11                            ld e,0x11
                           .data:0000c682 11 1e 10                         ld de,0x101e
                           .data:0000c685 10 10                            djnz 0xc697
                           .data:0000c687 00                               nop
                           .data:0000c688 0e 11                            ld c,0x11
                           .data:0000c68a 11 11 15                         ld de,0x1511
                           .data:0000c68d 12                               ld (de),a
                           .data:0000c68e 0d                               dec c
                           .data:0000c68f 00                               nop
                           .data:0000c690 1e 11                            ld e,0x11
                           .data:0000c692 11 1e 14                         ld de,0x141e
                           .data:0000c695 12                               ld (de),a
                           .data:0000c696 11 00 0e                         ld de,0x0e00
                           .data:0000c699 11 10 0e                         ld de,0x0e10
                           .data:0000c69c 01 11 0e                         ld bc,0x0e11
                           .data:0000c69f 00                               nop
                           .data:0000c6a0 1f                               rra
                           .data:0000c6a1 04                               inc b
                           .data:0000c6a2 04                               inc b
                           .data:0000c6a3 04                               inc b
                           .data:0000c6a4 04                               inc b
                           .data:0000c6a5 04                               inc b
                           .data:0000c6a6 04                               inc b
                           .data:0000c6a7 00                               nop
                           .data:0000c6a8 11 11 11                         ld de,0x1111
                           .data:0000c6ab 11 11 11                         ld de,0x1111
                           .data:0000c6ae 0e 00                            ld c,0x00
                           .data:0000c6b0 11 11 11                         ld de,0x1111
                           .data:0000c6b3 0a                               ld a,(bc)
                           .data:0000c6b4 0a                               ld a,(bc)
                           .data:0000c6b5 04                               inc b
                           .data:0000c6b6 04                               inc b
                           .data:0000c6b7 00                               nop
                           .data:0000c6b8 11 11 11                         ld de,0x1111
                           .data:0000c6bb 15                               dec d
                           .data:0000c6bc 15                               dec d
                           .data:0000c6bd 15                               dec d
                           .data:0000c6be 0a                               ld a,(bc)
                           .data:0000c6bf 00                               nop
                           .data:0000c6c0 11 11 0a                         ld de,0x0a11
                           .data:0000c6c3 04                               inc b
                           .data:0000c6c4 0a                               ld a,(bc)
                           .data:0000c6c5 11 11 00                         ld de,0x0011
                           .data:0000c6c8 11 11 0a                         ld de,0x0a11
                           .data:0000c6cb 04                               inc b
                           .data:0000c6cc 04                               inc b
                           .data:0000c6cd 04                               inc b
                           .data:0000c6ce 04                               inc b
                           .data:0000c6cf 00                               nop
                           .data:0000c6d0 1f                               rra
                           .data:0000c6d1 01 02 0e                         ld bc,0x0e02
                           .data:0000c6d4 08                               ex af,af'
                           .data:0000c6d5 10 1f                            djnz 0xc6f6
                           .data:0000c6d7 00                               nop
                           .data:0000c6d8 0e 08                            ld c,0x08
                           .data:0000c6da 08                               ex af,af'
                           .data:0000c6db 08                               ex af,af'
                           .data:0000c6dc 08                               ex af,af'
                           .data:0000c6dd 08                               ex af,af'
                           .data:0000c6de 0e 00                            ld c,0x00
                           .data:0000c6e0 00                               nop
                           .data:0000c6e1 10 08                            djnz 0xc6eb
                           .data:0000c6e3 04                               inc b
                           .data:0000c6e4 02                               ld (bc),a
                           .data:0000c6e5 01 00 00                         ld bc,0x0000
                           .data:0000c6e8 0e 02                            ld c,0x02
                           .data:0000c6ea 02                               ld (bc),a
                           .data:0000c6eb 02                               ld (bc),a
                           .data:0000c6ec 02                               ld (bc),a
                           .data:0000c6ed 02                               ld (bc),a
                           .data:0000c6ee 0e 00                            ld c,0x00
                           .data:0000c6f0 0e 11                            ld c,0x11
                           .data:0000c6f2 00                               nop
                           .data:0000c6f3 00                               nop
                           .data:0000c6f4 00                               nop
                           .data:0000c6f5 00                               nop
                           .data:0000c6f6 00                               nop
                           .data:0000c6f7 00                               nop
                           .data:0000c6f8 00                               nop
                           .data:0000c6f9 00                               nop
                           .data:0000c6fa 00                               nop
                           .data:0000c6fb 00                               nop
                           .data:0000c6fc 00                               nop
                           .data:0000c6fd 00                               nop
                           .data:0000c6fe 00                               nop
                           .data:0000c6ff 1f                               rra
                           .data:0000c700 12                               ld (de),a
                           .data:0000c701 15                               dec d
                           .data:0000c702 15                               dec d
                           .data:0000c703 1d                               dec e
                           .data:0000c704 15                               dec d
                           .data:0000c705 15                               dec d
                           .data:0000c706 12                               ld (de),a
                           .data:0000c707 00                               nop
                           .data:0000c708 04                               inc b
                           .data:0000c709 0a                               ld a,(bc)
                           .data:0000c70a 11 11 1f                         ld de,0x1f11
                           .data:0000c70d 11 11 00                         ld de,0x0011
                           .data:0000c710 1f                               rra
                           .data:0000c711 10 10                            djnz 0xc723
                           .data:0000c713 1e 11                            ld e,0x11
                           .data:0000c715 11 1e 00                         ld de,0x001e
                           .data:0000c718 12                               ld (de),a
                           .data:0000c719 12                               ld (de),a
                           .data:0000c71a 12                               ld (de),a
                           .data:0000c71b 12                               ld (de),a
                           .data:0000c71c 12                               ld (de),a
                           .data:0000c71d 1f                               rra
                           .data:0000c71e 01 00 06                         ld bc,0x0600
                           .data:0000c721 0a                               ld a,(bc)
                           .data:0000c722 0a                               ld a,(bc)
                           .data:0000c723 0a                               ld a,(bc)
                           .data:0000c724 0a                               ld a,(bc)
                           .data:0000c725 1f                               rra
                           .data:0000c726 11 00 1f                         ld de,0x1f00
                           .data:0000c729 10 10                            djnz 0xc73b
                           .data:0000c72b 1e 10                            ld e,0x10
                           .data:0000c72d 10 1f                            djnz 0xc74e
                           .data:0000c72f 00                               nop
                           .data:0000c730 04                               inc b
                           .data:0000c731 1f                               rra
                           .data:0000c732 15                               dec d
                           .data:0000c733 15                               dec d
                           .data:0000c734 1f                               rra
                           .data:0000c735 04                               inc b
                           .data:0000c736 04                               inc b
                           .data:0000c737 00                               nop
                           .data:0000c738 1f                               rra
                           .data:0000c739 11 10 10                         ld de,0x1010
                           .data:0000c73c 10 10                            djnz 0xc74e
                           .data:0000c73e 10 00                            djnz 0xc740
                           .data:0000c740 11 11 0a                         ld de,0x0a11
                           .data:0000c743 04                               inc b
                           .data:0000c744 0a                               ld a,(bc)
                           .data:0000c745 11 11 00                         ld de,0x0011
                           .data:0000c748 11 11 13                         ld de,0x1311
                           .data:0000c74b 15                               dec d
                           .data:0000c74c 19                               add hl,de
                           .data:0000c74d 11 11 00                         ld de,0x0011
                           .data:0000c750 15                               dec d
                           .data:0000c751 11 13 15                         ld de,0x1513
                           .data:0000c754 19                               add hl,de
                           .data:0000c755 11 11 00                         ld de,0x0011
                           .data:0000c758 11 12 14                         ld de,0x1412
                           .data:0000c75b 18 14                            jr 0xc771
                           .data:0000c75d 12                               ld (de),a
                           .data:0000c75e 11 00 07                         ld de,0x0700
                           .data:0000c761 09                               add hl,bc
                           .data:0000c762 09                               add hl,bc
                           .data:0000c763 09                               add hl,bc
                           .data:0000c764 09                               add hl,bc
                           .data:0000c765 09                               add hl,bc
                           .data:0000c766 19                               add hl,de
                           .data:0000c767 00                               nop
                           .data:0000c768 11 1b 15                         ld de,0x151b
                           .data:0000c76b 15                               dec d
                           .data:0000c76c 11 11 11                         ld de,0x1111
                           .data:0000c76f 00                               nop
                           .data:0000c770 11 11 11                         ld de,0x1111
                           .data:0000c773 1f                               rra
                           .data:0000c774 11 11 11                         ld de,0x1111
                           .data:0000c777 00                               nop
                           .data:0000c778 0e 11                            ld c,0x11
                           .data:0000c77a 11 11 11                         ld de,0x1111
                           .data:0000c77d 11 0e 00                         ld de,0x000e
                           .data:0000c780 1f                               rra
                           .data:0000c781 11 11 11                         ld de,0x1111
                           .data:0000c784 11 11 11                         ld de,0x1111
                           .data:0000c787 00                               nop
                           .data:0000c788 0f                               rrca
                           .data:0000c789 11 11 0f                         ld de,0x0f11
                           .data:0000c78c 05                               dec b
                           .data:0000c78d 09                               add hl,bc
                           .data:0000c78e 11 00 1e                         ld de,0x1e00
                           .data:0000c791 11 11 1e                         ld de,0x1e11
                           .data:0000c794 10 10                            djnz 0xc7a6
                           .data:0000c796 10 00                            djnz 0xc798
                           .data:0000c798 0e 11                            ld c,0x11
                           .data:0000c79a 10 10                            djnz 0xc7ac
                           .data:0000c79c 10 11                            djnz 0xc7af
                           .data:0000c79e 0e 00                            ld c,0x00
                           .data:0000c7a0 1f                               rra
                           .data:0000c7a1 04                               inc b
                           .data:0000c7a2 04                               inc b
                           .data:0000c7a3 04                               inc b
                           .data:0000c7a4 04                               inc b
                           .data:0000c7a5 04                               inc b
                           .data:0000c7a6 04                               inc b
                           .data:0000c7a7 00                               nop
                           .data:0000c7a8 11 11 11                         ld de,0x1111
                           .data:0000c7ab 0a                               ld a,(bc)
                           .data:0000c7ac 04                               inc b
                           .data:0000c7ad 08                               ex af,af'
                           .data:0000c7ae 10 00                            djnz 0xc7b0
                           .data:0000c7b0 11 15 15                         ld de,0x1515
                           .data:0000c7b3 0e 15                            ld c,0x15
                           .data:0000c7b5 15                               dec d
                           .data:0000c7b6 11 00 1e                         ld de,0x1e00
                           .data:0000c7b9 11 11 1e                         ld de,0x1e11
                           .data:0000c7bc 11 11 1e                         ld de,0x1e11
                           .data:0000c7bf 00                               nop
                           .data:0000c7c0 10 10                            djnz 0xc7d2
                           .data:0000c7c2 10 1e                            djnz 0xc7e2
                           .data:0000c7c4 11 11 1e                         ld de,0x1e11
                           .data:0000c7c7 00                               nop
                           .data:0000c7c8 11 11 11                         ld de,0x1111
                           .data:0000c7cb 19                               add hl,de
                           .data:0000c7cc 15                               dec d
                           .data:0000c7cd 15                               dec d
                           .data:0000c7ce 19                               add hl,de
                           .data:0000c7cf 00                               nop
                           .data:0000c7d0 0e 11                            ld c,0x11
                           .data:0000c7d2 01 06 01                         ld bc,0x0106
                           .data:0000c7d5 11 0e 00                         ld de,0x000e
                           .data:0000c7d8 11 15 15                         ld de,0x1515
                           .data:0000c7db 15                               dec d
                           .data:0000c7dc 15                               dec d
                           .data:0000c7dd 15                               dec d
                           .data:0000c7de 1f                               rra
                           .data:0000c7df 00                               nop
                           .data:0000c7e0 0e 11                            ld c,0x11
                           .data:0000c7e2 01 07 01                         ld bc,0x0107
                           .data:0000c7e5 11 0e 00                         ld de,0x000e
                           .data:0000c7e8 15                               dec d
                           .data:0000c7e9 15                               dec d
                           .data:0000c7ea 15                               dec d
                           .data:0000c7eb 15                               dec d
                           .data:0000c7ec 15                               dec d
                           .data:0000c7ed 1f                               rra
                           .data:0000c7ee 01 00 11                         ld bc,0x1100
                           .data:0000c7f1 11 11 1f                         ld de,0x1f11
                           .data:0000c7f4 01 01 01                         ld bc,0x0101
                           .data:0000c7f7 00                               nop
                           .data:0000c7f8 3f                               ccf
                           .data:0000c7f9 3f                               ccf
                           .data:0000c7fa 3f                               ccf
                           .data:0000c7fb 3f                               ccf
                           .data:0000c7fc 3f                               ccf
                           .data:0000c7fd 3f                               ccf
                           .data:0000c7fe 3f                               ccf
                           .data:0000c7ff 3f                               ccf
                           .data:0000c800 c3 42 c8                         jp 0xc842
                           .data:0000c803 c3 a5 8f                         jp 0x8fa5
                           .data:0000c806 c3 77 c3                         jp 0xc377
                           .data:0000c809 c3 a8 8f                         jp 0x8fa8
                           .data:0000c80c c3 d0 c3                         jp 0xc3d0
                           .data:0000c80f c3 c0 cb                         jp 0xcbc0
                           .data:0000c812 c3 ca ce                         jp 0xceca
                           .data:0000c815 c3 7c ca                         jp 0xca7c
                           .data:0000c818 c3 4d ca                         jp 0xca4d
                           .data:0000c81b c3 cb d0                         jp 0xd0cb
                           .data:0000c81e c3 64 c9                         jp 0xc964
                           .data:0000c821 c3 19 cb                         jp 0xcb19
                           .data:0000c824 c3 79 c8                         jp 0xc879
                           .data:0000c827 c3 88 c8                         jp 0xc888
                           .data:0000c82a c3 05 ca                         jp 0xca05
                           .data:0000c82d c3 2d cd                         jp 0xcd2d
                           .data:0000c830 c3 75 c8                         jp 0xc875
                           .data:0000c833 c3 71 c8                         jp 0xc871
                           .data:0000c836 c3 e3 d0                         jp 0xd0e3
                           .data:0000c839 c3 9b cb                         jp 0xcb9b
                           .data:0000c83c c3 6f cb                         jp 0xcb6f
                           .data:0000c83f c3 ae c8                         jp 0xc8ae
                           .data:0000c842 3e 93                            ld a,0x93
                           .data:0000c844 32 03 ff                         ld (0xff03),a
                           .data:0000c847 31 50 8f                         ld sp,0x8f50
                           .data:0000c84a 21 50 8f                         ld hl,0x8f50
                           .data:0000c84d 22 50 8f                         ld (0x8f50),hl
                           .data:0000c850 cd fb d0                         call 0xd0fb
                           .data:0000c853 cd 39 ce                         call 0xce39
                           .data:0000c856 21 cc cd                         ld hl,0xcdcc
                           .data:0000c859 cd 4d ca                         call 0xca4d
                           .data:0000c85c c3 6f cb                         jp 0xcb6f
                           .data:0000c85f e5                               push hl
                           .data:0000c860 19                               add hl,de
                           .data:0000c861 cd 2a cd                         call 0xcd2a
                           .data:0000c864 7b                               ld a,e
                           .data:0000c865 2f                               cpl
                           .data:0000c866 5f                               ld e,a
                           .data:0000c867 7a                               ld a,d
                           .data:0000c868 2f                               cpl
                           .data:0000c869 57                               ld d,a
                           .data:0000c86a 13                               inc de
                           .data:0000c86b e1                               pop hl
                           .data:0000c86c 19                               add hl,de
                           .data:0000c86d cd 2d cd                         call 0xcd2d
                           .data:0000c870 c9                               ret
                           .data:0000c871 22 50 8f                         ld (0x8f50),hl
                           .data:0000c874 c9                               ret
                           .data:0000c875 2a 50 8f                         ld hl,(0x8f50)
                           .data:0000c878 c9                               ret
                           .data:0000c879 7d                               ld a,l
                           .data:0000c87a b4                               or h
                           .data:0000c87b b2                               or d
                           .data:0000c87c b3                               or e
                           .data:0000c87d ca 02 c9                         jp z,0xc902
                           .data:0000c880 3e ff                            ld a,0xff
                           .data:0000c882 cd 16 c4                         call 0xc416
                           .data:0000c885 c3 0a c9                         jp 0xc90a
                           .data:0000c888 e5                               push hl
                           .data:0000c889 d5                               push de
                           .data:0000c88a c5                               push bc
                           .data:0000c88b cd 05 ca                         call 0xca05
                           .data:0000c88e 60                               ld h,b
                           .data:0000c88f 69                               ld l,c
                           .data:0000c890 22 52 8f                         ld (0x8f52),hl
                           .data:0000c893 c1                               pop bc
                           .data:0000c894 79                               ld a,c
                           .data:0000c895 b7                               or a
                           .data:0000c896 ca b6 c9                         jp z,0xc9b6
                           .data:0000c899 32 fe 8f                         ld (0x8ffe),a
                           .data:0000c89c c3 b6 c9                         jp 0xc9b6
                           .data:0000c89f 21 66 ca                         ld hl,0xca66
                           .data:0000c8a2 cd 4d ca                         call 0xca4d
                           .data:0000c8a5 11 58 8f                         ld de,0x8f58
                           .data:0000c8a8 3e 14                            ld a,0x14
                           .data:0000c8aa 12                               ld (de),a
                           .data:0000c8ab cd c0 cb                         call 0xcbc0
                           .data:0000c8ae 3e 92                            ld a,0x92
                           .data:0000c8b0 32 03 ff                         ld (0xff03),a
                           .data:0000c8b3 af                               xor a
                           .data:0000c8b4 32 f3 8f                         ld (0x8ff3),a
                           .data:0000c8b7 3e ff                            ld a,0xff
                           .data:0000c8b9 cd 77 c3                         call 0xc377
                           .data:0000c8bc fe d9                            cp 0xd9
                           .data:0000c8be c2 b3 c8                         jp nz,0xc8b3
                           .data:0000c8c1 0e 02                            ld c,0x02
                           .data:0000c8c3 3e 08                            ld a,0x08
                           .data:0000c8c5 cd 77 c3                         call 0xc377
                           .data:0000c8c8 fe d9                            cp 0xd9
                           .data:0000c8ca c2 b3 c8                         jp nz,0xc8b3
                           .data:0000c8cd 0d                               dec c
                           .data:0000c8ce c2 c3 c8                         jp nz,0xc8c3
                           .data:0000c8d1 21 6d 8f                         ld hl,0x8f6d
                           .data:0000c8d4 11 81 8f                         ld de,0x8f81
                           .data:0000c8d7 cd 14 c4                         call 0xc414
                           .data:0000c8da 21 66 ca                         ld hl,0xca66
                           .data:0000c8dd cd 4d ca                         call 0xca4d
                           .data:0000c8e0 21 6d 8f                         ld hl,0x8f6d
                           .data:0000c8e3 cd 4d ca                         call 0xca4d
                           .data:0000c8e6 3a 59 8f                         ld a,(0x8f59)
                           .data:0000c8e9 3d                               dec a
                           .data:0000c8ea ca 02 c9                         jp z,0xc902
                           .data:0000c8ed 21 5a 8f                         ld hl,0x8f5a
                           .data:0000c8f0 11 6d 8f                         ld de,0x8f6d
                           .data:0000c8f3 4e                               ld c,(hl)
                           .data:0000c8f4 1a                               ld a,(de)
                           .data:0000c8f5 b9                               cp c
                           .data:0000c8f6 c2 b3 c8                         jp nz,0xc8b3
                           .data:0000c8f9 b7                               or a
                           .data:0000c8fa ca 02 c9                         jp z,0xc902
                           .data:0000c8fd 23                               inc hl
                           .data:0000c8fe 13                               inc de
                           .data:0000c8ff c3 f3 c8                         jp 0xc8f3
                           .data:0000c902 3e 92                            ld a,0x92
                           .data:0000c904 32 03 ff                         ld (0xff03),a
                           .data:0000c907 cd f9 c3                         call 0xc3f9
                           .data:0000c90a 3e 08                            ld a,0x08
                           .data:0000c90c cd 77 c3                         call 0xc377
                           .data:0000c90f 6f                               ld l,a
                           .data:0000c910 3e 08                            ld a,0x08
                           .data:0000c912 cd 77 c3                         call 0xc377
                           .data:0000c915 67                               ld h,a
                           .data:0000c916 22 54 8f                         ld (0x8f54),hl
                           .data:0000c919 eb                               ex de,hl
                           .data:0000c91a 22 56 8f                         ld (0x8f56),hl
                           .data:0000c91d eb                               ex de,hl
                           .data:0000c91e 2a e3 8f                         ld hl,(0x8fe3)
                           .data:0000c921 cd 05 ca                         call 0xca05
                           .data:0000c924 2a 54 8f                         ld hl,(0x8f54)
                           .data:0000c927 59                               ld e,c
                           .data:0000c928 50                               ld d,b
                           .data:0000c929 cd 27 c4                         call 0xc427
                           .data:0000c92c c2 60 c9                         jp nz,0xc960
                           .data:0000c92f cd 1d cd                         call 0xcd1d
                           .data:0000c932 21 5f ca                         ld hl,0xca5f
                           .data:0000c935 cd 4d ca                         call 0xca4d
                           .data:0000c938 2a 54 8f                         ld hl,(0x8f54)
                           .data:0000c93b cd 2d cd                         call 0xcd2d
                           .data:0000c93e 21 6e ca                         ld hl,0xca6e
                           .data:0000c941 cd 4d ca                         call 0xca4d
                           .data:0000c944 2a e3 8f                         ld hl,(0x8fe3)
                           .data:0000c947 cd 2d cd                         call 0xcd2d
                           .data:0000c94a 21 75 ca                         ld hl,0xca75
                           .data:0000c94d cd 4d ca                         call 0xca4d
                           .data:0000c950 2a 56 8f                         ld hl,(0x8f56)
                           .data:0000c953 cd 2d cd                         call 0xcd2d
                           .data:0000c956 3a de 8f                         ld a,(0x8fde)
                           .data:0000c959 fe 41                            cp 0x41
                           .data:0000c95b c0                               ret nz
                           .data:0000c95c 2a e3 8f                         ld hl,(0x8fe3)
                           .data:0000c95f e9                               jp (hl)
                           .data:0000c960 cd 31 cc                         call 0xcc31
                           .data:0000c963 c9                               ret
                           .data:0000c964 2a 82 8f                         ld hl,(0x8f82)
                           .data:0000c967 c9                               ret
                           .data:0000c968 e5                               push hl
                           .data:0000c969 d5                               push de
                           .data:0000c96a c5                               push bc
                           .data:0000c96b cd 05 ca                         call 0xca05
                           .data:0000c96e 60                               ld h,b
                           .data:0000c96f 69                               ld l,c
                           .data:0000c970 22 52 8f                         ld (0x8f52),hl
                           .data:0000c973 21 66 ca                         ld hl,0xca66
                           .data:0000c976 cd 4d ca                         call 0xca4d
                           .data:0000c979 11 58 8f                         ld de,0x8f58
                           .data:0000c97c 3e 14                            ld a,0x14
                           .data:0000c97e 12                               ld (de),a
                           .data:0000c97f cd c0 cb                         call 0xcbc0
                           .data:0000c982 c1                               pop bc
                           .data:0000c983 79                               ld a,c
                           .data:0000c984 b7                               or a
                           .data:0000c985 ca 8b c9                         jp z,0xc98b
                           .data:0000c988 32 fe 8f                         ld (0x8ffe),a
                           .data:0000c98b cd ea c9                         call 0xc9ea
                           .data:0000c98e cd ea c9                         call 0xc9ea
                           .data:0000c991 3e e6                            ld a,0xe6
                           .data:0000c993 cd d0 c3                         call 0xc3d0
                           .data:0000c996 0e 03                            ld c,0x03
                           .data:0000c998 3e d9                            ld a,0xd9
                           .data:0000c99a cd d0 c3                         call 0xc3d0
                           .data:0000c99d 0d                               dec c
                           .data:0000c99e c2 98 c9                         jp nz,0xc998
                           .data:0000c9a1 21 5a 8f                         ld hl,0x8f5a
                           .data:0000c9a4 7e                               ld a,(hl)
                           .data:0000c9a5 b7                               or a
                           .data:0000c9a6 ca b0 c9                         jp z,0xc9b0
                           .data:0000c9a9 cd d0 c3                         call 0xc3d0
                           .data:0000c9ac 23                               inc hl
                           .data:0000c9ad c3 a4 c9                         jp 0xc9a4
                           .data:0000c9b0 cd ea c9                         call 0xc9ea
                           .data:0000c9b3 cd ea c9                         call 0xc9ea
                           .data:0000c9b6 cd ea c9                         call 0xc9ea
                           .data:0000c9b9 3e e6                            ld a,0xe6
                           .data:0000c9bb cd d0 c3                         call 0xc3d0
                           .data:0000c9be d1                               pop de
                           .data:0000c9bf e1                               pop hl
                           .data:0000c9c0 7d                               ld a,l
                           .data:0000c9c1 cd d0 c3                         call 0xc3d0
                           .data:0000c9c4 7c                               ld a,h
                           .data:0000c9c5 cd d0 c3                         call 0xc3d0
                           .data:0000c9c8 7b                               ld a,e
                           .data:0000c9c9 cd d0 c3                         call 0xc3d0
                           .data:0000c9cc 7a                               ld a,d
                           .data:0000c9cd cd d0 c3                         call 0xc3d0
                           .data:0000c9d0 7e                               ld a,(hl)
                           .data:0000c9d1 cd d0 c3                         call 0xc3d0
                           .data:0000c9d4 cd 27 c4                         call 0xc427
                           .data:0000c9d7 ca de c9                         jp z,0xc9de
                           .data:0000c9da 23                               inc hl
                           .data:0000c9db c3 d0 c9                         jp 0xc9d0
                           .data:0000c9de 2a 52 8f                         ld hl,(0x8f52)
                           .data:0000c9e1 7d                               ld a,l
                           .data:0000c9e2 cd d0 c3                         call 0xc3d0
                           .data:0000c9e5 7c                               ld a,h
                           .data:0000c9e6 cd d0 c3                         call 0xc3d0
                           .data:0000c9e9 c9                               ret
                           .data:0000c9ea 0e ff                            ld c,0xff
                           .data:0000c9ec 3e 00                            ld a,0x00
                           .data:0000c9ee cd d0 c3                         call 0xc3d0
                           .data:0000c9f1 0d                               dec c
                           .data:0000c9f2 c2 ec c9                         jp nz,0xc9ec
                           .data:0000c9f5 c9                               ret
                           .data:0000c9f6 cd 05 ca                         call 0xca05
                           .data:0000c9f9 c5                               push bc
                           .data:0000c9fa 21 5f ca                         ld hl,0xca5f
                           .data:0000c9fd cd 4d ca                         call 0xca4d
                           .data:0000ca00 e1                               pop hl
                           .data:0000ca01 cd 2d cd                         call 0xcd2d
                           .data:0000ca04 c9                               ret
                           .data:0000ca05 01 00 00                         ld bc,0x0000
                           .data:0000ca08 7e                               ld a,(hl)
                           .data:0000ca09 81                               add a,c
                           .data:0000ca0a 4f                               ld c,a
                           .data:0000ca0b f5                               push af
                           .data:0000ca0c cd 27 c4                         call 0xc427
                           .data:0000ca0f ca 27 ca                         jp z,0xca27
                           .data:0000ca12 f1                               pop af
                           .data:0000ca13 78                               ld a,b
                           .data:0000ca14 8e                               adc a,(hl)
                           .data:0000ca15 47                               ld b,a
                           .data:0000ca16 cd 1c ca                         call 0xca1c
                           .data:0000ca19 c3 08 ca                         jp 0xca08
                           .data:0000ca1c cd 27 c4                         call 0xc427
                           .data:0000ca1f c2 25 ca                         jp nz,0xca25
                           .data:0000ca22 33                               inc sp
                           .data:0000ca23 33                               inc sp
                           .data:0000ca24 c9                               ret
                           .data:0000ca25 23                               inc hl
                           .data:0000ca26 c9                               ret
                           .data:0000ca27 f1                               pop af
                           .data:0000ca28 c9                               ret
                           .data:0000ca29 cd 2a cd                         call 0xcd2a
                           .data:0000ca2c 7e                               ld a,(hl)
                           .data:0000ca2d cd 7c ca                         call 0xca7c
                           .data:0000ca30 cd 07 cc                         call 0xcc07
                           .data:0000ca33 cd 19 cb                         call 0xcb19
                           .data:0000ca36 d2 44 ca                         jp nc,0xca44
                           .data:0000ca39 fe 1a                            cp 0x1a
                           .data:0000ca3b ca 45 ca                         jp z,0xca45
                           .data:0000ca3e fe 08                            cp 0x08
                           .data:0000ca40 ca 49 ca                         jp z,0xca49
                           .data:0000ca43 c9                               ret
                           .data:0000ca44 77                               ld (hl),a
                           .data:0000ca45 23                               inc hl
                           .data:0000ca46 c3 29 ca                         jp 0xca29
                           .data:0000ca49 2b                               dec hl
                           .data:0000ca4a c3 29 ca                         jp 0xca29
                           .data:0000ca4d c5                               push bc
                           .data:0000ca4e f5                               push af
                           .data:0000ca4f 7e                               ld a,(hl)
                           .data:0000ca50 4f                               ld c,a
                           .data:0000ca51 b7                               or a
                           .data:0000ca52 ca 5c ca                         jp z,0xca5c
                           .data:0000ca55 cd 09 c8                         call 0xc809
                           .data:0000ca58 23                               inc hl
                           .data:0000ca59 c3 4f ca                         jp 0xca4f
                           .data:0000ca5c f1                               pop af
                           .data:0000ca5d c1                               pop bc
                           .data:0000ca5e c9                               ret
                           .data:0000ca5f 0a                               ld a,(bc)
                           .data:0000ca60 0d                               dec c
                           .data:0000ca61 43                               ld b,e
                           .data:0000ca62 53                               ld d,e
                           .data:0000ca63 20 3d                            jr nz,0xcaa2
                           .data:0000ca65 00                               nop
                           .data:0000ca66 0a                               ld a,(bc)
                           .data:0000ca67 0d                               dec c
                           .data:0000ca68 46                               ld b,(hl)
                           .data:0000ca69 49                               ld c,c
                           .data:0000ca6a 4c                               ld c,h
                           .data:0000ca6b 45                               ld b,l
                           .data:0000ca6c 3a 00 0a                         ld a,(0x0a00)
                           .data:0000ca6f 0d                               dec c
                           .data:0000ca70 42                               ld b,d
                           .data:0000ca71 45                               ld b,l
                           .data:0000ca72 47                               ld b,a
                           .data:0000ca73 3d                               dec a
                           .data:0000ca74 00                               nop
                           .data:0000ca75 0a                               ld a,(bc)
                           .data:0000ca76 0d                               dec c
                           .data:0000ca77 45                               ld b,l
                           .data:0000ca78 4e                               ld c,(hl)
                           .data:0000ca79 44                               ld b,h
                           .data:0000ca7a 3d                               dec a
                           .data:0000ca7b 00                               nop
                           .data:0000ca7c d5                               push de
                           .data:0000ca7d c5                               push bc
                           .data:0000ca7e cd 8c ca                         call 0xca8c
                           .data:0000ca81 4a                               ld c,d
                           .data:0000ca82 cd 09 c8                         call 0xc809
                           .data:0000ca85 4b                               ld c,e
                           .data:0000ca86 cd 09 c8                         call 0xc809
                           .data:0000ca89 c1                               pop bc
                           .data:0000ca8a d1                               pop de
                           .data:0000ca8b c9                               ret
                           .data:0000ca8c 5f                               ld e,a
                           .data:0000ca8d 0f                               rrca
                           .data:0000ca8e 0f                               rrca
                           .data:0000ca8f 0f                               rrca
                           .data:0000ca90 0f                               rrca
                           .data:0000ca91 cd 9b ca                         call 0xca9b
                           .data:0000ca94 57                               ld d,a
                           .data:0000ca95 7b                               ld a,e
                           .data:0000ca96 cd 9b ca                         call 0xca9b
                           .data:0000ca99 5f                               ld e,a
                           .data:0000ca9a c9                               ret
                           .data:0000ca9b e6 0f                            and 0x0f
                           .data:0000ca9d fe 0a                            cp 0x0a
                           .data:0000ca9f da a4 ca                         jp c,0xcaa4
                           .data:0000caa2 c6 07                            add a,0x07
                           .data:0000caa4 c6 30                            add a,0x30
                           .data:0000caa6 c9                               ret
                           .data:0000caa7 cd 1d cd                         call 0xcd1d
                           .data:0000caaa e5                               push hl
                           .data:0000caab cd 2d cd                         call 0xcd2d
                           .data:0000caae cd 07 cc                         call 0xcc07
                           .data:0000cab1 06 08                            ld b,0x08
                           .data:0000cab3 7e                               ld a,(hl)
                           .data:0000cab4 cd 7c ca                         call 0xca7c
                           .data:0000cab7 cd 07 cc                         call 0xcc07
                           .data:0000caba 05                               dec b
                           .data:0000cabb 23                               inc hl
                           .data:0000cabc c2 b3 ca                         jp nz,0xcab3
                           .data:0000cabf cd 07 cc                         call 0xcc07
                           .data:0000cac2 e1                               pop hl
                           .data:0000cac3 06 08                            ld b,0x08
                           .data:0000cac5 7e                               ld a,(hl)
                           .data:0000cac6 cd e4 ca                         call 0xcae4
                           .data:0000cac9 4f                               ld c,a
                           .data:0000caca cd 09 c8                         call 0xc809
                           .data:0000cacd cd 27 c4                         call 0xc427
                           .data:0000cad0 c8                               ret z
                           .data:0000cad1 05                               dec b
                           .data:0000cad2 23                               inc hl
                           .data:0000cad3 c2 c5 ca                         jp nz,0xcac5
                           .data:0000cad6 cd e3 d0                         call 0xd0e3
                           .data:0000cad9 fe 1f                            cp 0x1f
                           .data:0000cadb cc 03 c8                         call z,0xc803
                           .data:0000cade fe 1f                            cp 0x1f
                           .data:0000cae0 c8                               ret z
                           .data:0000cae1 c3 a7 ca                         jp 0xcaa7
                           .data:0000cae4 fe 20                            cp 0x20
                           .data:0000cae6 da ec ca                         jp c,0xcaec
                           .data:0000cae9 fe 80                            cp 0x80
                           .data:0000caeb d8                               ret c
                           .data:0000caec 3e 2e                            ld a,0x2e
                           .data:0000caee c9                               ret
                           .data:0000caef cd 03 c8                         call 0xc803
                           .data:0000caf2 fe 20                            cp 0x20
                           .data:0000caf4 da 17 cb                         jp c,0xcb17
                           .data:0000caf7 fe 41                            cp 0x41
                           .data:0000caf9 d2 0a cb                         jp nc,0xcb0a
                           .data:0000cafc f6 10                            or 0x10
                           .data:0000cafe fe 3a                            cp 0x3a
                           .data:0000cb00 d2 ef ca                         jp nc,0xcaef
                           .data:0000cb03 4f                               ld c,a
                           .data:0000cb04 d6 30                            sub 0x30
                           .data:0000cb06 cd 09 c8                         call 0xc809
                           .data:0000cb09 c9                               ret
                           .data:0000cb0a e6 5f                            and 0x5f
                           .data:0000cb0c fe 47                            cp 0x47
                           .data:0000cb0e d2 ef ca                         jp nc,0xcaef
                           .data:0000cb11 4f                               ld c,a
                           .data:0000cb12 d6 37                            sub 0x37
                           .data:0000cb14 c3 06 cb                         jp 0xcb06
                           .data:0000cb17 37                               scf
                           .data:0000cb18 c9                               ret
                           .data:0000cb19 c5                               push bc
                           .data:0000cb1a cd ef ca                         call 0xcaef
                           .data:0000cb1d da 2c cb                         jp c,0xcb2c
                           .data:0000cb20 07                               rlca
                           .data:0000cb21 07                               rlca
                           .data:0000cb22 07                               rlca
                           .data:0000cb23 07                               rlca
                           .data:0000cb24 47                               ld b,a
                           .data:0000cb25 cd ef ca                         call 0xcaef
                           .data:0000cb28 da 2c cb                         jp c,0xcb2c
                           .data:0000cb2b b0                               or b
                           .data:0000cb2c c1                               pop bc
                           .data:0000cb2d c9                               ret
                           .data:0000cb2e 11 00 00                         ld de,0x0000
                           .data:0000cb31 7e                               ld a,(hl)
                           .data:0000cb32 fe 20                            cp 0x20
                           .data:0000cb34 c2 3b cb                         jp nz,0xcb3b
                           .data:0000cb37 23                               inc hl
                           .data:0000cb38 c3 31 cb                         jp 0xcb31
                           .data:0000cb3b fe 2c                            cp 0x2c
                           .data:0000cb3d c8                               ret z
                           .data:0000cb3e b7                               or a
                           .data:0000cb3f c8                               ret z
                           .data:0000cb40 fe 0d                            cp 0x0d
                           .data:0000cb42 c8                               ret z
                           .data:0000cb43 fe 20                            cp 0x20
                           .data:0000cb45 d2 4a cb                         jp nc,0xcb4a
                           .data:0000cb48 37                               scf
                           .data:0000cb49 c9                               ret
                           .data:0000cb4a fe 41                            cp 0x41
                           .data:0000cb4c d2 63 cb                         jp nc,0xcb63
                           .data:0000cb4f f6 10                            or 0x10
                           .data:0000cb51 fe 3a                            cp 0x3a
                           .data:0000cb53 d2 48 cb                         jp nc,0xcb48
                           .data:0000cb56 d6 30                            sub 0x30
                           .data:0000cb58 eb                               ex de,hl
                           .data:0000cb59 29                               add hl,hl
                           .data:0000cb5a 29                               add hl,hl
                           .data:0000cb5b 29                               add hl,hl
                           .data:0000cb5c 29                               add hl,hl
                           .data:0000cb5d eb                               ex de,hl
                           .data:0000cb5e b3                               or e
                           .data:0000cb5f 5f                               ld e,a
                           .data:0000cb60 c3 37 cb                         jp 0xcb37
                           .data:0000cb63 e6 5f                            and 0x5f
                           .data:0000cb65 fe 47                            cp 0x47
                           .data:0000cb67 d2 48 cb                         jp nc,0xcb48
                           .data:0000cb6a d6 37                            sub 0x37
                           .data:0000cb6c c3 58 cb                         jp 0xcb58
                           .data:0000cb6f cd 75 c8                         call 0xc875
                           .data:0000cb72 f9                               ld sp,hl
                           .data:0000cb73 21 18 cc                         ld hl,0xcc18
                           .data:0000cb76 cd 4d ca                         call 0xca4d
                           .data:0000cb79 0e 00                            ld c,0x00
                           .data:0000cb7b 21 58 8f                         ld hl,0x8f58
                           .data:0000cb7e 11 81 8f                         ld de,0x8f81
                           .data:0000cb81 cd 0f cc                         call 0xcc0f
                           .data:0000cb84 11 58 8f                         ld de,0x8f58
                           .data:0000cb87 3e 28                            ld a,0x28
                           .data:0000cb89 12                               ld (de),a
                           .data:0000cb8a cd c0 cb                         call 0xcbc0
                           .data:0000cb8d 13                               inc de
                           .data:0000cb8e 1a                               ld a,(de)
                           .data:0000cb8f 13                               inc de
                           .data:0000cb90 3d                               dec a
                           .data:0000cb91 ca 6f cb                         jp z,0xcb6f
                           .data:0000cb94 eb                               ex de,hl
                           .data:0000cb95 cd 9b cb                         call 0xcb9b
                           .data:0000cb98 c3 6f cb                         jp 0xcb6f
                           .data:0000cb9b 7e                               ld a,(hl)
                           .data:0000cb9c 32 de 8f                         ld (0x8fde),a
                           .data:0000cb9f fe 41                            cp 0x41
                           .data:0000cba1 c2 a5 cb                         jp nz,0xcba5
                           .data:0000cba4 23                               inc hl
                           .data:0000cba5 22 56 8f                         ld (0x8f56),hl
                           .data:0000cba8 06 03                            ld b,0x03
                           .data:0000cbaa 23                               inc hl
                           .data:0000cbab cd 2e cb                         call 0xcb2e
                           .data:0000cbae dc 31 cc                         call c,0xcc31
                           .data:0000cbb1 da 6f cb                         jp c,0xcb6f
                           .data:0000cbb4 d5                               push de
                           .data:0000cbb5 05                               dec b
                           .data:0000cbb6 c2 aa cb                         jp nz,0xcbaa
                           .data:0000cbb9 c3 3a cc                         jp 0xcc3a
                           .data:0000cbbc 08                               ex af,af'
                           .data:0000cbbd 20 08                            jr nz,0xcbc7
                           .data:0000cbbf 00                               nop
                           .data:0000cbc0 e5                               push hl
                           .data:0000cbc1 c5                               push bc
                           .data:0000cbc2 f5                               push af
                           .data:0000cbc3 d5                               push de
                           .data:0000cbc4 06 00                            ld b,0x00
                           .data:0000cbc6 eb                               ex de,hl
                           .data:0000cbc7 56                               ld d,(hl)
                           .data:0000cbc8 23                               inc hl
                           .data:0000cbc9 23                               inc hl
                           .data:0000cbca cd 03 c8                         call 0xc803
                           .data:0000cbcd 4f                               ld c,a
                           .data:0000cbce fe 08                            cp 0x08
                           .data:0000cbd0 ca f4 cb                         jp z,0xcbf4
                           .data:0000cbd3 fe 7f                            cp 0x7f
                           .data:0000cbd5 ca f4 cb                         jp z,0xcbf4
                           .data:0000cbd8 cd 09 c8                         call 0xc809
                           .data:0000cbdb 15                               dec d
                           .data:0000cbdc c2 e1 cb                         jp nz,0xcbe1
                           .data:0000cbdf 3e 0d                            ld a,0x0d
                           .data:0000cbe1 77                               ld (hl),a
                           .data:0000cbe2 23                               inc hl
                           .data:0000cbe3 04                               inc b
                           .data:0000cbe4 fe 0d                            cp 0x0d
                           .data:0000cbe6 c2 ca cb                         jp nz,0xcbca
                           .data:0000cbe9 36 00                            ld (hl),0x00
                           .data:0000cbeb e1                               pop hl
                           .data:0000cbec e5                               push hl
                           .data:0000cbed 23                               inc hl
                           .data:0000cbee 70                               ld (hl),b
                           .data:0000cbef d1                               pop de
                           .data:0000cbf0 f1                               pop af
                           .data:0000cbf1 c1                               pop bc
                           .data:0000cbf2 e1                               pop hl
                           .data:0000cbf3 c9                               ret
                           .data:0000cbf4 78                               ld a,b
                           .data:0000cbf5 b7                               or a
                           .data:0000cbf6 ca ca cb                         jp z,0xcbca
                           .data:0000cbf9 14                               inc d
                           .data:0000cbfa 05                               dec b
                           .data:0000cbfb 2b                               dec hl
                           .data:0000cbfc e5                               push hl
                           .data:0000cbfd 21 bc cb                         ld hl,0xcbbc
                           .data:0000cc00 cd 4d ca                         call 0xca4d
                           .data:0000cc03 e1                               pop hl
                           .data:0000cc04 c3 ca cb                         jp 0xcbca
                           .data:0000cc07 c5                               push bc
                           .data:0000cc08 0e 20                            ld c,0x20
                           .data:0000cc0a cd 09 c8                         call 0xc809
                           .data:0000cc0d c1                               pop bc
                           .data:0000cc0e c9                               ret
                           .data:0000cc0f 71                               ld (hl),c
                           .data:0000cc10 cd 27 c4                         call 0xc427
                           .data:0000cc13 c8                               ret z
                           .data:0000cc14 23                               inc hl
                           .data:0000cc15 c3 0f cc                         jp 0xcc0f
                           .data:0000cc18 0a                               ld a,(bc)
                           .data:0000cc19 0d                               dec c
                           .data:0000cc1a 20 43                            jr nz,0xcc5f
                           .data:0000cc1c 4c                               ld c,h
                           .data:0000cc1d 49                               ld c,c
                           .data:0000cc1e 3e 20                            ld a,0x20
                           .data:0000cc20 00                               nop
                           .data:0000cc21 0a                               ld a,(bc)
                           .data:0000cc22 0d                               dec c
                           .data:0000cc23 20 43                            jr nz,0xcc68
                           .data:0000cc25 4c                               ld c,h
                           .data:0000cc26 49                               ld c,c
                           .data:0000cc27 20 2d                            jr nz,0xcc56
                           .data:0000cc29 20 45                            jr nz,0xcc70
                           .data:0000cc2b 52                               ld d,d
                           .data:0000cc2c 52                               ld d,d
                           .data:0000cc2d 4f                               ld c,a
                           .data:0000cc2e 52                               ld d,d
                           .data:0000cc2f 07                               rlca
                           .data:0000cc30 00                               nop
                           .data:0000cc31 f5                               push af
                           .data:0000cc32 21 21 cc                         ld hl,0xcc21
                           .data:0000cc35 cd 4d ca                         call 0xca4d
                           .data:0000cc38 f1                               pop af
                           .data:0000cc39 c9                               ret
                           .data:0000cc3a 2a 56 8f                         ld hl,(0x8f56)
                           .data:0000cc3d 7e                               ld a,(hl)
                           .data:0000cc3e e6 5f                            and 0x5f
                           .data:0000cc40 4f                               ld c,a
                           .data:0000cc41 21 a0 cd                         ld hl,0xcda0
                           .data:0000cc44 7e                               ld a,(hl)
                           .data:0000cc45 a6                               and (hl)
                           .data:0000cc46 cc 31 cc                         call z,0xcc31
                           .data:0000cc49 ca 6f cb                         jp z,0xcb6f
                           .data:0000cc4c b9                               cp c
                           .data:0000cc4d 23                               inc hl
                           .data:0000cc4e ca 56 cc                         jp z,0xcc56
                           .data:0000cc51 23                               inc hl
                           .data:0000cc52 23                               inc hl
                           .data:0000cc53 c3 44 cc                         jp 0xcc44
                           .data:0000cc56 5e                               ld e,(hl)
                           .data:0000cc57 23                               inc hl
                           .data:0000cc58 56                               ld d,(hl)
                           .data:0000cc59 c1                               pop bc
                           .data:0000cc5a eb                               ex de,hl
                           .data:0000cc5b e3                               ex (sp),hl
                           .data:0000cc5c eb                               ex de,hl
                           .data:0000cc5d e1                               pop hl
                           .data:0000cc5e e3                               ex (sp),hl
                           .data:0000cc5f c9                               ret
                           .data:0000cc60 e5                               push hl
                           .data:0000cc61 d5                               push de
                           .data:0000cc62 0e 00                            ld c,0x00
                           .data:0000cc64 21 5a 8f                         ld hl,0x8f5a
                           .data:0000cc67 cd 9e cc                         call 0xcc9e
                           .data:0000cc6a cd 9e cc                         call 0xcc9e
                           .data:0000cc6d cd 9e cc                         call 0xcc9e
                           .data:0000cc70 ca a6 cc                         jp z,0xcca6
                           .data:0000cc73 0c                               inc c
                           .data:0000cc74 32 54 8f                         ld (0x8f54),a
                           .data:0000cc77 cd 9e cc                         call 0xcc9e
                           .data:0000cc7a ca 8b cc                         jp z,0xcc8b
                           .data:0000cc7d 0c                               inc c
                           .data:0000cc7e 32 55 8f                         ld (0x8f55),a
                           .data:0000cc81 cd 9e cc                         call 0xcc9e
                           .data:0000cc84 ca 8b cc                         jp z,0xcc8b
                           .data:0000cc87 0c                               inc c
                           .data:0000cc88 32 56 8f                         ld (0x8f56),a
                           .data:0000cc8b d1                               pop de
                           .data:0000cc8c e1                               pop hl
                           .data:0000cc8d 41                               ld b,c
                           .data:0000cc8e 48                               ld c,b
                           .data:0000cc8f cd 27 c4                         call 0xc427
                           .data:0000cc92 c8                               ret z
                           .data:0000cc93 3a 54 8f                         ld a,(0x8f54)
                           .data:0000cc96 be                               cp (hl)
                           .data:0000cc97 ca a9 cc                         jp z,0xcca9
                           .data:0000cc9a 23                               inc hl
                           .data:0000cc9b c3 8e cc                         jp 0xcc8e
                           .data:0000cc9e 23                               inc hl
                           .data:0000cc9f cd 2e cb                         call 0xcb2e
                           .data:0000cca2 7e                               ld a,(hl)
                           .data:0000cca3 b7                               or a
                           .data:0000cca4 7b                               ld a,e
                           .data:0000cca5 c9                               ret
                           .data:0000cca6 d1                               pop de
                           .data:0000cca7 e1                               pop hl
                           .data:0000cca8 c9                               ret
                           .data:0000cca9 0d                               dec c
                           .data:0000ccaa ca cd cc                         jp z,0xcccd
                           .data:0000ccad 23                               inc hl
                           .data:0000ccae cd 27 c4                         call 0xc427
                           .data:0000ccb1 c8                               ret z
                           .data:0000ccb2 3a 55 8f                         ld a,(0x8f55)
                           .data:0000ccb5 be                               cp (hl)
                           .data:0000ccb6 c2 8e cc                         jp nz,0xcc8e
                           .data:0000ccb9 0d                               dec c
                           .data:0000ccba ca cd cc                         jp z,0xcccd
                           .data:0000ccbd 23                               inc hl
                           .data:0000ccbe cd 27 c4                         call 0xc427
                           .data:0000ccc1 c8                               ret z
                           .data:0000ccc2 3a 56 8f                         ld a,(0x8f56)
                           .data:0000ccc5 be                               cp (hl)
                           .data:0000ccc6 c2 8e cc                         jp nz,0xcc8e
                           .data:0000ccc9 0d                               dec c
                           .data:0000ccca ca cd cc                         jp z,0xcccd
                           .data:0000cccd 48                               ld c,b
                           .data:0000ccce 2b                               dec hl
                           .data:0000cccf 0d                               dec c
                           .data:0000ccd0 c2 ce cc                         jp nz,0xccce
                           .data:0000ccd3 23                               inc hl
                           .data:0000ccd4 cd e6 cc                         call 0xcce6
                           .data:0000ccd7 cd e3 d0                         call 0xd0e3
                           .data:0000ccda fe 1f                            cp 0x1f
                           .data:0000ccdc cc 03 c8                         call z,0xc803
                           .data:0000ccdf fe 1f                            cp 0x1f
                           .data:0000cce1 c8                               ret z
                           .data:0000cce2 23                               inc hl
                           .data:0000cce3 c3 8e cc                         jp 0xcc8e
                           .data:0000cce6 c5                               push bc
                           .data:0000cce7 cd 1d cd                         call 0xcd1d
                           .data:0000ccea cd 2d cd                         call 0xcd2d
                           .data:0000cced 78                               ld a,b
                           .data:0000ccee c6 04                            add a,0x04
                           .data:0000ccf0 47                               ld b,a
                           .data:0000ccf1 e5                               push hl
                           .data:0000ccf2 2b                               dec hl
                           .data:0000ccf3 2b                               dec hl
                           .data:0000ccf4 7e                               ld a,(hl)
                           .data:0000ccf5 cd 7c ca                         call 0xca7c
                           .data:0000ccf8 cd 07 cc                         call 0xcc07
                           .data:0000ccfb 23                               inc hl
                           .data:0000ccfc 05                               dec b
                           .data:0000ccfd c2 f4 cc                         jp nz,0xccf4
                           .data:0000cd00 cd 07 cc                         call 0xcc07
                           .data:0000cd03 e1                               pop hl
                           .data:0000cd04 c1                               pop bc
                           .data:0000cd05 c5                               push bc
                           .data:0000cd06 e5                               push hl
                           .data:0000cd07 2b                               dec hl
                           .data:0000cd08 2b                               dec hl
                           .data:0000cd09 78                               ld a,b
                           .data:0000cd0a c6 04                            add a,0x04
                           .data:0000cd0c 47                               ld b,a
                           .data:0000cd0d 7e                               ld a,(hl)
                           .data:0000cd0e cd e4 ca                         call 0xcae4
                           .data:0000cd11 4f                               ld c,a
                           .data:0000cd12 cd 09 c8                         call 0xc809
                           .data:0000cd15 23                               inc hl
                           .data:0000cd16 05                               dec b
                           .data:0000cd17 c2 0d cd                         jp nz,0xcd0d
                           .data:0000cd1a e1                               pop hl
                           .data:0000cd1b c1                               pop bc
                           .data:0000cd1c c9                               ret
                           .data:0000cd1d c5                               push bc
                           .data:0000cd1e 0e 0a                            ld c,0x0a
                           .data:0000cd20 cd 09 c8                         call 0xc809
                           .data:0000cd23 0e 0d                            ld c,0x0d
                           .data:0000cd25 cd 09 c8                         call 0xc809
                           .data:0000cd28 c1                               pop bc
                           .data:0000cd29 c9                               ret
                           .data:0000cd2a cd 1d cd                         call 0xcd1d
                           .data:0000cd2d 7c                               ld a,h
                           .data:0000cd2e cd 7c ca                         call 0xca7c
                           .data:0000cd31 7d                               ld a,l
                           .data:0000cd32 cd 7c ca                         call 0xca7c
                           .data:0000cd35 cd 07 cc                         call 0xcc07
                           .data:0000cd38 c9                               ret
                           .data:0000cd39 e9                               jp (hl)
                           .data:0000cd3a 0a                               ld a,(bc)
                           .data:0000cd3b be                               cp (hl)
                           .data:0000cd3c ca 54 cd                         jp z,0xcd54
                           .data:0000cd3f f5                               push af
                           .data:0000cd40 c4 2a cd                         call nz,0xcd2a
                           .data:0000cd43 f1                               pop af
                           .data:0000cd44 f5                               push af
                           .data:0000cd45 7e                               ld a,(hl)
                           .data:0000cd46 c4 7c ca                         call nz,0xca7c
                           .data:0000cd49 c5                               push bc
                           .data:0000cd4a 0e 20                            ld c,0x20
                           .data:0000cd4c cd 09 c8                         call 0xc809
                           .data:0000cd4f c1                               pop bc
                           .data:0000cd50 f1                               pop af
                           .data:0000cd51 c4 7c ca                         call nz,0xca7c
                           .data:0000cd54 cd 27 c4                         call 0xc427
                           .data:0000cd57 c8                               ret z
                           .data:0000cd58 23                               inc hl
                           .data:0000cd59 03                               inc bc
                           .data:0000cd5a cd e3 d0                         call 0xd0e3
                           .data:0000cd5d fe 1f                            cp 0x1f
                           .data:0000cd5f cc 03 c8                         call z,0xc803
                           .data:0000cd62 fe 1f                            cp 0x1f
                           .data:0000cd64 c8                               ret z
                           .data:0000cd65 c3 3a cd                         jp 0xcd3a
                           .data:0000cd68 7c                               ld a,h
                           .data:0000cd69 b8                               cp b
                           .data:0000cd6a da 92 cd                         jp c,0xcd92
                           .data:0000cd6d 7d                               ld a,l
                           .data:0000cd6e b9                               cp c
                           .data:0000cd6f da 92 cd                         jp c,0xcd92
                           .data:0000cd72 7e                               ld a,(hl)
                           .data:0000cd73 02                               ld (bc),a
                           .data:0000cd74 cd 27 c4                         call 0xc427
                           .data:0000cd77 c8                               ret z
                           .data:0000cd78 23                               inc hl
                           .data:0000cd79 03                               inc bc
                           .data:0000cd7a c3 72 cd                         jp 0xcd72
                           .data:0000cd7d e5                               push hl
                           .data:0000cd7e c5                               push bc
                           .data:0000cd7f b7                               or a
                           .data:0000cd80 7b                               ld a,e
                           .data:0000cd81 9d                               sbc a,l
                           .data:0000cd82 4f                               ld c,a
                           .data:0000cd83 7a                               ld a,d
                           .data:0000cd84 9c                               sbc a,h
                           .data:0000cd85 da 8f cd                         jp c,0xcd8f
                           .data:0000cd88 47                               ld b,a
                           .data:0000cd89 e1                               pop hl
                           .data:0000cd8a 09                               add hl,bc
                           .data:0000cd8b e5                               push hl
                           .data:0000cd8c c1                               pop bc
                           .data:0000cd8d e1                               pop hl
                           .data:0000cd8e c9                               ret
                           .data:0000cd8f c1                               pop bc
                           .data:0000cd90 e1                               pop hl
                           .data:0000cd91 c9                               ret
                           .data:0000cd92 cd 7d cd                         call 0xcd7d
                           .data:0000cd95 1a                               ld a,(de)
                           .data:0000cd96 02                               ld (bc),a
                           .data:0000cd97 cd 27 c4                         call 0xc427
                           .data:0000cd9a c8                               ret z
                           .data:0000cd9b 1b                               dec de
                           .data:0000cd9c 0b                               dec bc
                           .data:0000cd9d c3 95 cd                         jp 0xcd95
                           .data:0000cda0 44                               ld b,h
                           .data:0000cda1 a7                               and a
                           .data:0000cda2 ca 4d 29                         jp z,0x294d
                           .data:0000cda5 ca 46 0f                         jp z,0x0f46
                           .data:0000cda8 cc 53 60                         call z,0x6053
                           .data:0000cdab cc 47 39                         call z,0x3947
                           .data:0000cdae cd 43 3a                         call 0x3a43
                           .data:0000cdb1 cd 54 68                         call 0x6854
                           .data:0000cdb4 cd 4f 68                         call 0x684f
                           .data:0000cdb7 c9                               ret
                           .data:0000cdb8 49                               ld c,c
                           .data:0000cdb9 9f                               sbc a,a
                           .data:0000cdba c8                               ret z
                           .data:0000cdbb 48                               ld c,b
                           .data:0000cdbc 5f                               ld e,a
                           .data:0000cdbd c8                               ret z
                           .data:0000cdbe 57                               ld d,a
                           .data:0000cdbf 88                               adc a,b
                           .data:0000cdc0 c8                               ret z
                           .data:0000cdc1 52                               ld d,d
                           .data:0000cdc2 79                               ld a,c
                           .data:0000cdc3 c8                               ret z
                           .data:0000cdc4 4b                               ld c,e
                           .data:0000cdc5 f6 c9                            or 0xc9
                           .data:0000cdc7 56                               ld d,(hl)
                           .data:0000cdc8 e6 cd                            and 0xcd
                           .data:0000cdca 00                               nop
                           .data:0000cdcb 00                               nop
                           .data:0000cdcc 09                               add hl,bc
                           .data:0000cdcd 1b                               dec de
                           .data:0000cdce 5c                               ld e,h
                           .data:0000cdcf 20 28                            jr nz,0xcdf9
                           .data:0000cdd1 73                               ld (hl),e
                           .data:0000cdd2 29                               add hl,hl
                           .data:0000cdd3 20 73                            jr nz,0xce48
                           .data:0000cdd5 50                               ld d,b
                           .data:0000cdd6 45                               ld b,l
                           .data:0000cdd7 43                               ld b,e
                           .data:0000cdd8 49                               ld c,c
                           .data:0000cdd9 41                               ld b,c
                           .data:0000cdda 4c                               ld c,h
                           .data:0000cddb 49                               ld c,c
                           .data:0000cddc 53                               ld d,e
                           .data:0000cddd 54                               ld d,h
                           .data:0000cdde 1b                               dec de
                           .data:0000cddf 5d                               ld e,l
                           .data:0000cde0 20 56                            jr nz,0xce38
                           .data:0000cde2 35                               dec (hl)
                           .data:0000cde3 2e 37                            ld l,0x37
                           .data:0000cde5 00                               nop
                           .data:0000cde6 31 50 8f                         ld sp,0x8f50
                           .data:0000cde9 21 06 ce                         ld hl,0xce06
                           .data:0000cdec e5                               push hl
                           .data:0000cded 0e 3f                            ld c,0x3f
                           .data:0000cdef 21 0a ce                         ld hl,0xce0a
                           .data:0000cdf2 e5                               push hl
                           .data:0000cdf3 0d                               dec c
                           .data:0000cdf4 c2 f2 cd                         jp nz,0xcdf2
                           .data:0000cdf7 11 01 ff                         ld de,0xff01
                           .data:0000cdfa 21 fb ff                         ld hl,0xfffb
                           .data:0000cdfd 01 ae 00                         ld bc,0x00ae
                           .data:0000ce00 00                               nop
                           .data:0000ce01 00                               nop
                           .data:0000ce02 00                               nop
                           .data:0000ce03 00                               nop
                           .data:0000ce04 00                               nop
                           .data:0000ce05 00                               nop
                           .data:0000ce06 31 d0 8e                         ld sp,0x8ed0
                           .data:0000ce09 23                               inc hl
                           .data:0000ce0a 1a                               ld a,(de)
                           .data:0000ce0b b8                               cp b
                           .data:0000ce0c c8                               ret z
                           .data:0000ce0d 47                               ld b,a
                           .data:0000ce0e 0d                               dec c
                           .data:0000ce0f c0                               ret nz
                           .data:0000ce10 29                               add hl,hl
                           .data:0000ce11 29                               add hl,hl
                           .data:0000ce12 29                               add hl,hl
                           .data:0000ce13 29                               add hl,hl
                           .data:0000ce14 29                               add hl,hl
                           .data:0000ce15 29                               add hl,hl
                           .data:0000ce16 7c                               ld a,h
                           .data:0000ce17 29                               add hl,hl
                           .data:0000ce18 84                               add a,h
                           .data:0000ce19 57                               ld d,a
                           .data:0000ce1a 21 2a ce                         ld hl,0xce2a
                           .data:0000ce1d cd 4d ca                         call 0xca4d
                           .data:0000ce20 7a                               ld a,d
                           .data:0000ce21 32 ff 8f                         ld (0x8fff),a
                           .data:0000ce24 cd 7c ca                         call 0xca7c
                           .data:0000ce27 c3 6f cb                         jp 0xcb6f
                           .data:0000ce2a 0a                               ld a,(bc)
                           .data:0000ce2b 6b                               ld l,e
                           .data:0000ce2c 7e                               ld a,(hl)
                           .data:0000ce2d 3d                               dec a
                           .data:0000ce2e 00                               nop
                           .data:0000ce2f e5                               push hl
                           .data:0000ce30 d5                               push de
                           .data:0000ce31 c5                               push bc
                           .data:0000ce32 af                               xor a
                           .data:0000ce33 32 db 8f                         ld (0x8fdb),a
                           .data:0000ce36 c3 d0 8f                         jp 0x8fd0
                           .data:0000ce39 21 d0 8f                         ld hl,0x8fd0
                           .data:0000ce3c 11 bf ce                         ld de,0xcebf
                           .data:0000ce3f 06 0b                            ld b,0x0b
                           .data:0000ce41 cd cb d3                         call 0xd3cb
                           .data:0000ce44 3e 93                            ld a,0x93
                           .data:0000ce46 32 03 ff                         ld (0xff03),a
                           .data:0000ce49 3a 01 ff                         ld a,(0xff01)
                           .data:0000ce4c f6 03                            or 0x03
                           .data:0000ce4e 67                               ld h,a
                           .data:0000ce4f 3a 00 ff                         ld a,(0xff00)
                           .data:0000ce52 bc                               cp h
                           .data:0000ce53 c2 5c ce                         jp nz,0xce5c
                           .data:0000ce56 21 73 ce                         ld hl,0xce73
                           .data:0000ce59 c3 5f ce                         jp 0xce5f
                           .data:0000ce5c 21 9d d0                         ld hl,0xd09d
                           .data:0000ce5f 22 d1 8f                         ld (0x8fd1),hl
                           .data:0000ce62 c9                               ret
                           .data:0000ce63 cd 71 d0                         call 0xd071
                           .data:0000ce66 cd 91 d0                         call 0xd091
                           .data:0000ce69 3a dd 8f                         ld a,(0x8fdd)
                           .data:0000ce6c 32 dc 8f                         ld (0x8fdc),a
                           .data:0000ce6f c1                               pop bc
                           .data:0000ce70 d1                               pop de
                           .data:0000ce71 e1                               pop hl
                           .data:0000ce72 c9                               ret
                           .data:0000ce73 cd f1 ce                         call 0xcef1
                           .data:0000ce76 cd 72 cf                         call 0xcf72
                           .data:0000ce79 d2 89 ce                         jp nc,0xce89
                           .data:0000ce7c 32 dc 8f                         ld (0x8fdc),a
                           .data:0000ce7f af                               xor a
                           .data:0000ce80 32 d3 8f                         ld (0x8fd3),a
                           .data:0000ce83 32 d4 8f                         ld (0x8fd4),a
                           .data:0000ce86 c3 73 ce                         jp 0xce73
                           .data:0000ce89 4f                               ld c,a
                           .data:0000ce8a 3a dc 8f                         ld a,(0x8fdc)
                           .data:0000ce8d b9                               cp c
                           .data:0000ce8e ca 97 ce                         jp z,0xce97
                           .data:0000ce91 cd 63 d0                         call 0xd063
                           .data:0000ce94 c3 63 ce                         jp 0xce63
                           .data:0000ce97 3a d3 8f                         ld a,(0x8fd3)
                           .data:0000ce9a b7                               or a
                           .data:0000ce9b ca a7 ce                         jp z,0xcea7
                           .data:0000ce9e 21 00 0a                         ld hl,0x0a00
                           .data:0000cea1 cd 66 d0                         call 0xd066
                           .data:0000cea4 c3 66 ce                         jp 0xce66
                           .data:0000cea7 3a d6 8f                         ld a,(0x8fd6)
                           .data:0000ceaa 67                               ld h,a
                           .data:0000ceab cd f1 ce                         call 0xcef1
                           .data:0000ceae cd 72 cf                         call 0xcf72
                           .data:0000ceb1 da 7c ce                         jp c,0xce7c
                           .data:0000ceb4 25                               dec h
                           .data:0000ceb5 c2 ab ce                         jp nz,0xceab
                           .data:0000ceb8 2f                               cpl
                           .data:0000ceb9 32 d3 8f                         ld (0x8fd3),a
                           .data:0000cebc c3 91 ce                         jp 0xce91
                           .data:0000cebf c3 73 ce                         jp 0xce73
                           .data:0000cec2 00                               nop
                           .data:0000cec3 00                               nop
                           .data:0000cec4 00                               nop
                           .data:0000cec5 55                               ld d,l
                           .data:0000cec6 f0                               ret p
                           .data:0000cec7 00                               nop
                           .data:0000cec8 16 cf                            ld d,0xcf
                           .data:0000ceca 3a d1 8f                         ld a,(0x8fd1)
                           .data:0000cecd fe 73                            cp 0x73
                           .data:0000cecf ca e2 ce                         jp z,0xcee2
                           .data:0000ced2 3e 92                            ld a,0x92
                           .data:0000ced4 32 03 ff                         ld (0xff03),a
                           .data:0000ced7 3a 01 ff                         ld a,(0xff01)
                           .data:0000ceda f6 7f                            or 0x7f
                           .data:0000cedc ee ff                            xor 0xff
                           .data:0000cede c8                               ret z
                           .data:0000cedf 3e ff                            ld a,0xff
                           .data:0000cee1 c9                               ret
                           .data:0000cee2 3e 82                            ld a,0x82
                           .data:0000cee4 32 03 ff                         ld (0xff03),a
                           .data:0000cee7 3a 01 ff                         ld a,(0xff01)
                           .data:0000ceea f6 03                            or 0x03
                           .data:0000ceec ee ff                            xor 0xff
                           .data:0000ceee c3 de ce                         jp 0xcede
                           .data:0000cef1 e5                               push hl
                           .data:0000cef2 f5                               push af
                           .data:0000cef3 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000cef6 0f                               rrca
                           .data:0000cef7 0f                               rrca
                           .data:0000cef8 da 13 cf                         jp c,0xcf13
                           .data:0000cefb 21 db 8f                         ld hl,0x8fdb
                           .data:0000cefe 7e                               ld a,(hl)
                           .data:0000ceff 2f                               cpl
                           .data:0000cf00 77                               ld (hl),a
                           .data:0000cf01 2a 84 8f                         ld hl,(0x8f84)
                           .data:0000cf04 7e                               ld a,(hl)
                           .data:0000cf05 2f                               cpl
                           .data:0000cf06 77                               ld (hl),a
                           .data:0000cf07 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000cf0a fe 08                            cp 0x08
                           .data:0000cf0c ca 13 cf                         jp z,0xcf13
                           .data:0000cf0f 2c                               inc l
                           .data:0000cf10 7e                               ld a,(hl)
                           .data:0000cf11 2f                               cpl
                           .data:0000cf12 77                               ld (hl),a
                           .data:0000cf13 f1                               pop af
                           .data:0000cf14 e1                               pop hl
                           .data:0000cf15 c9                               ret
                           .data:0000cf16 00                               nop
                           .data:0000cf17 01 02 03                         ld bc,0x0302
                           .data:0000cf1a 04                               inc b
                           .data:0000cf1b 05                               dec b
                           .data:0000cf1c 06 07                            ld b,0x07
                           .data:0000cf1e 08                               ex af,af'
                           .data:0000cf1f 09                               add hl,bc
                           .data:0000cf20 fc 1f 00                         call m,0x001f
                           .data:0000cf23 00                               nop
                           .data:0000cf24 00                               nop
                           .data:0000cf25 00                               nop
                           .data:0000cf26 3b                               dec sp
                           .data:0000cf27 31 32 33                         ld sp,0x3332
                           .data:0000cf2a 34                               inc (hl)
                           .data:0000cf2b 35                               dec (hl)
                           .data:0000cf2c 36 37                            ld (hl),0x37
                           .data:0000cf2e 38 39                            jr c,0xcf69
                           .data:0000cf30 30 2d                            jr nc,0xcf5f
                           .data:0000cf32 00                               nop
                           .data:0000cf33 00                               nop
                           .data:0000cf34 00                               nop
                           .data:0000cf35 00                               nop
                           .data:0000cf36 4a                               ld c,d
                           .data:0000cf37 43                               ld b,e
                           .data:0000cf38 55                               ld d,l
                           .data:0000cf39 4b                               ld c,e
                           .data:0000cf3a 45                               ld b,l
                           .data:0000cf3b 4e                               ld c,(hl)
                           .data:0000cf3c 47                               ld b,a
                           .data:0000cf3d 5b                               ld e,e
                           .data:0000cf3e 5d                               ld e,l
                           .data:0000cf3f 5a                               ld e,d
                           .data:0000cf40 48                               ld c,b
                           .data:0000cf41 3a 00 00                         ld a,(0x0000)
                           .data:0000cf44 00                               nop
                           .data:0000cf45 00                               nop
                           .data:0000cf46 46                               ld b,(hl)
                           .data:0000cf47 59                               ld e,c
                           .data:0000cf48 57                               ld d,a
                           .data:0000cf49 41                               ld b,c
                           .data:0000cf4a 50                               ld d,b
                           .data:0000cf4b 52                               ld d,d
                           .data:0000cf4c 4f                               ld c,a
                           .data:0000cf4d 4c                               ld c,h
                           .data:0000cf4e 44                               ld b,h
                           .data:0000cf4f 56                               ld d,(hl)
                           .data:0000cf50 5c                               ld e,h
                           .data:0000cf51 2e 00                            ld l,0x00
                           .data:0000cf53 00                               nop
                           .data:0000cf54 00                               nop
                           .data:0000cf55 00                               nop
                           .data:0000cf56 51                               ld d,c
                           .data:0000cf57 5e                               ld e,(hl)
                           .data:0000cf58 53                               ld d,e
                           .data:0000cf59 4d                               ld c,l
                           .data:0000cf5a 49                               ld c,c
                           .data:0000cf5b 54                               ld d,h
                           .data:0000cf5c 58                               ld e,b
                           .data:0000cf5d 42                               ld b,d
                           .data:0000cf5e 40                               ld b,b
                           .data:0000cf5f 2c                               inc l
                           .data:0000cf60 2f                               cpl
                           .data:0000cf61 5f                               ld e,a
                           .data:0000cf62 00                               nop
                           .data:0000cf63 00                               nop
                           .data:0000cf64 00                               nop
                           .data:0000cf65 00                               nop
                           .data:0000cf66 fe 0c                            cp 0x0c
                           .data:0000cf68 19                               add hl,de
                           .data:0000cf69 1a                               ld a,(de)
                           .data:0000cf6a 09                               add hl,bc
                           .data:0000cf6b 1b                               dec de
                           .data:0000cf6c 20 08                            jr nz,0xcf76
                           .data:0000cf6e fd                               defb 0xfd
                           .data:0000cf6f 18 0a                            jr 0xcf7b
                           .data:0000cf71 0d                               dec c
                           .data:0000cf72 e5                               push hl
                           .data:0000cf73 d5                               push de
                           .data:0000cf74 c5                               push bc
                           .data:0000cf75 3e 82                            ld a,0x82
                           .data:0000cf77 32 03 ff                         ld (0xff03),a
                           .data:0000cf7a 01 00 80                         ld bc,0x8000
                           .data:0000cf7d 21 01 00                         ld hl,0x0001
                           .data:0000cf80 7d                               ld a,l
                           .data:0000cf81 2f                               cpl
                           .data:0000cf82 32 00 ff                         ld (0xff00),a
                           .data:0000cf85 7c                               ld a,h
                           .data:0000cf86 2f                               cpl
                           .data:0000cf87 e6 0f                            and 0x0f
                           .data:0000cf89 32 02 ff                         ld (0xff02),a
                           .data:0000cf8c 3a 01 ff                         ld a,(0xff01)
                           .data:0000cf8f f6 03                            or 0x03
                           .data:0000cf91 2f                               cpl
                           .data:0000cf92 a0                               and b
                           .data:0000cf93 ca 99 cf                         jp z,0xcf99
                           .data:0000cf96 e5                               push hl
                           .data:0000cf97 c5                               push bc
                           .data:0000cf98 0c                               inc c
                           .data:0000cf99 29                               add hl,hl
                           .data:0000cf9a d2 80 cf                         jp nc,0xcf80
                           .data:0000cf9d 78                               ld a,b
                           .data:0000cf9e 0f                               rrca
                           .data:0000cf9f 47                               ld b,a
                           .data:0000cfa0 fe 02                            cp 0x02
                           .data:0000cfa2 c2 7d cf                         jp nz,0xcf7d
                           .data:0000cfa5 79                               ld a,c
                           .data:0000cfa6 b7                               or a
                           .data:0000cfa7 ca 32 d0                         jp z,0xd032
                           .data:0000cfaa fe 03                            cp 0x03
                           .data:0000cfac da b8 cf                         jp c,0xcfb8
                           .data:0000cfaf d1                               pop de
                           .data:0000cfb0 e1                               pop hl
                           .data:0000cfb1 0d                               dec c
                           .data:0000cfb2 c2 af cf                         jp nz,0xcfaf
                           .data:0000cfb5 c3 32 d0                         jp 0xd032
                           .data:0000cfb8 06 ff                            ld b,0xff
                           .data:0000cfba e1                               pop hl
                           .data:0000cfbb d1                               pop de
                           .data:0000cfbc cd 38 d0                         call 0xd038
                           .data:0000cfbf 21 d4 8f                         ld hl,0x8fd4
                           .data:0000cfc2 fe fe                            cp 0xfe
                           .data:0000cfc4 c2 cc cf                         jp nz,0xcfcc
                           .data:0000cfc7 36 10                            ld (hl),0x10
                           .data:0000cfc9 c3 f5 cf                         jp 0xcff5
                           .data:0000cfcc fe fd                            cp 0xfd
                           .data:0000cfce c2 d6 cf                         jp nz,0xcfd6
                           .data:0000cfd1 36 1f                            ld (hl),0x1f
                           .data:0000cfd3 c3 f5 cf                         jp 0xcff5
                           .data:0000cfd6 fe fc                            cp 0xfc
                           .data:0000cfd8 c2 ec cf                         jp nz,0xcfec
                           .data:0000cfdb 23                               inc hl
                           .data:0000cfdc 3e 20                            ld a,0x20
                           .data:0000cfde ae                               xor (hl)
                           .data:0000cfdf 77                               ld (hl),a
                           .data:0000cfe0 cd 71 d0                         call 0xd071
                           .data:0000cfe3 cd ca ce                         call 0xceca
                           .data:0000cfe6 c2 e3 cf                         jp nz,0xcfe3
                           .data:0000cfe9 c3 f5 cf                         jp 0xcff5
                           .data:0000cfec fe fb                            cp 0xfb
                           .data:0000cfee c2 f4 cf                         jp nz,0xcff4
                           .data:0000cff1 3a dd 8f                         ld a,(0x8fdd)
                           .data:0000cff4 47                               ld b,a
                           .data:0000cff5 0d                               dec c
                           .data:0000cff6 c2 ba cf                         jp nz,0xcfba
                           .data:0000cff9 78                               ld a,b
                           .data:0000cffa fe ff                            cp 0xff
                           .data:0000cffc ca 32 d0                         jp z,0xd032
                           .data:0000cfff 32 dd 8f                         ld (0x8fdd),a
                           .data:0000d002 fe 40                            cp 0x40
                           .data:0000d004 da 0c d0                         jp c,0xd00c
                           .data:0000d007 3a d5 8f                         ld a,(0x8fd5)
                           .data:0000d00a b0                               or b
                           .data:0000d00b 47                               ld b,a
                           .data:0000d00c fe 40                            cp 0x40
                           .data:0000d00e d2 20 d0                         jp nc,0xd020
                           .data:0000d011 fe 20                            cp 0x20
                           .data:0000d013 da 20 d0                         jp c,0xd020
                           .data:0000d016 3a d4 8f                         ld a,(0x8fd4)
                           .data:0000d019 fe 10                            cp 0x10
                           .data:0000d01b c2 20 d0                         jp nz,0xd020
                           .data:0000d01e a8                               xor b
                           .data:0000d01f 47                               ld b,a
                           .data:0000d020 3a d4 8f                         ld a,(0x8fd4)
                           .data:0000d023 fe 1f                            cp 0x1f
                           .data:0000d025 c2 2a d0                         jp nz,0xd02a
                           .data:0000d028 a0                               and b
                           .data:0000d029 47                               ld b,a
                           .data:0000d02a 78                               ld a,b
                           .data:0000d02b 32 dd 8f                         ld (0x8fdd),a
                           .data:0000d02e b7                               or a
                           .data:0000d02f c3 6f ce                         jp 0xce6f
                           .data:0000d032 3e ff                            ld a,0xff
                           .data:0000d034 37                               scf
                           .data:0000d035 c3 6f ce                         jp 0xce6f
                           .data:0000d038 c5                               push bc
                           .data:0000d039 7c                               ld a,h
                           .data:0000d03a 01 00 00                         ld bc,0x0000
                           .data:0000d03d 07                               rlca
                           .data:0000d03e da 45 d0                         jp c,0xd045
                           .data:0000d041 04                               inc b
                           .data:0000d042 c3 3d d0                         jp 0xd03d
                           .data:0000d045 eb                               ex de,hl
                           .data:0000d046 29                               add hl,hl
                           .data:0000d047 29                               add hl,hl
                           .data:0000d048 29                               add hl,hl
                           .data:0000d049 29                               add hl,hl
                           .data:0000d04a 29                               add hl,hl
                           .data:0000d04b da 52 d0                         jp c,0xd052
                           .data:0000d04e 0c                               inc c
                           .data:0000d04f c3 4a d0                         jp 0xd04a
                           .data:0000d052 eb                               ex de,hl
                           .data:0000d053 78                               ld a,b
                           .data:0000d054 07                               rlca
                           .data:0000d055 07                               rlca
                           .data:0000d056 07                               rlca
                           .data:0000d057 07                               rlca
                           .data:0000d058 b1                               or c
                           .data:0000d059 4f                               ld c,a
                           .data:0000d05a 06 00                            ld b,0x00
                           .data:0000d05c 2a d9 8f                         ld hl,(0x8fd9)
                           .data:0000d05f 09                               add hl,bc
                           .data:0000d060 7e                               ld a,(hl)
                           .data:0000d061 c1                               pop bc
                           .data:0000d062 c9                               ret
                           .data:0000d063 3a d7 8f                         ld a,(0x8fd7)
                           .data:0000d066 3d                               dec a
                           .data:0000d067 c2 66 d0                         jp nz,0xd066
                           .data:0000d06a c9                               ret
                           .data:0000d06b 3e 50                            ld a,0x50
                           .data:0000d06d c9                               ret
                           .data:0000d06e 3e 40                            ld a,0x40
                           .data:0000d070 c9                               ret
                           .data:0000d071 f5                               push af
                           .data:0000d072 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d075 87                               add a,a
                           .data:0000d076 fa 8f d0                         jp m,0xd08f
                           .data:0000d079 3a d5 8f                         ld a,(0x8fd5)
                           .data:0000d07c e6 20                            and 0x20
                           .data:0000d07e c4 6e d0                         call nz,0xd06e
                           .data:0000d081 cc 6b d0                         call z,0xd06b
                           .data:0000d084 32 f1 8f                         ld (0x8ff1),a
                           .data:0000d087 3e 40                            ld a,0x40
                           .data:0000d089 32 f2 8f                         ld (0x8ff2),a
                           .data:0000d08c cd 70 c1                         call 0xc170
                           .data:0000d08f f1                               pop af
                           .data:0000d090 c9                               ret
                           .data:0000d091 2a 84 8f                         ld hl,(0x8f84)
                           .data:0000d094 3a db 8f                         ld a,(0x8fdb)
                           .data:0000d097 b7                               or a
                           .data:0000d098 c8                               ret z
                           .data:0000d099 cd f1 ce                         call 0xcef1
                           .data:0000d09c c9                               ret
                           .data:0000d09d cd f1 ce                         call 0xcef1
                           .data:0000d0a0 3e 92                            ld a,0x92
                           .data:0000d0a2 32 03 ff                         ld (0xff03),a
                           .data:0000d0a5 3a 01 ff                         ld a,(0xff01)
                           .data:0000d0a8 b7                               or a
                           .data:0000d0a9 fa 9d d0                         jp m,0xd09d
                           .data:0000d0ac cd b5 d0                         call 0xd0b5
                           .data:0000d0af 32 dd 8f                         ld (0x8fdd),a
                           .data:0000d0b2 c3 63 ce                         jp 0xce63
                           .data:0000d0b5 e5                               push hl
                           .data:0000d0b6 3e 92                            ld a,0x92
                           .data:0000d0b8 32 03 ff                         ld (0xff03),a
                           .data:0000d0bb 3a 00 ff                         ld a,(0xff00)
                           .data:0000d0be f5                               push af
                           .data:0000d0bf 3e 01                            ld a,0x01
                           .data:0000d0c1 32 02 ff                         ld (0xff02),a
                           .data:0000d0c4 af                               xor a
                           .data:0000d0c5 32 02 ff                         ld (0xff02),a
                           .data:0000d0c8 f1                               pop af
                           .data:0000d0c9 e1                               pop hl
                           .data:0000d0ca c9                               ret
                           .data:0000d0cb cd ca ce                         call 0xceca
                           .data:0000d0ce 2f                               cpl
                           .data:0000d0cf c8                               ret z
                           .data:0000d0d0 32 d4 8f                         ld (0x8fd4),a
                           .data:0000d0d3 3a d1 8f                         ld a,(0x8fd1)
                           .data:0000d0d6 fe 73                            cp 0x73
                           .data:0000d0d8 c2 df d0                         jp nz,0xd0df
                           .data:0000d0db cd 72 cf                         call 0xcf72
                           .data:0000d0de c9                               ret
                           .data:0000d0df cd b5 d0                         call 0xd0b5
                           .data:0000d0e2 c9                               ret
                           .data:0000d0e3 cd cb d0                         call 0xd0cb
                           .data:0000d0e6 c8                               ret z
                           .data:0000d0e7 f5                               push af
                           .data:0000d0e8 cd ca ce                         call 0xceca
                           .data:0000d0eb c2 e8 d0                         jp nz,0xd0e8
                           .data:0000d0ee cd cb d0                         call 0xd0cb
                           .data:0000d0f1 f1                               pop af
                           .data:0000d0f2 c9                               ret
                           .data:0000d0f3 e5                               push hl
                           .data:0000d0f4 d5                               push de
                           .data:0000d0f5 c5                               push bc
                           .data:0000d0f6 f5                               push af
                           .data:0000d0f7 79                               ld a,c
                           .data:0000d0f8 c3 a2 8f                         jp 0x8fa2
                           .data:0000d0fb e5                               push hl
                           .data:0000d0fc d5                               push de
                           .data:0000d0fd c5                               push bc
                           .data:0000d0fe f5                               push af
                           .data:0000d0ff 21 82 8f                         ld hl,0x8f82
                           .data:0000d102 11 84 d3                         ld de,0xd384
                           .data:0000d105 06 30                            ld b,0x30
                           .data:0000d107 cd cb d3                         call 0xd3cb
                           .data:0000d10a cd 10 c0                         call 0xc010
                           .data:0000d10d c3 e9 d4                         jp 0xd4e9
                           .data:0000d110 cd b4 d3                         call 0xd3b4
                           .data:0000d113 21 6d d1                         ld hl,0xd16d
                           .data:0000d116 22 a3 8f                         ld (0x8fa3),hl
                           .data:0000d119 af                               xor a
                           .data:0000d11a 32 ab 8f                         ld (0x8fab),a
                           .data:0000d11d f1                               pop af
                           .data:0000d11e c1                               pop bc
                           .data:0000d11f d1                               pop de
                           .data:0000d120 e1                               pop hl
                           .data:0000d121 c9                               ret
                           .data:0000d122 fe 01                            cp 0x01
                           .data:0000d124 c2 2d d1                         jp nz,0xd12d
                           .data:0000d127 21 72 d1                         ld hl,0xd172
                           .data:0000d12a c3 16 d1                         jp 0xd116
                           .data:0000d12d fe 1b                            cp 0x1b
                           .data:0000d12f c2 38 d1                         jp nz,0xd138
                           .data:0000d132 21 ab d1                         ld hl,0xd1ab
                           .data:0000d135 c3 16 d1                         jp 0xd116
                           .data:0000d138 fe 07                            cp 0x07
                           .data:0000d13a ca e3 d4                         jp z,0xd4e3
                           .data:0000d13d fe 08                            cp 0x08
                           .data:0000d13f ca 6e d2                         jp z,0xd26e
                           .data:0000d142 fe 09                            cp 0x09
                           .data:0000d144 ca b2 d2                         jp z,0xd2b2
                           .data:0000d147 fe 0a                            cp 0x0a
                           .data:0000d149 ca d6 d2                         jp z,0xd2d6
                           .data:0000d14c fe 0c                            cp 0x0c
                           .data:0000d14e ca c4 d2                         jp z,0xd2c4
                           .data:0000d151 fe 0d                            cp 0x0d
                           .data:0000d153 ca cd d2                         jp z,0xd2cd
                           .data:0000d156 fe 18                            cp 0x18
                           .data:0000d158 ca 8a d2                         jp z,0xd28a
                           .data:0000d15b fe 19                            cp 0x19
                           .data:0000d15d ca 28 d2                         jp z,0xd228
                           .data:0000d160 fe 1a                            cp 0x1a
                           .data:0000d162 ca 47 d2                         jp z,0xd247
                           .data:0000d165 fe 1f                            cp 0x1f
                           .data:0000d167 ca d7 d4                         jp z,0xd4d7
                           .data:0000d16a c3 1d d1                         jp 0xd11d
                           .data:0000d16d fe 20                            cp 0x20
                           .data:0000d16f da 22 d1                         jp c,0xd122
                           .data:0000d172 21 8d 8f                         ld hl,0x8f8d
                           .data:0000d175 0f                               rrca
                           .data:0000d176 0f                               rrca
                           .data:0000d177 0f                               rrca
                           .data:0000d178 0f                               rrca
                           .data:0000d179 e6 0e                            and 0x0e
                           .data:0000d17b 85                               add a,l
                           .data:0000d17c 6f                               ld l,a
                           .data:0000d17d 3e 00                            ld a,0x00
                           .data:0000d17f 8c                               adc a,h
                           .data:0000d180 67                               ld h,a
                           .data:0000d181 5e                               ld e,(hl)
                           .data:0000d182 23                               inc hl
                           .data:0000d183 56                               ld d,(hl)
                           .data:0000d184 79                               ld a,c
                           .data:0000d185 e6 1f                            and 0x1f
                           .data:0000d187 6f                               ld l,a
                           .data:0000d188 26 00                            ld h,0x00
                           .data:0000d18a 29                               add hl,hl
                           .data:0000d18b 29                               add hl,hl
                           .data:0000d18c 29                               add hl,hl
                           .data:0000d18d 19                               add hl,de
                           .data:0000d18e eb                               ex de,hl
                           .data:0000d18f 2a 84 8f                         ld hl,(0x8f84)
                           .data:0000d192 7d                               ld a,l
                           .data:0000d193 d6 07                            sub 0x07
                           .data:0000d195 6f                               ld l,a
                           .data:0000d196 0e 08                            ld c,0x08
                           .data:0000d198 1a                               ld a,(de)
                           .data:0000d199 cd 9d 8f                         call 0x8f9d
                           .data:0000d19c 77                               ld (hl),a
                           .data:0000d19d 2c                               inc l
                           .data:0000d19e 1c                               inc e
                           .data:0000d19f 0d                               dec c
                           .data:0000d1a0 c2 98 d1                         jp nz,0xd198
                           .data:0000d1a3 c3 8a d2                         jp 0xd28a
                           .data:0000d1a6 2f                               cpl
                           .data:0000d1a7 ae                               xor (hl)
                           .data:0000d1a8 c9                               ret
                           .data:0000d1a9 2f                               cpl
                           .data:0000d1aa c9                               ret
                           .data:0000d1ab fe 20                            cp 0x20
                           .data:0000d1ad da 13 d1                         jp c,0xd113
                           .data:0000d1b0 fe 60                            cp 0x60
                           .data:0000d1b2 d2 13 d1                         jp nc,0xd113
                           .data:0000d1b5 e6 1f                            and 0x1f
                           .data:0000d1b7 07                               rlca
                           .data:0000d1b8 21 c5 d1                         ld hl,0xd1c5
                           .data:0000d1bb 01 00 00                         ld bc,0x0000
                           .data:0000d1be 4f                               ld c,a
                           .data:0000d1bf 09                               add hl,bc
                           .data:0000d1c0 5e                               ld e,(hl)
                           .data:0000d1c1 23                               inc hl
                           .data:0000d1c2 56                               ld d,(hl)
                           .data:0000d1c3 eb                               ex de,hl
                           .data:0000d1c4 e9                               jp (hl)
                           .data:0000d1c5 e9                               jp (hl)
                           .data:0000d1c6 d5                               push de
                           .data:0000d1c7 28 d2                            jr z,0xd19b
                           .data:0000d1c9 47                               ld b,a
                           .data:0000d1ca d2 8a d2                         jp nc,0xd28a
                           .data:0000d1cd 6e                               ld l,(hl)
                           .data:0000d1ce d2 ff d0                         jp nc,0xd0ff
                           .data:0000d1d1 c2 d3 cd                         jp nz,0xcdd3
                           .data:0000d1d4 d2 c4 d2                         jp nc,0xd2c4
                           .data:0000d1d7 16 d6                            ld d,0xd6
                           .data:0000d1d9 93                               sub e
                           .data:0000d1da d4 64 d4                         call nc,0xd464
                           .data:0000d1dd ed d2                            defb 0xed, 0xd2
                           .data:0000d1df 01 d3 e9                         ld bc,0xe9d3
                           .data:0000d1e2 d4 01 d5                         call nc,0xd501
                           .data:0000d1e5 07                               rlca
                           .data:0000d1e6 d3 0f                            out (0x0f),a
                           .data:0000d1e8 d3 2d                            out (0x2d),a
                           .data:0000d1ea d6 e0                            sub 0xe0
                           .data:0000d1ec d3 09                            out (0x09),a
                           .data:0000d1ee d4 38 d6                         call nc,0xd638
                           .data:0000d1f1 17                               rla
                           .data:0000d1f2 d3 27                            out (0x27),a
                           .data:0000d1f4 d3 05                            out (0x05),a
                           .data:0000d1f6 d2 0a d5                         jp nc,0xd50a
                           .data:0000d1f9 10 d5                            djnz 0xd1d0
                           .data:0000d1fb 89                               adc a,c
                           .data:0000d1fc d5                               push de
                           .data:0000d1fd 8f                               adc a,a
                           .data:0000d1fe d5                               push de
                           .data:0000d1ff 95                               sub l
                           .data:0000d200 d5                               push de
                           .data:0000d201 9b                               sbc a,e
                           .data:0000d202 d5                               push de
                           .data:0000d203 6c                               ld l,h
                           .data:0000d204 d5                               push de
                           .data:0000d205 21 0b d2                         ld hl,0xd20b
                           .data:0000d208 c3 16 d1                         jp 0xd116
                           .data:0000d20b 32 8a 8f                         ld (0x8f8a),a
                           .data:0000d20e cd 17 d2                         call 0xd217
                           .data:0000d211 22 9e 8f                         ld (0x8f9e),hl
                           .data:0000d214 c3 10 d1                         jp 0xd110
                           .data:0000d217 21 a6 d1                         ld hl,0xd1a6
                           .data:0000d21a e6 a0                            and 0xa0
                           .data:0000d21c fe a0                            cp 0xa0
                           .data:0000d21e c8                               ret z
                           .data:0000d21f 23                               inc hl
                           .data:0000d220 fe 20                            cp 0x20
                           .data:0000d222 c8                               ret z
                           .data:0000d223 23                               inc hl
                           .data:0000d224 b7                               or a
                           .data:0000d225 c8                               ret z
                           .data:0000d226 23                               inc hl
                           .data:0000d227 c9                               ret
                           .data:0000d228 3a 87 8f                         ld a,(0x8f87)
                           .data:0000d22b 6f                               ld l,a
                           .data:0000d22c 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d22f 4f                               ld c,a
                           .data:0000d230 3a 83 8f                         ld a,(0x8f83)
                           .data:0000d233 91                               sub c
                           .data:0000d234 da 3e d2                         jp c,0xd23e
                           .data:0000d237 32 83 8f                         ld (0x8f83),a
                           .data:0000d23a bd                               cp l
                           .data:0000d23b d2 10 d1                         jp nc,0xd110
                           .data:0000d23e 3a 89 8f                         ld a,(0x8f89)
                           .data:0000d241 32 83 8f                         ld (0x8f83),a
                           .data:0000d244 c3 10 d1                         jp 0xd110
                           .data:0000d247 3a 83 8f                         ld a,(0x8f83)
                           .data:0000d24a 4f                               ld c,a
                           .data:0000d24b 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d24e 81                               add a,c
                           .data:0000d24f da 5d d2                         jp c,0xd25d
                           .data:0000d252 6f                               ld l,a
                           .data:0000d253 32 83 8f                         ld (0x8f83),a
                           .data:0000d256 3a 89 8f                         ld a,(0x8f89)
                           .data:0000d259 bd                               cp l
                           .data:0000d25a d2 10 d1                         jp nc,0xd110
                           .data:0000d25d 3a 89 8f                         ld a,(0x8f89)
                           .data:0000d260 32 83 8f                         ld (0x8f83),a
                           .data:0000d263 7c                               ld a,h
                           .data:0000d264 b7                               or a
                           .data:0000d265 ca d9 d3                         jp z,0xd3d9
                           .data:0000d268 3a 87 8f                         ld a,(0x8f87)
                           .data:0000d26b c3 41 d2                         jp 0xd241
                           .data:0000d26e 3a 86 8f                         ld a,(0x8f86)
                           .data:0000d271 6f                               ld l,a
                           .data:0000d272 3a 82 8f                         ld a,(0x8f82)
                           .data:0000d275 d6 01                            sub 0x01
                           .data:0000d277 da 81 d2                         jp c,0xd281
                           .data:0000d27a 32 82 8f                         ld (0x8f82),a
                           .data:0000d27d bd                               cp l
                           .data:0000d27e d2 10 d1                         jp nc,0xd110
                           .data:0000d281 3a 88 8f                         ld a,(0x8f88)
                           .data:0000d284 32 82 8f                         ld (0x8f82),a
                           .data:0000d287 c3 28 d2                         jp 0xd228
                           .data:0000d28a 3a 82 8f                         ld a,(0x8f82)
                           .data:0000d28d 3c                               inc a
                           .data:0000d28e 6f                               ld l,a
                           .data:0000d28f 3a 88 8f                         ld a,(0x8f88)
                           .data:0000d292 bd                               cp l
                           .data:0000d293 da 9d d2                         jp c,0xd29d
                           .data:0000d296 7d                               ld a,l
                           .data:0000d297 32 82 8f                         ld (0x8f82),a
                           .data:0000d29a c3 10 d1                         jp 0xd110
                           .data:0000d29d 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d2a0 e6 04                            and 0x04
                           .data:0000d2a2 3a 88 8f                         ld a,(0x8f88)
                           .data:0000d2a5 c2 97 d2                         jp nz,0xd297
                           .data:0000d2a8 3a 86 8f                         ld a,(0x8f86)
                           .data:0000d2ab 32 82 8f                         ld (0x8f82),a
                           .data:0000d2ae 61                               ld h,c
                           .data:0000d2af c3 47 d2                         jp 0xd247
                           .data:0000d2b2 3a 82 8f                         ld a,(0x8f82)
                           .data:0000d2b5 c6 04                            add a,0x04
                           .data:0000d2b7 e6 3c                            and 0x3c
                           .data:0000d2b9 6f                               ld l,a
                           .data:0000d2ba 3a 88 8f                         ld a,(0x8f88)
                           .data:0000d2bd bd                               cp l
                           .data:0000d2be da 97 d2                         jp c,0xd297
                           .data:0000d2c1 c3 96 d2                         jp 0xd296
                           .data:0000d2c4 2a 86 8f                         ld hl,(0x8f86)
                           .data:0000d2c7 22 82 8f                         ld (0x8f82),hl
                           .data:0000d2ca c3 10 d1                         jp 0xd110
                           .data:0000d2cd 3a 86 8f                         ld a,(0x8f86)
                           .data:0000d2d0 32 82 8f                         ld (0x8f82),a
                           .data:0000d2d3 c3 10 d1                         jp 0xd110
                           .data:0000d2d6 3a 83 8f                         ld a,(0x8f83)
                           .data:0000d2d9 4f                               ld c,a
                           .data:0000d2da 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d2dd 81                               add a,c
                           .data:0000d2de 6f                               ld l,a
                           .data:0000d2df 3a 89 8f                         ld a,(0x8f89)
                           .data:0000d2e2 da d9 d3                         jp c,0xd3d9
                           .data:0000d2e5 bd                               cp l
                           .data:0000d2e6 da d9 d3                         jp c,0xd3d9
                           .data:0000d2e9 7d                               ld a,l
                           .data:0000d2ea c3 41 d2                         jp 0xd241
                           .data:0000d2ed 2e 00                            ld l,0x00
                           .data:0000d2ef 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d2f2 3d                               dec a
                           .data:0000d2f3 67                               ld h,a
                           .data:0000d2f4 22 86 8f                         ld (0x8f86),hl
                           .data:0000d2f7 21 2f ff                         ld hl,0xff2f
                           .data:0000d2fa 22 88 8f                         ld (0x8f88),hl
                           .data:0000d2fd 7c                               ld a,h
                           .data:0000d2fe c3 5b d3                         jp 0xd35b
                           .data:0000d301 21 3d d6                         ld hl,0xd63d
                           .data:0000d304 c3 16 d1                         jp 0xd116
                           .data:0000d307 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d30a e6 7f                            and 0x7f
                           .data:0000d30c c3 0b d2                         jp 0xd20b
                           .data:0000d30f 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d312 f6 80                            or 0x80
                           .data:0000d314 c3 0b d2                         jp 0xd20b
                           .data:0000d317 21 1d d3                         ld hl,0xd31d
                           .data:0000d31a c3 16 d1                         jp 0xd116
                           .data:0000d31d 2a a0 8f                         ld hl,(0x8fa0)
                           .data:0000d320 77                               ld (hl),a
                           .data:0000d321 32 8c 8f                         ld (0x8f8c),a
                           .data:0000d324 c3 13 d1                         jp 0xd113
                           .data:0000d327 21 2d d3                         ld hl,0xd32d
                           .data:0000d32a c3 16 d1                         jp 0xd116
                           .data:0000d32d fe 2e                            cp 0x2e
                           .data:0000d32f da 34 d3                         jp c,0xd334
                           .data:0000d332 3e 2e                            ld a,0x2e
                           .data:0000d334 32 86 8f                         ld (0x8f86),a
                           .data:0000d337 21 3d d3                         ld hl,0xd33d
                           .data:0000d33a c3 16 d1                         jp 0xd116
                           .data:0000d33d 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d340 3d                               dec a
                           .data:0000d341 81                               add a,c
                           .data:0000d342 32 87 8f                         ld (0x8f87),a
                           .data:0000d345 21 4b d3                         ld hl,0xd34b
                           .data:0000d348 c3 16 d1                         jp 0xd116
                           .data:0000d34b fe 2f                            cp 0x2f
                           .data:0000d34d da 52 d3                         jp c,0xd352
                           .data:0000d350 3e 2f                            ld a,0x2f
                           .data:0000d352 32 88 8f                         ld (0x8f88),a
                           .data:0000d355 21 5b d3                         ld hl,0xd35b
                           .data:0000d358 c3 16 d1                         jp 0xd116
                           .data:0000d35b 32 89 8f                         ld (0x8f89),a
                           .data:0000d35e 47                               ld b,a
                           .data:0000d35f 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d362 4f                               ld c,a
                           .data:0000d363 3a 87 8f                         ld a,(0x8f87)
                           .data:0000d366 b8                               cp b
                           .data:0000d367 d2 01 d5                         jp nc,0xd501
                           .data:0000d36a b8                               cp b
                           .data:0000d36b ca c4 d2                         jp z,0xd2c4
                           .data:0000d36e d2 79 d3                         jp nc,0xd379
                           .data:0000d371 81                               add a,c
                           .data:0000d372 d2 6a d3                         jp nc,0xd36a
                           .data:0000d375 91                               sub c
                           .data:0000d376 c3 5b d3                         jp 0xd35b
                           .data:0000d379 5f                               ld e,a
                           .data:0000d37a 79                               ld a,c
                           .data:0000d37b fe 08                            cp 0x08
                           .data:0000d37d 7b                               ld a,e
                           .data:0000d37e ca 5b d3                         jp z,0xd35b
                           .data:0000d381 c3 75 d3                         jp 0xd375
                           .data:0000d384 04                               inc b
                           .data:0000d385 27                               daa
                           .data:0000d386 00                               nop
                           .data:0000d387 90                               sub b
                           .data:0000d388 04                               inc b
                           .data:0000d389 27                               daa
                           .data:0000d38a 2c                               inc l
                           .data:0000d38b e5                               push hl
                           .data:0000d38c 00                               nop
                           .data:0000d38d 0a                               ld a,(bc)
                           .data:0000d38e 0f                               rrca
                           .data:0000d38f 00                               nop
                           .data:0000d390 d8                               ret c
                           .data:0000d391 00                               nop
                           .data:0000d392 d9                               exx
                           .data:0000d393 00                               nop
                           .data:0000d394 da 00 df                         jp c,0xdf00
                           .data:0000d397 00                               nop
                           .data:0000d398 dc 00 d9                         call c,0xd900
                           .data:0000d39b 00                               nop
                           .data:0000d39c db 00                            in a,(0x00)
                           .data:0000d39e de c3                            sbc a,0xc3
                           .data:0000d3a0 a8                               xor b
                           .data:0000d3a1 d1                               pop de
                           .data:0000d3a2 00                               nop
                           .data:0000d3a3 f0                               ret p
                           .data:0000d3a4 c3 6d d1                         jp 0xd16d
                           .data:0000d3a7 c3 6a d7                         jp 0xd76a
                           .data:0000d3aa c3 75 d7                         jp 0xd775
                           .data:0000d3ad 00                               nop
                           .data:0000d3ae 10 01                            djnz 0xd3b1
                           .data:0000d3b0 00                               nop
                           .data:0000d3b1 00                               nop
                           .data:0000d3b2 00                               nop
                           .data:0000d3b3 00                               nop
                           .data:0000d3b4 e5                               push hl
                           .data:0000d3b5 2a 82 8f                         ld hl,(0x8f82)
                           .data:0000d3b8 3e 90                            ld a,0x90
                           .data:0000d3ba 85                               add a,l
                           .data:0000d3bb 6c                               ld l,h
                           .data:0000d3bc 67                               ld h,a
                           .data:0000d3bd 22 84 8f                         ld (0x8f84),hl
                           .data:0000d3c0 e1                               pop hl
                           .data:0000d3c1 c9                               ret
                           .data:0000d3c2 3a 86 8f                         ld a,(0x8f86)
                           .data:0000d3c5 32 82 8f                         ld (0x8f82),a
                           .data:0000d3c8 c3 d6 d2                         jp 0xd2d6
                           .data:0000d3cb e5                               push hl
                           .data:0000d3cc d5                               push de
                           .data:0000d3cd c5                               push bc
                           .data:0000d3ce 1a                               ld a,(de)
                           .data:0000d3cf 77                               ld (hl),a
                           .data:0000d3d0 13                               inc de
                           .data:0000d3d1 23                               inc hl
                           .data:0000d3d2 05                               dec b
                           .data:0000d3d3 c2 ce d3                         jp nz,0xd3ce
                           .data:0000d3d6 c3 1e d1                         jp 0xd11e
                           .data:0000d3d9 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d3dc 0f                               rrca
                           .data:0000d3dd da c4 d2                         jp c,0xd2c4
                           .data:0000d3e0 cd 2a d4                         call 0xd42a
                           .data:0000d3e3 cd cb d3                         call 0xd3cb
                           .data:0000d3e6 24                               inc h
                           .data:0000d3e7 14                               inc d
                           .data:0000d3e8 0d                               dec c
                           .data:0000d3e9 c2 e3 d3                         jp nz,0xd3e3
                           .data:0000d3ec 2a 82 8f                         ld hl,(0x8f82)
                           .data:0000d3ef e5                               push hl
                           .data:0000d3f0 2a 88 8f                         ld hl,(0x8f88)
                           .data:0000d3f3 22 82 8f                         ld (0x8f82),hl
                           .data:0000d3f6 3a 86 8f                         ld a,(0x8f86)
                           .data:0000d3f9 32 82 8f                         ld (0x8f82),a
                           .data:0000d3fc cd b4 d3                         call 0xd3b4
                           .data:0000d3ff cd 6a d4                         call 0xd46a
                           .data:0000d402 e1                               pop hl
                           .data:0000d403 22 82 8f                         ld (0x8f82),hl
                           .data:0000d406 c3 10 d1                         jp 0xd110
                           .data:0000d409 cd 2a d4                         call 0xd42a
                           .data:0000d40c 7d                               ld a,l
                           .data:0000d40d 80                               add a,b
                           .data:0000d40e 3d                               dec a
                           .data:0000d40f 6f                               ld l,a
                           .data:0000d410 7b                               ld a,e
                           .data:0000d411 80                               add a,b
                           .data:0000d412 3d                               dec a
                           .data:0000d413 5f                               ld e,a
                           .data:0000d414 cd 56 d4                         call 0xd456
                           .data:0000d417 24                               inc h
                           .data:0000d418 14                               inc d
                           .data:0000d419 0d                               dec c
                           .data:0000d41a c2 14 d4                         jp nz,0xd414
                           .data:0000d41d 2a 82 8f                         ld hl,(0x8f82)
                           .data:0000d420 e5                               push hl
                           .data:0000d421 2a 86 8f                         ld hl,(0x8f86)
                           .data:0000d424 22 82 8f                         ld (0x8f82),hl
                           .data:0000d427 c3 fc d3                         jp 0xd3fc
                           .data:0000d42a 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d42d 4f                               ld c,a
                           .data:0000d42e 3a 87 8f                         ld a,(0x8f87)
                           .data:0000d431 3c                               inc a
                           .data:0000d432 5f                               ld e,a
                           .data:0000d433 91                               sub c
                           .data:0000d434 d2 3f d4                         jp nc,0xd43f
                           .data:0000d437 79                               ld a,c
                           .data:0000d438 3d                               dec a
                           .data:0000d439 32 87 8f                         ld (0x8f87),a
                           .data:0000d43c c3 2a d4                         jp 0xd42a
                           .data:0000d43f 6f                               ld l,a
                           .data:0000d440 3a 89 8f                         ld a,(0x8f89)
                           .data:0000d443 93                               sub e
                           .data:0000d444 3c                               inc a
                           .data:0000d445 47                               ld b,a
                           .data:0000d446 3a 86 8f                         ld a,(0x8f86)
                           .data:0000d449 67                               ld h,a
                           .data:0000d44a 3a 88 8f                         ld a,(0x8f88)
                           .data:0000d44d 94                               sub h
                           .data:0000d44e 3c                               inc a
                           .data:0000d44f 4f                               ld c,a
                           .data:0000d450 7c                               ld a,h
                           .data:0000d451 c6 90                            add a,0x90
                           .data:0000d453 67                               ld h,a
                           .data:0000d454 54                               ld d,h
                           .data:0000d455 c9                               ret
                           .data:0000d456 e5                               push hl
                           .data:0000d457 d5                               push de
                           .data:0000d458 c5                               push bc
                           .data:0000d459 7e                               ld a,(hl)
                           .data:0000d45a 12                               ld (de),a
                           .data:0000d45b 2d                               dec l
                           .data:0000d45c 1d                               dec e
                           .data:0000d45d 05                               dec b
                           .data:0000d45e c2 59 d4                         jp nz,0xd459
                           .data:0000d461 c3 1e d1                         jp 0xd11e
                           .data:0000d464 cd 6a d4                         call 0xd46a
                           .data:0000d467 c3 10 d1                         jp 0xd110
                           .data:0000d46a 2a 84 8f                         ld hl,(0x8f84)
                           .data:0000d46d 3a 88 8f                         ld a,(0x8f88)
                           .data:0000d470 c6 90                            add a,0x90
                           .data:0000d472 94                               sub h
                           .data:0000d473 3c                               inc a
                           .data:0000d474 47                               ld b,a
                           .data:0000d475 58                               ld e,b
                           .data:0000d476 54                               ld d,h
                           .data:0000d477 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d47a 4f                               ld c,a
                           .data:0000d47b 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d47e b7                               or a
                           .data:0000d47f 3e ff                            ld a,0xff
                           .data:0000d481 fa 85 d4                         jp m,0xd485
                           .data:0000d484 2f                               cpl
                           .data:0000d485 77                               ld (hl),a
                           .data:0000d486 24                               inc h
                           .data:0000d487 05                               dec b
                           .data:0000d488 c2 85 d4                         jp nz,0xd485
                           .data:0000d48b 43                               ld b,e
                           .data:0000d48c 62                               ld h,d
                           .data:0000d48d 2d                               dec l
                           .data:0000d48e 0d                               dec c
                           .data:0000d48f c2 85 d4                         jp nz,0xd485
                           .data:0000d492 c9                               ret
                           .data:0000d493 cd 99 d4                         call 0xd499
                           .data:0000d496 c3 10 d1                         jp 0xd110
                           .data:0000d499 cd b4 d3                         call 0xd3b4
                           .data:0000d49c cd 6a d4                         call 0xd46a
                           .data:0000d49f 2a 84 8f                         ld hl,(0x8f84)
                           .data:0000d4a2 3a 86 8f                         ld a,(0x8f86)
                           .data:0000d4a5 c6 90                            add a,0x90
                           .data:0000d4a7 67                               ld h,a
                           .data:0000d4a8 2c                               inc l
                           .data:0000d4a9 3a 89 8f                         ld a,(0x8f89)
                           .data:0000d4ac 4f                               ld c,a
                           .data:0000d4ad 3a 88 8f                         ld a,(0x8f88)
                           .data:0000d4b0 c6 90                            add a,0x90
                           .data:0000d4b2 57                               ld d,a
                           .data:0000d4b3 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d4b6 b7                               or a
                           .data:0000d4b7 3e ff                            ld a,0xff
                           .data:0000d4b9 fa bd d4                         jp m,0xd4bd
                           .data:0000d4bc 2f                               cpl
                           .data:0000d4bd 47                               ld b,a
                           .data:0000d4be cd c8 d4                         call 0xd4c8
                           .data:0000d4c1 7c                               ld a,h
                           .data:0000d4c2 ba                               cp d
                           .data:0000d4c3 c8                               ret z
                           .data:0000d4c4 24                               inc h
                           .data:0000d4c5 c3 be d4                         jp 0xd4be
                           .data:0000d4c8 e5                               push hl
                           .data:0000d4c9 c5                               push bc
                           .data:0000d4ca 79                               ld a,c
                           .data:0000d4cb 70                               ld (hl),b
                           .data:0000d4cc 2c                               inc l
                           .data:0000d4cd ca d4 d4                         jp z,0xd4d4
                           .data:0000d4d0 bd                               cp l
                           .data:0000d4d1 d2 cb d4                         jp nc,0xd4cb
                           .data:0000d4d4 c1                               pop bc
                           .data:0000d4d5 e1                               pop hl
                           .data:0000d4d6 c9                               ret
                           .data:0000d4d7 2a 86 8f                         ld hl,(0x8f86)
                           .data:0000d4da 22 82 8f                         ld (0x8f82),hl
                           .data:0000d4dd cd 99 d4                         call 0xd499
                           .data:0000d4e0 c3 10 d1                         jp 0xd110
                           .data:0000d4e3 cd 70 c1                         call 0xc170
                           .data:0000d4e6 c3 13 d1                         jp 0xd113
                           .data:0000d4e9 11 82 8f                         ld de,0x8f82
                           .data:0000d4ec 21 b2 8f                         ld hl,0x8fb2
                           .data:0000d4ef 06 1e                            ld b,0x1e
                           .data:0000d4f1 cd cb d3                         call 0xd3cb
                           .data:0000d4f4 2a a0 8f                         ld hl,(0x8fa0)
                           .data:0000d4f7 3a 8c 8f                         ld a,(0x8f8c)
                           .data:0000d4fa 77                               ld (hl),a
                           .data:0000d4fb 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d4fe c3 0b d2                         jp 0xd20b
                           .data:0000d501 11 b2 8f                         ld de,0x8fb2
                           .data:0000d504 21 82 8f                         ld hl,0x8f82
                           .data:0000d507 c3 ef d4                         jp 0xd4ef
                           .data:0000d50a 21 1f d5                         ld hl,0xd51f
                           .data:0000d50d c3 16 d1                         jp 0xd116
                           .data:0000d510 21 3e d5                         ld hl,0xd53e
                           .data:0000d513 c3 16 d1                         jp 0xd116
                           .data:0000d516 d6 20                            sub 0x20
                           .data:0000d518 4f                               ld c,a
                           .data:0000d519 cd 47 d5                         call 0xd547
                           .data:0000d51c c3 10 d1                         jp 0xd110
                           .data:0000d51f 21 16 d5                         ld hl,0xd516
                           .data:0000d522 d6 20                            sub 0x20
                           .data:0000d524 07                               rlca
                           .data:0000d525 47                               ld b,a
                           .data:0000d526 07                               rlca
                           .data:0000d527 07                               rlca
                           .data:0000d528 80                               add a,b
                           .data:0000d529 4f                               ld c,a
                           .data:0000d52a 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d52d fe 0a                            cp 0x0a
                           .data:0000d52f ca 41 d5                         jp z,0xd541
                           .data:0000d532 79                               ld a,c
                           .data:0000d533 90                               sub b
                           .data:0000d534 4f                               ld c,a
                           .data:0000d535 c3 41 d5                         jp 0xd541
                           .data:0000d538 cd 47 d5                         call 0xd547
                           .data:0000d53b c3 10 d1                         jp 0xd110
                           .data:0000d53e 21 38 d5                         ld hl,0xd538
                           .data:0000d541 cd 58 d5                         call 0xd558
                           .data:0000d544 c3 16 d1                         jp 0xd116
                           .data:0000d547 3a 86 8f                         ld a,(0x8f86)
                           .data:0000d54a 81                               add a,c
                           .data:0000d54b 4f                               ld c,a
                           .data:0000d54c 3a 88 8f                         ld a,(0x8f88)
                           .data:0000d54f b9                               cp c
                           .data:0000d550 da 54 d5                         jp c,0xd554
                           .data:0000d553 79                               ld a,c
                           .data:0000d554 32 82 8f                         ld (0x8f82),a
                           .data:0000d557 c9                               ret
                           .data:0000d558 3a 87 8f                         ld a,(0x8f87)
                           .data:0000d55b 81                               add a,c
                           .data:0000d55c 4f                               ld c,a
                           .data:0000d55d 3a 89 8f                         ld a,(0x8f89)
                           .data:0000d560 da 68 d5                         jp c,0xd568
                           .data:0000d563 b9                               cp c
                           .data:0000d564 da 68 d5                         jp c,0xd568
                           .data:0000d567 79                               ld a,c
                           .data:0000d568 32 83 8f                         ld (0x8f83),a
                           .data:0000d56b c9                               ret
                           .data:0000d56c 21 72 d5                         ld hl,0xd572
                           .data:0000d56f c3 16 d1                         jp 0xd116
                           .data:0000d572 3a ab 8f                         ld a,(0x8fab)
                           .data:0000d575 21 8d 8f                         ld hl,0x8f8d
                           .data:0000d578 5f                               ld e,a
                           .data:0000d579 3c                               inc a
                           .data:0000d57a 16 00                            ld d,0x00
                           .data:0000d57c 19                               add hl,de
                           .data:0000d57d 71                               ld (hl),c
                           .data:0000d57e fe 10                            cp 0x10
                           .data:0000d580 ca 13 d1                         jp z,0xd113
                           .data:0000d583 32 ab 8f                         ld (0x8fab),a
                           .data:0000d586 c3 1d d1                         jp 0xd11d
                           .data:0000d589 11 a9 d5                         ld de,0xd5a9
                           .data:0000d58c c3 9e d5                         jp 0xd59e
                           .data:0000d58f 11 b9 d5                         ld de,0xd5b9
                           .data:0000d592 c3 9e d5                         jp 0xd59e
                           .data:0000d595 11 c9 d5                         ld de,0xd5c9
                           .data:0000d598 c3 9e d5                         jp 0xd59e
                           .data:0000d59b 11 d9 d5                         ld de,0xd5d9
                           .data:0000d59e 21 8d 8f                         ld hl,0x8f8d
                           .data:0000d5a1 06 10                            ld b,0x10
                           .data:0000d5a3 cd cb d3                         call 0xd3cb
                           .data:0000d5a6 c3 13 d1                         jp 0xd113
                           .data:0000d5a9 00                               nop
                           .data:0000d5aa d8                               ret c
                           .data:0000d5ab 00                               nop
                           .data:0000d5ac d9                               exx
                           .data:0000d5ad 00                               nop
                           .data:0000d5ae da 00 db                         jp c,0xdb00
                           .data:0000d5b1 00                               nop
                           .data:0000d5b2 dc 00 d9                         call c,0xd900
                           .data:0000d5b5 00                               nop
                           .data:0000d5b6 de 00                            sbc a,0x00
                           .data:0000d5b8 df                               rst 0x18
                           .data:0000d5b9 00                               nop
                           .data:0000d5ba d8                               ret c
                           .data:0000d5bb 00                               nop
                           .data:0000d5bc d9                               exx
                           .data:0000d5bd 00                               nop
                           .data:0000d5be de 00                            sbc a,0x00
                           .data:0000d5c0 df                               rst 0x18
                           .data:0000d5c1 00                               nop
                           .data:0000d5c2 dc 00 d9                         call c,0xd900
                           .data:0000d5c5 00                               nop
                           .data:0000d5c6 da 00 db                         jp c,0xdb00
                           .data:0000d5c9 00                               nop
                           .data:0000d5ca d8                               ret c
                           .data:0000d5cb 00                               nop
                           .data:0000d5cc d9                               exx
                           .data:0000d5cd 00                               nop
                           .data:0000d5ce da 00 df                         jp c,0xdf00
                           .data:0000d5d1 00                               nop
                           .data:0000d5d2 dc 00 d9                         call c,0xd900
                           .data:0000d5d5 00                               nop
                           .data:0000d5d6 db 00                            in a,(0x00)
                           .data:0000d5d8 de 00                            sbc a,0x00
                           .data:0000d5da d8                               ret c
                           .data:0000d5db 00                               nop
                           .data:0000d5dc d9                               exx
                           .data:0000d5dd 00                               nop
                           .data:0000d5de da 00 db                         jp c,0xdb00
                           .data:0000d5e1 00                               nop
                           .data:0000d5e2 dc 00 dd                         call c,0xdd00
                           .data:0000d5e5 00                               nop
                           .data:0000d5e6 de 00                            sbc a,0x00
                           .data:0000d5e8 df                               rst 0x18
                           .data:0000d5e9 cd 2a d4                         call 0xd42a
                           .data:0000d5ec 0d                               dec c
                           .data:0000d5ed 79                               ld a,c
                           .data:0000d5ee 84                               add a,h
                           .data:0000d5ef 67                               ld h,a
                           .data:0000d5f0 5d                               ld e,l
                           .data:0000d5f1 54                               ld d,h
                           .data:0000d5f2 15                               dec d
                           .data:0000d5f3 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d5f6 80                               add a,b
                           .data:0000d5f7 47                               ld b,a
                           .data:0000d5f8 cd cb d3                         call 0xd3cb
                           .data:0000d5fb 15                               dec d
                           .data:0000d5fc 25                               dec h
                           .data:0000d5fd 0d                               dec c
                           .data:0000d5fe c2 f8 d5                         jp nz,0xd5f8
                           .data:0000d601 3a 89 8f                         ld a,(0x8f89)
                           .data:0000d604 4f                               ld c,a
                           .data:0000d605 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d608 b7                               or a
                           .data:0000d609 3e ff                            ld a,0xff
                           .data:0000d60b fa 0f d6                         jp m,0xd60f
                           .data:0000d60e 2f                               cpl
                           .data:0000d60f 47                               ld b,a
                           .data:0000d610 cd c8 d4                         call 0xd4c8
                           .data:0000d613 c3 10 d1                         jp 0xd110
                           .data:0000d616 cd 2a d4                         call 0xd42a
                           .data:0000d619 0d                               dec c
                           .data:0000d61a 3a 8b 8f                         ld a,(0x8f8b)
                           .data:0000d61d 80                               add a,b
                           .data:0000d61e 47                               ld b,a
                           .data:0000d61f 5d                               ld e,l
                           .data:0000d620 14                               inc d
                           .data:0000d621 cd cb d3                         call 0xd3cb
                           .data:0000d624 24                               inc h
                           .data:0000d625 14                               inc d
                           .data:0000d626 0d                               dec c
                           .data:0000d627 c2 21 d6                         jp nz,0xd621
                           .data:0000d62a c3 01 d6                         jp 0xd601
                           .data:0000d62d 3e 0a                            ld a,0x0a
                           .data:0000d62f 32 8b 8f                         ld (0x8f8b),a
                           .data:0000d632 3a 89 8f                         ld a,(0x8f89)
                           .data:0000d635 c3 5b d3                         jp 0xd35b
                           .data:0000d638 3e 08                            ld a,0x08
                           .data:0000d63a c3 2f d6                         jp 0xd62f
                           .data:0000d63d fe 2b                            cp 0x2b
                           .data:0000d63f da 13 d1                         jp c,0xd113
                           .data:0000d642 fe 41                            cp 0x41
                           .data:0000d644 c2 4c d6                         jp nz,0xd64c
                           .data:0000d647 3e 48                            ld a,0x48
                           .data:0000d649 c3 06 d7                         jp 0xd706
                           .data:0000d64c fe 48                            cp 0x48
                           .data:0000d64e c2 56 d6                         jp nz,0xd656
                           .data:0000d651 3e 49                            ld a,0x49
                           .data:0000d653 c3 06 d7                         jp 0xd706
                           .data:0000d656 fe 43                            cp 0x43
                           .data:0000d658 d2 06 d7                         jp nc,0xd706
                           .data:0000d65b fe 30                            cp 0x30
                           .data:0000d65d d2 30 d7                         jp nc,0xd730
                           .data:0000d660 fe 2e                            cp 0x2e
                           .data:0000d662 ca 7c d6                         jp z,0xd67c
                           .data:0000d665 fe 2d                            cp 0x2d
                           .data:0000d667 ca 77 d6                         jp z,0xd677
                           .data:0000d66a fe 2b                            cp 0x2b
                           .data:0000d66c c2 1d d1                         jp nz,0xd11d
                           .data:0000d66f 3e 01                            ld a,0x01
                           .data:0000d671 32 ae 8f                         ld (0x8fae),a
                           .data:0000d674 c3 1d d1                         jp 0xd11d
                           .data:0000d677 3e ff                            ld a,0xff
                           .data:0000d679 c3 71 d6                         jp 0xd671
                           .data:0000d67c 3a ac 8f                         ld a,(0x8fac)
                           .data:0000d67f 4f                               ld c,a
                           .data:0000d680 0f                               rrca
                           .data:0000d681 81                               add a,c
                           .data:0000d682 32 ac 8f                         ld (0x8fac),a
                           .data:0000d685 c3 1d d1                         jp 0xd11d
                           .data:0000d688 07                               rlca
                           .data:0000d689 4f                               ld c,a
                           .data:0000d68a 3a ae 8f                         ld a,(0x8fae)
                           .data:0000d68d 81                               add a,c
                           .data:0000d68e 4f                               ld c,a
                           .data:0000d68f 06 00                            ld b,0x00
                           .data:0000d691 21 5c d7                         ld hl,0xd75c
                           .data:0000d694 09                               add hl,bc
                           .data:0000d695 5e                               ld e,(hl)
                           .data:0000d696 16 00                            ld d,0x00
                           .data:0000d698 eb                               ex de,hl
                           .data:0000d699 3a ad 8f                         ld a,(0x8fad)
                           .data:0000d69c 3d                               dec a
                           .data:0000d69d ca a4 d6                         jp z,0xd6a4
                           .data:0000d6a0 29                               add hl,hl
                           .data:0000d6a1 c3 9c d6                         jp 0xd69c
                           .data:0000d6a4 eb                               ex de,hl
                           .data:0000d6a5 cd af d6                         call 0xd6af
                           .data:0000d6a8 af                               xor a
                           .data:0000d6a9 32 ae 8f                         ld (0x8fae),a
                           .data:0000d6ac c3 1d d1                         jp 0xd11d
                           .data:0000d6af 2a af 8f                         ld hl,(0x8faf)
                           .data:0000d6b2 3a ac 8f                         ld a,(0x8fac)
                           .data:0000d6b5 47                               ld b,a
                           .data:0000d6b6 0e 00                            ld c,0x00
                           .data:0000d6b8 19                               add hl,de
                           .data:0000d6b9 7c                               ld a,h
                           .data:0000d6ba e6 20                            and 0x20
                           .data:0000d6bc 32 02 ff                         ld (0xff02),a
                           .data:0000d6bf 0b                               dec bc
                           .data:0000d6c0 78                               ld a,b
                           .data:0000d6c1 b1                               or c
                           .data:0000d6c2 c2 b8 d6                         jp nz,0xd6b8
                           .data:0000d6c5 22 af 8f                         ld (0x8faf),hl
                           .data:0000d6c8 c9                               ret
                           .data:0000d6c9 07                               rlca
                           .data:0000d6ca 4f                               ld c,a
                           .data:0000d6cb 06 00                            ld b,0x00
                           .data:0000d6cd 21 00 d0                         ld hl,0xd000
                           .data:0000d6d0 09                               add hl,bc
                           .data:0000d6d1 5e                               ld e,(hl)
                           .data:0000d6d2 23                               inc hl
                           .data:0000d6d3 56                               ld d,(hl)
                           .data:0000d6d4 eb                               ex de,hl
                           .data:0000d6d5 cd db d6                         call 0xd6db
                           .data:0000d6d8 c3 1d d1                         jp 0xd11d
                           .data:0000d6db 3a ac 8f                         ld a,(0x8fac)
                           .data:0000d6de 57                               ld d,a
                           .data:0000d6df 1e 00                            ld e,0x00
                           .data:0000d6e1 2b                               dec hl
                           .data:0000d6e2 06 01                            ld b,0x01
                           .data:0000d6e4 05                               dec b
                           .data:0000d6e5 c2 fb d6                         jp nz,0xd6fb
                           .data:0000d6e8 23                               inc hl
                           .data:0000d6e9 06 08                            ld b,0x08
                           .data:0000d6eb 4e                               ld c,(hl)
                           .data:0000d6ec 79                               ld a,c
                           .data:0000d6ed 07                               rlca
                           .data:0000d6ee 4f                               ld c,a
                           .data:0000d6ef e6 20                            and 0x20
                           .data:0000d6f1 32 02 ff                         ld (0xff02),a
                           .data:0000d6f4 1b                               dec de
                           .data:0000d6f5 7a                               ld a,d
                           .data:0000d6f6 b3                               or e
                           .data:0000d6f7 c2 e4 d6                         jp nz,0xd6e4
                           .data:0000d6fa c9                               ret
                           .data:0000d6fb 7f                               ld a,a
                           .data:0000d6fc 00                               nop
                           .data:0000d6fd c3 ec d6                         jp 0xd6ec
                           .data:0000d700 11 00 00                         ld de,0x0000
                           .data:0000d703 c3 a5 d6                         jp 0xd6a5
                           .data:0000d706 e6 5f                            and 0x5f
                           .data:0000d708 fe 54                            cp 0x54
                           .data:0000d70a ca 2b d7                         jp z,0xd72b
                           .data:0000d70d fe 52                            cp 0x52
                           .data:0000d70f ca 00 d7                         jp z,0xd700
                           .data:0000d712 fe 4f                            cp 0x4f
                           .data:0000d714 ca 23 d7                         jp z,0xd723
                           .data:0000d717 d6 43                            sub 0x43
                           .data:0000d719 fe 07                            cp 0x07
                           .data:0000d71b da 88 d6                         jp c,0xd688
                           .data:0000d71e d6 07                            sub 0x07
                           .data:0000d720 c3 c9 d6                         jp 0xd6c9
                           .data:0000d723 3e 01                            ld a,0x01
                           .data:0000d725 32 b1 8f                         ld (0x8fb1),a
                           .data:0000d728 c3 1d d1                         jp 0xd11d
                           .data:0000d72b 3e 80                            ld a,0x80
                           .data:0000d72d c3 25 d7                         jp 0xd725
                           .data:0000d730 e6 0f                            and 0x0f
                           .data:0000d732 4f                               ld c,a
                           .data:0000d733 3a b1 8f                         ld a,(0x8fb1)
                           .data:0000d736 b7                               or a
                           .data:0000d737 c2 4c d7                         jp nz,0xd74c
                           .data:0000d73a 3a ac 8f                         ld a,(0x8fac)
                           .data:0000d73d 07                               rlca
                           .data:0000d73e 07                               rlca
                           .data:0000d73f 07                               rlca
                           .data:0000d740 07                               rlca
                           .data:0000d741 b1                               or c
                           .data:0000d742 32 ac 8f                         ld (0x8fac),a
                           .data:0000d745 af                               xor a
                           .data:0000d746 32 b1 8f                         ld (0x8fb1),a
                           .data:0000d749 c3 1d d1                         jp 0xd11d
                           .data:0000d74c 79                               ld a,c
                           .data:0000d74d fa 56 d7                         jp m,0xd756
                           .data:0000d750 32 ad 8f                         ld (0x8fad),a
                           .data:0000d753 c3 45 d7                         jp 0xd745
                           .data:0000d756 32 ac 8f                         ld (0x8fac),a
                           .data:0000d759 c3 45 d7                         jp 0xd745
                           .data:0000d75c 80                               add a,b
                           .data:0000d75d 87                               add a,a
                           .data:0000d75e 90                               sub b
                           .data:0000d75f 98                               sbc a,b
                           .data:0000d760 a1                               and c
                           .data:0000d761 a1                               and c
                           .data:0000d762 ab                               xor e
                           .data:0000d763 b5                               or l
                           .data:0000d764 bf                               cp a
                           .data:0000d765 cb d7                            set 2,a
                           .data:0000d767 e4 f2 ff                         call po,0xfff2
                           .data:0000d76a 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d76d e6 10                            and 0x10
                           .data:0000d76f ca 2f ce                         jp z,0xce2f
                           .data:0000d772 c3 37 c3                         jp 0xc337
                           .data:0000d775 f5                               push af
                           .data:0000d776 3a 8a 8f                         ld a,(0x8f8a)
                           .data:0000d779 e6 10                            and 0x10
                           .data:0000d77b c2 82 d7                         jp nz,0xd782
                           .data:0000d77e f1                               pop af
                           .data:0000d77f c3 f3 d0                         jp 0xd0f3
                           .data:0000d782 79                               ld a,c
                           .data:0000d783 fe 07                            cp 0x07
                           .data:0000d785 cc 70 c1                         call z,0xc170
                           .data:0000d788 fe 20                            cp 0x20
                           .data:0000d78a da 99 d7                         jp c,0xd799
                           .data:0000d78d c5                               push bc
                           .data:0000d78e 0e 7f                            ld c,0x7f
                           .data:0000d790 cd 9e d7                         call 0xd79e
                           .data:0000d793 0e 08                            ld c,0x08
                           .data:0000d795 cd 9e d7                         call 0xd79e
                           .data:0000d798 c1                               pop bc
                           .data:0000d799 cd 9e d7                         call 0xd79e
                           .data:0000d79c f1                               pop af
                           .data:0000d79d c9                               ret
                           .data:0000d79e 3a fc 8f                         ld a,(0x8ffc)
                           .data:0000d7a1 fe f8                            cp 0xf8
                           .data:0000d7a3 d4 aa d7                         call nc,0xd7aa
                           .data:0000d7a6 cd 37 c0                         call 0xc037
                           .data:0000d7a9 c9                               ret
                           .data:0000d7aa 79                               ld a,c
                           .data:0000d7ab fe 0a                            cp 0x0a
                           .data:0000d7ad ca bb d7                         jp z,0xd7bb
                           .data:0000d7b0 fe 1a                            cp 0x1a
                           .data:0000d7b2 ca bb d7                         jp z,0xd7bb
                           .data:0000d7b5 3a fd 8f                         ld a,(0x8ffd)
                           .data:0000d7b8 fe be                            cp 0xbe
                           .data:0000d7ba d8                               ret c
                           .data:0000d7bb e5                               push hl
                           .data:0000d7bc d5                               push de
                           .data:0000d7bd c5                               push bc
                           .data:0000d7be 21 00 90                         ld hl,0x9000
                           .data:0000d7c1 11 0a 90                         ld de,0x900a
                           .data:0000d7c4 06 30                            ld b,0x30
                           .data:0000d7c6 0e 78                            ld c,0x78
                           .data:0000d7c8 1a                               ld a,(de)
                           .data:0000d7c9 77                               ld (hl),a
                           .data:0000d7ca 23                               inc hl
                           .data:0000d7cb 13                               inc de
                           .data:0000d7cc 1a                               ld a,(de)
                           .data:0000d7cd 77                               ld (hl),a
                           .data:0000d7ce 23                               inc hl
                           .data:0000d7cf 13                               inc de
                           .data:0000d7d0 0d                               dec c
                           .data:0000d7d1 c2 c8 d7                         jp nz,0xd7c8
                           .data:0000d7d4 3a fa 8f                         ld a,(0x8ffa)
                           .data:0000d7d7 0e 10                            ld c,0x10
                           .data:0000d7d9 77                               ld (hl),a
                           .data:0000d7da 23                               inc hl
                           .data:0000d7db 13                               inc de
                           .data:0000d7dc 0d                               dec c
                           .data:0000d7dd c2 d9 d7                         jp nz,0xd7d9
                           .data:0000d7e0 05                               dec b
                           .data:0000d7e1 c2 c6 d7                         jp nz,0xd7c6
                           .data:0000d7e4 21 f8 00                         ld hl,0x00f8
                           .data:0000d7e7 22 fc 8f                         ld (0x8ffc),hl
                           .data:0000d7ea c1                               pop bc
                           .data:0000d7eb d1                               pop de
                           .data:0000d7ec e1                               pop hl
                           .data:0000d7ed 79                               ld a,c
                           .data:0000d7ee fe 0a                            cp 0x0a
                           .data:0000d7f0 c0                               ret nz
                           .data:0000d7f1 0e 1a                            ld c,0x1a
                           .data:0000d7f3 c9                               ret
                           .data:0000d7f4 ff                               rst 0x38
                           .data:0000d7f5 ff                               rst 0x38
                           .data:0000d7f6 ff                               rst 0x38
                           .data:0000d7f7 ff                               rst 0x38
                           .data:0000d7f8 ff                               rst 0x38
                           .data:0000d7f9 ff                               rst 0x38
                           .data:0000d7fa ff                               rst 0x38
                           .data:0000d7fb ff                               rst 0x38
                           .data:0000d7fc ff                               rst 0x38
                           .data:0000d7fd ff                               rst 0x38
                           .data:0000d7fe ff                               rst 0x38
                           .data:0000d7ff ff                               rst 0x38
                           .data:0000d800 00                               nop
                           .data:0000d801 00                               nop
                           .data:0000d802 00                               nop
                           .data:0000d803 00                               nop
                           .data:0000d804 00                               nop
                           .data:0000d805 00                               nop
                           .data:0000d806 00                               nop
                           .data:0000d807 00                               nop
                           .data:0000d808 7e                               ld a,(hl)
                           .data:0000d809 81                               add a,c
                           .data:0000d80a a5                               and l
                           .data:0000d80b 81                               add a,c
                           .data:0000d80c bd                               cp l
                           .data:0000d80d 99                               sbc a,c
                           .data:0000d80e 81                               add a,c
                           .data:0000d80f 7e                               ld a,(hl)
                           .data:0000d810 7e                               ld a,(hl)
                           .data:0000d811 ff                               rst 0x38
                           .data:0000d812 db ff                            in a,(0xff)
                           .data:0000d814 c3 e7 ff                         jp 0xffe7
                           .data:0000d817 7e                               ld a,(hl)
                           .data:0000d818 6c                               ld l,h
                           .data:0000d819 fe fe                            cp 0xfe
                           .data:0000d81b fe 7c                            cp 0x7c
                           .data:0000d81d 38 10                            jr c,0xd82f
                           .data:0000d81f 00                               nop
                           .data:0000d820 10 38                            djnz 0xd85a
                           .data:0000d822 7c                               ld a,h
                           .data:0000d823 fe 7c                            cp 0x7c
                           .data:0000d825 38 10                            jr c,0xd837
                           .data:0000d827 00                               nop
                           .data:0000d828 38 7c                            jr c,0xd8a6
                           .data:0000d82a 38 fe                            jr c,0xd82a
                           .data:0000d82c fe d6                            cp 0xd6
                           .data:0000d82e 10 38                            djnz 0xd868
                           .data:0000d830 10 10                            djnz 0xd842
                           .data:0000d832 38 7c                            jr c,0xd8b0
                           .data:0000d834 fe 7c                            cp 0x7c
                           .data:0000d836 10 38                            djnz 0xd870
                           .data:0000d838 00                               nop
                           .data:0000d839 00                               nop
                           .data:0000d83a 18 3c                            jr 0xd878
                           .data:0000d83c 3c                               inc a
                           .data:0000d83d 18 00                            jr 0xd83f
                           .data:0000d83f 00                               nop
                           .data:0000d840 ff                               rst 0x38
                           .data:0000d841 ff                               rst 0x38
                           .data:0000d842 e7                               rst 0x20
                           .data:0000d843 c3 c3 e7                         jp 0xe7c3
                           .data:0000d846 ff                               rst 0x38
                           .data:0000d847 ff                               rst 0x38
                           .data:0000d848 00                               nop
                           .data:0000d849 3c                               inc a
                           .data:0000d84a 66                               ld h,(hl)
                           .data:0000d84b 42                               ld b,d
                           .data:0000d84c 42                               ld b,d
                           .data:0000d84d 66                               ld h,(hl)
                           .data:0000d84e 3c                               inc a
                           .data:0000d84f 00                               nop
                           .data:0000d850 ff                               rst 0x38
                           .data:0000d851 c3 99 bd                         jp 0xbd99
                           .data:0000d854 bd                               cp l
                           .data:0000d855 99                               sbc a,c
                           .data:0000d856 c3 ff 0f                         jp 0x0fff
                           .data:0000d859 07                               rlca
                           .data:0000d85a 0f                               rrca
                           .data:0000d85b 7d                               ld a,l
                           .data:0000d85c cc cc cc                         call z,0xcccc
                           .data:0000d85f 78                               ld a,b
                           .data:0000d860 3c                               inc a
                           .data:0000d861 66                               ld h,(hl)
                           .data:0000d862 66                               ld h,(hl)
                           .data:0000d863 66                               ld h,(hl)
                           .data:0000d864 3c                               inc a
                           .data:0000d865 18 7e                            jr 0xd8e5
                           .data:0000d867 18 3f                            jr 0xd8a8
                           .data:0000d869 33                               inc sp
                           .data:0000d86a 3f                               ccf
                           .data:0000d86b 30 30                            jr nc,0xd89d
                           .data:0000d86d 70                               ld (hl),b
                           .data:0000d86e f0                               ret p
                           .data:0000d86f e0                               ret po
                           .data:0000d870 7f                               ld a,a
                           .data:0000d871 63                               ld h,e
                           .data:0000d872 7f                               ld a,a
                           .data:0000d873 63                               ld h,e
                           .data:0000d874 63                               ld h,e
                           .data:0000d875 67                               ld h,a
                           .data:0000d876 e6 c0                            and 0xc0
                           .data:0000d878 18 db                            jr 0xd855
                           .data:0000d87a 3c                               inc a
                           .data:0000d87b e7                               rst 0x20
                           .data:0000d87c e7                               rst 0x20
                           .data:0000d87d 3c                               inc a
                           .data:0000d87e db 18                            in a,(0x18)
                           .data:0000d880 80                               add a,b
                           .data:0000d881 e0                               ret po
                           .data:0000d882 f8                               ret m
                           .data:0000d883 fe f8                            cp 0xf8
                           .data:0000d885 e0                               ret po
                           .data:0000d886 80                               add a,b
                           .data:0000d887 00                               nop
                           .data:0000d888 02                               ld (bc),a
                           .data:0000d889 0e 3e                            ld c,0x3e
                           .data:0000d88b fe 3e                            cp 0x3e
                           .data:0000d88d 0e 02                            ld c,0x02
                           .data:0000d88f 00                               nop
                           .data:0000d890 18 3c                            jr 0xd8ce
                           .data:0000d892 7e                               ld a,(hl)
                           .data:0000d893 18 18                            jr 0xd8ad
                           .data:0000d895 7e                               ld a,(hl)
                           .data:0000d896 3c                               inc a
                           .data:0000d897 18 66                            jr 0xd8ff
                           .data:0000d899 66                               ld h,(hl)
                           .data:0000d89a 66                               ld h,(hl)
                           .data:0000d89b 66                               ld h,(hl)
                           .data:0000d89c 66                               ld h,(hl)
                           .data:0000d89d 00                               nop
                           .data:0000d89e 66                               ld h,(hl)
                           .data:0000d89f 00                               nop
                           .data:0000d8a0 7f                               ld a,a
                           .data:0000d8a1 db db                            in a,(0xdb)
                           .data:0000d8a3 7b                               ld a,e
                           .data:0000d8a4 1b                               dec de
                           .data:0000d8a5 1b                               dec de
                           .data:0000d8a6 1b                               dec de
                           .data:0000d8a7 00                               nop
                           .data:0000d8a8 3e 63                            ld a,0x63
                           .data:0000d8aa 38 6c                            jr c,0xd918
                           .data:0000d8ac 6c                               ld l,h
                           .data:0000d8ad 38 cc                            jr c,0xd87b
                           .data:0000d8af 78                               ld a,b
                           .data:0000d8b0 00                               nop
                           .data:0000d8b1 00                               nop
                           .data:0000d8b2 00                               nop
                           .data:0000d8b3 00                               nop
                           .data:0000d8b4 7e                               ld a,(hl)
                           .data:0000d8b5 7e                               ld a,(hl)
                           .data:0000d8b6 7e                               ld a,(hl)
                           .data:0000d8b7 00                               nop
                           .data:0000d8b8 18 3c                            jr 0xd8f6
                           .data:0000d8ba 7e                               ld a,(hl)
                           .data:0000d8bb 18 7e                            jr 0xd93b
                           .data:0000d8bd 3c                               inc a
                           .data:0000d8be 18 ff                            jr 0xd8bf
                           .data:0000d8c0 18 3c                            jr 0xd8fe
                           .data:0000d8c2 7e                               ld a,(hl)
                           .data:0000d8c3 18 18                            jr 0xd8dd
                           .data:0000d8c5 18 18                            jr 0xd8df
                           .data:0000d8c7 00                               nop
                           .data:0000d8c8 18 18                            jr 0xd8e2
                           .data:0000d8ca 18 18                            jr 0xd8e4
                           .data:0000d8cc 7e                               ld a,(hl)
                           .data:0000d8cd 3c                               inc a
                           .data:0000d8ce 18 00                            jr 0xd8d0
                           .data:0000d8d0 00                               nop
                           .data:0000d8d1 18 0c                            jr 0xd8df
                           .data:0000d8d3 fe 0c                            cp 0x0c
                           .data:0000d8d5 18 00                            jr 0xd8d7
                           .data:0000d8d7 00                               nop
                           .data:0000d8d8 00                               nop
                           .data:0000d8d9 30 60                            jr nc,0xd93b
                           .data:0000d8db fe 60                            cp 0x60
                           .data:0000d8dd 30 00                            jr nc,0xd8df
                           .data:0000d8df 00                               nop
                           .data:0000d8e0 00                               nop
                           .data:0000d8e1 00                               nop
                           .data:0000d8e2 c0                               ret nz
                           .data:0000d8e3 c0                               ret nz
                           .data:0000d8e4 c0                               ret nz
                           .data:0000d8e5 fe 00                            cp 0x00
                           .data:0000d8e7 00                               nop
                           .data:0000d8e8 00                               nop
                           .data:0000d8e9 24                               inc h
                           .data:0000d8ea 66                               ld h,(hl)
                           .data:0000d8eb ff                               rst 0x38
                           .data:0000d8ec 66                               ld h,(hl)
                           .data:0000d8ed 24                               inc h
                           .data:0000d8ee 00                               nop
                           .data:0000d8ef 00                               nop
                           .data:0000d8f0 00                               nop
                           .data:0000d8f1 18 3c                            jr 0xd92f
                           .data:0000d8f3 7e                               ld a,(hl)
                           .data:0000d8f4 ff                               rst 0x38
                           .data:0000d8f5 ff                               rst 0x38
                           .data:0000d8f6 00                               nop
                           .data:0000d8f7 00                               nop
                           .data:0000d8f8 00                               nop
                           .data:0000d8f9 ff                               rst 0x38
                           .data:0000d8fa ff                               rst 0x38
                           .data:0000d8fb 7e                               ld a,(hl)
                           .data:0000d8fc 3c                               inc a
                           .data:0000d8fd 18 00                            jr 0xd8ff
                           .data:0000d8ff 00                               nop
                           .data:0000d900 00                               nop
                           .data:0000d901 00                               nop
                           .data:0000d902 00                               nop
                           .data:0000d903 00                               nop
                           .data:0000d904 00                               nop
                           .data:0000d905 00                               nop
                           .data:0000d906 00                               nop
                           .data:0000d907 00                               nop
                           .data:0000d908 30 78                            jr nc,0xd982
                           .data:0000d90a 78                               ld a,b
                           .data:0000d90b 30 30                            jr nc,0xd93d
                           .data:0000d90d 00                               nop
                           .data:0000d90e 30 00                            jr nc,0xd910
                           .data:0000d910 6c                               ld l,h
                           .data:0000d911 6c                               ld l,h
                           .data:0000d912 6c                               ld l,h
                           .data:0000d913 00                               nop
                           .data:0000d914 00                               nop
                           .data:0000d915 00                               nop
                           .data:0000d916 00                               nop
                           .data:0000d917 00                               nop
                           .data:0000d918 6c                               ld l,h
                           .data:0000d919 6c                               ld l,h
                           .data:0000d91a fe 6c                            cp 0x6c
                           .data:0000d91c fe 6c                            cp 0x6c
                           .data:0000d91e 6c                               ld l,h
                           .data:0000d91f 00                               nop
                           .data:0000d920 30 7c                            jr nc,0xd99e
                           .data:0000d922 c0                               ret nz
                           .data:0000d923 78                               ld a,b
                           .data:0000d924 0c                               inc c
                           .data:0000d925 f8                               ret m
                           .data:0000d926 30 00                            jr nc,0xd928
                           .data:0000d928 00                               nop
                           .data:0000d929 c6 cc                            add a,0xcc
                           .data:0000d92b 18 30                            jr 0xd95d
                           .data:0000d92d 66                               ld h,(hl)
                           .data:0000d92e c6 00                            add a,0x00
                           .data:0000d930 38 6c                            jr c,0xd99e
                           .data:0000d932 38 76                            jr c,0xd9aa
                           .data:0000d934 dc cc 76                         call c,0x76cc
                           .data:0000d937 00                               nop
                           .data:0000d938 60                               ld h,b
                           .data:0000d939 60                               ld h,b
                           .data:0000d93a c0                               ret nz
                           .data:0000d93b 00                               nop
                           .data:0000d93c 00                               nop
                           .data:0000d93d 00                               nop
                           .data:0000d93e 00                               nop
                           .data:0000d93f 00                               nop
                           .data:0000d940 18 30                            jr 0xd972
                           .data:0000d942 60                               ld h,b
                           .data:0000d943 60                               ld h,b
                           .data:0000d944 60                               ld h,b
                           .data:0000d945 30 18                            jr nc,0xd95f
                           .data:0000d947 00                               nop
                           .data:0000d948 60                               ld h,b
                           .data:0000d949 30 18                            jr nc,0xd963
                           .data:0000d94b 18 18                            jr 0xd965
                           .data:0000d94d 30 60                            jr nc,0xd9af
                           .data:0000d94f 00                               nop
                           .data:0000d950 00                               nop
                           .data:0000d951 66                               ld h,(hl)
                           .data:0000d952 3c                               inc a
                           .data:0000d953 ff                               rst 0x38
                           .data:0000d954 3c                               inc a
                           .data:0000d955 66                               ld h,(hl)
                           .data:0000d956 00                               nop
                           .data:0000d957 00                               nop
                           .data:0000d958 00                               nop
                           .data:0000d959 30 30                            jr nc,0xd98b
                           .data:0000d95b fc 30 30                         call m,0x3030
                           .data:0000d95e 00                               nop
                           .data:0000d95f 00                               nop
                           .data:0000d960 00                               nop
                           .data:0000d961 00                               nop
                           .data:0000d962 00                               nop
                           .data:0000d963 00                               nop
                           .data:0000d964 00                               nop
                           .data:0000d965 30 30                            jr nc,0xd997
                           .data:0000d967 60                               ld h,b
                           .data:0000d968 00                               nop
                           .data:0000d969 00                               nop
                           .data:0000d96a 00                               nop
                           .data:0000d96b fc 00 00                         call m,0x0000
                           .data:0000d96e 00                               nop
                           .data:0000d96f 00                               nop
                           .data:0000d970 00                               nop
                           .data:0000d971 00                               nop
                           .data:0000d972 00                               nop
                           .data:0000d973 00                               nop
                           .data:0000d974 00                               nop
                           .data:0000d975 30 30                            jr nc,0xd9a7
                           .data:0000d977 00                               nop
                           .data:0000d978 06 0c                            ld b,0x0c
                           .data:0000d97a 18 30                            jr 0xd9ac
                           .data:0000d97c 60                               ld h,b
                           .data:0000d97d c0                               ret nz
                           .data:0000d97e 80                               add a,b
                           .data:0000d97f 00                               nop
                           .data:0000d980 7c                               ld a,h
                           .data:0000d981 c6 ce                            add a,0xce
                           .data:0000d983 de f6                            sbc a,0xf6
                           .data:0000d985 e6 7c                            and 0x7c
                           .data:0000d987 00                               nop
                           .data:0000d988 30 70                            jr nc,0xd9fa
                           .data:0000d98a 30 30                            jr nc,0xd9bc
                           .data:0000d98c 30 30                            jr nc,0xd9be
                           .data:0000d98e fc 00 78                         call m,0x7800
                           .data:0000d991 cc 0c 38                         call z,0x380c
                           .data:0000d994 60                               ld h,b
                           .data:0000d995 cc fc 00                         call z,0x00fc
                           .data:0000d998 78                               ld a,b
                           .data:0000d999 cc 0c 38                         call z,0x380c
                           .data:0000d99c 0c                               inc c
                           .data:0000d99d cc 78 00                         call z,0x0078
                           .data:0000d9a0 1c                               inc e
                           .data:0000d9a1 3c                               inc a
                           .data:0000d9a2 6c                               ld l,h
                           .data:0000d9a3 cc fe 0c                         call z,0x0cfe
                           .data:0000d9a6 1e 00                            ld e,0x00
                           .data:0000d9a8 fc c0 f8                         call m,0xf8c0
                           .data:0000d9ab 0c                               inc c
                           .data:0000d9ac 0c                               inc c
                           .data:0000d9ad cc 78 00                         call z,0x0078
                           .data:0000d9b0 38 60                            jr c,0xda12
                           .data:0000d9b2 c0                               ret nz
                           .data:0000d9b3 f8                               ret m
                           .data:0000d9b4 cc cc 78                         call z,0x78cc
                           .data:0000d9b7 00                               nop
                           .data:0000d9b8 fc cc 0c                         call m,0x0ccc
                           .data:0000d9bb 18 30                            jr 0xd9ed
                           .data:0000d9bd 30 30                            jr nc,0xd9ef
                           .data:0000d9bf 00                               nop
                           .data:0000d9c0 78                               ld a,b
                           .data:0000d9c1 cc cc 78                         call z,0x78cc
                           .data:0000d9c4 cc cc 78                         call z,0x78cc
                           .data:0000d9c7 00                               nop
                           .data:0000d9c8 78                               ld a,b
                           .data:0000d9c9 cc cc 7c                         call z,0x7ccc
                           .data:0000d9cc 0c                               inc c
                           .data:0000d9cd 18 70                            jr 0xda3f
                           .data:0000d9cf 00                               nop
                           .data:0000d9d0 00                               nop
                           .data:0000d9d1 30 30                            jr nc,0xda03
                           .data:0000d9d3 00                               nop
                           .data:0000d9d4 00                               nop
                           .data:0000d9d5 30 30                            jr nc,0xda07
                           .data:0000d9d7 00                               nop
                           .data:0000d9d8 00                               nop
                           .data:0000d9d9 30 30                            jr nc,0xda0b
                           .data:0000d9db 00                               nop
                           .data:0000d9dc 00                               nop
                           .data:0000d9dd 30 30                            jr nc,0xda0f
                           .data:0000d9df 60                               ld h,b
                           .data:0000d9e0 18 30                            jr 0xda12
                           .data:0000d9e2 60                               ld h,b
                           .data:0000d9e3 c0                               ret nz
                           .data:0000d9e4 60                               ld h,b
                           .data:0000d9e5 30 18                            jr nc,0xd9ff
                           .data:0000d9e7 00                               nop
                           .data:0000d9e8 00                               nop
                           .data:0000d9e9 00                               nop
                           .data:0000d9ea fc 00 fc                         call m,0xfc00
                           .data:0000d9ed 00                               nop
                           .data:0000d9ee 00                               nop
                           .data:0000d9ef 00                               nop
                           .data:0000d9f0 60                               ld h,b
                           .data:0000d9f1 30 18                            jr nc,0xda0b
                           .data:0000d9f3 0c                               inc c
                           .data:0000d9f4 18 30                            jr 0xda26
                           .data:0000d9f6 60                               ld h,b
                           .data:0000d9f7 00                               nop
                           .data:0000d9f8 78                               ld a,b
                           .data:0000d9f9 cc 0c 18                         call z,0x180c
                           .data:0000d9fc 30 00                            jr nc,0xd9fe
                           .data:0000d9fe 30 00                            jr nc,0xda00
                           .data:0000da00 7c                               ld a,h
                           .data:0000da01 c6 de                            add a,0xde
                           .data:0000da03 de de                            sbc a,0xde
                           .data:0000da05 c0                               ret nz
                           .data:0000da06 78                               ld a,b
                           .data:0000da07 00                               nop
                           .data:0000da08 30 78                            jr nc,0xda82
                           .data:0000da0a cc cc fc                         call z,0xfccc
                           .data:0000da0d cc cc 00                         call z,0x00cc
                           .data:0000da10 fc 66 66                         call m,0x6666
                           .data:0000da13 7c                               ld a,h
                           .data:0000da14 66                               ld h,(hl)
                           .data:0000da15 66                               ld h,(hl)
                           .data:0000da16 fc 00 3c                         call m,0x3c00
                           .data:0000da19 66                               ld h,(hl)
                           .data:0000da1a c0                               ret nz
                           .data:0000da1b c0                               ret nz
                           .data:0000da1c c0                               ret nz
                           .data:0000da1d 66                               ld h,(hl)
                           .data:0000da1e 3c                               inc a
                           .data:0000da1f 00                               nop
                           .data:0000da20 f8                               ret m
                           .data:0000da21 6c                               ld l,h
                           .data:0000da22 66                               ld h,(hl)
                           .data:0000da23 66                               ld h,(hl)
                           .data:0000da24 66                               ld h,(hl)
                           .data:0000da25 6c                               ld l,h
                           .data:0000da26 f8                               ret m
                           .data:0000da27 00                               nop
                           .data:0000da28 fe 62                            cp 0x62
                           .data:0000da2a 68                               ld l,b
                           .data:0000da2b 78                               ld a,b
                           .data:0000da2c 68                               ld l,b
                           .data:0000da2d 62                               ld h,d
                           .data:0000da2e fe 00                            cp 0x00
                           .data:0000da30 fe 62                            cp 0x62
                           .data:0000da32 68                               ld l,b
                           .data:0000da33 78                               ld a,b
                           .data:0000da34 68                               ld l,b
                           .data:0000da35 60                               ld h,b
                           .data:0000da36 f0                               ret p
                           .data:0000da37 00                               nop
                           .data:0000da38 3c                               inc a
                           .data:0000da39 66                               ld h,(hl)
                           .data:0000da3a c0                               ret nz
                           .data:0000da3b c0                               ret nz
                           .data:0000da3c ce 66                            adc a,0x66
                           .data:0000da3e 3e 00                            ld a,0x00
                           .data:0000da40 cc cc cc                         call z,0xcccc
                           .data:0000da43 fc cc cc                         call m,0xcccc
                           .data:0000da46 cc 00 78                         call z,0x7800
                           .data:0000da49 30 30                            jr nc,0xda7b
                           .data:0000da4b 30 30                            jr nc,0xda7d
                           .data:0000da4d 30 78                            jr nc,0xdac7
                           .data:0000da4f 00                               nop
                           .data:0000da50 1e 0c                            ld e,0x0c
                           .data:0000da52 0c                               inc c
                           .data:0000da53 0c                               inc c
                           .data:0000da54 cc cc 78                         call z,0x78cc
                           .data:0000da57 00                               nop
                           .data:0000da58 e6 66                            and 0x66
                           .data:0000da5a 6c                               ld l,h
                           .data:0000da5b 78                               ld a,b
                           .data:0000da5c 6c                               ld l,h
                           .data:0000da5d 66                               ld h,(hl)
                           .data:0000da5e e6 00                            and 0x00
                           .data:0000da60 f0                               ret p
                           .data:0000da61 60                               ld h,b
                           .data:0000da62 60                               ld h,b
                           .data:0000da63 60                               ld h,b
                           .data:0000da64 62                               ld h,d
                           .data:0000da65 66                               ld h,(hl)
                           .data:0000da66 fe 00                            cp 0x00
                           .data:0000da68 c6 ee                            add a,0xee
                           .data:0000da6a fe fe                            cp 0xfe
                           .data:0000da6c d6 c6                            sub 0xc6
                           .data:0000da6e c6 00                            add a,0x00
                           .data:0000da70 c6 e6                            add a,0xe6
                           .data:0000da72 f6 de                            or 0xde
                           .data:0000da74 ce c6                            adc a,0xc6
                           .data:0000da76 c6 00                            add a,0x00
                           .data:0000da78 38 6c                            jr c,0xdae6
                           .data:0000da7a c6 c6                            add a,0xc6
                           .data:0000da7c c6 6c                            add a,0x6c
                           .data:0000da7e 38 00                            jr c,0xda80
                           .data:0000da80 fc 66 66                         call m,0x6666
                           .data:0000da83 7c                               ld a,h
                           .data:0000da84 60                               ld h,b
                           .data:0000da85 60                               ld h,b
                           .data:0000da86 f0                               ret p
                           .data:0000da87 00                               nop
                           .data:0000da88 78                               ld a,b
                           .data:0000da89 cc cc cc                         call z,0xcccc
                           .data:0000da8c dc 78 1c                         call c,0x1c78
                           .data:0000da8f 00                               nop
                           .data:0000da90 fc 66 66                         call m,0x6666
                           .data:0000da93 7c                               ld a,h
                           .data:0000da94 6c                               ld l,h
                           .data:0000da95 66                               ld h,(hl)
                           .data:0000da96 e6 00                            and 0x00
                           .data:0000da98 78                               ld a,b
                           .data:0000da99 cc 60 30                         call z,0x3060
                           .data:0000da9c 18 cc                            jr 0xda6a
                           .data:0000da9e 78                               ld a,b
                           .data:0000da9f 00                               nop
                           .data:0000daa0 fc b4 30                         call m,0x30b4
                           .data:0000daa3 30 30                            jr nc,0xdad5
                           .data:0000daa5 30 78                            jr nc,0xdb1f
                           .data:0000daa7 00                               nop
                           .data:0000daa8 cc cc cc                         call z,0xcccc
                           .data:0000daab cc cc cc                         call z,0xcccc
                           .data:0000daae 78                               ld a,b
                           .data:0000daaf 00                               nop
                           .data:0000dab0 cc cc cc                         call z,0xcccc
                           .data:0000dab3 cc cc 78                         call z,0x78cc
                           .data:0000dab6 30 00                            jr nc,0xdab8
                           .data:0000dab8 c6 c6                            add a,0xc6
                           .data:0000daba c6 d6                            add a,0xd6
                           .data:0000dabc fe ee                            cp 0xee
                           .data:0000dabe c6 00                            add a,0x00
                           .data:0000dac0 c6 c6                            add a,0xc6
                           .data:0000dac2 6c                               ld l,h
                           .data:0000dac3 38 38                            jr c,0xdafd
                           .data:0000dac5 6c                               ld l,h
                           .data:0000dac6 c6 00                            add a,0x00
                           .data:0000dac8 cc cc cc                         call z,0xcccc
                           .data:0000dacb 78                               ld a,b
                           .data:0000dacc 30 30                            jr nc,0xdafe
                           .data:0000dace 78                               ld a,b
                           .data:0000dacf 00                               nop
                           .data:0000dad0 fe c6                            cp 0xc6
                           .data:0000dad2 8c                               adc a,h
                           .data:0000dad3 18 32                            jr 0xdb07
                           .data:0000dad5 66                               ld h,(hl)
                           .data:0000dad6 fe 00                            cp 0x00
                           .data:0000dad8 78                               ld a,b
                           .data:0000dad9 60                               ld h,b
                           .data:0000dada 60                               ld h,b
                           .data:0000dadb 60                               ld h,b
                           .data:0000dadc 60                               ld h,b
                           .data:0000dadd 60                               ld h,b
                           .data:0000dade 78                               ld a,b
                           .data:0000dadf 00                               nop
                           .data:0000dae0 c0                               ret nz
                           .data:0000dae1 60                               ld h,b
                           .data:0000dae2 30 18                            jr nc,0xdafc
                           .data:0000dae4 0c                               inc c
                           .data:0000dae5 06 02                            ld b,0x02
                           .data:0000dae7 00                               nop
                           .data:0000dae8 78                               ld a,b
                           .data:0000dae9 18 18                            jr 0xdb03
                           .data:0000daeb 18 18                            jr 0xdb05
                           .data:0000daed 18 78                            jr 0xdb67
                           .data:0000daef 00                               nop
                           .data:0000daf0 10 38                            djnz 0xdb2a
                           .data:0000daf2 6c                               ld l,h
                           .data:0000daf3 c6 00                            add a,0x00
                           .data:0000daf5 00                               nop
                           .data:0000daf6 00                               nop
                           .data:0000daf7 00                               nop
                           .data:0000daf8 00                               nop
                           .data:0000daf9 00                               nop
                           .data:0000dafa 00                               nop
                           .data:0000dafb 00                               nop
                           .data:0000dafc 00                               nop
                           .data:0000dafd 00                               nop
                           .data:0000dafe 00                               nop
                           .data:0000daff ff                               rst 0x38
                           .data:0000db00 30 30                            jr nc,0xdb32
                           .data:0000db02 18 00                            jr 0xdb04
                           .data:0000db04 00                               nop
                           .data:0000db05 00                               nop
                           .data:0000db06 00                               nop
                           .data:0000db07 00                               nop
                           .data:0000db08 00                               nop
                           .data:0000db09 00                               nop
                           .data:0000db0a 78                               ld a,b
                           .data:0000db0b 0c                               inc c
                           .data:0000db0c 7c                               ld a,h
                           .data:0000db0d cc 76 00                         call z,0x0076
                           .data:0000db10 e0                               ret po
                           .data:0000db11 60                               ld h,b
                           .data:0000db12 60                               ld h,b
                           .data:0000db13 7c                               ld a,h
                           .data:0000db14 66                               ld h,(hl)
                           .data:0000db15 66                               ld h,(hl)
                           .data:0000db16 dc 00 00                         call c,0x0000
                           .data:0000db19 00                               nop
                           .data:0000db1a 78                               ld a,b
                           .data:0000db1b cc c0 cc                         call z,0xccc0
                           .data:0000db1e 78                               ld a,b
                           .data:0000db1f 00                               nop
                           .data:0000db20 1c                               inc e
                           .data:0000db21 0c                               inc c
                           .data:0000db22 0c                               inc c
                           .data:0000db23 7c                               ld a,h
                           .data:0000db24 cc cc 76                         call z,0x76cc
                           .data:0000db27 00                               nop
                           .data:0000db28 00                               nop
                           .data:0000db29 00                               nop
                           .data:0000db2a 78                               ld a,b
                           .data:0000db2b cc fc c0                         call z,0xc0fc
                           .data:0000db2e 78                               ld a,b
                           .data:0000db2f 00                               nop
                           .data:0000db30 38 6c                            jr c,0xdb9e
                           .data:0000db32 60                               ld h,b
                           .data:0000db33 f0                               ret p
                           .data:0000db34 60                               ld h,b
                           .data:0000db35 60                               ld h,b
                           .data:0000db36 f0                               ret p
                           .data:0000db37 00                               nop
                           .data:0000db38 00                               nop
                           .data:0000db39 00                               nop
                           .data:0000db3a 76                               halt
                           .data:0000db3b cc cc 7c                         call z,0x7ccc
                           .data:0000db3e 0c                               inc c
                           .data:0000db3f f8                               ret m
                           .data:0000db40 e0                               ret po
                           .data:0000db41 60                               ld h,b
                           .data:0000db42 6c                               ld l,h
                           .data:0000db43 76                               halt
                           .data:0000db44 66                               ld h,(hl)
                           .data:0000db45 66                               ld h,(hl)
                           .data:0000db46 e6 00                            and 0x00
                           .data:0000db48 30 00                            jr nc,0xdb4a
                           .data:0000db4a 70                               ld (hl),b
                           .data:0000db4b 30 30                            jr nc,0xdb7d
                           .data:0000db4d 30 78                            jr nc,0xdbc7
                           .data:0000db4f 00                               nop
                           .data:0000db50 0c                               inc c
                           .data:0000db51 00                               nop
                           .data:0000db52 0c                               inc c
                           .data:0000db53 0c                               inc c
                           .data:0000db54 0c                               inc c
                           .data:0000db55 cc cc 78                         call z,0x78cc
                           .data:0000db58 e0                               ret po
                           .data:0000db59 60                               ld h,b
                           .data:0000db5a 66                               ld h,(hl)
                           .data:0000db5b 6c                               ld l,h
                           .data:0000db5c 78                               ld a,b
                           .data:0000db5d 6c                               ld l,h
                           .data:0000db5e e6 00                            and 0x00
                           .data:0000db60 70                               ld (hl),b
                           .data:0000db61 30 30                            jr nc,0xdb93
                           .data:0000db63 30 30                            jr nc,0xdb95
                           .data:0000db65 30 78                            jr nc,0xdbdf
                           .data:0000db67 00                               nop
                           .data:0000db68 00                               nop
                           .data:0000db69 00                               nop
                           .data:0000db6a cc fe fe                         call z,0xfefe
                           .data:0000db6d d6 c6                            sub 0xc6
                           .data:0000db6f 00                               nop
                           .data:0000db70 00                               nop
                           .data:0000db71 00                               nop
                           .data:0000db72 f8                               ret m
                           .data:0000db73 cc cc cc                         call z,0xcccc
                           .data:0000db76 cc 00 00                         call z,0x0000
                           .data:0000db79 00                               nop
                           .data:0000db7a 78                               ld a,b
                           .data:0000db7b cc cc cc                         call z,0xcccc
                           .data:0000db7e 78                               ld a,b
                           .data:0000db7f 00                               nop
                           .data:0000db80 00                               nop
                           .data:0000db81 00                               nop
                           .data:0000db82 dc 66 66                         call c,0x6666
                           .data:0000db85 7c                               ld a,h
                           .data:0000db86 60                               ld h,b
                           .data:0000db87 f0                               ret p
                           .data:0000db88 00                               nop
                           .data:0000db89 00                               nop
                           .data:0000db8a 76                               halt
                           .data:0000db8b cc cc 7c                         call z,0x7ccc
                           .data:0000db8e 0c                               inc c
                           .data:0000db8f 1e 00                            ld e,0x00
                           .data:0000db91 00                               nop
                           .data:0000db92 dc 76 66                         call c,0x6676
                           .data:0000db95 60                               ld h,b
                           .data:0000db96 f0                               ret p
                           .data:0000db97 00                               nop
                           .data:0000db98 00                               nop
                           .data:0000db99 00                               nop
                           .data:0000db9a 7c                               ld a,h
                           .data:0000db9b c0                               ret nz
                           .data:0000db9c 78                               ld a,b
                           .data:0000db9d 0c                               inc c
                           .data:0000db9e f8                               ret m
                           .data:0000db9f 00                               nop
                           .data:0000dba0 10 30                            djnz 0xdbd2
                           .data:0000dba2 7c                               ld a,h
                           .data:0000dba3 30 30                            jr nc,0xdbd5
                           .data:0000dba5 34                               inc (hl)
                           .data:0000dba6 18 00                            jr 0xdba8
                           .data:0000dba8 00                               nop
                           .data:0000dba9 00                               nop
                           .data:0000dbaa cc cc cc                         call z,0xcccc
                           .data:0000dbad cc 76 00                         call z,0x0076
                           .data:0000dbb0 00                               nop
                           .data:0000dbb1 00                               nop
                           .data:0000dbb2 cc cc cc                         call z,0xcccc
                           .data:0000dbb5 78                               ld a,b
                           .data:0000dbb6 30 00                            jr nc,0xdbb8
                           .data:0000dbb8 00                               nop
                           .data:0000dbb9 00                               nop
                           .data:0000dbba c6 d6                            add a,0xd6
                           .data:0000dbbc fe fe                            cp 0xfe
                           .data:0000dbbe 6c                               ld l,h
                           .data:0000dbbf 00                               nop
                           .data:0000dbc0 00                               nop
                           .data:0000dbc1 00                               nop
                           .data:0000dbc2 c6 6c                            add a,0x6c
                           .data:0000dbc4 38 6c                            jr c,0xdc32
                           .data:0000dbc6 c6 00                            add a,0x00
                           .data:0000dbc8 00                               nop
                           .data:0000dbc9 00                               nop
                           .data:0000dbca cc cc cc                         call z,0xcccc
                           .data:0000dbcd 7c                               ld a,h
                           .data:0000dbce 0c                               inc c
                           .data:0000dbcf f8                               ret m
                           .data:0000dbd0 00                               nop
                           .data:0000dbd1 00                               nop
                           .data:0000dbd2 fc 98 30                         call m,0x3098
                           .data:0000dbd5 64                               ld h,h
                           .data:0000dbd6 fc 00 1c                         call m,0x1c00
                           .data:0000dbd9 30 30                            jr nc,0xdc0b
                           .data:0000dbdb e0                               ret po
                           .data:0000dbdc 30 30                            jr nc,0xdc0e
                           .data:0000dbde 1c                               inc e
                           .data:0000dbdf 00                               nop
                           .data:0000dbe0 18 18                            jr 0xdbfa
                           .data:0000dbe2 18 00                            jr 0xdbe4
                           .data:0000dbe4 18 18                            jr 0xdbfe
                           .data:0000dbe6 18 00                            jr 0xdbe8
                           .data:0000dbe8 e0                               ret po
                           .data:0000dbe9 30 30                            jr nc,0xdc1b
                           .data:0000dbeb 1c                               inc e
                           .data:0000dbec 30 30                            jr nc,0xdc1e
                           .data:0000dbee e0                               ret po
                           .data:0000dbef 00                               nop
                           .data:0000dbf0 76                               halt
                           .data:0000dbf1 dc 00 00                         call c,0x0000
                           .data:0000dbf4 00                               nop
                           .data:0000dbf5 00                               nop
                           .data:0000dbf6 00                               nop
                           .data:0000dbf7 00                               nop
                           .data:0000dbf8 00                               nop
                           .data:0000dbf9 10 38                            djnz 0xdc33
                           .data:0000dbfb 6c                               ld l,h
                           .data:0000dbfc c6 c6                            add a,0xc6
                           .data:0000dbfe fe 00                            cp 0x00
                           .data:0000dc00 22 88 22                         ld (0x2288),hl
                           .data:0000dc03 88                               adc a,b
                           .data:0000dc04 22 88 22                         ld (0x2288),hl
                           .data:0000dc07 88                               adc a,b
                           .data:0000dc08 55                               ld d,l
                           .data:0000dc09 aa                               xor d
                           .data:0000dc0a 55                               ld d,l
                           .data:0000dc0b aa                               xor d
                           .data:0000dc0c 55                               ld d,l
                           .data:0000dc0d aa                               xor d
                           .data:0000dc0e 55                               ld d,l
                           .data:0000dc0f aa                               xor d
                           .data:0000dc10 ee bb                            xor 0xbb
                           .data:0000dc12 ee bb                            xor 0xbb
                           .data:0000dc14 ee bb                            xor 0xbb
                           .data:0000dc16 ee bb                            xor 0xbb
                           .data:0000dc18 18 18                            jr 0xdc32
                           .data:0000dc1a 18 18                            jr 0xdc34
                           .data:0000dc1c 18 18                            jr 0xdc36
                           .data:0000dc1e 18 18                            jr 0xdc38
                           .data:0000dc20 18 18                            jr 0xdc3a
                           .data:0000dc22 18 18                            jr 0xdc3c
                           .data:0000dc24 f8                               ret m
                           .data:0000dc25 18 18                            jr 0xdc3f
                           .data:0000dc27 18 18                            jr 0xdc41
                           .data:0000dc29 18 f8                            jr 0xdc23
                           .data:0000dc2b 18 f8                            jr 0xdc25
                           .data:0000dc2d 18 18                            jr 0xdc47
                           .data:0000dc2f 18 36                            jr 0xdc67
                           .data:0000dc31 36 36                            ld (hl),0x36
                           .data:0000dc33 36 f6                            ld (hl),0xf6
                           .data:0000dc35 36 36                            ld (hl),0x36
                           .data:0000dc37 36 00                            ld (hl),0x00
                           .data:0000dc39 00                               nop
                           .data:0000dc3a 00                               nop
                           .data:0000dc3b 00                               nop
                           .data:0000dc3c fe 36                            cp 0x36
                           .data:0000dc3e 36 36                            ld (hl),0x36
                           .data:0000dc40 00                               nop
                           .data:0000dc41 00                               nop
                           .data:0000dc42 f8                               ret m
                           .data:0000dc43 18 f8                            jr 0xdc3d
                           .data:0000dc45 18 18                            jr 0xdc5f
                           .data:0000dc47 18 36                            jr 0xdc7f
                           .data:0000dc49 36 f6                            ld (hl),0xf6
                           .data:0000dc4b 06 f6                            ld b,0xf6
                           .data:0000dc4d 36 36                            ld (hl),0x36
                           .data:0000dc4f 36 36                            ld (hl),0x36
                           .data:0000dc51 36 36                            ld (hl),0x36
                           .data:0000dc53 36 36                            ld (hl),0x36
                           .data:0000dc55 36 36                            ld (hl),0x36
                           .data:0000dc57 36 00                            ld (hl),0x00
                           .data:0000dc59 00                               nop
                           .data:0000dc5a fe 06                            cp 0x06
                           .data:0000dc5c f6 36                            or 0x36
                           .data:0000dc5e 36 36                            ld (hl),0x36
                           .data:0000dc60 36 36                            ld (hl),0x36
                           .data:0000dc62 f6 06                            or 0x06
                           .data:0000dc64 fe 00                            cp 0x00
                           .data:0000dc66 00                               nop
                           .data:0000dc67 00                               nop
                           .data:0000dc68 36 36                            ld (hl),0x36
                           .data:0000dc6a 36 36                            ld (hl),0x36
                           .data:0000dc6c fe 00                            cp 0x00
                           .data:0000dc6e 00                               nop
                           .data:0000dc6f 00                               nop
                           .data:0000dc70 18 18                            jr 0xdc8a
                           .data:0000dc72 f8                               ret m
                           .data:0000dc73 18 f8                            jr 0xdc6d
                           .data:0000dc75 00                               nop
                           .data:0000dc76 00                               nop
                           .data:0000dc77 00                               nop
                           .data:0000dc78 00                               nop
                           .data:0000dc79 00                               nop
                           .data:0000dc7a 00                               nop
                           .data:0000dc7b 00                               nop
                           .data:0000dc7c f8                               ret m
                           .data:0000dc7d 18 18                            jr 0xdc97
                           .data:0000dc7f 18 18                            jr 0xdc99
                           .data:0000dc81 18 18                            jr 0xdc9b
                           .data:0000dc83 18 1f                            jr 0xdca4
                           .data:0000dc85 00                               nop
                           .data:0000dc86 00                               nop
                           .data:0000dc87 00                               nop
                           .data:0000dc88 18 18                            jr 0xdca2
                           .data:0000dc8a 18 18                            jr 0xdca4
                           .data:0000dc8c ff                               rst 0x38
                           .data:0000dc8d 00                               nop
                           .data:0000dc8e 00                               nop
                           .data:0000dc8f 00                               nop
                           .data:0000dc90 00                               nop
                           .data:0000dc91 00                               nop
                           .data:0000dc92 00                               nop
                           .data:0000dc93 00                               nop
                           .data:0000dc94 ff                               rst 0x38
                           .data:0000dc95 18 18                            jr 0xdcaf
                           .data:0000dc97 18 18                            jr 0xdcb1
                           .data:0000dc99 18 18                            jr 0xdcb3
                           .data:0000dc9b 18 1f                            jr 0xdcbc
                           .data:0000dc9d 18 18                            jr 0xdcb7
                           .data:0000dc9f 18 00                            jr 0xdca1
                           .data:0000dca1 00                               nop
                           .data:0000dca2 00                               nop
                           .data:0000dca3 00                               nop
                           .data:0000dca4 ff                               rst 0x38
                           .data:0000dca5 00                               nop
                           .data:0000dca6 00                               nop
                           .data:0000dca7 00                               nop
                           .data:0000dca8 18 18                            jr 0xdcc2
                           .data:0000dcaa 18 18                            jr 0xdcc4
                           .data:0000dcac ff                               rst 0x38
                           .data:0000dcad 18 18                            jr 0xdcc7
                           .data:0000dcaf 18 18                            jr 0xdcc9
                           .data:0000dcb1 18 1f                            jr 0xdcd2
                           .data:0000dcb3 18 1f                            jr 0xdcd4
                           .data:0000dcb5 18 18                            jr 0xdccf
                           .data:0000dcb7 18 36                            jr 0xdcef
                           .data:0000dcb9 36 36                            ld (hl),0x36
                           .data:0000dcbb 36 37                            ld (hl),0x37
                           .data:0000dcbd 36 36                            ld (hl),0x36
                           .data:0000dcbf 36 36                            ld (hl),0x36
                           .data:0000dcc1 36 37                            ld (hl),0x37
                           .data:0000dcc3 30 3f                            jr nc,0xdd04
                           .data:0000dcc5 00                               nop
                           .data:0000dcc6 00                               nop
                           .data:0000dcc7 00                               nop
                           .data:0000dcc8 00                               nop
                           .data:0000dcc9 00                               nop
                           .data:0000dcca 3f                               ccf
                           .data:0000dccb 30 37                            jr nc,0xdd04
                           .data:0000dccd 36 36                            ld (hl),0x36
                           .data:0000dccf 36 36                            ld (hl),0x36
                           .data:0000dcd1 36 f7                            ld (hl),0xf7
                           .data:0000dcd3 00                               nop
                           .data:0000dcd4 ff                               rst 0x38
                           .data:0000dcd5 00                               nop
                           .data:0000dcd6 00                               nop
                           .data:0000dcd7 00                               nop
                           .data:0000dcd8 00                               nop
                           .data:0000dcd9 00                               nop
                           .data:0000dcda ff                               rst 0x38
                           .data:0000dcdb 00                               nop
                           .data:0000dcdc f7                               rst 0x30
                           .data:0000dcdd 36 36                            ld (hl),0x36
                           .data:0000dcdf 36 36                            ld (hl),0x36
                           .data:0000dce1 36 37                            ld (hl),0x37
                           .data:0000dce3 30 37                            jr nc,0xdd1c
                           .data:0000dce5 36 36                            ld (hl),0x36
                           .data:0000dce7 36 00                            ld (hl),0x00
                           .data:0000dce9 00                               nop
                           .data:0000dcea ff                               rst 0x38
                           .data:0000dceb 00                               nop
                           .data:0000dcec ff                               rst 0x38
                           .data:0000dced 00                               nop
                           .data:0000dcee 00                               nop
                           .data:0000dcef 00                               nop
                           .data:0000dcf0 36 36                            ld (hl),0x36
                           .data:0000dcf2 f7                               rst 0x30
                           .data:0000dcf3 00                               nop
                           .data:0000dcf4 f7                               rst 0x30
                           .data:0000dcf5 36 36                            ld (hl),0x36
                           .data:0000dcf7 36 18                            ld (hl),0x18
                           .data:0000dcf9 18 ff                            jr 0xdcfa
                           .data:0000dcfb 00                               nop
                           .data:0000dcfc ff                               rst 0x38
                           .data:0000dcfd 00                               nop
                           .data:0000dcfe 00                               nop
                           .data:0000dcff 00                               nop
                           .data:0000dd00 00                               nop
                           .data:0000dd01 00                               nop
                           .data:0000dd02 76                               halt
                           .data:0000dd03 dc c8 dc                         call c,0xdcc8
                           .data:0000dd06 76                               halt
                           .data:0000dd07 00                               nop
                           .data:0000dd08 00                               nop
                           .data:0000dd09 78                               ld a,b
                           .data:0000dd0a cc f8 cc                         call z,0xccf8
                           .data:0000dd0d f8                               ret m
                           .data:0000dd0e c0                               ret nz
                           .data:0000dd0f c0                               ret nz
                           .data:0000dd10 00                               nop
                           .data:0000dd11 fc cc c0                         call m,0xc0cc
                           .data:0000dd14 c0                               ret nz
                           .data:0000dd15 c0                               ret nz
                           .data:0000dd16 c0                               ret nz
                           .data:0000dd17 00                               nop
                           .data:0000dd18 00                               nop
                           .data:0000dd19 fe 6c                            cp 0x6c
                           .data:0000dd1b 6c                               ld l,h
                           .data:0000dd1c 6c                               ld l,h
                           .data:0000dd1d 6c                               ld l,h
                           .data:0000dd1e 6c                               ld l,h
                           .data:0000dd1f 00                               nop
                           .data:0000dd20 fc cc 60                         call m,0x60cc
                           .data:0000dd23 30 60                            jr nc,0xdd85
                           .data:0000dd25 cc fc 00                         call z,0x00fc
                           .data:0000dd28 00                               nop
                           .data:0000dd29 00                               nop
                           .data:0000dd2a 7e                               ld a,(hl)
                           .data:0000dd2b d8                               ret c
                           .data:0000dd2c d8                               ret c
                           .data:0000dd2d d8                               ret c
                           .data:0000dd2e 70                               ld (hl),b
                           .data:0000dd2f 00                               nop
                           .data:0000dd30 00                               nop
                           .data:0000dd31 66                               ld h,(hl)
                           .data:0000dd32 66                               ld h,(hl)
                           .data:0000dd33 66                               ld h,(hl)
                           .data:0000dd34 66                               ld h,(hl)
                           .data:0000dd35 7c                               ld a,h
                           .data:0000dd36 60                               ld h,b
                           .data:0000dd37 c0                               ret nz
                           .data:0000dd38 00                               nop
                           .data:0000dd39 76                               halt
                           .data:0000dd3a dc 18 18                         call c,0x1818
                           .data:0000dd3d 18 18                            jr 0xdd57
                           .data:0000dd3f 00                               nop
                           .data:0000dd40 fc 30 78                         call m,0x7830
                           .data:0000dd43 cc cc 78                         call z,0x78cc
                           .data:0000dd46 30 fc                            jr nc,0xdd44
                           .data:0000dd48 38 6c                            jr c,0xddb6
                           .data:0000dd4a c6 fe                            add a,0xfe
                           .data:0000dd4c c6 6c                            add a,0x6c
                           .data:0000dd4e 38 00                            jr c,0xdd50
                           .data:0000dd50 38 6c                            jr c,0xddbe
                           .data:0000dd52 c6 c6                            add a,0xc6
                           .data:0000dd54 6c                               ld l,h
                           .data:0000dd55 6c                               ld l,h
                           .data:0000dd56 ee 00                            xor 0x00
                           .data:0000dd58 1c                               inc e
                           .data:0000dd59 30 18                            jr nc,0xdd73
                           .data:0000dd5b 7c                               ld a,h
                           .data:0000dd5c cc cc 78                         call z,0x78cc
                           .data:0000dd5f 00                               nop
                           .data:0000dd60 00                               nop
                           .data:0000dd61 00                               nop
                           .data:0000dd62 7e                               ld a,(hl)
                           .data:0000dd63 db db                            in a,(0xdb)
                           .data:0000dd65 7e                               ld a,(hl)
                           .data:0000dd66 00                               nop
                           .data:0000dd67 00                               nop
                           .data:0000dd68 06 0c                            ld b,0x0c
                           .data:0000dd6a 7e                               ld a,(hl)
                           .data:0000dd6b db db                            in a,(0xdb)
                           .data:0000dd6d 7e                               ld a,(hl)
                           .data:0000dd6e 60                               ld h,b
                           .data:0000dd6f c0                               ret nz
                           .data:0000dd70 38 60                            jr c,0xddd2
                           .data:0000dd72 c0                               ret nz
                           .data:0000dd73 f8                               ret m
                           .data:0000dd74 c0                               ret nz
                           .data:0000dd75 60                               ld h,b
                           .data:0000dd76 38 00                            jr c,0xdd78
                           .data:0000dd78 78                               ld a,b
                           .data:0000dd79 cc cc cc                         call z,0xcccc
                           .data:0000dd7c cc cc cc                         call z,0xcccc
                           .data:0000dd7f 00                               nop
                           .data:0000dd80 00                               nop
                           .data:0000dd81 00                               nop
                           .data:0000dd82 00                               nop
                           .data:0000dd83 08                               ex af,af'
                           .data:0000dd84 18 38                            jr 0xddbe
                           .data:0000dd86 18 18                            jr 0xdda0
                           .data:0000dd88 00                               nop
                           .data:0000dd89 00                               nop
                           .data:0000dd8a 00                               nop
                           .data:0000dd8b 38 6c                            jr c,0xddf9
                           .data:0000dd8d 18 30                            jr 0xddbf
                           .data:0000dd8f 7c                               ld a,h
                           .data:0000dd90 00                               nop
                           .data:0000dd91 00                               nop
                           .data:0000dd92 00                               nop
                           .data:0000dd93 38 6c                            jr c,0xde01
                           .data:0000dd95 18 6c                            jr 0xde03
                           .data:0000dd97 38 00                            jr c,0xdd99
                           .data:0000dd99 00                               nop
                           .data:0000dd9a 00                               nop
                           .data:0000dd9b 6c                               ld l,h
                           .data:0000dd9c 6c                               ld l,h
                           .data:0000dd9d 7c                               ld a,h
                           .data:0000dd9e 0c                               inc c
                           .data:0000dd9f 0c                               inc c
                           .data:0000dda0 00                               nop
                           .data:0000dda1 00                               nop
                           .data:0000dda2 00                               nop
                           .data:0000dda3 7c                               ld a,h
                           .data:0000dda4 60                               ld h,b
                           .data:0000dda5 78                               ld a,b
                           .data:0000dda6 0c                               inc c
                           .data:0000dda7 78                               ld a,b
                           .data:0000dda8 00                               nop
                           .data:0000dda9 00                               nop
                           .data:0000ddaa 00                               nop
                           .data:0000ddab 38 60                            jr c,0xde0d
                           .data:0000ddad 78                               ld a,b
                           .data:0000ddae 6c                               ld l,h
                           .data:0000ddaf 38 00                            jr c,0xddb1
                           .data:0000ddb1 00                               nop
                           .data:0000ddb2 00                               nop
                           .data:0000ddb3 7c                               ld a,h
                           .data:0000ddb4 0c                               inc c
                           .data:0000ddb5 0c                               inc c
                           .data:0000ddb6 18 30                            jr 0xdde8
                           .data:0000ddb8 00                               nop
                           .data:0000ddb9 00                               nop
                           .data:0000ddba 00                               nop
                           .data:0000ddbb 38 6c                            jr c,0xde29
                           .data:0000ddbd 38 6c                            jr c,0xde2b
                           .data:0000ddbf 38 00                            jr c,0xddc1
                           .data:0000ddc1 00                               nop
                           .data:0000ddc2 00                               nop
                           .data:0000ddc3 38 6c                            jr c,0xde31
                           .data:0000ddc5 3c                               inc a
                           .data:0000ddc6 0c                               inc c
                           .data:0000ddc7 38 00                            jr c,0xddc9
                           .data:0000ddc9 00                               nop
                           .data:0000ddca 00                               nop
                           .data:0000ddcb 38 6c                            jr c,0xde39
                           .data:0000ddcd 6c                               ld l,h
                           .data:0000ddce 6c                               ld l,h
                           .data:0000ddcf 38 38                            jr c,0xde09
                           .data:0000ddd1 6c                               ld l,h
                           .data:0000ddd2 6c                               ld l,h
                           .data:0000ddd3 38 00                            jr c,0xddd5
                           .data:0000ddd5 00                               nop
                           .data:0000ddd6 00                               nop
                           .data:0000ddd7 00                               nop
                           .data:0000ddd8 38 6c                            jr c,0xde46
                           .data:0000ddda 6c                               ld l,h
                           .data:0000dddb 6c                               ld l,h
                           .data:0000dddc 38 00                            jr c,0xddde
                           .data:0000ddde 00                               nop
                           .data:0000dddf 00                               nop
                           .data:0000dde0 08                               ex af,af'
                           .data:0000dde1 18 38                            jr 0xde1b
                           .data:0000dde3 18 18                            jr 0xddfd
                           .data:0000dde5 00                               nop
                           .data:0000dde6 00                               nop
                           .data:0000dde7 00                               nop
                           .data:0000dde8 38 6c                            jr c,0xde56
                           .data:0000ddea 18 30                            jr 0xde1c
                           .data:0000ddec 7c                               ld a,h
                           .data:0000dded 00                               nop
                           .data:0000ddee 00                               nop
                           .data:0000ddef 00                               nop
                           .data:0000ddf0 38 6c                            jr c,0xde5e
                           .data:0000ddf2 18 6c                            jr 0xde60
                           .data:0000ddf4 38 00                            jr c,0xddf6
                           .data:0000ddf6 00                               nop
                           .data:0000ddf7 00                               nop
                           .data:0000ddf8 6c                               ld l,h
                           .data:0000ddf9 6c                               ld l,h
                           .data:0000ddfa 7c                               ld a,h
                           .data:0000ddfb 0c                               inc c
                           .data:0000ddfc 0c                               inc c
                           .data:0000ddfd 00                               nop
                           .data:0000ddfe 00                               nop
                           .data:0000ddff 00                               nop
                           .data:0000de00 00                               nop
                           .data:0000de01 00                               nop
                           .data:0000de02 cc d6 f6                         call z,0xf6d6
                           .data:0000de05 d6 cc                            sub 0xcc
                           .data:0000de07 00                               nop
                           .data:0000de08 00                               nop
                           .data:0000de09 00                               nop
                           .data:0000de0a 78                               ld a,b
                           .data:0000de0b 0c                               inc c
                           .data:0000de0c 7c                               ld a,h
                           .data:0000de0d cc 7e 00                         call z,0x007e
                           .data:0000de10 00                               nop
                           .data:0000de11 00                               nop
                           .data:0000de12 fc c0 fc                         call m,0xfcc0
                           .data:0000de15 c6 fc                            add a,0xfc
                           .data:0000de17 00                               nop
                           .data:0000de18 00                               nop
                           .data:0000de19 00                               nop
                           .data:0000de1a cc cc cc                         call z,0xcccc
                           .data:0000de1d cc fe 06                         call z,0x06fe
                           .data:0000de20 00                               nop
                           .data:0000de21 00                               nop
                           .data:0000de22 3c                               inc a
                           .data:0000de23 6c                               ld l,h
                           .data:0000de24 6c                               ld l,h
                           .data:0000de25 6c                               ld l,h
                           .data:0000de26 fe c6                            cp 0xc6
                           .data:0000de28 00                               nop
                           .data:0000de29 00                               nop
                           .data:0000de2a 78                               ld a,b
                           .data:0000de2b cc fc c0                         call z,0xc0fc
                           .data:0000de2e 78                               ld a,b
                           .data:0000de2f 00                               nop
                           .data:0000de30 00                               nop
                           .data:0000de31 00                               nop
                           .data:0000de32 10 7e                            djnz 0xdeb2
                           .data:0000de34 db 7e                            in a,(0x7e)
                           .data:0000de36 18 18                            jr 0xde50
                           .data:0000de38 00                               nop
                           .data:0000de39 00                               nop
                           .data:0000de3a fc c0 c0                         call m,0xc0c0
                           .data:0000de3d c0                               ret nz
                           .data:0000de3e c0                               ret nz
                           .data:0000de3f 00                               nop
                           .data:0000de40 00                               nop
                           .data:0000de41 00                               nop
                           .data:0000de42 c6 6c                            add a,0x6c
                           .data:0000de44 38 6c                            jr c,0xdeb2
                           .data:0000de46 c6 00                            add a,0x00
                           .data:0000de48 00                               nop
                           .data:0000de49 00                               nop
                           .data:0000de4a c6 c6                            add a,0xc6
                           .data:0000de4c ce fe                            adc a,0xfe
                           .data:0000de4e 66                               ld h,(hl)
                           .data:0000de4f 00                               nop
                           .data:0000de50 0c                               inc c
                           .data:0000de51 18 c6                            jr 0xde19
                           .data:0000de53 c6 ce                            add a,0xce
                           .data:0000de55 fe 66                            cp 0x66
                           .data:0000de57 00                               nop
                           .data:0000de58 00                               nop
                           .data:0000de59 00                               nop
                           .data:0000de5a cc d8 f0                         call z,0xf0d8
                           .data:0000de5d d8                               ret c
                           .data:0000de5e ce 00                            adc a,0x00
                           .data:0000de60 00                               nop
                           .data:0000de61 00                               nop
                           .data:0000de62 1e 36                            ld e,0x36
                           .data:0000de64 66                               ld h,(hl)
                           .data:0000de65 66                               ld h,(hl)
                           .data:0000de66 e6 00                            and 0x00
                           .data:0000de68 00                               nop
                           .data:0000de69 00                               nop
                           .data:0000de6a c6 ee                            add a,0xee
                           .data:0000de6c fe d6                            cp 0xd6
                           .data:0000de6e c6 00                            add a,0x00
                           .data:0000de70 00                               nop
                           .data:0000de71 00                               nop
                           .data:0000de72 c6 c6                            add a,0xc6
                           .data:0000de74 fe c6                            cp 0xc6
                           .data:0000de76 c6 00                            add a,0x00
                           .data:0000de78 00                               nop
                           .data:0000de79 00                               nop
                           .data:0000de7a 7c                               ld a,h
                           .data:0000de7b c6 c6                            add a,0xc6
                           .data:0000de7d c6 7c                            add a,0x7c
                           .data:0000de7f 00                               nop
                           .data:0000de80 00                               nop
                           .data:0000de81 00                               nop
                           .data:0000de82 fe c6                            cp 0xc6
                           .data:0000de84 c6 c6                            add a,0xc6
                           .data:0000de86 c6 00                            add a,0x00
                           .data:0000de88 00                               nop
                           .data:0000de89 00                               nop
                           .data:0000de8a 7e                               ld a,(hl)
                           .data:0000de8b c6 7e                            add a,0x7e
                           .data:0000de8d 66                               ld h,(hl)
                           .data:0000de8e c6 00                            add a,0x00
                           .data:0000de90 00                               nop
                           .data:0000de91 00                               nop
                           .data:0000de92 fc c6 c6                         call m,0xc6c6
                           .data:0000de95 fc c0 c0                         call m,0xc0c0
                           .data:0000de98 00                               nop
                           .data:0000de99 00                               nop
                           .data:0000de9a 7c                               ld a,h
                           .data:0000de9b c6 c0                            add a,0xc0
                           .data:0000de9d c6 7c                            add a,0x7c
                           .data:0000de9f 00                               nop
                           .data:0000dea0 00                               nop
                           .data:0000dea1 00                               nop
                           .data:0000dea2 fc 30 30                         call m,0x3030
                           .data:0000dea5 30 30                            jr nc,0xded7
                           .data:0000dea7 00                               nop
                           .data:0000dea8 00                               nop
                           .data:0000dea9 00                               nop
                           .data:0000deaa c6 66                            add a,0x66
                           .data:0000deac 3e 06                            ld a,0x06
                           .data:0000deae 7c                               ld a,h
                           .data:0000deaf 00                               nop
                           .data:0000deb0 00                               nop
                           .data:0000deb1 00                               nop
                           .data:0000deb2 d6 d6                            sub 0xd6
                           .data:0000deb4 7c                               ld a,h
                           .data:0000deb5 d6 d6                            sub 0xd6
                           .data:0000deb7 00                               nop
                           .data:0000deb8 00                               nop
                           .data:0000deb9 00                               nop
                           .data:0000deba f8                               ret m
                           .data:0000debb cc f8 cc                         call z,0xccf8
                           .data:0000debe f8                               ret m
                           .data:0000debf 00                               nop
                           .data:0000dec0 00                               nop
                           .data:0000dec1 00                               nop
                           .data:0000dec2 c0                               ret nz
                           .data:0000dec3 c0                               ret nz
                           .data:0000dec4 fc c6 fc                         call m,0xfcc6
                           .data:0000dec7 00                               nop
                           .data:0000dec8 00                               nop
                           .data:0000dec9 00                               nop
                           .data:0000deca c6 c6                            add a,0xc6
                           .data:0000decc f6 da                            or 0xda
                           .data:0000dece f6 00                            or 0x00
                           .data:0000ded0 00                               nop
                           .data:0000ded1 00                               nop
                           .data:0000ded2 78                               ld a,b
                           .data:0000ded3 cc 18 cc                         call z,0xcc18
                           .data:0000ded6 78                               ld a,b
                           .data:0000ded7 00                               nop
                           .data:0000ded8 00                               nop
                           .data:0000ded9 00                               nop
                           .data:0000deda d6 d6                            sub 0xd6
                           .data:0000dedc d6 d6                            sub 0xd6
                           .data:0000dede fe 00                            cp 0x00
                           .data:0000dee0 00                               nop
                           .data:0000dee1 00                               nop
                           .data:0000dee2 fc 06 1e                         call m,0x1e06
                           .data:0000dee5 06 fc                            ld b,0xfc
                           .data:0000dee7 00                               nop
                           .data:0000dee8 00                               nop
                           .data:0000dee9 00                               nop
                           .data:0000deea d6 d6                            sub 0xd6
                           .data:0000deec d6 d6                            sub 0xd6
                           .data:0000deee fe 06                            cp 0x06
                           .data:0000def0 00                               nop
                           .data:0000def1 00                               nop
                           .data:0000def2 cc cc 7c                         call z,0x7ccc
                           .data:0000def5 0c                               inc c
                           .data:0000def6 0c                               inc c
                           .data:0000def7 00                               nop
                           .data:0000def8 00                               nop
                           .data:0000def9 00                               nop
                           .data:0000defa e0                               ret po
                           .data:0000defb 60                               ld h,b
                           .data:0000defc 7c                               ld a,h
                           .data:0000defd 66                               ld h,(hl)
                           .data:0000defe 7c                               ld a,h
                           .data:0000deff 00                               nop
                           .data:0000df00 cc d6 d6                         call z,0xd6d6
                           .data:0000df03 f6 d6                            or 0xd6
                           .data:0000df05 d6 cc                            sub 0xcc
                           .data:0000df07 00                               nop
                           .data:0000df08 30 78                            jr nc,0xdf82
                           .data:0000df0a cc cc fc                         call z,0xfccc
                           .data:0000df0d cc cc 00                         call z,0x00cc
                           .data:0000df10 fe 60                            cp 0x60
                           .data:0000df12 60                               ld h,b
                           .data:0000df13 7c                               ld a,h
                           .data:0000df14 66                               ld h,(hl)
                           .data:0000df15 66                               ld h,(hl)
                           .data:0000df16 fc 00 cc                         call m,0xcc00
                           .data:0000df19 cc cc cc                         call z,0xcccc
                           .data:0000df1c cc cc fe                         call z,0xfecc
                           .data:0000df1f 06 3c                            ld b,0x3c
                           .data:0000df21 6c                               ld l,h
                           .data:0000df22 6c                               ld l,h
                           .data:0000df23 6c                               ld l,h
                           .data:0000df24 6c                               ld l,h
                           .data:0000df25 6c                               ld l,h
                           .data:0000df26 fe c6                            cp 0xc6
                           .data:0000df28 fe 62                            cp 0x62
                           .data:0000df2a 68                               ld l,b
                           .data:0000df2b 78                               ld a,b
                           .data:0000df2c 68                               ld l,b
                           .data:0000df2d 62                               ld h,d
                           .data:0000df2e fe 00                            cp 0x00
                           .data:0000df30 18 7e                            jr 0xdfb0
                           .data:0000df32 db db                            in a,(0xdb)
                           .data:0000df34 7e                               ld a,(hl)
                           .data:0000df35 18 3c                            jr 0xdf73
                           .data:0000df37 00                               nop
                           .data:0000df38 fe 66                            cp 0x66
                           .data:0000df3a 60                               ld h,b
                           .data:0000df3b 60                               ld h,b
                           .data:0000df3c 60                               ld h,b
                           .data:0000df3d 60                               ld h,b
                           .data:0000df3e f0                               ret p
                           .data:0000df3f 00                               nop
                           .data:0000df40 c6 6c                            add a,0x6c
                           .data:0000df42 38 38                            jr c,0xdf7c
                           .data:0000df44 6c                               ld l,h
                           .data:0000df45 c6 c6                            add a,0xc6
                           .data:0000df47 00                               nop
                           .data:0000df48 c6 c6                            add a,0xc6
                           .data:0000df4a ce de                            adc a,0xde
                           .data:0000df4c f6 e6                            or 0xe6
                           .data:0000df4e c6 00                            add a,0x00
                           .data:0000df50 38 c6                            jr c,0xdf18
                           .data:0000df52 ce de                            adc a,0xde
                           .data:0000df54 f6 e6                            or 0xe6
                           .data:0000df56 c6 00                            add a,0x00
                           .data:0000df58 c6 cc                            add a,0xcc
                           .data:0000df5a d8                               ret c
                           .data:0000df5b f0                               ret p
                           .data:0000df5c d8                               ret c
                           .data:0000df5d cc c6 00                         call z,0x00c6
                           .data:0000df60 1e 36                            ld e,0x36
                           .data:0000df62 66                               ld h,(hl)
                           .data:0000df63 66                               ld h,(hl)
                           .data:0000df64 66                               ld h,(hl)
                           .data:0000df65 66                               ld h,(hl)
                           .data:0000df66 c6 00                            add a,0x00
                           .data:0000df68 c6 ee                            add a,0xee
                           .data:0000df6a fe fe                            cp 0xfe
                           .data:0000df6c d6 c6                            sub 0xc6
                           .data:0000df6e c6 00                            add a,0x00
                           .data:0000df70 cc cc cc                         call z,0xcccc
                           .data:0000df73 fc cc cc                         call m,0xcccc
                           .data:0000df76 cc 00 7c                         call z,0x7c00
                           .data:0000df79 c6 c6                            add a,0xc6
                           .data:0000df7b c6 c6                            add a,0xc6
                           .data:0000df7d c6 7c                            add a,0x7c
                           .data:0000df7f 00                               nop
                           .data:0000df80 fe c6                            cp 0xc6
                           .data:0000df82 c6 c6                            add a,0xc6
                           .data:0000df84 c6 c6                            add a,0xc6
                           .data:0000df86 c6 00                            add a,0x00
                           .data:0000df88 7e                               ld a,(hl)
                           .data:0000df89 c6 c6                            add a,0xc6
                           .data:0000df8b 7e                               ld a,(hl)
                           .data:0000df8c 36 66                            ld (hl),0x66
                           .data:0000df8e c6 00                            add a,0x00
                           .data:0000df90 fc 66 66                         call m,0x6666
                           .data:0000df93 7c                               ld a,h
                           .data:0000df94 60                               ld h,b
                           .data:0000df95 60                               ld h,b
                           .data:0000df96 f0                               ret p
                           .data:0000df97 00                               nop
                           .data:0000df98 3c                               inc a
                           .data:0000df99 66                               ld h,(hl)
                           .data:0000df9a c0                               ret nz
                           .data:0000df9b c0                               ret nz
                           .data:0000df9c c0                               ret nz
                           .data:0000df9d 66                               ld h,(hl)
                           .data:0000df9e 3c                               inc a
                           .data:0000df9f 00                               nop
                           .data:0000dfa0 fc b4 30                         call m,0x30b4
                           .data:0000dfa3 30 30                            jr nc,0xdfd5
                           .data:0000dfa5 30 78                            jr nc,0xe01f
                           .data:0000dfa7 00                               nop
                           .data:0000dfa8 c6 c6                            add a,0xc6
                           .data:0000dfaa c6 7e                            add a,0x7e
                           .data:0000dfac 06 0c                            ld b,0x0c
                           .data:0000dfae 78                               ld a,b
                           .data:0000dfaf 00                               nop
                           .data:0000dfb0 d6 d6                            sub 0xd6
                           .data:0000dfb2 7c                               ld a,h
                           .data:0000dfb3 38 7c                            jr c,0xe031
                           .data:0000dfb5 d6 d6                            sub 0xd6
                           .data:0000dfb7 00                               nop
                           .data:0000dfb8 fc 66 66                         call m,0x6666
                           .data:0000dfbb 7c                               ld a,h
                           .data:0000dfbc 66                               ld h,(hl)
                           .data:0000dfbd 66                               ld h,(hl)
                           .data:0000dfbe fc 00 c0                         call m,0xc000
                           .data:0000dfc1 c0                               ret nz
                           .data:0000dfc2 c0                               ret nz
                           .data:0000dfc3 fc c6 c6                         call m,0xc6c6
                           .data:0000dfc6 fc 00 c6                         call m,0xc600
                           .data:0000dfc9 c6 c6                            add a,0xc6
                           .data:0000dfcb f6 da                            or 0xda
                           .data:0000dfcd da f6 00                         jp c,0x00f6
                           .data:0000dfd0 7c                               ld a,h
                           .data:0000dfd1 c6 06                            add a,0x06
                           .data:0000dfd3 1c                               inc e
                           .data:0000dfd4 06 c6                            ld b,0xc6
                           .data:0000dfd6 7c                               ld a,h
                           .data:0000dfd7 00                               nop
                           .data:0000dfd8 c6 d6                            add a,0xd6
                           .data:0000dfda d6 d6                            sub 0xd6
                           .data:0000dfdc d6 d6                            sub 0xd6
                           .data:0000dfde fe 00                            cp 0x00
                           .data:0000dfe0 7c                               ld a,h
                           .data:0000dfe1 c6 06                            add a,0x06
                           .data:0000dfe3 3e 06                            ld a,0x06
                           .data:0000dfe5 c6 7c                            add a,0x7c
                           .data:0000dfe7 00                               nop
                           .data:0000dfe8 c6 d6                            add a,0xd6
                           .data:0000dfea d6 d6                            sub 0xd6
                           .data:0000dfec d6 d6                            sub 0xd6
                           .data:0000dfee fe 06                            cp 0x06
                           .data:0000dff0 c6 c6                            add a,0xc6
                           .data:0000dff2 c6 7e                            add a,0x7e
                           .data:0000dff4 06 06                            ld b,0x06
                           .data:0000dff6 06 00                            ld b,0x00
                           .data:0000dff8 ff                               rst 0x38
                           .data:0000dff9 ff                               rst 0x38
                           .data:0000dffa ff                               rst 0x38
                           .data:0000dffb ff                               rst 0x38
                           .data:0000dffc ff                               rst 0x38
                           .data:0000dffd ff                               rst 0x38
                           .data:0000dffe ff                               rst 0x38
                           .data:0000dfff ff                               rst 0x38

