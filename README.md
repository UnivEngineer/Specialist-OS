# Операционная система для ПК "Специалист-MX/MX2"

Основана на двух существующих для "Специалиста-MX" ОС - Commander 1.4 (MXOS) и Ramfos, дизассемблированных и доработанных vinxru.
Идея в том, чтобы сделать гибридную ОС, позволяющую запускать программы как от Ramfos (коих много) и от MXOS (коих чуть-чуть),
так и классические по механизму MX2 (программное переключение оборудования в стандартный режим). Но в основе архитектуры будет MXOS
как более продвинутая.

## Как собрать

Открыть проект в Visual Studio, нажать Build. Используется кросс-ассемблер SjASMPlus.

## Как запустить в эмуляторе

В эмуляторе emu - нажать в Visual Studio Run. Запустится файл Release\run.bat, который сначала создаст образ ПЗУ с помощью
js скрипа из проекта vinxru (ROM\MY_MXOS.bin), затем скопирует его в папку эмулятора, и запустит его. В run.bat надо прописать
путь к эмулятору. Конфиг для эмулятора лежит рядом с run.bat, его надо скопировать в папку emu\config.

## Как запустить на реальном железе

Необходим Специалист-MX или Специалист-MX2 или совместимый клон (Сябр, ...), и программатор ПЗУ. Образ ПЗУ собирается при
запуске Release\run.bat или ROM\makeRom\\-makeRom.js и лежит в папке ROM\MY_MXOS.bin, а файлы, из которого он собирается -
в папке ROM\makeRom. Образ рассчитан на ПЗУ объёмом 64 кб.

В папке FlashDrive - набор образов для прошивания Флеш-диска, и скрипт для сборки образов из файлов. Приложена коллекция игр
для Специалиста почти на 2 Мб (проверены не все!).

## Исправления/улучшения

----

### 2022.01.28

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
- Запуск RKS файлов в STD режиме Специалиста-MX2. Монитор-2 взят из Ramfos Vinxru.

----
### 2022.02.03

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

----
### 2022.02.07

DOS.SYS
- В предыдущем апдейте потеряна совместимость с программами, обрабатывающими дескрипторы каталога и имя файла в старом формате.
- Продолжение перехода на FAT12/16:
    - реализованы 16-битные номера кластеров;
    - увеличена таблица FAT и корневой каталог (сейчас максимум 512 кб на разделе и 32 файла в каталоге);
    - изменены стандартные п/п драйвера диска (потеряна совместимость с программами, использующими низкоуровневые вызовы).

NC.COM
- Переработана инфопанель, теперь на нее выводится объем основной памяти и ДОЗУ

E.COM
- Дизассемблирован, добавлен в проект.

MON2.COM
- Дизассемблирован, добавлен в проект.

----
### 2022.02.11
E.COM
- Пресобран для работы на другом адресе. В процессе стало ясно назначение клавиш. Получилась инструкция.

Инструкция к редактору E.COM

F1 - переключение режима Insert/Overwrite\
F2 - курсор на страницу вверх (PageUp)\
F3 - курсор на страницу вниз (PageDn)\
F4 - открыть файл\
F5 - сохранить файл\
F6 - открыть файл и вставить его в конец документа\
F7 - поиск текста в строке курсора и ниже (перематывает документ на строку с найденным текстом)\
F8 - отмена изменений в строке\

Стрелки - управление курсором. Можно его разместить где угодно, в т.ч. за концом строки и за концом документа\
Del - удалить символ, на котором курсор\
Enter - вставить строку ниже курсора (не разбивает текущую строку)\
Tab - курсор вправо на следующий ближайший столбец шириной 4 (8) знакомест (не совсем понятно, шаг постоянно меняется, может баг?), не вставляет символ tab, а просто передвигает курсор\
Home - курсор в начало строки\
End (ПС) - курсор в конец строки\
СТР - выход (без сохранения!)\

Esc-Home - курсор в начало первой страницы\
Esc-End - курсор в начало последней страницы\
Esc-S - разбить строку в положении курсора на две\
Esc-J - объединить текущую строку (на которой курсор) и следующую\
Esc-L - выделить текущую строку; потом можно переместить курсор на другую строку и снова нажать Esc-L - выделить все строки от первой выделенной до текущей\
Esc-U - снять выделение строк\
Esc-C - вставить выделенные строки в строку ниже текущей\
Esc-M - переместить выделенные строки в строку ниже текущей\
Esc-D - удалить выделенные строки\
Esc-N - новый документ (очистить буфер), выводится запрос для подтверждения\
Esc-O - сохранить файл на магнитофон\
Esc-I - загрузить файл с магнитофона\
Esc-V - загрузить файл с магнитофона (в другом формате?)\
Esc-G - загрузить файл с магнитофона и вставить в конец документа\

