# Ввод команд в NC

После подтверждения PID и пути своего Emu80 сверь имена клавиш и параметры с `tools/list` этой версии. Используй `kbd_send` по одной логической клавише. Фокус окна Windows и SendKeys для гостевой клавиатуры не нужны. `kbd_text` требует KbdTapper, которого в штатном профиле нет. Ввод через редактор NC проверяет цвет и положение текста.

1. Дождись загрузки нужного профиля NC. Бери адреса mainLoop и vars.cmdLine из символов именно его сборки (`bin/NC.sym` для MAIN, `bin/NC_CORE.sym` для банкового ядра). Если использован breakpoint на mainLoop, сними его перед набором (`bp_clear`), дай NC 500 мс через `emu_run_for`. Сохраняй PC/SP и используй штатный опрос клавиатуры.
2. Очисти строку Escape и дай машине ещё 250 мс. Для каждого символа используй `kbd_send(action="tap", duration_ms=100)`, после отпускания — `emu_run_for(ms=120)`. Это интервалы эмулируемой машины при штатном антидребезге BIOS.
3. Имена клавиш: буквы a–z, цифры 0–9, пробел — space, двоеточие — colon, точка — period, дефис — minus, косая черта — slash, Enter — enter, Escape — esc. Для Ctrl в MX/RAMFOS/оконном мониторе удерживай историческую ПС (`rpt`), нажми и отпусти основную клавишу, отпусти `rpt`. `ctrl` — другая виртуальная клавиша. Сравнивай команды и имена без учёта регистра.
4. Перед Enter проверь снимок: команда должна быть в нижней строке у приглашения диска. Дополнительно можно прочитать ровно её длину из vars.cmdLine: нулевой терминатор в этот момент необязателен. Breakpoint целевой программы ставь после набора. Нажми Enter тем же способом.

Фрагмент для уже проверенного слота. `machine` — экземпляр Machine из `scripts/emu80/check-flash.py` с URL выбранного порта.

```python
def tap(key):
    result = machine.call('kbd_send', keys=[key], action='tap', duration_ms=100)
    if not result.get('stoppedByBreakpoint'):
        machine.call('emu_run_for', ms=120)

machine.call('bp_clear')
machine.call('emu_run_for', ms=500)
tap('esc')
machine.call('emu_run_for', ms=250)
keys = {' ': 'space', ':': 'colon', '.': 'period', '-': 'minus', '/': 'slash'}
for char in command:
    tap(keys.get(char, char.lower()))
# Сверь снимок нижней строки, затем вызови tap('enter').
```

Исторический снимок и прогон LAUNCH находятся в локальном `docs/compat/budi-002/nc-command.png` и `check-nc-command.py`. Нынешний загрузчик — RUN.COM. Старые прогоны с прямым вызовом через RAM подтверждали только ABI и загрузку.
