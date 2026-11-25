# Физический CPM32 и Reset в Emu80

Штатный MX2 C-flash rev70, SHA-256 `bd2d10e2dfd4112732b7cdfcabd9683daa8082cb0fc3ceae5debb6479a7041ab`. Проверка использовала частные процессы Emu80 на порту 19781, запущенные через `run.ps1 -Hidden -Flash -Mcp` с проверенной Qt offscreen, без звука и с фиксированной скоростью 32×. По завершении стенд закрыл свои процессы.

Прошёл 91 вызов копирования длиной от 0 до 8192 байт, включая 27 физических переносов из ROM 0, 1 и 5. Проверены MAIN, DOS, EXEC и DATA, пакет запроса, регистры, стек, возврат страницы, сторожевые байты и последние данные в окне. Для ROM стенд дополнительно сверял байты с упакованным образом flash. В каждом из десяти маршрутов остановки FF2Eh и FF81h подтвердили содержимое окна и записанную половину назначения. Повторный вход FF2Eh пришёл через 1022 такта. [Результаты CPM32](result.json)

У rev65 и rev68 прежние байты FED0h–FED7h равны `77 3A DE FF B7 C2 00 FC`. У rev69 и rev70 они равны `77 3A D5 FF FE 03 DA 07`. В диапазоне FF0Eh–FF15h у всех четырёх ревизий лежит `E1 22 C0 FF E1 22 C2 FF`, две пары POP/SHLD. Новый bootstrap сравнивает этот диапазон с установочным образом по прежнему правилу полного восьмибайтового совпадения.

Тёплый Reset rev70 восстановил по 16 намеренно испорченных байтов хвоста в DATA 2 и 12 с сохранением сектора B:. При переходе с rev68 и rev69 стенд перенёс полные снимки DOS и первого банка B: из реально запущенных старых образов. Новый bootstrap заменил несовместимую DOS, сохранил `BOOT_RAM_FRESH=0` и восстановил резидентный хвост. На входе `bootDone` совпали все 65024 байта первой страницы B: до FE00h. После запуска NC совпали 512 контрольных байт в двух областях. [Миграция rev68](migration-68.json) · [Миграция rev69](result.json)

Сборка выполнялась в каталоге из `docs/bench/text32-dos/checkout.txt`.

```powershell
python -X utf8 scripts/build/build-bootstrap.py --fonts-ready
powershell.exe -NoProfile -File scripts/images/build-image.ps1 SystemEmuWritable
```

Воспроизведение из основного рабочего каталога.

```powershell
$checkout = (Get-Content docs/bench/text32-dos/checkout.txt -Raw).Trim()
$sha = 'bd2d10e2dfd4112732b7cdfcabd9683daa8082cb0fc3ceae5debb6479a7041ab'
python -X utf8 docs/bench/text32-dos/cpm32-emulator/check-hidden.py $checkout --image-sha256 $sha --old-bundle docs/bench/text32-dos/after
python -X utf8 docs/bench/text32-dos/cpm32-emulator/check-hidden.py $checkout --image-sha256 $sha --old-bundle docs/bench/text32-dos/before --migration-only
```
