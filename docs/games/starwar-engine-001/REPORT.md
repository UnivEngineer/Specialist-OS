# Семейство 3D-движка StarWar: поиск по машинному коду

Дата: 01.11.2026. Исходные игры, образы и архивы не изменялись.

## Результат

Найдены 6 различных нагрузок игр для «Специалиста» (47 файлов с учётом контейнеров, копий и извлечённых архивных членов), а также родственный порт `PATROL3D.BIN` для «Ориона». Других самостоятельных игр с подтверждённым общим ядром в обследованном корпусе не найдено. Дополнительно 134 файла образов/ROM содержат фрагменты этого семейства; они не считаются самостоятельными новыми играми.

Полный перечень всех 182 совпавших файлов, исходных архивов, SHA-256 и смещений сигнатур: [all-files.csv](all-files.csv). Смещения в CSV относятся к началу файла, в десятичной записи; у обычного RKS нагрузка начинается с +4.

## Подтверждённые варианты

| Группа | Размер нагрузки | Копий | Основные имена |
|---|---:|---:|---|
| W1, вариант для MXOS | 5856 | 8 | `StarW.exe`, `STARWARS_MY.RKS`, `STARWAR.rks`, `STARW.0.EXE`; также `STARWARS.RKS` в RAMFOS Games-Std |
| W1, вариант Monitor | 5888 | 9 | `StarWar2.rks`, `STARWAR2.RKS`, `starwar2.rks` |
| W2, базовый | 9216 | 15 | `StarW2.exe`, `StarW2.rks`, `STAR_W2.RKS`, `STAR-W2.RKS`, `STAR_W2.I80`, `star_w2.i80`, `STARW2.0.EXE` |
| W2, вариант MX | 9353 | 11 | `StarW2X.exe`, `Starwar..EXE`, `Star-W2.0.EXE`, `StarW2.0.EXE`, `Starwar.i80` |
| W2, лента с заголовком | 9216 | 2 | `Soft/games_rks/12.rks` и `Games/rks_games.rar :: 12.rks` |
| W2, архив Germanovitsh | 9216 | 2 | `Soft/germanovitsh.rar :: Games/STAR WAR 0_23FF_B8A2.rks` и `STAR WAR-2 0_23FF_B8A2.rks` |

Количество копий включает файлы в проекте и временно извлечённые архивные члены. Группировка выполнена по SHA-256 нагрузки после отделения контейнера, не только по названию.

`12.rks` начинается с `D9 D9 D9` и имени `STAR WARS`; нагрузка находится с файлового смещения `030Eh` (782), а не +4. В сравнении с базовым W2 различаются 12 из 9216 байтов. Два файла Germanovitsh побайтно одинаковы между собой; с базовым W2 различаются 428 байтов.

## Доказательства общего ядра

Проверены дизассемблированные процедуры 8080. Для поиска переносимых вариантов маскируются только два байта адреса у трёхбайтовых инструкций (CALL/JMP, загрузки пар, обращения к памяти); коды инструкций и восьмибитные константы сохраняются.

| Фрагмент | W1: адрес | W2: адрес | Длина сигнатуры | Назначение |
|---|---|---|---:|---|
| W1-transform | `0697h` | отдельная версия `0C36h` | 70 / 72 | Обход XYZ вершин: три вклада в каждую из двух выходных координат |
| W1-matrix | `074Ah` | `0CEBh` | 82 | Подготовка коэффициентов поворота; одинаковая последовательность после маскирования адресов |
| W1-line | `086Fh` | изменена | 41 | Растеризация отрезка с последовательным делением пополам |

У W1 и базового W2 также есть 1265 байтов точных совпадений блоками не короче 32 байт; в это число входят код и данные. Само по себе это число не используется как доказательство общего движка: решающими являются процедуры матрицы и преобразования вершин.

Дизассемблирование: [starw.dis](starw.dis), [starw2.dis](starw2.dis). Сигнатуры сохранены в `*.signature.txt`; поиски воспроизводятся `probe.py` и `short-probe.py`. JSON содержит машинно читаемые смещения.

## PATROL3D для Ориона

Файл: `D:/Projects/RetroPC/Specialist/Emulator/emu/Orion/PATROL3D.BIN`, 46384 байта, SHA-256 `6787e310a7a879d35de113c9b5896f5788eeb5d9d8e0fac84212f7369e55fc2c`.

