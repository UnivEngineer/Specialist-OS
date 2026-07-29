# Генерируемые образы

В этот каталог сборщик `scripts/images/build-image.ps1` записывает образы
системного ПЗУ и flash-диска. Бинарные образы не отслеживаются Git.

Выходные файлы:

- `SystemRom_emulator.bin` — системное ПЗУ объёмом 64 КБ для эмулятора;
- `SystemRom_128K.bin`, `SystemRom_256K.bin` и `SystemRom_512K.bin` — сырые
  образы перезаписываемого системного ПЗУ выбранного объёма;
- `FlashDisk_emulator.bin` — flash-диск объёмом 64 КБ для эмулятора;
- `FlashDisk_chip1_512K.bin` … `FlashDisk_chip4_512K.bin` — четыре микросхемы
  физического flash-диска общим объёмом 2 МБ.
