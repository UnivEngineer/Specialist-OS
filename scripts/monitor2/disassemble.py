"""Восстановление исходников точного проектного образа Monitor-2 (8080).

Запускается вручную из любого каталога; сборка использует готовые ASM/INC.
Карта кода и данных задана явно: шрифт и строки не декодируются как команды.
"""
import hashlib
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / 'source/monitor2'
IMAGE = DEST / 'originals/Monitor2.mon'
SHA256 = 'e8eabdee87cbbb679f0d6e7c30bcdb2e48a793fc0a7938d8d9411f8bcaa9555a'


def hexn(value, width=2):
    return '0' + f'{value:0{width}X}' + 'h'


def decode(data, offset):
    """Только документированные коды 8080, синтаксис SjASMPlus --i8080."""
    op = data[offset]
    regs = ['b', 'c', 'd', 'e', 'h', 'l', '(hl)', 'a']
    pairs = ['bc', 'de', 'hl', 'sp']
    cond = ['nz', 'z', 'nc', 'c', 'po', 'pe', 'p', 'm']
    byte = lambda: hexn(data[offset + 1])
    word = lambda: hexn(int.from_bytes(data[offset+1:offset+3], 'little'), 4)
    if 0x40 <= op <= 0x7F:
        return (1, 'halt' if op == 0x76 else f'ld {regs[(op >> 3) & 7]},{regs[op & 7]}')
    if 0x80 <= op <= 0xBF:
        return 1, ['add a,', 'adc a,', 'sub ', 'sbc a,', 'and ', 'xor ', 'or ', 'cp '][(op >> 3) & 7] + regs[op & 7]
    if op < 0x40:
        if op & 0xCF == 0x01:
            return 3, f'ld {pairs[op >> 4]},{word()}'
        if op & 0xCF in (0x03, 0x0B, 0x09):
            prefix = {3: 'inc ', 11: 'dec ', 9: 'add hl,'}[op & 15]
            return 1, prefix + pairs[op >> 4]
        if op & 7 in (4, 5, 6):
            reg = regs[(op >> 3) & 7]
            return (2, f'ld {reg},{byte()}') if op & 7 == 6 else (1, ('inc ' if op & 7 == 4 else 'dec ') + reg)
    if op & 0xC7 == 0xC0:
        return 1, 'ret ' + cond[(op >> 3) & 7]
    if op & 0xC7 in (0xC2, 0xC4):
        return 3, ('jp ' if op & 7 == 2 else 'call ') + cond[(op >> 3) & 7] + ',' + word()
    if op & 0xCF in (0xC1, 0xC5):
        return 1, ('pop ' if op & 15 == 1 else 'push ') + ['bc', 'de', 'hl', 'af'][(op >> 4) & 3]
    if op & 0xC7 == 0xC6:
        return 2, ['add a,', 'adc a,', 'sub ', 'sbc a,', 'and ', 'xor ', 'or ', 'cp '][(op >> 3) & 7] + byte()
    if op & 0xC7 == 0xC7:
        return 1, 'rst ' + hexn(op & 0x38)
    single = {0x00:'nop', 0x02:'ld (bc),a', 0x0A:'ld a,(bc)',
              0x12:'ld (de),a', 0x1A:'ld a,(de)', 0x07:'rlca', 0x0F:'rrca',
              0x17:'rla', 0x1F:'rra', 0x27:'daa', 0x2F:'cpl', 0x37:'scf',
              0x3F:'ccf', 0xC9:'ret', 0xE3:'ex (sp),hl', 0xE9:'jp (hl)',
              0xEB:'ex de,hl', 0xF3:'di', 0xF9:'ld sp,hl', 0xFB:'ei'}
    if op in single:
        return 1, single[op]
    if op in (0x22, 0x2A, 0x32, 0x3A, 0xC3, 0xCD):
        return 3, {0x22:lambda:f'ld ({word()}),hl', 0x2A:lambda:f'ld hl,({word()})',
                   0x32:lambda:f'ld ({word()}),a', 0x3A:lambda:f'ld a,({word()})',
                   0xC3:lambda:'jp '+word(), 0xCD:lambda:'call '+word()}[op]()
    if op in (0xD3, 0xDB):
        return 2, f'out ({byte()}),a' if op == 0xD3 else f'in a,({byte()})'
    raise ValueError(f'Недокументированный код {op:02X} по {offset+0xC000:04X}')


