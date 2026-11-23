# ZOO

[Zoo-std.rks](originals/Zoo-std.rks), архивный `ZOO.RKS` из `Soft/games1_rks`, используется на H:. В [описании подборки LIK](../../std/manuals/games_lik_rks/README.md) у редакции LIK, редактора и уровней указан «Монитор '88» со специальной сборкой составителя.

| Файл в коллекции | Архивное имя | Назначение |
|---|---|---|
| [Zoo-lik-game.rks.bin](originals/Zoo-lik-game.rks.bin) | `ZOO.RKS` | Редакция игры для LIK. Нагрузка `0000h–187Fh` отличается от версии в обычном RKS. |
| [Zoo-lik-editor.rks.bin](originals/Zoo-lik-editor.rks.bin) | `ZOO-ED.RKS` | Редактор уровней, `7000h–769Fh`. В описании указан запуск командой `J7000`. |
| [Zoo-lik-levels.rks.bin](originals/Zoo-lik-levels.rks.bin) | `ZOO-LEV.RKS` | Данные уровней, `17C0h–253Fh`. Составитель подборки называет их собственными уровнями. |

Все три файла сохранены побайтно в исходном формате LIK. Суффикс `.bin` отличает их от обычных RKS. Способ запуска и подключения уровней в текущей MXOS пока не проверен.
