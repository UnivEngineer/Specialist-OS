:: Параметры:
::
:: make filename.asm address
::
:: address - десятичный адрес загрузки файла, добавляется в имя файла

::@echo off
:: очистка экрана
cls

:: путь\имя asm-файла уже передаётся без расширения
set filefull=%~1
set filename=%~n1
set ext=%2
set address=%3

:: если существует exe-файл с таким же именем ― удаляем его
::if exist %filefull%.%ext% del %filefull%.%address%.%ext%

:: компиляция
:: если во время компиляции будут ошибки, тогда они будут перечислены в файле errors.txt
::..\tasm\TASM.EXE -gb -b -85 %filefull%.asm %filefull%.%ext%
::..\tasm\ASM80WIN.EXE %filefull%.asm
..\tasm\sjasmplus.exe --i8080 --lst=%filefull%.lst --raw=%filefull%.%ext% %filefull%.asm
:: > errors.txt

:: обнаружены ошибки компиляции ― переходим на err
if errorlevel 1 goto err

:: раз мы здесь ― значит ошибок нет
:: копируем бинарник в папку создания ром диска
echo %cd%
echo copy %filename%.%ext% ..\ROM\makeRom\%filename%.%address%.%ext%
copy %filename%.%ext% ..\ROM\makeRom\%filename%.%address%.%ext%
exit

:err
:: печатаем errors.txt
::type errors.txt
exit 1
