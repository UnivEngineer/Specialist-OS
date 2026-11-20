"""Split the historical logo into column RLE text and a small raw digit."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def pack_column(data):
    """0..127: 1..128 literal bytes; 128..255: repeat next byte 1..128 times."""
    size = len(data)
    costs = [0] * (size + 1)
    choices = [None] * size
    for start in range(size - 1, -1, -1):
        limit = min(128, size - start)
        options = [(1 + count + costs[start + count], False, count)
                   for count in range(1, limit + 1)]
        run = 1
        while run < limit and data[start + run] == data[start]:
            run += 1
        options.extend((2 + costs[start + count], True, count)
                       for count in range(2, run + 1))
        costs[start], repeat, count = min(options)
        choices[start] = repeat, count
    result = bytearray()
    start = 0
    while start < size:
        repeat, count = choices[start]
        result.append((128 if repeat else 0) | (count - 1))
        result.extend(data[start:start + 1] if repeat else data[start:start + count])
        start += count
    return bytes(result)


def trim_rows(data, width):
    rows = [row for row in range(39)
            if any(data[column * 39 + row] for column in range(width))]
    top, bottom = min(rows), max(rows) + 1
    return top, bottom - top, [data[column * 39 + top:column * 39 + bottom]
                              for column in range(width)]


def main():
    original = (ROOT / 'source/bootstrap/splash.bin').read_bytes()
    assert len(original) == 46 * 39
    text_top, text_height, text = trim_rows(original[:44 * 39], 44)
    digit_top, digit_height, digit = trim_rows(original[44 * 39:], 2)
    packed = b''.join(pack_column(column) for column in text)
    raw_digit = b''.join(digit)
    out = ROOT / 'bin/bootstrap'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'splash-text.rle').write_bytes(packed)
    (out / 'splash-digit.bin').write_bytes(raw_digit)
    (out / 'splash-config.inc').write_text(
        f'BOOT_LOGO_TEXT_Y = {112 + text_top}\n'
        f'BOOT_LOGO_TEXT_HEIGHT = {text_height}\n'
        f'BOOT_LOGO_DIGIT_Y = {112 + digit_top}\n'
        f'BOOT_LOGO_DIGIT_HEIGHT = {digit_height}\n', encoding='utf-8')
    print(f'Boot splash: text {len(packed)} B RLE, digit {len(raw_digit)} B raw')


if __name__ == '__main__':
    main()
