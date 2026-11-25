# MXOS2. Исходный маршрут MEM.SYS

Итоговая сопоставимая база — 244 строки с исходной сборки в видимом Emu80 при 32×. В 48 строках COMMAND_BATCH исполняется пятибайтная запись SET_CURSOR_POS с проверкой координат. Ранние серии before/ оставлены для сравнения отдельных изменений, в них строка COMMAND_BATCH содержит только терминатор. Все остальные 196 исходных результатов повторились с точностью до такта. [Сводный JSON](baseline.json) · [Этапы и регистры](baseline32/profile.json) · [Параметры повтора](baseline32/README.md)

Исходники и образы получены из 2791c88ae940d63df2db01f0df7a616fbf0cd18d. В рабочем дереве было выравнивание одного пробела у RAM_ALTERNATE_DRIVE. SjASMPlus работает в режиме --i8080. Измерения выполняются на модели MX2 с CPU и ВИ53 по 2 МГц, в собственных видимых экземплярах Emu80 без звука. Параметры эмулятора и SHA-256 образа сохранены в before/system.json.

| Инициатор → исполнитель | F800h, BC = операция, DE → пакет HL/DE/BC/AF | F806h, операция в DW после CALL |
|---|---|---|
| MAIN → DOS | memCall → MEM_NATIVE_CALL → sysPacketReady → sysBankCall → sysRawRoute → sysBankDoCall → bankCallPacket → kernelDispatch. У RAW есть собственный маршрут. | bankMxCallRegs создаёт пакет на стеке, bankMxRegsRoute → MEM_NATIVE_CALL+3 → sysPacketReady, затем тот же путь. |
| EXEC → DOS | memCall → bankMxCall → bankMxDos → bankMxInvoke → bankCallPacket → kernelDispatch. | bankMxRegsRoute → bankMxRegsDos → bankCallPacket → kernelDispatch. |
| EXEC → BIOS | memCall → bankMxCall → memBiosRoute. Для скаляра bankMxInvoke → bankCallPacket → F003h → bbEntry → bbScalarFast → MEM_NATIVE_CALL+6 → sysBiosRegs → исполнитель. | bankMxRegsRoute сразу выбирает bankCallPacket для скаляра. Строки и COMMAND_BATCH повторно входят через F800h. |
| DOS → BIOS | Тот же путь, что из EXEC, источник имеет FAR-код 1. | Тот же путь, что из EXEC. |
| DOS → DOS | bankMxLocalDos → BANK_DOS_GATE+3 → kernelLocalDispatch, DE указывает в текущий банк. bankMxResult записывает результат в пакет при STATUS=0. | bankMxRegsDos → BANK_DOS_GATE+3, результат уже в регистрах. |
| MAIN → BIOS | sysFindTarget → sysLoadRegs → исполнитель → sysReturned, результат также в исходном пакете. | MEM_NATIVE_CALL+3 → sysBiosRegs → sysLoadRegs → исполнитель, результат непосредственно в CPU. |

Внешний банк передаёт логический адрес назначения в A, адрес входа в HL, операцию в BC и адрес локального MX_REGS в DE. Стек инициатора содержит старый bankServiceSP, HL, DE, BC, AF и адрес возврата (12 байт). Исполнитель получает отдельный стек и сохраняет в нём адрес bankServiceReturn, SP кадра инициатора, исходный FAR-код и прежние owner/home (8 байт). При повторном входе в приостановленный банк используется сохранённая граница bankServiceSP. В MAIN стек приложения под экраном и служебный стек BIOS имеют разные вершины. Поддерживаемую глубину определяет запас стеков вызывающего кода и исполнителя. В этой версии bankCall не проверяет BANK_MAX_DEPTH.

Шлюз использует FFC0h–FFC3h для выбора банка, FFC4h–FFC7h для возврата скалярных GET/PUT. В универсальной форме FFC8h–FFCFh содержат HL, DE, BC, AF, FFD0h–FFD1h содержат SP, FFD2h — банк, FFD3h–FFD4h — owner/home, FFD5h–FFDBh — kind, target, status, entry и operation. FFDEh выбирает форму доставки, FFDFh задаёт профиль. При строковом маршруте вместо этого кадра используются до 20 байт по FFC8h и завершающий ноль. При COPY доступны все 32 байта, снимок COMMAND локален банку.

Для внешней DOS kernelDispatch копирует вход в kdRegs, выбирает адаптер по номеру и банку источника, выполняет kdInvoke/kdCapture и восстанавливает COMMAND в kdFinish. Для скалярного BIOS bbModes выбирает форму, sysBIOSTable даёт адрес обработчика, sysLoadRegs распаковывает регистры. На возврате bankServiceReturn записывает четыре пары в окно, восстанавливает контекст исполнителя и меняет банк. bankResume копирует 8 байт из окна в стек инициатора и восстанавливает четыре пары через POP. F800h делает ещё одну копию в пользовательский пакет, F806h пропускает свой временный пакет.

## Измерения

Штатный benchmark-mxcall.py включает CALL и заключительный JMP на метку остановки (10 тактов). В двух независимых прогонах Emu80 получены следующие суммы для UPPERCASE. Каждое значение повторилось дважды. Штатный сценарий заново задаёт исходный профиль даже перед повтором. В расширенной матрице повтор стартует с профилем, который оставил первый вызов, а измерение заканчивается сразу за CALL/DW.

| Маршрут | F800h COMMAND | F806h COMMAND | F800h COPY | F806h COPY |
|---|---:|---:|---:|---:|
| MAIN → BIOS | 866 | 664 | 866 | 664 |
| DOS → BIOS | 3521 | 3109 | 4738 | 4326 |
| EXEC → BIOS | 3521 | 3109 | 4738 | 4326 |

Отдельный стенд z88dk-ticks для 8080 дал 75 тактов на bankCommandWindow с активным COMMAND и 1211 из COPY. Подготовка сектора 256 байт до bankCopyTail заняла 3481 и 2556 тактов соответственно. Для публичного bankCopyFar те же пути заняли 3971 и 3046 тактов. Исходные данные находятся в setup-before/result.json.

Штатные eмульторные данные UPPERCASE с интервалами между контрольными точками находятся в emu-command/original-command.json и emu-copy/original-copy.json. Входные бинарники и символы сохранены в input/. Сценарий profile-mxcall.py останавливается перед следующей инструкцией приложения. В samples.jsonl указаны отметки от CALL, отдельные переключения маппера и следы использования стеков. Класс NESTED использует тестовый DOS callback на внешнем bankCallPacket и настоящий вложенный F806h к BIOS. Замеры реального MODULE_SCOPE ведутся отдельно.
