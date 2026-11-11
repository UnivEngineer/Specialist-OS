; Тест MXOS: физическое чтение и создание временного файла через DOS.
; ВИ53: CLK1 / 256 -> CLK2, GATE1/2=1, без прерываний.
; Запись включает выделение кластеров, FAT, каталог и штатные барьеры DOS.
    INCLUDE "../../source/include/mxos.inc"
    IFDEF DISKBEN_BANK
IMAGE_FLAGS = IMAGE_FLAG_BANK
    ELSE
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ENDIF
    IFNDEF PIT_HZ
PIT_HZ = 2000000
    ENDIF
    ASSERT PIT_HZ == 2000000 || PIT_HZ == 2500000
RATE_FACTOR = PIT_HZ / 1024
TEST_SECTORS = 32
DEFAULT_FILE_KIB = 8
    IFNDEF MAX_FILE_KIB
MAX_FILE_KIB = 24
    ENDIF
MAX_FILE_SECTORS = MAX_FILE_KIB * 4
LAYOUT_SIMPLE = 0
LAYOUT_MAPPED = 1
LAYOUT_MECHANICAL = 2
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
entry:
    di
    IFNDEF DISKBEN_BANK
    STORE_DE v_arguments
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
    IFNDEF DISKBEN_BANK
    call choose_executor
    ENDIF
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
    MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
    jp c,error
    ld b,3
    call raw_driver
    ld (v_info),hl
    ld a,d
    or e
    jp z,error
    STORE_DE v_capacity
    call detect_driver
    ld de,0
    ld hl,sectorBuffer
    ld b,2
    call raw_driver
    jp c,error
    call geometry
    jp c,error
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
    cp 'F'
    jp nz,choose_verify
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
    ; Текущий кластер = сектор = 256 байт: КиБ * 4 кластеров.
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
    MXOS_CALL MXOS_BIOS_ARG_NEXT
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
    MXOS_CALL MXOS_BIOS_ARG_NEXT
    jp nc,bad_geometry        ; Второе слово не является частью размера.
    ld a,c
    or a
    ret nz
    scf
    ret
prepare:
    IFNDEF DISKBEN_BANK
    ld a,(argBank)
    or a
    jp nz,run_bank_module
    ENDIF
    call select_layout
    ; Для физического READ не требуется свободный непрерывный участок.
    ld a,(v_budget)
    push af
    xor a
    ld (v_budget),a
    call find_region
    pop bc
    ld a,b
    ld (v_budget),a
    jp c,error
    call print_summary
    ld a,(v_budget)
    or a
    jp nz,prepare_timer
    ld hl,txtRawAccess
    call puts
    ld hl,txtRegion
    call puts
    ld hl,(v_start)
    call bench_printHexWord
    ld hl,txtDash
    call puts
    ld hl,(v_start)
    ld de,TEST_SECTORS-1
    add hl,de
    call bench_printHexWord
    ld hl,txtBlankLines
    call puts
prepare_timer:
    call timer_init
    call timer_probe
    jp z,timer_error
run_tests:
    ld a,(v_budget)
    or a
    ld a,2
    ld (v_case),a
    jp nz,file_write_case
    ; Интерфейс и поиск свободного места не входят в измерение.
    xor a
    ld (v_case),a
case_loop:
    ld hl,sectorBuffer
    ld a,(v_layout)
    cp LAYOUT_MAPPED
    jp nz,case_buffer_ready
    ld a,(v_case)
    and 1
    jp nz,case_buffer_ready
    ld hl,highBuffer
case_buffer_ready:
    ld (io_buffer+1),hl
    ld a,(v_case)
    cp 2
    ld a,32
    jp c,case_passes
    ld a,(v_budget)
case_passes:
    or a
    jp z,case_skipped
    ld l,a
    ld h,0
    ld de,TEST_SECTORS
    call bench_mul_hl_de
    ld (v_operations),hl
    ld (v_left),hl
    ld a,(v_case)
    cp 2
    jp c,case_pattern_ready
    jp file_write_case
case_pattern_ready:
    ld hl,raw_local
    ld (io_call+1),hl
    xor a
    ld (v_index),a
    LOAD_DE v_start
    call raw_warmup
    jp c,error
    call reset_timing
