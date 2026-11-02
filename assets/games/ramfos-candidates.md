# Реестр игровых RAMFOS-кандидатов

Дата первичного поиска и регистрации: 02.11.2026.

Зарегистрированы **62 различные игровые нагрузки** из быстрого поиска по ABI. Побайтные копии объединены по SHA-256; разные нагрузки одной игры оставлены отдельными строками. Полные хеши, все найденные копии, смещения вызовов и проверенный источник: [ramfos-candidates.json](ramfos-candidates.json).

Это реестр признаков для дальнейшей проверки, а не доказательство происхождения от RAMFOS или совместимости. `strong-candidate` означает сочетание признаков либо передачу символа в A перед C80Fh; `weak-candidate` — неоднозначный вызов, требующий ручной проверки. `known-ramfos` — уже известная адаптация/сборка; `user-tested-issues` — пользователь сообщил об ошибках под текущей MXOS. Даже сильный кандидат может оказаться программой для другого BIOS с похожими точками.

Фильтр: отдельные программы из игровых каталогов с характерным вызовом, найденным прямым обходом кода, либо записью C63Eh. Совпадения только в данных, образы и системные утилиты в этот игровой реестр не включены. Ничего из реестра автоматически не копируется в библиотеку и не добавляется в GAMES.

## Имя `[ahmaty2.EXE`

Файл существует именно под таким именем: `D:/Projects/RetroPC/Specialist/Soft/Lafan/LafanG1/GameMX4/[ahmaty2.EXE`, 8192 байта. Рядом находится `[ahmaty.EXE`. Начальная квадратная скобка — буквальный символ `5Bh`; в KOI-7 он занимает позицию «Ш», поэтому имя читается как «Шахматы2». Архивное имя не исправлялось. В этой нагрузке обнаружены C80Fh после A=A5h/A0h и C84Bh. SHA-256: `95143a60077e13e998dd11f9f61307677d5c3e8fcc3a58dc820f1e94f3a8f462`.

## Записи

Дополнительная ручная гипотеза вне автоматического ABI-фильтра:
**BUDI2** (`BUDI/legacy/BUDI2.rks`, нормализованная копия
`BUDI/Budi2-color-f000.rks`) — кандидат на незавершённую RAMFOS-адаптацию
по уточнению автора 02.11.2026. Происхождение не подтверждено; записи цвета
F000h не доказывают отдельный нестандартный контроллер. Это дополнение
не меняет 62 записи результатов сканирования в JSON. Не путать с RF-006,
который относится к другому бинарнику Budi-mx.exe. Подробности и хеши —
в [README BUDI](BUDI/README.md).

База внешних путей в таблице: `D:/Projects/RetroPC/Specialist/`. `{repo}` означает текущий репозиторий. Смещения в JSON относятся к началу нагрузки, без RKS-заголовка. Адрес загрузки для части raw-файлов предположен или взят из имени; его надо подтвердить перед запуском.

