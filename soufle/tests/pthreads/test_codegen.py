#!/usr/bin/env python3
"""End-to-end tests for generated pthread code, independent of OpenMP."""
import argparse
from collections import defaultdict
from pathlib import Path
import runpy
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]


def run(command, **kwargs):
    return subprocess.run(list(map(str, command)), check=True, text=True, capture_output=True, **kwargs)


def tables(directory):
    return {p.stem: {tuple(map(int, row.split())) for row in p.read_text().splitlines() if row.strip()}
            for p in directory.glob('*.csv')}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--souffle', type=Path, required=True)
    parser.add_argument('--project', type=Path, default=ROOT.parent / 'parallel_datalog')
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--work-dir', type=Path)
    args = parser.parse_args()
    compiler = args.souffle.resolve()
    project = args.project.resolve()
    with tempfile.TemporaryDirectory(prefix='souffle-pthreads-') as temporary:
        work = args.work_dir.resolve() if args.work_dir else Path(temporary)
        work.mkdir(parents=True, exist_ok=True)

        def compile_source(source, binary, extra=()):
            run([args.cxx, '-std=c++17', '-O1', '-g', '-pthread', '-DRAM_DOMAIN_SIZE=64',
                 '-I' + str(ROOT / 'src/include'), *extra, source, '-o', binary])

        runtime = work / 'runtime_test'
        compile_source(ROOT / 'tests/pthreads/runtime.cpp', runtime, ['-DSOUFFLE_USE_PTHREADS'])
        print(run([runtime], timeout=30).stdout.strip(), flush=True)

        fixture = runpy.run_path(str(project / 'tests/make_cases.py'))
        cases = dict(fixture['CASES'])
        cases.update(fixture['random_cases']())
        count = 512
        cases['cspa_chain'] = ('cspa', {
            'assign': [(i, i + 1) for i in range(count - 1)],
            'dereference': [(i, count + i) for i in range(0, count, 8)],
        }, None)
        grouped = defaultdict(list)
        for name, (rule, facts, expected) in cases.items():
            fixture['write'](work, name, rule, facts, expected)
            grouped[rule].append((name, expected))

        for rule, fixtures in grouped.items():
            binaries = {}
            for backend in ['sequential', 'pthreads']:
                source = work / f'{rule}_{backend}.cpp'
                binary = source.with_suffix('')
                run([compiler, '--execution-backend=' + backend, '-g', source,
                     project / 'rules' / (rule + '.dl')])
                text = source.read_text()
                if backend == 'pthreads':
                    assert 'souffle::pthread_runtime::parallelFor' in text, rule
                    assert 'PARALLEL_START' not in text and 'pfor(' not in text, rule
                else:
                    assert 'SOUFFLE_USE_PTHREADS' not in text, rule
                compile_source(source, binary)
                binaries[backend] = binary

            for name, expected in fixtures:
                facts = work / name / 'facts'
                reference_dir = work / name / 'serial'
                reference_dir.mkdir(exist_ok=True)
                run([binaries['sequential'], '-F', facts, '-D', reference_dir], timeout=120)
                reference = tables(reference_dir)
                assert reference, name
                if expected:
                    assert reference == {key: set(rows) for key, rows in expected.items()}, name
                for mode, threads in [('sequential', 4), ('pthreads', 1), ('pthreads', 2), ('pthreads', 4)]:
                    out = work / name / f'{mode}_{threads}'
                    out.mkdir(exist_ok=True)
                    command = [binaries['pthreads'], '--execution-mode=' + mode, '-j', threads,
                               '-F', facts, '-D', out]
                    if name == 'cspa_chain' and mode == 'pthreads' and threads == 4:
                        process = subprocess.Popen(list(map(str, command)), stdout=subprocess.PIPE,
                                                   stderr=subprocess.PIPE, text=True)
                        peak = 0
                        deadline = time.monotonic() + 120
                        while process.poll() is None:
                            if time.monotonic() > deadline:
                                process.kill()
                                raise AssertionError('CSPA worker execution timed out')
                            try:
                                peak = max(peak, len(list(Path(f'/proc/{process.pid}/task').iterdir())))
                            except FileNotFoundError:
                                pass
                            time.sleep(0.002)
                        stdout, stderr = process.communicate()
                        assert process.returncode == 0, stderr
                        assert peak > 1, 'CSPA did not execute pthread workers'
                        print(f'PASS CSPA executed workers: peak {peak} OS threads', flush=True)
                    else:
                        run(command, timeout=120)
                    assert tables(out) == reference, (name, mode, threads)
                # Repeat concurrent insertion on the large fixture to catch intermittent failures.
                if name == 'cspa_chain':
                    for _ in range(3):
                        run([binaries['pthreads'], '-j', 4, '-F', facts, '-D', out], timeout=120)
                        assert tables(out) == reference, name
                print(f'PASS {name}: identical complete tables in all execution modes', flush=True)

            failure = subprocess.run([str(binaries['pthreads']), '--execution-mode=invalid'], capture_output=True)
            assert failure.returncode != 0

        invalid = subprocess.run([str(compiler), '--execution-backend=invalid', '-g', str(work / 'bad.cpp'),
                                  str(project / 'rules/cspa.dl')], capture_output=True)
        assert invalid.returncode != 0
        print('PASS invalid backend/runtime selections', flush=True)


if __name__ == '__main__':
    main()
