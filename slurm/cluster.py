"""Shared paths and container commands; Python runs on the Slurm host."""
import hashlib
import json
import os
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'slurm/work'
BACKENDS = ('sequential', 'pthreads', 'openmp', 'mpi')
DEFAULT_SIF = '/export/home/acs/prof/stefan_dan.ciocirlan/TMP/DO_NOT_DELETE_IMGS/app-labs.sif'


def environment_command(command, facts=None):
    mode = os.environ.get('USE_CONTAINER', 'auto')
    if mode not in ('auto', '0', '1'):
        raise ValueError('USE_CONTAINER must be auto, 0 or 1')
    image = Path(os.environ.get('SIF', DEFAULT_SIF))
    available = shutil.which('apptainer') and image.is_file()
    if mode == '1' and not available:
        raise RuntimeError('Apptainer/image missing; set SIF or USE_CONTAINER=0')
    if mode == '0' or (mode == 'auto' and not available):
        return list(map(str, command))
    result = ['apptainer', 'exec', '--bind', str(ROOT) + ':' + str(ROOT)]
    if facts:
        result += ['--bind', str(facts) + ':' + str(facts)]
    return result + ['--pwd', str(ROOT), str(image), *map(str, command)]


def atomic_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.tmp-' + str(os.getpid()))
    temporary.write_text(json.dumps(value, indent=2) + '\n')
    temporary.replace(path)


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()