| ID | Исторический файл | Статус | ABI / дополнительные признаки | Источник |
|---|---|---|---|---|
| RF-001 | `Almaz_2.COM` | `strong-candidate` | C851h; C63Eh | `Soft/Lafan/LafanG1/GameMX1/Almaz_2.COM` |
| RF-002 | `BEAT.I80` | `strong-candidate` | C84Bh, C84Eh, C851h | `Soft/games_mx_i80/BEAT.I80` |
| RF-003 | `Beat2.COM` | `strong-candidate` | C84Eh, C851h | `Soft/Lafan/LafanG2/GameMX1/Beat2.COM` |
| RF-004 | `Beat_3.COM` | `strong-candidate` | C84Eh, C851h | `Soft/Lafan/LafanG2/GameMX1/Beat_3.COM` |
| RF-005 | `Boulder1.COM` | `strong-candidate` | C84Eh, C851h; C63Eh | `Soft/Lafan/LafanG2/GameMX1/Boulder1.COM` |
| RF-006 | `Budi-mx.exe` | `strong-candidate` | C84Eh, C851h | `{repo}/assets/games/BUDI/Budi-mx.exe` |
| RF-007 | `Budi.COM` | `strong-candidate` | C84Eh, C851h; C63Eh | `Soft/Lafan/LafanG2/GameMX1/Budi.COM` |
| RF-008 | `Budi_NEW.EXE` | `strong-candidate` | C84Eh, C851h; C63Eh | `Soft/Lafan/LafanG2/GameMX1/Budi_NEW.EXE` |
| RF-009 | `DIGGER.I80` | `strong-candidate` | C80Fh, C84Bh; символ в A → C80Fh | `Soft/games_mx_i80/DIGGER.I80` |
| RF-010 | `DOWN_E.I80` | `strong-candidate` | C63Eh | `Soft/games_mx_i80/DOWN_E.I80` |
| RF-011 | `DUNGEON.I80` | `strong-candidate` | C80Fh, C84Eh; символ в A → C80Fh | `Soft/games_mx_i80/DUNGEON.I80` |
| RF-012 | `Klad_M..EXE` | `strong-candidate` | C80Fh, C84Eh, C851h, C854h; C63Eh; символ в A → C80Fh | `Soft/Lafan/bst_mx0/Games/Gam1/Klad_M..EXE` |
| RF-013 | `Krot..EXE` | `strong-candidate` | C84Eh, C851h, C854h; C63Eh | `Soft/Lafan/bst_mx0/Games/Gam0/Krot..EXE` |
| RF-014 | `Krot.COM` | `strong-candidate` | C84Eh, C851h, C854h; C63Eh | `Soft/Lafan/LafanG1/GameMX1/Krot.COM` |
| RF-015 | `Lode_Run.COM` | `strong-candidate` | C84Eh, C854h; C63Eh | `Soft/Lafan/LafanG2/GameMX2/Lode_Run.COM` |
| RF-016 | `Mars..EXE` | `strong-candidate` | C80Fh; символ в A → C80Fh | `Soft/Lafan/bst_mx0/Games/Gam0/Mars..EXE` |
| RF-017 | `Nardy..EXE` | `strong-candidate` | C80Fh; C63Eh; символ в A → C80Fh | `Soft/Lafan/bst_mx0/Games/Gam1/Nardy..EXE` |
| RF-018 | `Piton_.COM` | `strong-candidate` | C84Eh, C851h | `Soft/Lafan/LafanG1/GameMX2/Piton_.COM` |
| RF-019 | `Pqtna{k..EXE` | `strong-candidate` | C84Bh, C84Eh, C851h | `Soft/Lafan/bst_mx0/Games/Gam1/Pqtna{k..EXE` |
| RF-020 | `Pqtna{ki.COM` | `strong-candidate` | C84Eh, C851h | `Soft/Lafan/LafanG1/GameMX2/Pqtna{ki.COM` |
| RF-021 | `Presled..COM` | `strong-candidate` | C84Eh, C851h, C854h; C63Eh | `Soft/Lafan/LafanG1/GameMX2/Presled..COM` |
| RF-022 | `Ralli..EXE` | `strong-candidate` | C80Fh, C851h; символ в A → C80Fh | `Soft/Lafan/bst_mx0/Games/Gam1/Ralli..EXE` |
| RF-023 | `RISEOUT.COM` | `strong-candidate` | C84Eh, C851h, C854h | `Soft/Lafan/LafanG2/GameMX2/RISEOUT.COM` |
| RF-024 | `Saboteur.EXE` | `strong-candidate` | C63Eh | `Soft/Lafan/LafanG2/GameMX2/Saboteur.EXE` |
| RF-025 | `Samantha.COM` | `strong-candidate` | C84Eh, C854h; C63Eh | `Soft/Lafan/LafanG2/GameMX2/Samantha.COM` |
| RF-026 | `shahm_mx.i80` | `strong-candidate` | C80Fh, C84Bh, C84Eh; символ в A → C80Fh | `Soft/games_mx_i80/shahm_mx.i80` |
| RF-027 | `Sky_scr..EXE` | `strong-candidate` | C84Bh; C63Eh | `Soft/Lafan/bst_mx0/Games/Gam0/Sky_scr..EXE` |
| RF-028 | `Sky_Scra.COM` | `strong-candidate` | C84Eh, C851h; C63Eh | `Soft/Lafan/LafanG2/GameMX2/Sky_Scra.COM` |
| RF-029 | `Stena_1.COM` | `strong-candidate` | C851h; C63Eh | `Soft/Lafan/LafanG1/GameMX3/Stena_1.COM` |
| RF-030 | `Tetris1..EXE` | `strong-candidate` | C80Fh; символ в A → C80Fh | `Soft/Lafan/bst_mx0/Games/Gam1/Tetris1..EXE` |
| RF-031 | `Tetris2..EXE` | `strong-candidate` | C851h; C63Eh | `Soft/Lafan/bst_mx0/Games/Gam1/Tetris2..EXE` |
| RF-032 | `Tetris3..EXE` | `strong-candidate` | C80Fh, C84Bh, C84Eh; символ в A → C80Fh | `Soft/Lafan/bst_mx0/Games/Gam1/Tetris3..EXE` |
| RF-033 | `Tetris_s.COM` | `strong-candidate` | C84Eh, C851h; C63Eh | `Soft/Lafan/LafanG2/GameMX3/Tetris_s.COM` |
| RF-034 | `Titan.EXE` | `strong-candidate` | C84Eh, C851h | `Soft/Lafan/LafanG2/GameMX3/Titan.EXE` |
| RF-035 | `TUTANK.I80` | `strong-candidate` | C80Fh, C84Bh; символ в A → C80Fh | `Soft/games_mx_i80/TUTANK.I80` |
| RF-036 | `UDAV.I80` | `strong-candidate` | C80Fh, C84Eh, C851h; символ в A → C80Fh | `Soft/games_mx_i80/UDAV.I80` |
| RF-037 | `Wertole..EXE` | `strong-candidate` | C63Eh | `Soft/Lafan/bst_mx0/Games/Gam1/Wertole..EXE` |
| RF-038 | `ZOO_.I80` | `strong-candidate` | C84Eh, C851h; C63Eh | `Soft/games_mx_i80/ZOO_.I80` |
| RF-039 | `[ahmaty2.EXE` | `strong-candidate` | C80Fh, C84Bh; символ в A → C80Fh | `Soft/Lafan/LafanG1/GameMX4/[ahmaty2.EXE` |
| RF-040 | `Buran-ramfos.exe` | `user-tested-issues` | C80Fh; C63Eh; символ в A → C80Fh | `{repo}/assets/games/Buran/Buran-ramfos.exe` |
| RF-041 | `StarWars2-ramfos.exe` | `user-tested-issues` | C84Bh; C63Eh | `{repo}/assets/games/StarWars/StarWars2-ramfos.exe` |
| RF-042 | `30.rks` | `weak-candidate` | C836h | `Soft/games_rks/30.rks` |
| RF-043 | `Boulder2.COM` | `weak-candidate` | C84Eh | `Soft/Lafan/LafanG2/GameMX1/Boulder2.COM` |
| RF-044 | `GOLD.I80` | `weak-candidate` | C854h | `Soft/games_mx_i80/GOLD.I80` |
| RF-045 | `JET_SET.EXE` | `weak-candidate` | F80Fh | `Soft/Lafan/LafanG2/GameMX2/JET_SET.EXE` |
| RF-046 | `LABIRINT.I80` | `weak-candidate` | C836h | `Soft/games_mx_i80/LABIRINT.I80` |
| RF-047 | `PACMAN2.I80` | `weak-candidate` | C851h, F83Ch | `Soft/games_mx_i80/PACMAN2.I80` |
| RF-048 | `Paket.rks` | `weak-candidate` | C836h | `{repo}/assets/games/Paket.rks` |
| RF-049 | `PIKFAMA.I80` | `weak-candidate` | F83Ch | `Soft/games_mx_i80/PIKFAMA.I80` |
| RF-050 | `Poker.COM` | `weak-candidate` | C84Eh | `Soft/Lafan/LafanG2/GameMX2/Poker.COM` |
| RF-051 | `POKER.I80` | `weak-candidate` | C84Eh | `Soft/games_mx_i80/POKER.I80` |
| RF-052 | `SAPPER.RK` | `weak-candidate` | F836h | `Emulator/Emu80/rk86/sdcard/GAMES/SAPPER.RK` |
| RF-053 | `SEX1.COM` | `weak-candidate` | C84Eh | `Soft/Lafan/LafanG2/GameMX2/SEX1.COM` |
| RF-054 | `SEX2.COM` | `weak-candidate` | C84Eh | `Soft/Lafan/LafanG2/GameMX2/SEX2.COM` |
| RF-055 | `Ska~ki.COM` | `weak-candidate` | C84Eh | `Soft/Lafan/LafanG1/GameMX3/Ska~ki.COM` |
| RF-056 | `Sohrani.COM` | `weak-candidate` | C84Eh | `Soft/Lafan/LafanG1/GameMX3/Sohrani.COM` |
| RF-057 | `VAVILON.I80` | `weak-candidate` | F80Fh | `Soft/games_mx_i80/VAVILON.I80` |
| RF-058 | `XTREE.COM` | `weak-candidate` | C84Eh | `Soft/Lafan/LafanG2/GameMX3/XTREE.COM` |
| RF-059 | `Almaz2-ramfos-optimized.exe` | `known-ramfos` | C851h; C63Eh | `{repo}/assets/games/Almaz-2/Almaz2-ramfos-optimized.exe` |
| RF-060 | `Almaz2-ramfos-original-broken.exe` | `known-ramfos` | C851h; C63Eh | `{repo}/assets/games/Almaz-2/legacy/Almaz2-ramfos-original-broken.exe` |
| RF-061 | `Almaz2-ramfos-original-fixed.exe` | `known-ramfos` | C851h; C63Eh | `{repo}/assets/games/Almaz-2/legacy/Almaz2-ramfos-original-fixed.exe` |
| RF-062 | `Almaz2-ramfos.exe` | `known-ramfos` | C851h; C63Eh | `{repo}/assets/games/Almaz-2/Almaz2-ramfos.exe` |

## Контроль происхождения

Для всех 62 нагрузок при регистрации найден существующий исходный файл с совпадающим SHA-256 после снятия контейнера. Проверенный путь указан в поле `verified_source` машинно читаемого реестра.

Архивные имена, включая странное написание и регистр, сохранены. Существующие записи `catalog.csv` и статусы игровых проверок не заменяются этим реестром. Исходный локальный отчёт: `docs/compat/ramfos-archive-scan-001/REPORT.md`; это рабочий материал, который может отсутствовать в Git. Сам реестр содержит необходимые для дальнейшего поиска имена, происхождение, хеши и ABI-признаки.
