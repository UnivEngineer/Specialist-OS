; Тест MXOS: физическое чтение и создание временного файла через DOS.
; ВИ53: CLK1 / 256 -> CLK2, GATE1/2=1, без прерываний.
; Запись включает выделение кластеров, FAT, каталог и штатные барьеры DOS.
    INCLUDE "../../source/common/system.inc"
    INCLUDE "layout.inc"
    IFDEF DISKBEN_BANK
IMAGE_FLAGS = IMAGE_FLAG_BANK
    ELSE
IMAGE_FLAGS         = IMAGE_FLAG_MAIN
IMAGE_MEMORY_POLICY = IMAGE_POLICY_MANAGED
IMAGE_API           = 2
    ENDIF
    IFNDEF PIT_HZ
PIT_HZ = 2000000
    ENDIF
    ASSERT PIT_HZ == 2000000 || PIT_HZ == 2500000
RATE_FACTOR      = PIT_HZ / 1024
TEST_SECTORS     = 32
DEFAULT_FILE_KIB = 8
    IFNDEF MAX_FILE_KIB
MAX_FILE_KIB = 24
    ENDIF
MAX_FILE_SECTORS  = MAX_FILE_KIB * 4
LAYOUT_SIMPLE     = 0
LAYOUT_MAPPED     = 1
    MACRO LOAD_DE address
    push hl
    ld hl,(address)
    ex de,hl
    pop hl
    ENDM
    MACRO STORE_DE address
    push hl
    ex de,hl
    ld (address),hl
    ex de,hl
    pop hl
    ENDM
    ORG 0
;----------------------------------------------------------------------------
; Выбрать режим дискового теста, запустить его и вывести результат.
;
; вход:
;   MAIN: de = указатель на аргументы командной строки. BANK: b = диск, c = режим
;   проверки, d = число проходов записи, e = размер файла в КиБ.
;
; выход:
;   Возврат на исходный стек с cf = 0. Ошибки выводятся на экран. MAIN восстанавливает
;   кодировку шрифта. Значения рабочих регистров и остальных флагов не заданы.
;----------------------------------------------------------------------------

entry:
    di
    IFNDEF DISKBEN_BANK
    STORE_DE v_arguments
    call appProgramDrive
    ld (modulePath),a
    ENDIF
    ld hl,0
    add hl,sp
    ld (callerSP),hl
    IFNDEF DISKBEN_BANK
    ld a,(bios_upper.koi7)
    ld (callerKoi),a
    xor a
    call bios_setFontEncoding       ; Имена файлов — KOI-8.
    ENDIF
    IF FAT_CACHE_ADDR == 0FB00h
    call bench_getMemTop
    ld de,program_end-1
    call bench_cmp_hl_de
    jp nc,benchMemoryReady
    ld hl,txtNoMemory
    jp show_error
benchMemoryReady:
    ENDIF
    IFDEF DISKBEN_BANK
    ; Один MODULE_CALL на полный прогон. Параметры переданы значениями.
    ld a,b
    ld (argDrive),a
    ld a,c
    ld (argCheck),a
    ld a,d
    ld (argWrites),a
    ld a,e
    ld (argSize),a
    ELSE
    ld hl,(v_arguments)
    call parse_arguments
    jp c,argument_error
    ld a,(argHelp)
    or a
    jp nz,argument_help
    ENDIF
    ld hl,txtTitle
    call puts
    ld a,(argDrive)
    cp 0FFh
    jp nz,drive_selected
menu_drive:
    ld hl,txtDrive
    call puts
    call read_choice
    jp c,menu_drive
    or a
    jp nz,drive_choice
    ld a,'A'
drive_choice:
    and 0DFh
    cp 'Q'
    jp z,finished
drive_value:
    sub 'A'
    cp 8
    jp nc,menu_drive
