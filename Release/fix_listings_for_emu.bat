:: Исправление листингов для эмулятора emu:
:: удаление всех строк, начинающихся с #

@echo off

del /s ..\*.lst.fix.lst

dir ..\*.lst /s /b >  list.txt

for /f "tokens=*" %%A in (list.txt) do (
    echo Fixing %%A
    type %%A | findstr /b /v "#" > %%A.fix.lst
)
 