operation_loop:
    ld a,(v_layout)
    cp LAYOUT_MECHANICAL
    jp nz,sequential
    ld a,(v_case)
    and 1
    ld a,(v_index)
    jp z,sequential
    ; LCG modulo 32, полный период: каждый сектор ровно один раз за проход.
    ld b,a
    add a,a
    add a,b
    add a,a
    add a,a
    add a,b
    add a,7
    jp index_ready
sequential:
    ld a,(v_index)
    inc a
index_ready:
    and TEST_SECTORS-1
    ld (v_index),a
    ld e,a
    ld d,0
    ld hl,(v_start)
    add hl,de
    ex de,hl
    STORE_DE v_sector
    ld b,2
operation_ready:
    call timer_read
    ld (v_last),hl
io_buffer:
    ld hl,sectorBuffer
io_call:
    call raw_local
    push af
    call timer_sample
    pop af
    jp c,error
operation_done:
    ld hl,(v_left)
    dec hl
    ld (v_left),hl
    ld a,h
    or l
    jp nz,operation_loop
    call print_result
    jp next_case
case_skipped:
    call print_case_name
    ld hl,txtSkipped
    call puts
next_case:
    ; CF=1 для простого плана: переход 0->2->4 вместо 0->1->2->3->4.
    ld a,(v_layout)
    cp 1
    ld a,(v_case)
    adc a,1
    ld (v_case),a
    cp 4
    jp nz,case_loop
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

; Выбор строк теста по технологии. Механика пока только зарезервирована
; в B=FFh: физических драйверов Floppy/HDD здесь нет. A: ROM/Flash и сектор
; 256 байт используют два буфера; прочие RAM/ROM/Flash — один, без RND.
; Вход — переменные геометрии/драйвера. AF/HL/флаги разрушаются.
select_layout:
    xor a
    ld (v_layout),a
    IFDEF DISKBEN_BANK
    ret
    ENDIF
    ld a,(v_technology)
    cp DISK_TECH_FLOPPY
    jp z,layout_mechanical
    cp DISK_TECH_HDD
    jp z,layout_mechanical
    cp DISK_TECH_ROM
    jp z,layout_mapped_check
    cp DISK_TECH_FLASH
    ret nz
layout_mapped_check:
    ld a,(v_drive)
    or a
    ret nz
    ld hl,(v_sectorSize)
    dec h
    ld a,h
    or l
    ret nz
    ld a,LAYOUT_MAPPED
    jp layout_store
layout_mechanical:
    ld a,LAYOUT_MECHANICAL
layout_store:
    ld (v_layout),a
    ret

; Строковый ввод BIOS с редактированием. Enter полностью потребляется здесь.
; Выход HL=буфер с нулём, остальные регистры и флаги разрушаются.
; Выбор нужен только при отсутствии -C:... или совместимого старого ключа.
choose_executor:
    ld a,(argMode)
    or a
    jp nz,executorModeReady
    ld a,3
    ld (argMode),a
executorModeReady:
    ld a,(argBank)
    or a
    ret nz
menu_executor:
    ld hl,txtExecutor
    call puts
    call read_line
    MXOS_CALL MXOS_BIOS_ARG_NEXT
    ld c,3                     ; Enter: MAIN DLL.
    jp c,argument_bank
    push hl
    ex de,hl
    call argument_context
    pop hl
    jp c,menu_executor
    MXOS_CALL MXOS_BIOS_ARG_NEXT
    ret c
    xor a
    ld (argBank),a
    jp menu_executor

txtExecutor: DB 10,"Caller MAIN/DOS/BANK/ALL [MAIN]: ",0
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
    MXOS_CALL MXOS_BIOS_ARG_NEXT
    jp nc,choice_word
    xor a
    ret
choice_word:
    ld a,b
    dec a
    jp nz,choice_bad
    ld a,(de)
    ld c,a
    MXOS_CALL MXOS_BIOS_ARG_NEXT
    jp nc,choice_bad
    ld a,c
    or a
    ret
choice_bad:
    scf
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

; Сводка отделена пустыми строками; поиск участка и печать вне измерения.
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
    ld hl,txtROM
    cp 'O'
    jp z,summary_type
    ld hl,txtRAM
    cp 'R'
    jp z,summary_type
    ld hl,txtFlash
    cp 'F'
    jp z,summary_type
    ld hl,txtFloppy
    cp DISK_TECH_FLOPPY
    jp z,summary_type
    ld hl,txtHDD
    cp DISK_TECH_HDD
    jp z,summary_type
    ld hl,txtUnknown
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
    call bench_div_hl_2
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
    ld a,(v_fileSectors)
    ld e,a
    ld d,0
    call bench_printDecWord