drive_selected:
    ld (v_drive),a
    ld e,1
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    jp c,error
    ld b,3
    call raw_driver
    ld (v_info),hl
    ld a,d
    or e
    jp z,error
    STORE_DE v_capacity
    call detect_driver
    IFNDEF DISKBEN_LEGACY_FIXTURE
    ld de,512
    call reserveBenchBuffer
    jp c,benchMemoryError
    ENDIF
    ld de,0
    ld hl,bootSectorBuffer
    ld b,2
    call raw_driver
    jp c,error
    call geometry
    jp c,error
    IFNDEF DISKBEN_BANK
    call choose_executor
    call choose_access
    call choose_alignment
    ENDIF
    IFDEF DISKBEN_BANK
    ; MAIN уже прочитал и проверил интерактивные параметры.
    ld a,(argCheck)
    ld (v_writeMode),a
    ld a,(argWrites)
    ld (v_budget),a
    ld a,(argSize)
    jp file_size_value
    ENDIF
    ld a,1
    ld (v_writeMode),a
    xor a
    ld (v_budget),a
    ld a,(v_caps)
    and DISK_CAP_WRITE
    jp z,prepare
    ld a,(argWrites)
    or a
    jp z,prepare              ; -W=0: размер файла и проверка не нужны.
    ld a,(v_technology)
    cp DISK_TECH_MX_SYSTEM_FLASH
    jp z,flashNotice
    cp DISK_TECH_MX_VV55_FLASH
    jp z,flashNotice
    cp DISK_TECH_FLASH
    jp nz,choose_verify
flashNotice:
    ld hl,txtFlashLife
    call puts
choose_verify:
    ld a,(argCheck)
    cp 0FFh
    jp z,menu_verify
    call select_check
    jp c,argument_error
    jp write_budget
menu_verify:
    ld hl,txtVerify
    call puts
    call read_choice
    jp c,menu_verify
    or a
    jp z,verify_choice
    sub '0'
verify_choice:
    call select_check
    jp c,menu_verify
write_budget:
    ld a,(argWrites)
    cp 0FFh
    jp nz,budget_value
menu_writes:
    ld hl,txtWrites
    call puts
    call read_choice
    jp c,menu_writes
    or a
    jp nz,budget_digit
    ld a,1
    jp budget_value
budget_digit:
    sub '0'
    cp 10
    jp nc,menu_writes
budget_value:
    ld (v_budget),a
    or a
    jp z,prepare
file_size_menu:
    ld a,(argSize)
    or a
    jp nz,file_size_value
menu_size:
    ld hl,txtFileSizePrompt
    call puts
    call read_line
    call parse_file_kib
    jp c,menu_size
file_size_value:
    ld (v_fileKiB),a
    ; Рабочий буфер измерителя адресуется страницами по 256 байт.
    add a,a
    add a,a
    ld (v_fileSectors),a
    ld h,a
    ld l,0
    ld (tmpDescriptor+FILE_DESCRIPTOR.size),hl
set_buffer_end:
    ld a,h
    add a,sectorBuffer / 256
    ld (v_bufferEndHi),a
    ld a,(argFrag)
    cp 0FFh
    jp nz,fragment_selected
menu_fragment:
    ld hl,txtFragment
    call puts
    call read_choice
    jp c,menu_fragment
    or a
    jp z,fragment_selected
    sub '0'
    cp 2
    jp nc,menu_fragment
fragment_selected:
    ld (argFrag),a
    jp prepare

; HL=строка десятичного размера; пустая строка = 8 КиБ.
; Выход A=1..24, CF=ошибка; AF/BC/DE/HL разрушены. Нет усечения/переполнения.
parse_file_kib:
    MXOS_CALL_REGS MXOS_BIOS_ARG_NEXT
    ld a,DEFAULT_FILE_KIB
    jp nc,parse_size_word
    or a
    ret
parse_size_word:
    ex de,hl
    ld c,0
parse_size_digit:
    ld a,(hl)
    cp ' '+1
    jp c,parse_size_done
    sub '0'
    cp 10
    jp nc,bad_geometry
    ld b,a
    ld a,c
    add a,a
    add a,a
    add a,c
    add a,a
    add a,b
    cp MAX_FILE_KIB+1
    jp nc,bad_geometry
    ld c,a
    inc hl
    jp parse_size_digit
parse_size_done:
    MXOS_CALL_REGS MXOS_BIOS_ARG_NEXT
    jp nc,bad_geometry        ; Второе слово не является частью размера.
    ld a,c
    or a
    ret nz
    scf
    ret
prepare:
    jp run_bank_module
done:
    ld hl,txtDone
    call puts
exit_wait:
    call bench_getch
    ld c,00Ah
    call bench_printChar
finished:
    IFNDEF DISKBEN_BANK
    ld a,(callerKoi)
    call bios_setFontEncoding
    ENDIF
    ld hl,(callerSP)
    ld sp,hl
    or a
    ret
timer_error:
    ld hl,txtTimerError
    jp show_error
error:
    call file_cleanup
    ld hl,txtError
show_error:
    call puts
    jp done
