"""Проверка точной сборки Monitor-2 и отказа при повреждении результата."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
COMMAND = ['powershell.exe', '-NoProfile', '-File',
           str(ROOT/'scripts/build/build-if-needed.ps1'), 'Monitor2', 'mon', '0xC000']


def build():
    return subprocess.run(COMMAND, cwd=ROOT/'source/monitor2',
                          capture_output=True, text=True, errors='replace')


def main():
    result = build()
    assert result.returncode == 0, result.stdout + result.stderr
    path = ROOT/'bin/Monitor2.mon'
    original = (ROOT/'source/monitor2/originals/Monitor2.mon').read_bytes()
    assert path.read_bytes() == original
    try:
        for name, damaged, message in [
            ('byte', bytes((original[0] ^ 1,)) + original[1:], 'Monitor-2 mismatch at offset 0000h'),
            ('size', original[:-1], 'Monitor-2 size mismatch'),
        ]:
            path.write_bytes(damaged)
            result = build()
            assert result.returncode != 0, f'{name}: повреждение не остановило сборку'
            assert '[up-to-date]' in result.stdout, result.stdout
            assert message in result.stdout + result.stderr, result.stdout + result.stderr
            print(f'{name}: отклонено, код завершения {result.returncode}')
    finally:
        path.write_bytes(original)
    result = build()
    assert result.returncode == 0, result.stdout + result.stderr
    print('Monitor-2: 4096 байт совпадают; обе проверки отказа пройдены')


if __name__ == '__main__':
    main()
