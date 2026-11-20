#!/usr/bin/env python3
"""Compare complete MPI-generated tables with the pure sequential backend."""
import argparse
from collections import defaultdict
import os
from pathlib import Path
import runpy
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def run(command, **kwargs):
    result = subprocess.run(list(map(str, command)), text=True, capture_output=True,
                            timeout=180, **kwargs)
    if result.returncode:
        raise AssertionError(f'{command}\n{result.stdout}\n{result.stderr}')
    return result


def tables(directory):
    return {p.stem: set(p.read_text().splitlines()) for p in directory.glob('*.csv')}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--souffle', type=Path, required=True)
    parser.add_argument('--work-dir', type=Path)
    parser.add_argument('--mpicxx', default='mpicxx')
    parser.add_argument('--mpirun', default='mpirun')
    args = parser.parse_args()
    compiler = args.souffle.resolve()
    env = dict(os.environ, OMPI_ALLOW_RUN_AS_ROOT='1', OMPI_ALLOW_RUN_AS_ROOT_CONFIRM='1')
    with tempfile.TemporaryDirectory(prefix='souffle-mpi-') as temporary:
        work = args.work_dir.resolve() if args.work_dir else Path(temporary)
        work.mkdir(parents=True, exist_ok=True)

        def mpi(binary, ranks, *options):
            return [args.mpirun, '--oversubscribe', '-np', ranks, binary, *options]

        def compile_source(cxx, source, binary):
            run([cxx, '-std=c++17', '-O1', '-pthread', '-DRAM_DOMAIN_SIZE=64',
                 '-I' + str(ROOT / 'src/include'), source, '-o', binary])

        runtime = work / 'runtime'
        compile_source(args.mpicxx, ROOT / 'tests/mpi/runtime.cpp', runtime)
        for ranks in (1, 2, 4):
            for mode in ('owned', 'external'):
                print(run(mpi(runtime, ranks, mode), env=env).stdout.strip(), ranks, mode, flush=True)
        for mode in ('exit', 'exception'):
            result = subprocess.run(list(map(str, mpi(runtime, 2, mode))), env=env,
                                    capture_output=True, text=True, timeout=30)
            assert result.returncode != 0, mode
        print('PASS rank failure terminates peers', flush=True)

        fixture = runpy.run_path(str(ROOT.parent / 'parallel_datalog/tests/make_cases.py'))
        cases = dict(fixture['CASES'])
        cases.update(fixture['random_cases']())
        cases['cspa_empty'] = ('cspa', {'assign': [], 'dereference': []}, None)
        cases['cspa_chain'] = ('cspa', {
            'assign': [(i, i + 1) for i in range(511)],
            'dereference': [(i, 512 + i) for i in range(0, 512, 8)]}, None)
        grouped = defaultdict(list)
        for name, (rule, facts, expected) in cases.items():
            fixture['write'](work, name, rule, facts, expected)
            grouped[rule].append(name)

        for rule, names in grouped.items():
            binaries = {}
            for backend in ('sequential', 'mpi'):
                source = work / f'{rule}_{backend}.cpp'
                binary = source.with_suffix('')
                run([compiler, '--execution-backend=' + backend, '-g', source,
                     ROOT.parent / 'parallel_datalog/rules' / (rule + '.dl')])
                text = source.read_text()
                if backend == 'mpi':
                    assert 'mpi_runtime::synchronize<' in text, rule
                    assert 'mpi_runtime::owns(env0)' in text, rule
                    assert '.push_back(tuple[' in text, rule
                    assert 'pthread_runtime' not in text and 'PARALLEL_START' not in text
                else:
                    assert 'mpi_runtime' not in text
                compile_source(args.mpicxx if backend == 'mpi' else 'g++', source, binary)
                binaries[backend] = binary
            for name in names:
                facts = work / name / 'facts'
                reference_dir = work / name / 'sequential'
                reference_dir.mkdir(exist_ok=True)
                run([binaries['sequential'], '-F', facts, '-D', reference_dir])
                reference = tables(reference_dir)
                assert reference, name
                for ranks in (1, 2, 4):
                    out = work / name / f'mpi_{ranks}'
                    out.mkdir(exist_ok=True)
                    run(mpi(binaries['mpi'], ranks, '-F', facts, '-D', out), env=env)
                    assert tables(out) == reference, (name, ranks)
                print(f'PASS {name}: complete tables equal at 1, 2, 4 ranks', flush=True)

        # Aggregates and outer existence scans stay replicated; integer negation,
        # nullaries, high-arity fallback, and multiple strata must stay in sync.
        source_dl = work / 'fallback.dl'
        source_dl.write_text('''.decl A(x:number)
.decl B(x:number)
.decl Count(n:number)
.decl Present()
.decl Wide(a:number,b:number,c:number,d:number,e:number,f:number,g:number)
.decl Unsigned(x:unsigned)
.decl Seed(k:number,v:number)
.decl Selected(v:number)
.output B
.output Count
.printsize Count
.output Present
.output Wide
.output Unsigned
.output Selected
A(-1). A(2). A(3).
B(x) :- A(x), x > 0.
Count(n) :- n = count : B(_).
Present() :- B(_).
Wide(x,x,x,x,x,x,x) :- B(x).
Unsigned(4294967297).
Seed(7,10). Seed(7,20). Seed(8,99).
Selected(v) :- Seed(7,v).
''')
        fallback_stdout = {}
        for backend in ('sequential', 'mpi'):
            source = work / f'fallback_{backend}.cpp'
            run([compiler, '--execution-backend=' + backend, '-g', source, source_dl])
            compile_source(args.mpicxx if backend == 'mpi' else 'g++', source, source.with_suffix(''))
            if backend == 'mpi':
                assert 'for (const auto& env0 : range)' in source.read_text(), 'missing indexed outer scan'
            out = work / f'fallback_out_{backend}'
            out.mkdir(exist_ok=True)
            command = [source.with_suffix(''), '-D', out]
            fallback_stdout[backend] = run(
                mpi(command[0], 4, *command[1:]) if backend == 'mpi' else command, env=env).stdout
        assert tables(work / 'fallback_out_mpi') == tables(work / 'fallback_out_sequential')
        assert fallback_stdout['mpi'] == fallback_stdout['sequential']
        assert len(fallback_stdout['mpi'].splitlines()) == 1, 'duplicate printsize output'
        print('PASS indexed scans, fallback queries, nullaries, wide/unsigned values, rank-0 printsize', flush=True)

        for declaration in ('.decl S(x:symbol)\nS("text").\n.output S\n',
                            '.decl F(x:float)\nF(1.5).\n.output F\n',
                            '.decl O(x:number)\nO(ord("text")).\n.output O\n',
                            '.decl C(x:number)\nC($).\n.output C\n'):
            source_dl.write_text(declaration)
            bad = subprocess.run([str(compiler), '--execution-backend=mpi', '-g',
                                  str(work / 'bad.cpp'), str(source_dl)], capture_output=True)
            assert bad.returncode != 0
        for options in ([], ['-o', str(work / 'bad')], ['-g', str(work / 'bad.cpp'), '-j', '2'],
                        ['-g', str(work / 'bad.cpp'), '-p', str(work / 'profile')]):
            bad = subprocess.run([str(compiler), '--execution-backend=mpi', *options,
                                  str(source_dl)], capture_output=True)
            assert bad.returncode != 0
        # Existing I/O exit paths must not strand another rank in a collective.
        bad = subprocess.run(list(map(str, mpi(binaries['mpi'], 2, '-F', work / 'missing'))),
                             env=env, capture_output=True, text=True, timeout=30)
        assert bad.returncode != 0
        print('PASS unsupported programs/options and input errors', flush=True)


if __name__ == '__main__':
    main()