txtNoMemory: DB "NOT ENOUGH MEMORY",0
puts:
    jp bench_printString

    INCLUDE "api.inc"
    IFNDEF DISKBEN_BANK
    INCLUDE "module.inc"
    ENDIF
    INCLUDE "args.inc"
    INCLUDE "fileTest.inc"

; Строковый ввод BIOS с редактированием. Enter полностью потребляется здесь.
; Выход HL=буфер с нулём, остальные регистры и флаги разрушаются.
; Выбор нужен только при отсутствии -C:... или совместимого старого ключа.
choose_executor:
    ld a,(argBank)
    or a
    ret nz
menu_executor:
    ld hl,txtExecutor
    call puts
    call read_line
    MXOS_CALL_REGS MXOS_BIOS_ARG_NEXT
    ld c,3                     ; Enter: MAIN DLL.
    jp c,argument_bank
    push hl
    ex de,hl
    call argument_context
    pop hl
    jp c,menu_executor
    MXOS_CALL_REGS MXOS_BIOS_ARG_NEXT
    ret c
    xor a
    ld (argBank),a
    jp menu_executor

; Вход: разобраны ключи -M/-V/-W и известен носитель. Выход: argMode =
; 1 RAW, 2 FILE, 3 ALL. Неполный интерактивный запуск запрашивает выбор.
; При явно заданной проверке или числе проходов сохраняется пакетный запуск.
choose_access:
    ld a,(argMode)
    or a
    ret nz
    ld a,(v_technology)
    cp DISK_TECH_FLOPPY
    ld a,2
    jp nz,accessDefault
    dec a
accessDefault:
    push af
    ld a,(argWrites)
    inc a
    jp nz,accessUseDefault
    ld a,(argCheck)
    inc a
    jp nz,accessUseDefault
menu_access:
    ld hl,txtAccess
    call puts
    pop af
    push af
    add a,'0'
    ld c,a
    call bench_printChar
    ld hl,txtAccessEnd
    call puts
    call read_choice
    jp c,menu_access
    or a
    jp z,accessUseDefault
    sub '0'
    cp 3
    jp nc,menu_access
    pop bc
    jp accessStore
accessUseDefault:
    pop af
accessStore:
    inc a
    ld (argMode),a
    ret
txtAccess:     DB 10,"Access: 0=RAW, 1=FILE, 2=ALL [",0
txtAccessEnd:  DB "]: ",0

choose_alignment:
    ld a,(argAlign)
    cp 0FFh
    ret nz
    ld a,(v_technology)
    cp DISK_TECH_MX_VV55_FLASH
    jp z,menu_alignment
    cp DISK_TECH_MX_VV55_ROM
    ld a,0
    jp nz,alignment_selected
menu_alignment:
    ld hl,txtAlignmentPrompt
    call puts
    call read_choice
    jp c,menu_alignment
    or a
    jp z,alignment_selected
    sub '0'
    cp 2
    jp nc,menu_alignment
alignment_selected:
    ld (argAlign),a
    ret

txtAlignmentPrompt:     DB 10,"Buffer: 0=aligned, 1=unaligned [0]: ",0
txtExecutor:            DB 10,"Caller MAIN/DOS/BANK/ALL [MAIN]: ",0
read_line:
    ld hl,inputBuffer
    ld de,inputBuffer+16
    call bench_input
    xor a
    ld (de),a
    ld c,00Ah
    call bench_printChar
    ld hl,inputBuffer
    ret

; Один символ либо пустая строка (значение по умолчанию).
; Выход A=символ/0, CF=1 при лишнем тексте; HL/BC/DE разрушаются.
read_choice:
    call read_line
    MXOS_CALL_REGS MXOS_BIOS_ARG_NEXT
    jp nc,choice_word
    xor a
    ret
choice_word:
    ld a,b
    dec a
    jp nz,choice_bad
    ld a,(de)
    ld c,a
    MXOS_CALL_REGS MXOS_BIOS_ARG_NEXT
    jp nc,choice_bad
    ld a,c
    or a
    ret
choice_bad:
    scf
    ret

; Z=1 для системного ROM/flash. A и флаги рабочие, BC/DE/HL сохранены.
benchMapped:
    ld a,(v_technology)
    cp DISK_TECH_MX_SYSTEM_FLASH
    ret z
    cp DISK_TECH_MX_SYSTEM_ROM
    ret