70-байтовая W1-transform найдена с файлового смещения `1AEBh`; отличается адресация данных и вызовов. Дополнительно короткие 16-инструкционные сигнатуры обнаружили общие участки матрицы поворота, относительных координат, насыщения и умножения. Всего совпали 20 из 39 перекрывающихся поисковых фрагментов; это число сигнатур, не независимых процедур. Переменные перенесены в `B000h`, графический вывод адаптирован. Это родственная реализация ядра, а не побайтный дубль всей игры. Адреса в `patrol-fragment.dis` — файловые смещения: файл содержит загрузчик/другие блоки, поэтому их нельзя автоматически считать runtime-адресами.

## Исключённые и неполные совпадения

- `assets/games/StarWars.rks`: нагрузка побайтно совпадает с `assets/games/Buran.exe` (5790 байт). Это исключённая статическая игра; в архиве есть другие одноимённые `STARWARS.RKS`, относящиеся к W1. Название без пути ненадёжно.
- `Buran.rks`, `17.rks`, архивный `Star War … 0_16D8_1627.rks`: семейство статического «Бурана», целевые сигнатуры 3D-ядра не найдены.
- `Soft/games_rks/36.rks` (текстовый заголовок `podwodnaq lodka`): совпали 255 байтов начала W2, включая непрерывные 188 байтов. Матрица и преобразование вершин отсутствуют. Контейнер заявляет другую короткую нагрузку, а код после общего начала отличается. Это частичное/подозрительное совпадение, не подтверждённая ещё одна игра на полном ядре.
- Общие короткие процедуры монитора, печати, тестов памяти и таблицы в других играх не считаются общим 3D-ядром.

## Охват и ограничения

Проверено 5477 файлов, 562462495 байт: `assets/games` и дерево `D:/Projects/RetroPC/Specialist`, плюс извлечённые члены 143 уникальных ZIP/RAR/7z архивов из Soft, Games, Systems и MySoft/RAMFOS. Все эти архивы извлечены без ошибки. Учтены `.rks`, `.exe`, `.com`, `.gam`, `.rom`, `.bin`, `.odi`, `.img`, `.fdd`, `.rkm`, `.rk`, `.i80`, `.tap`; ограничение размера снято. Исключены каталоги с компонентами пути `Tools`, `z88dk`, `emu80-build`, `.git`.

Архивные члены сопоставляются с исходным архивом через `archives.json`; образы просматривались как байтовые потоки. Произвольно упакованные, зашифрованные либо сильно переписанные реализации без совпадения выбранных фрагментов поиск не исключает. Исполнение игр и совместимость с MXOS в этой задаче не проверялись. Сборка ОС не требовалась: asm/inc, бинарники и состав образов не менялись.

## Все самостоятельные файлы по группам

### Нагрузка 5856 байт; SHA-256 `a8ac2914135e32f47aea3c2408565974dceac8d51a3c615515c93dc911d21bab`

- `assets\games\StarW.exe`
- `D:\Projects\RetroPC\Specialist\Soft\games_my\STARWARS_MY.RKS`
- `D:\Projects\RetroPC\Specialist\Soft\games_my_test\STARWAR.rks`
- `D:\Projects\RetroPC\Specialist\OS\RAMFOS-master\ROM-disabled\Games-Std\STARWARS.RKS`
- `D:\Projects\RetroPC\Specialist\OS\MXOS-master\ROM-disabled\games-std\STARW.0.EXE`
- `D:\Projects\RetroPC\Specialist\MySoft\RAMFOS\ROM\disabled\Games-Std\STARWARS.RKS`
- `D:\Projects\RetroPC\Specialist\MySoft\MXOS-Z80\ROM\disabled\games-std\STARW.0.EXE`
- `D:\Projects\RetroPC\Specialist\MySoft\MXOS\ROM\disabled\games-std\STARW.0.EXE`

### Нагрузка 9216 байт; SHA-256 `de9287ea25309a25bcc796246d55c1938729823ed76f4f977b9a742b78af2796`

- `assets\games\StarW2.exe`
- `assets\games\StarW2.rks`
- `assets\games\originals\rks-checksums\StarW2.rks.bin`
- `D:\Projects\RetroPC\Specialist\Soft\games_mx_i80\STAR_W2.I80`
- `D:\Projects\RetroPC\Specialist\Soft\games_my_test\STAR_W2.RKS`
- `D:\Projects\RetroPC\Specialist\OS\RAMFOS-master\ROM-disabled\Games-Std\STAR-W2.RKS`
- `D:\Projects\RetroPC\Specialist\OS\MXOS-master\makeRom\STARW2.0.EXE`
- `D:\Projects\RetroPC\Specialist\OS\MXOS-master\ROM-disabled\games-std\STARW2.0.EXE`
- `D:\Projects\RetroPC\Specialist\MySoft\RAMFOS\ROM\disabled\Games-Std\STAR-W2.RKS`
- `D:\Projects\RetroPC\Specialist\MySoft\MXOS-Z80\ROM\disabled\games-std\STARW2.0.EXE`
- `D:\Projects\RetroPC\Specialist\MySoft\MXOS-Z80\FlashDrive\Games\StarW2.rks`
- `D:\Projects\RetroPC\Specialist\MySoft\MXOS\ROM\makeRom\STARW2.0.EXE`
- `D:\Projects\RetroPC\Specialist\MySoft\MXOS\ROM\disabled\games-std\STARW2.0.EXE`
- `D:\Projects\RetroPC\Specialist\specialist-mx.org\Games\games_mx_i80.rar :: STAR_W2.I80`
- `D:\Projects\RetroPC\Specialist\specialist-mx.org\Soft\SPMX_java.rar :: SPMX_java\files_cpu\star_w2.i80`

