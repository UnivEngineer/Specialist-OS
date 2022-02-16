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

DOS.SYS:
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

----2020.02.07 ----

DOS.SYS
- В предыдущем апдейте потеряна совместимость с программами, обрабатывающими дескрипторы каталога и имя файла в старом формате.
- Продолжение перехода на FAT12/16:
--- реализованы 16-битные номера кластеров;
--- увеличена таблица FAT и корневой каталог (сейчас максимум 512 кб на разделе и 32 файла в каталоге);
--- изменены стандартные п/п драйвера диска (потеряна совместимость с программами, использующими низкоуровневые вызовы).

NC.COM
- Переработана инфопанель, теперь на нее выводится объем основной памяти и ДОЗУ

E.COM
- Дизассемблирован, добавлен в проект.

MON2.COM
- Дизассемблирован, добавлен в проект.

----2020.02.16 ----

- Убрана возможность вернуться на FAT8, слишком много накопилось изменений.
- Перерисованы некоторые символы в шрифте FONT.FNT, чтобы сделать его обычным, а не italic, и добавить недостающие символы по стандарту КОИ-8.
- Изменено распределение памяти, чтобы освободить место для разрастающегося DOS.SYS:
--- системные программы (NC.COM, E.COM, DL-red.COM) переехали на E800h (совместимо с Ramfos);
--- шрифт переехал на DE00h;
--- драйверы флеш/ром диска переехали на DE00 (512 байт), драйвер магнитофона на DC00 (512 байт).
- Новые стандартные подпрограммы (точки перехода C800h):
--- сравнение строк (чувствительное и нечувствительное к регистру КОИ-8);
--- перевод кода символа в верхний  регистр (КОИ-8);
--- 16-битная арифметика (вычитание, умножение, деление hl и de).
- Скрипт makeRom.js унифицирован:
--- создаёт образы ПЗУ для Специалиста-MX, MX2 (системные), и флеш-диска (разбитый на отдельные файлы для прошивания каждой из микросхем);
--- формат "почти FAT16", с загрузочным сектором (пока ещё есть небольшие отличия от спецификации);
--- в загрузочном секторе системной ПЗУ находится программа-загрузчик (копирует DOS.SYS в ОЗУ);
--- все параметры настраиваются несколькими строчками в начале скрипта (тип образа, размер ПЗУ, размер каталога, пути к файлам).

DOS.SYS
- Режим КОИ-8 по умолчанию. Имена файлов в КОИ-8. Ввод строчных английских букв по умолчанию. Двухтональный звуковой сигнал при переключении Caps Lock.
- Файловые операции не чувствительны к регистру (КОИ-8).
- Файл AUTOEX.BAT переименован в autoexec.bat.
- Буфер fat и каталога в памяти заменен на кэш секторов активного устройства. Объём кэша можно настраивать. По умолчанию - как и было, 1 кб, FB00-FEFF.
- Поддержка дисков с разным размером fat и корневого каталога. Пока ещё сектор = кластер = 256 байт.
- Реализован загрузочный сектор FAT16, откуда читается информация о размерах fat и каталога, метка тома.
- Драйвер диска в режиме 3 (узнать размер) возвращает в hl адрес структуры с информацией о диске.

NC.COM
- Добавлены кнопки Home (стрелка влево-вверх) и End (ПС) - переход на первый и последний файлы в каталоге.
- Скроллинг списка файлов. Размер каталога - до 256 элементов.
- В случае отсутствия драйвера или не отформатированного диска, на инфопанели выводится текст "Drive C: has no driver" или "Drive C: is not formatted".
- Отображение метки тома на инфопанели.

E.COM
- Перенесен на новый рабочий адрес (E800h), доработан для совместимости с новым форматом дескриптора файла, простенько раскрашен.

FORMAT.COM
- Форматирует диски в FAT16 с созданием загрузочного сектора.

ROM.COM
- Драйвер стандартного 64 кб ROM-диска, подключаемого через ВВ55. Адаптация драйвера DISK-H.COM из оригинального Коммандера. Добавлен с целью отладки, т.к. эмулятор не поддерживает флеш-диск HardwareMan'а.