; B=FFh возвращает описание, технологию и возможности самого драйвера.
; До обращения к неизвестному драйверу выбираем безопасный режим чтения.
detect_driver:
    ld b,0FFh
    ld hl,0
    call raw_driver
    jp c,detect_unknown
    ld a,h
    or l
    jp z,detect_unknown
    ld (v_driverInfo),hl
    ld a,c
    ld (v_caps),a
    ld a,b
    ld (v_technology),a
    ret
detect_unknown:
    ld hl,txtUnknown
    ld (v_driverInfo),hl
    ld a,DISK_CAP_READ
    ld (v_caps),a
    xor a
    ld (v_technology),a
    or a                        ; Отсутствие метаданных не является ошибкой чтения.
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Вывести сводку настроек теста до начала измерения.
;
; вход:
;   v_drive, v_info и v_driverInfo описывают выбранный том.
;   Геометрия проверена. Размер файла и v_budget заданы.
;
; выход:
;   Сводка напечатана. AF, BC, DE, HL и флаги могут измениться.
;----------------------------------------------------------------------------
print_summary:
    ld a,(argBank)
    ld hl,txtCaller
    cp 1
    jp z,summaryBank
    ld hl,txtCallerDos
    cp 2
    jp z,summaryCaller
    ld hl,txtCallerMainDll
    cp 3
    jp z,summaryCaller
    ld hl,txtCallerBatch
    cp 4
    jp z,summaryCaller
    ld hl,txtCaller
    jp summaryCaller
summaryBank:
    ld hl,txtCallerBank
summaryCaller:
    call puts
    ld hl,txtSummaryDrive
    call puts
    ld a,(v_drive)
    add a,'A'
    ld c,a
    call bench_printChar
    ld hl,txtLabel
    call puts
    ld hl,(v_info)
    ld de,DISK_INFO.volumeLabel
    add hl,de
    call puts
    ld hl,txtType
    call puts
    ld a,(v_technology)
    ld c,a
    ld hl,summaryTypes
summaryTypeNext:
    ld a,(hl)
    inc hl
    or a
    jp z,summaryTypeFound
    cp c
    jp z,summaryTypeFound
    inc hl
    inc hl
    jp summaryTypeNext
summaryTypeFound:
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
summary_type:
    call puts
    ld hl,txtDriver
    call puts
    ld hl,(v_driverInfo)
    call puts
    inc hl
    ld c,' '
    call bench_printChar
    call puts
    ld hl,txtCapacity
    call puts
    ld hl,(v_capacity)
    call bench_div_hl_2
    ld a,(v_sectorSize+1)
    cp 2
    jp z,summary_capacity
    call bench_div_hl_2
summary_capacity:
    ex de,hl
    ld b,0
    call bench_printDecWord
    ld hl,txtSectorSize
    call puts
    LOAD_DE v_sectorSize
    call bench_printDecWord
    ld hl,txtSpc
    call puts
    ld a,(v_spc)
    ld e,a
    ld d,0
    call bench_printDecWord
    ld hl,txtMode
    call puts
    ld a,(v_budget)
    or a
    ld hl,txtReadOnly
    jp z,summary_mode
    ld a,(v_writeMode)
    cp 1
    ld hl,txtWrite
    jp z,summary_mode
    ld hl,txtWriteVerify
    cp 5
    jp z,summary_mode
    ld hl,txtWriteFileVerify
summary_mode:
    call puts
    ld a,(argAlign)
    or a
    ld hl,txtAligned
    jp z,summary_alignment
    ld hl,txtUnaligned
summary_alignment:
    call puts
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    jp nz,summary_passes
    ld hl,txtBuffers
    call puts
summary_passes:
    ld hl,txtPasses
    call puts
    ld a,(v_budget)
    ld e,a
    ld d,0
    ld b,0
    call bench_printDecWord
    ld a,(v_budget)
    or a
    jp z,summary_end
    ld hl,txtFileSize
    call puts
    ld a,(v_fileKiB)
    ld e,a
    ld d,0
    ld b,0
    call bench_printDecWord
    ld hl,txtClusters
    call puts
    call requiredClusters
    ex de,hl
    ld b,0
    call bench_printDecWord
summary_end:
    ld hl,txtLineBreak
    jp puts

