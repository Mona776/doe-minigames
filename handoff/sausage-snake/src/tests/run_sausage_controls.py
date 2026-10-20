#!/usr/bin/env python3
"""Build and run the UI regression using the existing simulator build configuration."""
import os, shlex, subprocess, tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
build = root / 'build_simulator'
flags = {}
for line in (build / 'CMakeFiles/game_simulator.dir/flags.make').read_text().splitlines():
    if ' = ' in line:
        key, value = line.split(' = ', 1)
        flags[key] = value
with tempfile.TemporaryDirectory(prefix='sausage-controls-') as td:
    td = Path(td)
    command = ['/usr/bin/cc']
    for key in ('C_DEFINES', 'C_INCLUDES', 'C_FLAGS'):
        command.extend(shlex.split(flags[key]))
    command.extend(['-c', str(root / 'tests/test_sausage_controls.c'), '-o', str(td / 'controls.o')])
    subprocess.run(command, cwd=build, check=True)
    command = shlex.split((build / 'CMakeFiles/game_simulator.dir/link.txt').read_text())
    command = [arg for arg in command if not arg.endswith('/platform/mac/main.c.o')]
    command.insert(1, str(td / 'controls.o'))
    command[command.index('-o') + 1] = str(td / 'test_controls')
    subprocess.run(command, cwd=build, check=True)
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SIM_NO_AUDIO='1')
    subprocess.run([str(td / 'test_controls')], cwd=root, env=env, check=True)