NAMES = {
    0xC000:'monitor2_start', 0xC010:'loader_clearScreen', 0xC037:'loader_printChar',
    0xC170:'loader_beep', 0xC190:'loader_delay', 0xC254:'loader_keybMode82',
    0xC25A:'loader_keybMode91', 0xC337:'loader_getch', 0xC377:'loader_tapeRead',
    0xC3D0:'loader_tapeWrite', 0xC427:'loader_compareHLDE', 0xC42D:'loader_copy',
    0xC438:'loader_printString', 0xC444:'loader_init', 0xC473:'loader_initialState',
    0xC488:'loader_writeDelay', 0xC494:'loader_prompt', 0xC4A0:'loader_keyTable',
    0xC500:'loader_font', 0xC800:'monitor2_vectors', 0xC836:'monitor2_restart',
    0xC84B:'monitor2_command', 0xCF49:'monitor2_messages', 0xCFD6:'monitor2_registerNames',
}


def main():
    data = IMAGE.read_bytes()
    if len(data) != 4096 or hashlib.sha256(data).hexdigest() != SHA256:
        raise ValueError('Неизвестный эталон Monitor2.mon')
    # C473h..C487h — начальные переменные, C48Fh..C4FFh — данные и клавиатура.
    ranges = [(0xC000, 0xC454), (0xC45A, 0xC473), (0xC488, 0xC48E), (0xC800, 0xCF49)]
    instructions = {}
    for start, end in ranges:
        addr = start
        while addr < end:
            size, text = decode(data, addr - 0xC000)
            if addr + size > end:
                raise ValueError(f'Команда пересекает границу данных: {addr:04X}')
            instructions[addr] = size, text
            addr += size
    labels = dict(NAMES)
    for _, text in instructions.values():
        for match in re.finditer(r'0([0-9A-F]{4})h', text):
            addr = int(match[1], 16)
            if 0xC000 <= addr < 0xD000:
                labels.setdefault(addr, f'L{addr:04X}')
    for addr in labels:
        for start, (size, _) in instructions.items():
            if start < addr < start + size:
                raise ValueError(f'Метка {addr:04X} внутри команды {start:04X}')
    comments = {
        0xC000:'Вход LAUNCH: HL лежит на стеке; POP BC; запись A в FFFFh; режим STD, 8 цветов.',
        0xC010:'Очистка экрана стековыми PUSH. Сохраняет HL/BC и SP; AF разрушается.',
        0xC037:'Вывод символа C. Сохраняет AF/BC/DE/HL; использует подэкранные переменные.',
        0xC254:'ВВ55: режим 82h. Выход A=82h; флаги и BC/DE/HL сохраняются.',
        0xC25A:'ВВ55: режим 91h. Выход A=91h; флаги и BC/DE/HL сохраняются.',
        0xC337:'Ожидание клавиши. Выход A=код; BC/DE/HL сохраняются, флаги меняются.',
        0xC427:'Сравнение HL и DE: Z при равенстве; A и флаги разрушаются.',
        0xC42D:'Копирование HL..DE исключительно в BC. Разрушает A/BC/HL и флаги.',
        0xC438:'Строка HL с нулевым завершением. Разрушает A/C/HL и флаги.',
        0xC454:'Остаточные байты исторического варианта, без установленного входа.',
        0xC473:'Шаблон C473h..C493h копируется в 8FDFh..8FFFh; внутри также код C488h.',
        0xC494:'Приглашение загрузчика, затем таблица клавиатуры C4A0h..C4FFh.',
        0xC500:'Знакогенератор: 96 знаков по 8 байт, включая исторический последний знак.',
        0xC800:'Стандартная таблица переходов Monitor-2; контракт отличается от MXOS.',
        0xC836:'Перезапуск: SP=7FFFh; системный вектор ошибки и верхняя граница памяти.',
        0xCF49:'Строки Monitor-2: исходные байты КОИ-7 сохранены без перекодирования.',
    }
    for name, start, end in [('loader.inc',0xC000,0xC500), ('font.inc',0xC500,0xC800), ('monitor.inc',0xC800,0xD000)]:
        lines = ['; Восстановлено из originals/Monitor2.mon; адреса и байты сохранены.', '']
        addr = start
        while addr < end:
            if addr in comments:
                lines += ['', '; ' + comments[addr]]
            if addr in labels:
                lines.append(labels[addr] + ':')
            if addr in instructions:
                size, text = instructions[addr]
                text = re.sub(r'0([0-9A-F]{4})h', lambda m: labels.get(int(m[1],16),m[0]), text)
            else:
                size = 1
                while size < 8 and addr+size < end and addr+size not in labels and addr+size not in instructions and addr+size not in comments:
                    size += 1
                text = 'DB ' + ','.join(hexn(x) for x in data[addr-0xC000:addr-0xC000+size])
            lines.append(f'    {text:<42} ; {addr:04X}h')
            addr += size
        (DEST/name).write_text('\n'.join(lines)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