;----------------------------------------------------------------------------
; MXOS
;
; Вычислить число кластеров для одного файла с округлением вверх.
;
; вход:
;   v_sectorSize, v_spc — проверенные размер сектора в байтах и секторов
;   в кластере. v_fileSectors — размер файла в страницах по 256 байт.
;
; выход:
;   hl = число кластеров. AF, BC, DE и флаги разрушаются.
;----------------------------------------------------------------------------
requiredClusters:
    ld a,(v_sectorSize+1)
    ld h,0
    ld l,a
    ld a,(v_spc)
    ld e,a
    ld d,0
    call bench_mul_hl_de
    ex de,hl                   ; DE = страниц по 256 байт на кластер.
    ld a,(v_fileSectors)
    ld h,0
    ld l,a
    add hl,de
    dec hl
    jp bench_div_hl_de

; BPB -> геометрия физического ABI. AF/BC/DE/HL/флаги разрушаются.
; Поддерживаются секторы 256..4096, один/два FAT, корень с округлением вверх.
; Для дискеты допускается FAT12, у остальных драйверов требуется FAT16.
geometry:
    ld hl,bootSectorBuffer+036h
    ld a,(v_technology)
    cp DISK_TECH_FLOPPY
    ld de,txtFAT16
    jp nz,geometry_expected
    ld de,txtFAT12
geometry_expected:
    ld b,8
geometry_signature:
    ld a,(de)
    cp (hl)
    jp nz,bad_geometry
    inc hl
    inc de
    dec b
    jp nz,geometry_signature
    ld hl,(bootSectorBuffer+00Bh)
    ld a,l
    or a
    jp nz,bad_geometry
    ld a,h
    or a
    jp z,bad_geometry
    cp 17
    jp nc,bad_geometry
    dec a
    and h
    jp nz,bad_geometry
    ld (v_sectorSize),hl
    ld a,(bootSectorBuffer+00Dh)
    or a
    jp z,bad_geometry
    ld (v_spc),a
    dec a
    ld b,a
    inc a
    and b
    jp nz,bad_geometry
    ld hl,(bootSectorBuffer+00Eh)
    ld a,h
    or l
    jp z,bad_geometry
    ld (v_reserved),hl
    ld hl,(bootSectorBuffer+016h)
    ld a,h
    and 080h
    jp nz,bad_geometry
    ld a,h
    or l
    jp z,bad_geometry
    ld (v_fatSize),hl
    ld a,(bootSectorBuffer+010h)
    dec a
    cp 2
    jp nc,bad_geometry
    inc a
    ld e,a
    ld d,0
    call bench_mul_hl_de
    LOAD_DE v_reserved
    add hl,de
    jp c,bad_geometry
    ld (v_dataStart),hl
    ld hl,(bootSectorBuffer+011h)
    ld a,h
    cp 4
    jp nc,bad_geometry
    ld de,32
    call bench_mul_hl_de
    LOAD_DE v_sectorSize
    add hl,de
    dec hl
    call bench_div_hl_de
    LOAD_DE v_dataStart
    add hl,de
    jp c,bad_geometry
    ld (v_dataStart),hl
    ld hl,(bootSectorBuffer+013h)
    ld a,h
    or l
    jp z,bad_geometry
    ex de,hl
    ld hl,(v_capacity)
    call bench_cmp_hl_de
    jp c,bad_geometry
    STORE_DE v_capacity
    ld hl,(v_dataStart)
    call bench_cmp_hl_de
    jp nc,bad_geometry
    or a
    ret
bad_geometry:
    scf
    ret

    IFDEF DISKBEN_LEGACY_FIXTURE
; Участок старого теста READ без временного файла. Данные могут быть заняты:
; запись здесь не выполняется. Для RAM исключаем служебный сектор xxFFh.
; Выход v_start; CF=ошибка, все регистры разрушены.
find_region:
    ld hl,(v_dataStart)
    ld a,(v_technology)
    cp DISK_TECH_RAM
    jp nz,region_bounds
    ld a,l
    cp 0E0h
    jp c,region_bounds
    ld l,0
    inc h
region_bounds:
    ld (v_start),hl
    ld de,TEST_SECTORS-1
    add hl,de
    jp c,bad_geometry
    LOAD_DE v_capacity
    call bench_cmp_hl_de
    jp nc,bad_geometry
    or a
    ret

; Вывод миллисекунд и КиБ/с с двумя знаками, без вычитания расходов.
; RATE_FACTOR округлён вниз, погрешность <0,017% для поддержанных CLK1.
print_case_name:
    ld a,(v_layout)
    add a,a
    add a,a
    add a,a
    ld e,a
    ld a,(v_case)
    add a,a
    add a,e
    ld e,a
    ld d,0
    ld hl,case_names
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    jp puts

