# Генерируемые образы

В этот каталог сборщик `scripts/images/build-image.ps1` записывает образы
системного ПЗУ и flash-диска. Бинарные образы не отслеживаются Git.

Для обновления одной системной AT29C040A используется `C-Flash-512k-MMX-System.bin` (524288 байт). Исправленный StarTrek находится в `H:\MXOS\StarTrek.exe`. Бенчмарки находятся в `H:\BENCH` вместе с подкаталогами и DLL; у них нет атрибутов System/Read-only.

Выходные файлы:

- `C-ROM-32k-MX-RAMFOS.bin` — самостоятельное 32-КиБ ПЗУ RAMFOS с родным каталогом. Сборка: `scripts/images/build-image.ps1 MxRamfos32`.
- `C-ROM-2k-MX-boot.bin` — загрузчик MX для одной К573РФ2/5.
- `A-Flash-1M-MX-System.bin` — внешний загрузочный том MX для Emu80. Для прошивки первых двух микросхем используются части с суффиксами `-chip1` и `-chip2`.
- `C-ROM-48k-MX-System.bin`, `C-ROM-32k-MX-System.bin` и общая дискета `A-Floppy-720k-MX-System.img` — оригинальный «Специалист-MX» с одним банком ДОЗУ. Сборка и запуск описаны в [профиле MX](../scripts/images/README.md#специалист-mx-с-одним-банком-дозу).
- `C-ROM-64k-MMX-System-Emu80.bin` — системное ПЗУ объёмом 64 КБ для эмулятора;
- `C-Flash-128k-MMX-System.bin`, `C-Flash-256k-MMX-System.bin` и `C-Flash-512k-MMX-System.bin` — сырые
  образы перезаписываемого системного ПЗУ выбранного объёма;
- `A-Floppy-720k-Programs.img` — дискета программ MMX.
- `H-Flash-2M-Games.bin` — внешний flash-диск целиком для Emu80.
- `H-ROM-64k-Games.bin` — образ внешнего ROM-диска объёмом 64 КБ для прошивки и эмулятора;
- `H-Flash-2M-Games-chip1.bin` … `H-Flash-2M-Games-chip4.bin` — четыре микросхемы
  физического flash-диска общим объёмом 2 МБ.
