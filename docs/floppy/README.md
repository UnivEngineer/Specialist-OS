# Дисковод «Специалиста МХ»

Подборка для изучения контроллера К1818ВГ93 и исторических способов работы с ним. Файлы скопированы побайтно из локального архива `D:/Projects/RetroPC/Specialist`. Листинги и исходники сохраняют исходные кодировки. Бинарники исторические, с собственными адресами загрузки и соглашениями о вызовах.

## Схемы и документация

- [FDDController1.png](schemes/mx/FDDController1.png) — исходная схема контроллера МХ. [FDDController2.png](schemes/mx/FDDController2.png) — общая схема с доработками. [FDDController3.png](schemes/mx/FDDController3.png) — поздняя схема для дисководов 3,5 и 5,25 дюйма. [fdd_upgrade.png](schemes/mx/fdd_upgrade.png) — доработки для CP/M. Источник файлов — `specialist-mx.org/schemes`, описание на странице [«Контроллер НГМД»](https://spetsialist-mx.softrest.eu/index22.html).
- [MX_DOS.pdf](manuals/MX_DOS.pdf) — описание контроллера, подключения и MX-DOS от Л. Н. Афанасьева. [RAMFOS.pdf](manuals/RAMFOS.pdf) — руководство к базовой среде. [MX-DOS.txt](manuals/MX-DOS.txt) — архивный текст. Источник — каталоги `Docs` и `texts` зеркала Fifan. [Онлайн-копия документации контроллера](https://oldcomputer.info/media/disk525/EC5088-pinout.pdf).
- [Архив LafanL3](schemes/lafan/) — исходные чертежи НГМД и файл платы из `Soft/Lafan/LafanL3`. В [schemes/other](schemes/other/) лежат материалы для сравнения, в том числе схема «Линёвского» варианта с регистрами ВГ93 по FFF4h–FFF7h. [Страница «Линёвского»](https://spetsialist-mx.softrest.eu/index14.html).

## Код и бинарники

| Каталог | Что внутри | Исходный путь в архиве |
|---|---|---|
| [fifan](fifan/) | `floppy.a80`, листинг и `floppy.bin`. Драйвер собирался с F300h и устанавливался как F:. Есть исходный и итоговый ROM, а также сопровождающий `fileCreate.a80`. | `OS/Fifan/MXOS-floppy` |
| [ramfos-boot](ramfos-boot/) | `BootDOS.asm`, `BootDOS.0.EXE` и листинг. Внешний загрузчик для RAMFOS читает с дисковода и передаёт управление MX-DOS по E400h. | `OS/RAMFOS-master/RunMXDOS` |
| [lafan-mxdos](lafan-mxdos/) | MX-DOS 1.71, форматтер 0.8, более поздние бинарники и программы работы с дискетами. | `Soft/Lafan/LafanS1/MX-DOS`, `Soft/Lafan/LafanS2/Systems/Disk`, `Soft/Lafan/bst_mx0/System/MX-DOS` |
| [disk-images](disk-images/) | Три архивных образа ODI по 819 200 байт после распаковки. | `specialist-mx.org/Systems` |
| [emulator](emulator/) | Конфигурации и ROM. В `floppy.rom` и `fifan/ramfos.bin` полный `floppy.bin` найден побайтно со смещения 2E00h. | `Emulator/emu80-src/dist/specmx`, `Emulator/emu/config` |

Выход на ВГ93 есть и в драйвере Fifan, и в загрузчике из дерева RAMFOS. В исходнике Fifan указаны FFE8h–FFEBh для ВГ93, FFF0h для синхронизации передачи, FFF2h для стороны и FFF3h для выбора дисковода. Эмуляторный профиль задаёт для ODI геометрию 80 дорожек, 2 стороны, 5 секторов по 1024 байта. Это историческая конфигурация образов. Совместимость с дискетами FAT12 MS-DOS требует отдельной проверки геометрии и обмена на выбранной ревизии платы. [Каталог ПО и образов](https://spetsialist-mx.softrest.eu/index19.html) · [Карта MX2](https://spetsialist-mx.softrest.eu/index21.html)

## «Линёвский» Himac и Emu80

- [rd_dos.asm](linevo/rd_dos.asm) и [boot_sec.asm](linevo/boot_sec.asm) из [rd_dos.rar](linevo/rd_dos.rar) показывают интерфейс линёвского варианта. ВГ93 занимает FFF4h–FFF7h, порт FFF3h служит для управления и опроса DRQ/INTRQ. Ассемблерные файлы в UTF-16LE. [Источник](https://spetsialist-mx.softrest.eu/index14.html)
- В штатных профилях Emu80 «Специалист-MX», «Специалист-MX (MXOS)» и «Специалист-MX (Commander)» один интерфейс МХ. ВГ93 находится на FFE8h–FFEBh, дополнительные регистры — FFF0h–FFF3h. Модель SpecMxFddControlRegisters обрабатывает записи FFF2h и FFF3h для выбора стороны и привода. Запись в FFF0h и FFF1h в этой модели пропускается. Штатная геометрия ODI — 80×2×5×1024. [Конфигурация](emulator/spmx-mxos.inc)
- Проектный профиль [specialist-mx2.conf](../../scripts/emu80/specialist-mx2.conf) отображает ту же модель МХ. В существующей конфигурации отсутствует подключение FdImage к fdc. Готового профиля линёвского Himac в проверенном дереве Emu80 нет.

## zx-pk.ru

В зеркале Fifan из описания MX2 есть ссылка на [обсуждение HardWareMan](https://zx-pk.ru/showthread.php?t=12316&page=20). Вход в раздел «Специалист» указан как [f=57](https://zx-pk.ru/forumdisplay.php?f=57). При прямой проверке форум вернул HTTP 403, поэтому форумные вложения в подборку не вошли. Ссылки сохранены для проверки с доступом к форуму.

[Контрольные суммы файлов](SHA256SUMS.txt). Архивы проверены командой `7z t`. Три схемы MX, MX_DOS.pdf и bst_mx0.rar совпали побайтно с онлайн-зеркалом Fifan. Бинарники не запускались.
