#!/usr/bin/env python3
"""Compile/link the real-engine harness against an existing trainer-enabled Ninja build.

No CMake/source changes, downloaded assets or user save directories are required.
Example: python3 tests/run-trainer-integration.py --resdir /path/to/GOTY
Pass --mobile to exercise the in-game mobile overlay on desktop.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', type=Path, default=Path('build'))
parser.add_argument('--resdir', type=Path, required=True)
parser.add_argument('--mobile', action='store_true')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
build = args.build.resolve()
resources = args.resdir.resolve()
if not (resources / 'main.pak').is_file():
    parser.error('--resdir must contain main.pak')
entries = json.loads((build / 'compile_commands.json').read_text())
entry = next(e for e in entries if Path(e['file']).resolve() == root / 'src/main.cpp')
command = shlex.split(entry['command'])
if '-DPVZ_TRAINER' not in command:
    parser.error('build must have PVZ_TRAINER=ON')
artifacts = build / 'trainer-tests'
artifacts.mkdir(exist_ok=True)
run = Path(tempfile.mkdtemp(prefix='run-', dir=artifacts))
obj = run / 'TrainerIntegration.cpp.o'
exe = run / 'trainer-integration'
command[command.index('-o') + 1] = str(obj)
command[command.index('-c') + 1] = str(root / 'tests/TrainerIntegration.cpp')
subprocess.run(command, cwd=entry['directory'], check=True)
commands = subprocess.check_output(['ninja', '-C', str(build), '-t', 'commands', 'pvz-portable'], text=True)
link = next(line for line in reversed(commands.splitlines()) if 'main.cpp.o' in line and ' -o ' in line)
# Ninja's linker rule wraps the executable invocation in ': && ... && :'.
link = link.removeprefix(': && ').removesuffix(' && :')
link_args = shlex.split(link)
main_obj = next(i for i, value in enumerate(link_args) if value.endswith('/src/main.cpp.o'))
link_args[main_obj] = str(obj)
link_args[link_args.index('-o') + 1] = str(exe)
subprocess.run(link_args, cwd=build, check=True)
env = os.environ.copy()
env['PVZ_TRAINER_TEST_OUT'] = str(run)
(run / 'savedata').mkdir()
argv = [str(exe), '-resdir', str(resources), '-savedir', str(run / 'savedata'), '-trainer']
if args.mobile:
    argv.append('-trainer-mobile')
print('Artifacts:', run, flush=True)
with (run / 'run.log').open('w') as log:
    try:
        result = subprocess.run(argv, cwd=build, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=55)
    except subprocess.TimeoutExpired:
        print((run / 'run.log').read_text(errors='replace'))
        raise SystemExit('FAIL: integration harness exceeded 55 seconds')
log_text = (run / 'run.log').read_text(errors='replace')
for line in log_text.splitlines():
    if line.startswith(('PASS ', 'FAIL ', 'RESULT ')):
        print(line)
if result.returncode or not any(line.startswith('RESULT checks=') and line.endswith('failures=0') for line in log_text.splitlines()):
    print('Last engine log lines:')
    print('\n'.join(log_text.splitlines()[-30:]))
raise SystemExit(result.returncode or (0 if any(line.startswith('RESULT checks=') and line.endswith('failures=0') for line in log_text.splitlines()) else 1))
