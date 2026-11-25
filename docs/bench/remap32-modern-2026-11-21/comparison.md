# FontBen 6×8 и REMAP32

Обычный вывод, мс на корпусе 924 байта.

| Движок | API | Вызов | Прямой код | Карта 8 байт | Таблица 256 байт |
|---|---|---|---:|---:|---:|
| Rotate | CharRaw | MAIN | 508,160 | 554,368 | 535,808 |
| Rotate | StringZ | MAIN | 303,744 | 339,456 | 321,792 |
| Rotate | RawZ | MAIN | 275,200 | 312,064 | 293,632 |
| Rotate | RawLim | MAIN | 258,560 | 295,424 | 276,864 |
| Rotate | RawLim | BANK NEAR | 323,712 | 360,448 | 342,016 |
| Packed | CharRaw | MAIN | 492,800 | 539,008 | 520,576 |
| Packed | StringZ | MAIN | 228,096 | 263,680 | 246,016 |
| Packed | RawZ | MAIN | 193,792 | 230,784 | 212,352 |
| Packed | RawLim | MAIN | 177,152 | 214,144 | 195,584 |
| Packed | RawLim | BANK NEAR | 242,304 | 279,168 | 260,736 |
| Fast | CharRaw | MAIN | 436,096 | 482,304 | 463,872 |
| Fast | StringZ | MAIN | 190,336 | 226,048 | 208,384 |
| Fast | RawZ | MAIN | 155,008 | 191,872 | 173,440 |
| Fast | RawLim | MAIN | 138,368 | 175,232 | 156,672 |
| Fast | RawLim | BANK NEAR | 203,520 | 240,384 | 221,952 |
