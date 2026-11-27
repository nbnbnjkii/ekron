#!/usr/bin/env python3
"""Validate the actual embedded GLES 3 shader pairs; needs glslangValidator."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'sys/android/app/src/main/cpp/Renderer.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='ekron-shaders-') as tmp:
    for vs, fs in [('vertex', 'fragment'), ('skyVertex', 'skyFragment'), ('particleVertex', 'particleFragment'), ('vs', 'fs')]:
        source = (root / ('sys/android/app/src/main/cpp/MilitaryEffects.cpp' if vs == 'particleVertex' else 'sys/android/app/src/main/cpp/LobbyRenderer.h' if vs == 'vs' else 'sys/android/app/src/main/cpp/Renderer.cpp')).read_text()
        paths = []
        for name, suffix in [(vs, '.vert'), (fs, '.frag')]:
            match = re.search(r'const char\s*\*\s*' + name + r'=R"\((.*?)\)";', source, re.S)
            if match is None:
                raise RuntimeError(f'Shader source not found: {name}')
            path = Path(tmp) / (name + suffix)
            path.write_text(match.group(1))
            paths.append(str(path))
        result = subprocess.run(['glslangValidator', '-l', *paths], text=True, capture_output=True)
        print(result.stdout, end='')
        if result.returncode:
            message = (result.stdout + result.stderr).replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
            print('::error::' + message)
            raise SystemExit(result.returncode)
        print(f'{vs} + {fs}: GLSL ES compilation/link validation OK')
