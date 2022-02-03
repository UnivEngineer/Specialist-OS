Операционная система для ПК "Специалист-MX/MX2"

Основана на двух существующих для "Специалиста-MX" ОС - Commander 1.4 (MXOS) и Ramfos, дизассемблированных и доработанных vinxru.
Идея в том, чтобы сделать гибридную ОС, позволяющую запускать программы как от Ramfos (коих много) и от MXOS (коих чуть-чуть),
так и классические по механизму MX2 (программное переключение оборудования в стандартный режим). Но в основе архитектуры будет MXOS
как более продвинутая.

---------------- Как собрать -------------------

Открыть проект в Visual Studio, нажать Build. Используется кросс-ассемблер SjASMPlus.

--------- Как запустить в эмуляторе ------------

В эмуляторе emu - нажать в Visual Studio "Run". Запустится файл Release\run.bat, который сначала создаст образ ПЗУ с помощью
js скрипа из проекта vinxru (ROM\MY_MXOS.bin), затем скопирует его в папку эмулятора, и запустит его. В run.bat надо прописать
путь к эмулятору. Конфиг для эмулятора лежит рядом с run.bat, его надо скопировать в папку emu\config.

------- Как запустить на реальном железе -------

Необходим Специалист-MX или Специалист-MX2 или совместимый клон (Сябр, ...), и программатор ПЗУ. Образ ПЗУ собирается при
запуске Release\run.bat или ROM\makeRom\-makeRom.js и лежит в папке ROM\MY_MXOS.bin, а файлы, из которого он собирается -
в папке ROM\makeRom. Образ рассчитан на ПЗУ объёмом 64 кб.

В папке FlashDrive - набор образов для прошивания Флеш-диска, и скрипт для сборки образов из файлов. Приложена коллекция игр
для Специалиста почти на 2 Мб (проверены не все!).

----------- Исправления/улучшения --------------

----2020.01.28----

DOS.SYS
- Цвет, с которым запускаются ч/б программы, изменён на белый (F0h).
- Исправлена функция fileLoad2 - теперь она корректно работает с ROM диском.

NC.COM
- Цвет, с которым запускаются ч/б программы, изменён на белый (F0h).
- При печати списка файлов в панели игнорируются управляющие коды, чтобы не портить экран при открытии не отформатированного диска.
- При теплой перезагрузке вместо A:FORMAT.COM B: запускается A:FORMAT.BAT, содержащий команды форматирования всех RAM-дисков.
- Убрана печать лишних переводов строк при выполнении команд из BAT файла.
- Исправлен English.
- Исправлен механизм обработки файла NC.EXT. Раньше его приходилось копировать на каждый накопитель, чтобы работало, теперь он нужен только на A:.
- Реализован запуск RKS файлов в STD режиме Специалиста-MX2. Файл Monitor2.sys взят из доработанного Ramfos от vinxru.

FORMAT.COM
- Добавлена опция Y для принудительного форматирования указанного диска без запроса подтверждения (FORMAT.COM B: Y).

FLASH.COM (New)
- Драйвер флеш-диска HardwareMan'a с поддержкой записи.
- Форматирование страниц флеш-диска с помощью FORMAT.COM работает "из коробки".

FPAGE.COM (New)
- Переключение страниц флеш-диска. MXOS имеет максимальный объём накопителя 64 кб, тогда как флеш-диск может быть до 2 Мб (32 страницы).

LAUNCHER.COM (New)
- Запуск RKS файлов STD режиме Специалиста-MX2. Монитор-2 взят из Ramfos Vinxru.

----2020.02.03----

- Проект переведён в мнемоники Z80, собирается в SjASMPlus под i8080/КР580ВМ80А (опция --i8080)
- Сделан общий include файл с настройками, стандартными точками входа и переменными.
- Начат переход на FAT12/16. Реализованы дескрипторы файлов FAT12/16 (32 байта), имена файлов теперь в формате 8.3. Пока есть ограничение - максимум 24 файла на накопителе.
- Пока ещё есть опции компиляции в старый формат дескрипторов (16 байт, имена 6.3), и соотв. опции сборки образов ПЗУ и флеш-диска.
- Часть функционала перенесена из NC.COM в DOS.SYS, добавлены новые "стандартные" точки входа.
- В DOS.SYS теперь два драйвера обмена с ДОЗУ: стандартный блочный (по 256 байт) и побайтовый. Второй перенесен из NC.COM, но сделан совместимым с Ramfos. Добавлена п/п переключения драйверов.
- Определение объема ДОЗУ и вывод на экран (взято из Ramfos Vinxru).
- Однократный запуск A:FORMAT.BAT перенесен в DOS.SYS.
- Загрузка шрифта (FONT.FNT) из файла, а не из хвоста DOS.SYS.
- Подпрограммы работы с магнитофоном вынесены из DOS.SYS во внешний драйвер (TAPE.COM), чтобы освободить место. Стандартные точки входа сохранены, но ведут на теплую перезагрузку.

TAPE.COM (NEW)
- Драйвер магнитофона. Может быть запущен вручную или из AUTOEX.BAT. После запуска патчит DOS.SYS в памяти, чтобы стандартные точки входа п/п магнитофона вели на него.
