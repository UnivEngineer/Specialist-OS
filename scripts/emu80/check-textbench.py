"""TEXTBEN: launch from NC, verify four measurements and return to the shell."""
import argparse
import json
import os
from pathlib import Path
import runpy
import time

ROOT = Path(__file__).resolve().parents[2]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime', type=Path)
parser.add_argument('--port', type=int, choices=range(19781, 19797), default=19782)
parser.add_argument('--report', type=Path, default=ROOT / 'docs/bench/text-current')
args = parser.parse_args()
runtime = args.runtime or Path(os.environ['LOCALAPPDATA']) / 'Specialist-OS/emu80' / str(args.port)
output = args.report.resolve()
output.mkdir(parents=True, exist_ok=True)

helpers = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
matrix_class = helpers['Matrix']
settings = matrix_class.__init__.__globals__
settings.update(OUT=output, RUNTIME=runtime.resolve())
settings['mcp_owner'] = lambda: helpers['shell'](
    f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; '
    'if($p){$p[0].OwningProcess};exit 0'
)
matrix = matrix_class()
machine = matrix.m
machine.url = f'http://127.0.0.1:{args.port}/mcp'

symbols = helpers['symbols']
textbench = symbols(ROOT / 'bin/TEXTCOM.sym')
nc = symbols(ROOT / 'bin/NC.sym')
kernel = symbols(ROOT / 'bin/banked/KERNEL.sym')


def dos_state():
    machine.write(0xFFFD, b'\0')
    try:
        allocations = machine.read(kernel['MEM_ALLOC_TABLE'], kernel['MEM_ALLOC_RECORDS'] * 6)
        modules = machine.read(kernel['MODULE_TABLE'], kernel['MODULE_MAX'] * kernel['MODULE_RECORD_SIZE'])
        timer_owner = machine.read(kernel['timerOwner'], 1)
    finally:
        machine.write(0xFFFC, b'\0')
    # The relaunched MAIN NC obtains a new owner ID. Compare live ranges and
    # sizes, keeping the number and order of allocation records.
    occupied = [allocations[i:i + 5] for i in range(0, len(allocations), 6) if allocations[i] != 255]
    stride = kernel['MODULE_RECORD_SIZE']
    loaded = [modules[i:i + stride] for i in range(0, len(modules), stride) if modules[i:i + 2] != b'\0\0']
    return occupied, loaded, timer_owner


try:
    matrix.start('textbench', disk=ROOT / 'images/H-Flash-2M-Games.bin')
    initial = dos_state()
    machine.call('bp_clear')
    machine.call('emu_run_for', ms=500)
    machine.tap('esc')
    machine.call('emu_run_for', ms=250)
    command = 'H:/BENCH/FONTBEN/TEXTBEN.COM'
    keys = {'.': 'period', ':': 'colon', '/': 'slash'}
    for character in command:
        machine.tap(keys.get(character, character.lower()))
    assert machine.read(nc['CMD_LINE'], len(command)).upper() == command.encode()
    machine.screenshot(output / 'command.png')
    machine.stop_at(textbench['entry'])
    machine.tap('enter')
    machine.until(textbench['entry'], 120, page=0)
    assert machine.read(0, 32) == (ROOT / 'bin/TEXTCOM.i80').read_bytes()[:32]

    machine.stop_at(textbench['waitKey'])
    machine.call('emu_run')
    deadline = time.monotonic() + 180
    while time.monotonic() < deadline:
        if machine.call('cpu_state')['registers']['pc']['dec'] == textbench['waitKey']:
            break
        time.sleep(0.25)
    else:
        raise RuntimeError('TEXTBEN timeout')

    counters = [('mainChar', 'charTicks'), ('mainString', 'stringTicks'),
                ('bankChar', 'bankCharTicks'), ('bankString', 'v_elapsed')]
    times = {name: round(int.from_bytes(machine.read(textbench[counter], 4), 'little') * .128, 3)
             for name, counter in counters}
    machine.screenshot(output / 'results.png')
    assert machine.read(textbench['bankHandle'], 2) == b'\0\0', 'module not unloaded'
    assert all(value > 0 for value in times.values()), times
    assert dos_state()[2] == initial[2] == b'\0', 'timer lease retained during exit prompt'

    machine.call('bp_clear')
    machine.call('emu_run_for', ms=500)
    machine.stop_at(nc['mainLoop'])
    machine.tap('enter')
    machine.until(nc['mainLoop'], 120, page=0)
    machine.screenshot(output / 'returned.png')
    assert dos_state() == initial, 'allocation, module or timer state changed after returning to NC'

    result = dict(ms=times, returnedToNC=True, publicModuleABI=True,
                  modulesUnloaded=True, allocationLeaks=0, timerReleased=True)
    (output / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(times, flush=True)
finally:
    matrix.stop()