print_result:
    call print_case_name
    ENDIF
print_result_value:
    ld hl,v_elapsed
    call print_time_ms
    ld hl,txtRate
    call puts
    call calculate_rate
    call print_rate_value
    ld hl,txtUnits
    jp puts

calculate_rate:
    ; FILE на дискете считает страницы буфера по 256 байт, а не LBA.
    xor a
    ld hl,v_dividend
    ld b,4
clear_dividend:
    ld (hl),a
    inc hl
    dec b
    jp nz,clear_dividend
    ld a,(v_technology)
    cp DISK_TECH_FLOPPY
    ld hl,(v_operations)
    jp z,rate_multiply
    ld a,(v_sectorSize+1)
    ld e,a
    ld d,0
    call bench_mul_hl_de
rate_multiply:
    push hl
    ld hl,v_dividend
    ld de,RATE_FACTOR
    call add_u16
    pop hl
    dec hl
    ld a,h
    or l
    jp nz,rate_multiply
    ld hl,(v_elapsed)
    LOAD_DE v_elapsed+2
    ld a,h
    or l
    or d
    or e
    jp z,timer_error
    ret

; Скорость в KiB/s, два знака после точки. Исходный числитель в v_dividend,
; делитель — U32 тики v_elapsed. Сначала целая часть, затем два десятичных
; деления остатка: это не раздувает цикл вычитания в 100 раз.
; Все регистры разрушены, печать целиком вне измеряемого интервала.
print_rate_value:
    call rate_quotient
    ld h,b
    ld l,c
    ld (v_number),hl
    ld hl,0
    ld (v_number+2),hl
    call print_number
    ld a,'.'
    call timing_printChar
    ld a,2
    ld (v_rateDigits),a
rate_fraction:
    ; 10*r = 8*r+2*r; r < TIMING_MAX_TICKS, результат помещается в U32.
    call rate_double
    ld hl,v_dividend
    ld de,v_number
    ld b,4
    call copy_bytes
    call rate_double
    call rate_double
    ld hl,v_number
    ld de,v_dividend
    ld b,4
    call add_bytes
    call rate_quotient
    ld a,c
    add a,'0'
    call timing_printChar
    ld hl,v_rateDigits
    dec (hl)
    jp nz,rate_fraction
    ret
rate_double:
    ld hl,v_dividend
    ld c,4
    or a
rate_double_byte:
    ld a,(hl)
    rla
    ld (hl),a
    inc hl
    dec c
    jp nz,rate_double_byte
    ret
v_rateDigits: DB 0

rate_quotient:
    ld bc,0
rate_divide:
    ; U32 сравнение от старших байтов, затем вычитание с переносом.
    ld hl,v_dividend+3
    ld de,v_elapsed+3
    ld a,(de)
    cp (hl)
    jp c,rate_subtract
    jp nz,rate_quotient_done
    dec hl
    dec de
    ld a,(de)
    cp (hl)
    jp c,rate_subtract
    jp nz,rate_quotient_done
    dec hl
    dec de
    ld a,(de)
    cp (hl)
    jp c,rate_subtract
    jp nz,rate_quotient_done
    dec hl
    dec de
    ld a,(de)
    cp (hl)
    jp c,rate_subtract
    jp nz,rate_quotient_done
rate_subtract:
    ld hl,v_dividend
    ld de,v_elapsed
    ld a,(de)
    ld (v_subByte),a
    ld a,(hl)
    push bc
    ld b,a
    ld a,(v_subByte)
    ld c,a
    ld a,b
    sub c
    ld (hl),a
    DUP 3
    inc hl
    inc de
    ld a,(de)
    ld c,a
    ld a,(hl)
    sbc a,c
    ld (hl),a
    EDUP
    pop bc
    inc bc
    jp rate_divide
rate_quotient_done:
    ret

inputBuffer:     BLOCK  17,0
callerSP:        DW     0
callerKoi:       DB     0
    INCLUDE "timer.inc"
txtTitle:        DB "Disk benchmark",00Ah,"DISKBEN / MXOS / PIT "
    IF PIT_HZ == 2000000
                 DB "2 MHz",00Ah,0
    ELSE
                 DB "2.5 MHz",00Ah,0
    ENDIF