Редактор поддерживает запуск из командной строки с передачей имени открываемого файла в виде аргумента. Это используется в Коммандере - редактор назначен на клавишу F4. Также можно ассоциировать с ним любые типы файлов, например TXT, прописав их в NC.EXT.
Редактор хранит переменные в области памяти под экраном, ниже переменных BIOS. Это позволяет ему помнить некоторые настройки между сессиями, даже загруженный/введенный ранее текст, если ничто не попортило буфер в памяти.
У кнопки F3 есть второй обработчик, но до него управление никогда не доходит. Поменял их местами ради интереса - просто нажатие F3 выбрасывает из редактора. Если перед этим загрузить файл по F4, то стирается экран, вместе со строкой состояния.
Команды F4-F6 - работа с файлами на дисках Коммандера. В строке состояния выводится запрос имени файла. Если не вводить букву диска, имеется в виду текущий диск. При этом буфер для ввода имени файла - всего 9 символов, т.е. хватает только на имя, точку и расширение (6.3), а на букву и двоеточие уже не хватит, если имя длинное.
Кнопка СТР злая, выход из редактора сразу без сохранения и каких-либо запросов. Получается, нет простого способа просто взять и сохранить изменения: надо нажать F5, вспомнить имя редактируемого файла, сохранить. Это если редактор запускается с передачей файла через командную строку. Если запускать его отдельно и открывать файл через F4, то он запомнит имя файла и будет сразу его показывать в строке ввода команд F4-F6. Может быть, это просто недоработка.
Кнопка Tab не вставляет символ tab, а просто передвигает курсор. Вроде бы должна двигать на ближайший столбец шириной 4 (8) знакомест, но шаг постоянно меняется. Может, это баг, а может как-то хитро заточено под форматирование кода на ассемблере.
Переключение на ввод русского текста делается так. HP+Рус переключает язык. В режиме КОИ-8 Рус работает как Caps Lock. А в режиме КОИ-7 Рус переключает язык. Это общий принцип в этой ОС. Редактор нормально отображает файлы в кодировке КОИ-8, но как печатать? Кстати, код символа Ъ = 0FFh используется как признак конца файла, так что нельзя, чтобы он был в тексте.

----
### 2022.02.16

- Убрана возможность вернуться на FAT8, слишком много накопилось изменений.
- Перерисованы некоторые символы в шрифте FONT.FNT, чтобы сделать его обычным, а не italic, и добавить недостающие символы по стандарту КОИ-8.
- Изменено распределение памяти, чтобы освободить место для разрастающегося DOS.SYS:
    - системные программы (NC.COM, E.COM, DL-red.COM) переехали на E800h (совместимо с Ramfos);
    - шрифт переехал на DE00h;
    - драйверы флеш/ром диска переехали на DE00 (512 байт), драйвер магнитофона на DC00 (512 байт).
- Скрипт makeRom.js унифицирован:
    - создаёт образы ПЗУ для Специалиста-MX, MX2 (системные), и флеш-диска (разбитый на отдельные файлы для прошивания каждой из микросхем);
    - формат "почти FAT16", с загрузочным сектором (пока ещё есть небольшие отличия от спецификации);
    - в загрузочном секторе системной ПЗУ находится программа-загрузчик (копирует DOS.SYS в ОЗУ);
    - все параметры настраиваются несколькими строчками в начале скрипта (тип образа, размер ПЗУ, размер каталога, пути к файлам).

DOS.SYS
- Режим КОИ-8 по умолчанию. Имена файлов в КОИ-8. Ввод строчных английских букв по умолчанию. Двухтональный звуковой сигнал при переключении Caps Lock.
- Файловые операции не чувствительны к регистру (КОИ-8).
- Файл AUTOEX.BAT переименован в autoexec.bat.
- Буфер fat и каталога в памяти заменен на кэш секторов активного устройства. Объём кэша можно настраивать. По умолчанию - как и было, 1 кб, FB00-FEFF.
- Поддержка дисков с разным размером fat и корневого каталога. Пока ещё сектор = кластер = 256 байт.
- Реализован загрузочный сектор FAT16, откуда читается информация о размерах fat и каталога, метка тома.
- Драйвер диска в режиме 3 (узнать размер) возвращает в hl адрес структуры с информацией о диске.
- Новые стандартные подпрограммы (точки перехода C800h):
    - сравнение строк (чувствительное и нечувствительное к регистру КОИ-8);
    - перевод кода символа в верхний  регистр (КОИ-8);
    - 16-битная арифметика (вычитание, умножение, деление hl и de).

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

----
### 2022.02.18

MON2.COM
- Обновлен для совместимости с измененной ОС, составлен список директив.

Большинство директив стандартные, но интересное начинается с директивы ? - файловые операции с дисками. Директива вводится как обычно, например B1000,1FFFF <BK>, но затем появляется запрос имени файла. Можно вводить его с буквой диска, если текущий диск отличается, при этом произойдет переключение диска.
Загадочная штука - наличие в мониторе таблицы подпрограмм F800h, большинство из которых дублируют стандартные C800h. Вероятно, это сделано для экспериментов/адаптации программ от Ориона, или же от Рамфоса (но его подпрограммы F800h заметно отличаются по назначению).

G - запуск программы по указанному адресу, второй параметр - инъекция точки останова\
D - дамп блока памяти в HEX виде\
L - дамп блока памяти в текстовом виде\
M - побайтовое редактирование блока памяти\
T - копирование блока памяти на новый адрес (простое, пересекающиеся блоки будут испорчены)\
F - заполнение блока памяти байтом\
C - сравнение двух блоков памяти\
S - поиск последовательности байт в блоке памяти\
N - печать всех байт в блоке памяти, не равных данному\
X - печать содержимого регистров процессора\
H - печать суммы и разности двух HEX слов\
K - подсчет контрольной суммы блока памяти\
W - запись блока памяти на ленту без имени\
R - чтение файла с ленты\
J - выход из Монитора (теплая перезагрузка ОС)\

? - листинг каталога текущего диска (выводится имя, адрес загрузки, размер в HEX виде)\
V - загрузка файла в память по указанному адресу\
U - загрузка файла в память по адресу, указанному в его дескрипторе\
B - запись блока памяти в файл\
A - установка нового адреса загрузки файла\
Q - печать байта атрибутов файла\
Y - установка нового байта атрибутов файла\
A: ... H: - переключиться на другой диск\