summary_end:
    ld hl,txtLineBreak
    jp puts

; BPB -> геометрия физического ABI. AF/BC/DE/HL/флаги разрушаются.
; Поддерживаются секторы 256..4096, один/два FAT, корень с округлением вверх.
; При неизвестном формате нет доступа на запись. FAT12/FAT32 не вводятся.
geometry:
    ld hl,sectorBuffer+036h
    ld de,txtFAT16
    ld b,8
geometry_signature:
    ld a,(de)
    cp (hl)
    jp nz,bad_geometry
    inc hl
    inc de
    dec b
    jp nz,geometry_signature
    ld hl,(sectorBuffer+00Bh)
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
    ld a,(sectorBuffer+00Dh)
    or a
    jp z,bad_geometry
    ld (v_spc),a
    dec a
    ld b,a
    inc a
    and b
    jp nz,bad_geometry
    ld hl,(sectorBuffer+00Eh)
    ld a,h
    or l
    jp z,bad_geometry
    ld (v_reserved),hl
    ld hl,(sectorBuffer+016h)
    ld a,h
    and 080h
    jp nz,bad_geometry
    ld a,h
    or l
    jp z,bad_geometry
    ld (v_fatSize),hl
    ld a,(sectorBuffer+010h)
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
    ld hl,(sectorBuffer+011h)
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
    ld hl,(sectorBuffer+013h)
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
    ; dividend = operations * (sectorSize/256) * floor(CLK1/1024).
    xor a
    ld hl,v_dividend
    ld b,4
clear_dividend:
    ld (hl),a
    inc hl
    dec b
    jp nz,clear_dividend
    ld hl,(v_sectorSize)
    ld e,h
    ld d,0
    ld hl,(v_operations)
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

inputBuffer:     BLOCK 17,0
callerSP:        DW 0
callerKoi:       DB 0
    INCLUDE "timer.inc"
txtTitle:        DB "Disk benchmark",00Ah,"DISKBEN / MXOS / PIT "
    IF PIT_HZ == 2000000
                 DB "2 MHz",00Ah,0
    ELSE
                 DB "2.5 MHz",00Ah,0
    ENDIF
txtDrive:        DB 00Ah,"Drive A-H [A], Q=exit: ",0
txtFlashLife:    DB 00Ah,"FLASH: writes consume limited program life.",0
txtVerify:       DB 00Ah,"Check [0]: 0=plain, 1=file, 2=sector verify: ",0
txtFragment:     DB 10,"Allocation [0]: 0=sequential, 1=random: ",0
txtWrites:       DB 00Ah,"Each pass uses and removes DISKBEN.TMP.",00Ah
                 DB "Creation excluded; WRITE includes flush.",00Ah
                 DB "Passes (0-9) [1], 0=skip: ",0
txtDosLimit: DB 10,"DOS buffer limit: 8 KiB",0
txtFileSizePrompt: DB 00Ah,"File size in KiB (1-24) [8]: ",0
txtFileSize:     DB 00Ah,"File size (KiB):",009h,009h,0
txtClusters:     DB 00Ah,"Clusters:   ",009h,009h,009h,0
txtRegion:       DB 00Ah,"Sectors (hex): ",0
txtDash:         DB "-",0
txtError:        DB 00Ah,"I/O / format / free space error",0
txtTimerError:   DB 00Ah,"PIT clock stopped / invalid",0
txtDone:         DB 00Ah,"Press any key to exit",0
txtRate:         DB " ms",009h,0
txtUnits:        DB " KiB/s",00Ah,0
    IFDEF DISKBEN_BANK
txtCaller:       DB 00Ah,"Caller: BANK",0
    ELSE
txtCaller:       DB 00Ah,"Caller: MAIN",0
txtCallerBank:   DB 00Ah,"Caller: BANK",0
txtCallerDos:    DB 00Ah,"Caller: DOS",0
txtCallerMainDll = txtCaller
txtCallerBatch:   DB 00Ah,"Caller: MAIN / DOS / BANK",0
    ENDIF
