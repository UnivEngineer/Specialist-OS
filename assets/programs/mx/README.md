# Penx и Trs_MX для RAMFOS

Бинарники, руководства и образцы взяты из образа `LafanL3.odi` и его распакованной копии в архиве `Soft/Lafan`. Адреса взяты из каталога самого образа. При сборке дискеты MXOS упаковщик добавляет к программам заголовки EXE v2.

| Архивный файл | Файл в `A:\LAFAN` | Загрузка и вход |
|---|---|---:|
| `Penx_2.3.COM` | `PENX23.EXE` | `D400h` |
| `Penx_MX.EXE` | `PENX28.EXE` | `D400h` |
| `Penx_MX.COM` | `PENX32.EXE` | `D400h` |
| `Trs_MX-3.COM` | `TRS-MX-3.EXE` | `0D30h` |
| `Trs_MX-p.COM` | `TRS-MX-P.EXE` | `0000h` |
| `Trs_MX-u.COM` | `TRS-MX-U.EXE` | `0000h` |

Для пробы запуска с дискеты в MXOS используйте, например, `A:RUN.COM -RF A:\LAFAN\PENX32.EXE`. Для проверки LOAD/SAVE в родной среде подключите `LafanL3.odi` и запустите RAMFOS с доступным RAM-диском. Диалог файлов использует файловую систему RAMFOS. Доступ к каталогу FAT12 потребует файлового адаптера RAMFOS.

В `A:\LAFAN\DOCS` лежат архивные тексты КОИ-8. `RP_penx0.TXT` и `RP_penx!.TXT` описывают Penx, `Tras0_MX.TXT` и `Tras!_MX.TXT` — Trs_MX, `Shemy-14.TXT` — комплект схем. В `A:\LAFAN\EXAMPLES` лежат схемы контроллера дисковода `C_ngmd.PNX` и `M_ngmd.PNX` и проект платы `P_ngmd.PLT`. Все десять проектов из `LafanL3/Platy` доступны в каталоге `examples`. Penx открывает PNX через F3 LOAD, Trs_MX открывает PLT через F3 LOADp.