txtDrive:        DB 00Ah,"Drive A-H [A], Q=exit: ",0
txtFlashLife:    DB 00Ah,"FLASH: writes consume limited program life.",0
txtVerify:       DB 00Ah,"Verify [0]: 0=none, 1=read back, 2=sector: ",0
txtFragment:     DB 10,"Allocation [0]: 0=sequential, 1=random: ",0
txtWrites:       DB 00Ah,"Each pass uses and removes DISKBEN.TMP.",00Ah
                 DB "Creation excluded; WRITE includes flush.",00Ah
                 DB "Passes (0-9) [1], 0=skip: ",0
txtDosLimit:        DB 10,"DOS buffer limit: 8 KiB",0
txtFileSizePrompt:  DB 00Ah,"File size in KiB (1-24) [8]: ",0
txtFileSize:        DB 00Ah,"File size (KiB):",009h,009h,0
txtClusters:        DB 00Ah,"Clusters:   ",009h,009h,009h,0
txtDash:            DB "-",0
txtError:           DB 00Ah,"I/O / format / free space error",0
txtTimerError:      DB 00Ah,"PIT clock stopped/invalid",0
txtDone:            DB 00Ah,"Press any key to exit",0
txtRate:            DB " ms",009h,0
txtUnits:           DB " KiB/s",00Ah,0
    IFDEF DISKBEN_BANK
txtCaller:       DB 00Ah,"Caller: BANK",0
    ELSE
txtCaller:       DB 00Ah,"Caller: MAIN",0
txtCallerBank:   DB 00Ah,"Caller: BANK",0
txtCallerDos:    DB 00Ah,"Caller: DOS",0
txtCallerMainDll = txtCaller
txtCallerBatch:   DB 00Ah,"Caller: MAIN / DOS / BANK",0
    ENDIF
txtSummaryDrive:    DB 00Ah,"Drive:  ",009h,009h,009h,009h,0
txtLabel:           DB 00Ah,"Label:  ",009h,009h,009h,009h,0
txtCapacity:        DB 00Ah,"Size (KiB): ",009h,009h,009h,0
txtSectorSize:      DB 00Ah,"Sector (bytes): ",009h,009h,0
txtSpc:             DB 00Ah,"Sectors/cluster:",009h,009h,0
txtType:            DB 00Ah,"Type:   ",009h,009h,009h,009h,0
txtDriver:          DB 00Ah,"Driver: ",009h,009h,009h,009h,0
txtMode:            DB 00Ah,"Write mode: ",009h,009h,009h,0
txtAligned:         DB 10,"Buffer: ",9,9,9,9,"aligned (256 bytes)",0
txtUnaligned:       DB 10,"Buffer: ",9,9,9,9,"unaligned (+1 byte)",0
txtBuffers:         DB 00Ah,"Buffers:",009h,009h,009h,009h,"Low / High direct",0
txtPasses:          DB 00Ah,"Passes: ",009h,009h,009h,009h,0
txtBlankLines:      DB 00Ah,00Ah,0
txtLineBreak = txtBlankLines+1
summaryTypes:
    DB DISK_TECH_ROM
    DW txtROM
    DB DISK_TECH_RAM
    DW txtRAM
    DB DISK_TECH_FLASH
    DW txtFlash
    DB DISK_TECH_FLOPPY
    DW txtFloppy
    DB DISK_TECH_HDD
    DW txtHDD
    DB DISK_TECH_MX_SYSTEM_FLASH
    DW txtSystemFlash
    DB DISK_TECH_MX_SYSTEM_ROM
    DW txtSystemRom
    DB DISK_TECH_MX_VV55_FLASH
    DW txtVv55Flash
    DB DISK_TECH_MX_VV55_ROM
    DW txtVv55Rom
    DB 0
    DW txtUnknown
txtROM:                 DB "ROM",0
txtRAM:                 DB "RAM",0
txtFlash:               DB "Flash",0
txtSystemFlash:         DB "MX System Flash",0
txtSystemRom:           DB "MX System ROM",0
txtVv55Flash:           DB "MX VV55 Flash",0
txtVv55Rom:             DB "MX VV55 ROM",0
txtFloppy:              DB "Floppy",0
txtHDD:                 DB "HDD",0
txtReadOnly:            DB "read only",0
txtWrite:               DB "write",0
txtWriteVerify:         DB "driver sector verify",0
txtWriteFileVerify:     DB "write + readback",0
txtUnknown:             DB "Unknown",0,0
txtSkipped:             DB "skipped (read only / 0 passes)",00Ah,0