txtSummaryDrive: DB 00Ah,"Drive:  ",009h,009h,009h,009h,0
txtLabel:        DB 00Ah,"Label:  ",009h,009h,009h,009h,0
txtCapacity:    DB 00Ah,"Size (KiB): ",009h,009h,009h,0
txtSectorSize:  DB 00Ah,"Sector (bytes): ",009h,009h,0
txtSpc:         DB 00Ah,"Sectors/cluster:",009h,009h,0
txtType:         DB 00Ah,"Type:   ",009h,009h,009h,009h,0
txtDriver:       DB 00Ah,"Driver: ",009h,009h,009h,009h,0
txtMode:         DB 00Ah,"Test mode:  ",009h,009h,009h,0
txtBuffers:      DB 00Ah,"Buffers:",009h,009h,009h,009h,"Low / High direct",0
txtPasses:       DB 00Ah,"Passes: ",009h,009h,009h,009h,0
txtLineBreak:    DB 00Ah,0
txtBlankLines:   DB 00Ah,00Ah,0
txtROM:          DB "ROM",0
txtRAM:          DB "RAM",0
txtFlash:        DB "Flash",0
txtFloppy:       DB "Floppy",0
txtHDD:          DB "HDD",0
txtReadOnly:     DB "read only",0
txtWrite:        DB "write",0
txtWriteVerify:  DB "driver sector verify",0
txtWriteFileVerify: DB "write + file verify",0
txtUnknown:      DB "Unknown",0,0
txtSkipped:      DB "skipped (read only / 0 passes)",00Ah,0

txtFAT16:        DB "FAT16   "
txtRead:         DB "Read:   ",009h,009h,0
txtWriteRow:     DB "Write:  ",009h,009h,0
txtReadLow:      DB "Read low:   ",009h,0
txtReadHigh:     DB "Read high:  ",009h,0
txtWriteLow:     DB "Write low:  ",009h,0
txtWriteHigh:    DB "Write high: ",009h,0
txtReadSeq:      DB 00Ah,"Read seq: ",0
txtReadRnd:      DB "Read rnd: ",0
txtWriteSeq:     DB "Write seq: ",0
txtWriteRnd:     DB "Write rnd: ",0
case_names:      DW txtRead,txtRead,txtWriteRow,txtWriteRow
                 DW txtReadHigh,txtReadLow,txtWriteHigh,txtWriteLow
                 DW txtReadSeq,txtReadRnd,txtWriteSeq,txtWriteRnd
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
sectorBuffer = 02000h
    ENDIF
    ASSERT code_end <= sectorBuffer
    ASSERT batchStats == sectorBuffer
verifyBuffer = sectorBuffer
    IFNDEF fatBuffer
fatBuffer = 08000h
    ENDIF
rawSectors = fatBuffer+256
    ASSERT MAX_FILE_SECTORS*2 <= 256
    ASSERT rawSectors+MAX_FILE_SECTORS*2 <= highBuffer
lower_buffers_end = sectorBuffer + MAX_FILE_KIB*1024
    ASSERT lower_buffers_end <= fatBuffer
    IFNDEF highBuffer
highBuffer = 08200h
    ENDIF
    IFNDEF HIGH_SECTORS
HIGH_SECTORS = (LEGACY_APP_MEMORY_END-highBuffer)/256
    ENDIF
    ASSERT HIGH_SECTORS > 0
    ASSERT fatBuffer+256 <= highBuffer || highBuffer+HIGH_SECTORS*256 <= fatBuffer
program_end = highBuffer + HIGH_SECTORS*256
; EQU-псевдонимы сохраняют адреса в .sym даже при задании через -D.
diskbench_source = sectorBuffer
diskbench_high = highBuffer
diskbench_fat = fatBuffer
    IF highBuffer >= 0FB00h
    ; Только опыт с нижним кэшем: прежняя область FB00h–FEFFh свободна.
    ASSERT FAT_CACHE_ADDR != 0FB00h
    ASSERT program_end <= 0FF00h && fatBuffer+256 <= 0FF00h
    ASSERT lower_buffers_end <= FAT_CACHE_ADDR
    ASSERT FAT_CACHE_ADDR+FAT_CACHE_SIZE <= 08E00h
    ELSE
    ASSERT program_end <= 08FDFh - 320
    ENDIF