### Нагрузка 9353 байт; SHA-256 `b7dd022fc87f3280a7f54ea1a4d0fc87916d4f2d5cb91a80f8afa6c518f4cd5c`

- `assets\games\StarW2X.exe`
- `D:\Projects\RetroPC\Specialist\Soft\Lafan\LafanG2\GameMX3\Starwar..EXE`
- `D:\Projects\RetroPC\Specialist\Soft\Lafan\bst_mx0\Games\Gam0\Starwar..EXE`
- `D:\Projects\RetroPC\Specialist\OS\RAMFOS-master\RomDisk\Star-W2.0.EXE`
- `D:\Projects\RetroPC\Specialist\OS\RAMFOS-master\ROM-disabled\Games-MX\Star-W2.0.EXE`
- `D:\Projects\RetroPC\Specialist\OS\MXOS-master\ROM-disabled\games-mx\StarW2.0.EXE`
- `D:\Projects\RetroPC\Specialist\MySoft\RAMFOS\ROM\makeROM\Star-W2.0.EXE`
- `D:\Projects\RetroPC\Specialist\MySoft\RAMFOS\ROM\disabled\Games-MX\Star-W2.0.EXE`
- `D:\Projects\RetroPC\Specialist\MySoft\MXOS-Z80\ROM\disabled\games-mx\StarW2.0.EXE`
- `D:\Projects\RetroPC\Specialist\MySoft\MXOS\ROM\disabled\games-mx\StarW2.0.EXE`
- `D:\Projects\RetroPC\Specialist\specialist-mx.org\Soft\SPMX_java.rar :: SPMX_java\files_cpu\Starwar.i80`

### Нагрузка 5888 байт; SHA-256 `593793b8f9a43ba3ae55fb497b1f172c9ba615ad28d20d5a948d69652dbec832`

- `assets\games\StarWar2.rks`
- `D:\Projects\RetroPC\Specialist\Soft\games1_rks\STARWAR2.RKS`
- `D:\Projects\RetroPC\Specialist\Soft\games_my\STARWAR2.RKS`
- `D:\Projects\RetroPC\Specialist\MySoft\MXOS-Z80\FlashDrive\Games\StarWar2.rks`
- `D:\Projects\RetroPC\Specialist\Emulator\emu80-src\dist\spec\sdcard\GAMES\starwar2.rks`
- `D:\Projects\RetroPC\Specialist\Emulator\Emu80-MXOS-flash\spec\sdcard\GAMES\starwar2.rks`
- `D:\Projects\RetroPC\Specialist\Emulator\Emu80-MXOS\Emu80qt_40571\spec\sdcard\GAMES\starwar2.rks`
- `D:\Projects\RetroPC\Specialist\Emulator\Emu80-flash-test\spec\sdcard\GAMES\starwar2.rks`
- `D:\Projects\RetroPC\Specialist\specialist-mx.org\Games\games1_rks.rar :: STARWAR2.RKS`

### Нагрузка 9216 байт; SHA-256 `98959fdba5ab812c245701418785f7e74b6da8d31f2e6b009cce2a2232082b73`

- `D:\Projects\RetroPC\Specialist\Soft\games_rks\12.rks`
- `D:\Projects\RetroPC\Specialist\specialist-mx.org\Games\rks_games.rar :: 12.rks`

### Нагрузка 9216 байт; SHA-256 `fb3cf520959842c98084963749555e0f1e9ec49771f67467e176da7ebc4db8b1`

- `D:\Projects\RetroPC\Specialist\specialist-mx.org\Soft\germanovitsh.rar :: Games\STAR WAR 0_23FF_B8A2.rks`
- `D:\Projects\RetroPC\Specialist\specialist-mx.org\Soft\germanovitsh.rar :: Games\STAR WAR-2 0_23FF_B8A2.rks`