txtFAT16:        DB "FAT16   "
txtFAT12:        DB "FAT12   "
txtRead:         DB "Read:   ",009h,009h,0
txtWriteRow:     DB "Write:  ",009h,009h,0
txtReadLow:      DB "Read low:   ",009h,0
txtReadHigh:     DB "Read high:  ",009h,0
txtWriteLow:     DB "Write low:  ",009h,0
txtWriteHigh:    DB "Write high: ",009h,0
    IFDEF DISKBEN_LEGACY_FIXTURE
txtReadSeq:      DB 00Ah,"Read seq: ",0
txtReadRnd:      DB "Read rnd: ",0
txtWriteSeq:     DB "Write seq: ",0
txtWriteRnd:     DB "Write rnd: ",0
case_names:      DW txtRead,txtRead,txtWriteRow,txtWriteRow
                 DW txtReadHigh,txtReadLow,txtWriteHigh,txtWriteLow
                 DW txtReadSeq,txtReadRnd,txtWriteSeq,txtWriteRnd
    ENDIF
v_layout:        DB LAYOUT_SIMPLE
v_drive:         DB 0
v_technology:    DB 0
v_caps:          DB 0
v_info:          DW 0
v_driverInfo:    DW 0
v_writeMode:     DB 1
v_budget:        DB 0
v_case:          DB 0
v_index:         DB 0
v_spc:           DB 0
v_capacity:      DW 0
v_sectorSize:    DW 0
v_reserved:      DW 0
v_fatSize:       DW 0
v_dataStart:     DW 0
v_start:         DW 0
v_fileKiB:       DB DEFAULT_FILE_KIB
v_fileSectors:   DB DEFAULT_FILE_KIB*4
v_bufferEndHi:   DB (sectorBuffer + DEFAULT_FILE_KIB*1024) / 256
v_operations:    DW 0
v_left:          DW 0
v_sector:        DW 0
v_elapsed:       DD 0
v_setup:         DD 0
v_last:          DW 0
v_dividend:      DD 0
v_subByte:       DB 0
code_end:
    IFNDEF sectorBuffer
sectorBuffer = BENCH_CONTROLLER_BUFFER
    ENDIF
    ASSERT code_end <= sectorBuffer
    ASSERT code_end <= batchStats
verifyBuffer = sectorBuffer
    IFNDEF fatBuffer
fatBuffer = 08200h
    ENDIF
rawSectors = fatBuffer+256
    ASSERT MAX_FILE_SECTORS*2 <= 256
    ASSERT rawSectors+MAX_FILE_SECTORS*2 <= highBuffer
lower_buffers_end = sectorBuffer + MAX_FILE_KIB*1024
    ASSERT lower_buffers_end <= fatBuffer
    IFNDEF highBuffer
highBuffer = 08400h
    ENDIF
    IFNDEF HIGH_SECTORS
HIGH_SECTORS = (LEGACY_APP_MEMORY_END-highBuffer)/256
    ENDIF
    ASSERT HIGH_SECTORS > 0
    ASSERT fatBuffer+256 <= highBuffer || highBuffer+HIGH_SECTORS*256 <= fatBuffer
program_end = highBuffer + HIGH_SECTORS*256
    IFNDEF DISKBEN_BANK
    IFDEF DISKBEN_LEGACY_FIXTURE
IMAGE_BSS_SIZE = 0
IMAGE_MEM_END = code_end
bootSectorBuffer = sectorBuffer
    ELSE
    ASSERT code_end <= BENCH_MAIN_STATS_ADDR
; Статистика MAIN живёт в контроллере, в том числе при копировании результата.
IMAGE_BSS_SIZE = BENCH_MAIN_WORK_ADDR-code_end
IMAGE_MEM_END = BENCH_MAIN_WORK_ADDR
bootSectorBuffer = BENCH_MAIN_BUFFER_ADDR
    ENDIF
    ENDIF
; EQU-псевдонимы сохраняют адреса в .sym даже при задании через -D.
diskbench_source = sectorBuffer
diskbench_high   = highBuffer
diskbench_fat    = fatBuffer
    IF highBuffer >= 0FB00h
    ; Только опыт с нижним кэшем: прежняя область FB00h–FEFFh свободна.
    ASSERT FAT_CACHE_ADDR != 0FB00h
    ASSERT program_end <= 0FF00h && fatBuffer+256 <= 0FF00h
    ASSERT lower_buffers_end <= FAT_CACHE_ADDR
    ASSERT FAT_CACHE_ADDR+FAT_CACHE_SIZE <= 08E00h
    ELSE
    ASSERT program_end <= 08FDFh - 320
    ENDIF
