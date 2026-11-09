# SPDX-License-Identifier: MIT
"""Compile port callbacks shared by independently overridable features."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from kymo_configurator.model import default_project
from kymo_configurator.port_template import render_port_template
from kymo_configurator.presets import apply_preset

CORE = Path(__file__).resolve().parents[3]


class PortTemplateContractTests(unittest.TestCase):
    def test_shared_callback_survives_either_feature_being_disabled(self):
        env = os.environ.copy()
        fallback = Path.home() / '.platformio/packages/toolchain-gccmingw32/bin'
        env['PATH'] = str(fallback) + os.pathsep + env.get('PATH', '')
        compiler = shutil.which('g++', path=env['PATH'])
        if not compiler:
            self.skipTest('GCC C++11 required')
        project = default_project()
        profile = project['profiles']['development']
        apply_preset(profile, 'trace')
        profile['trace']['transport']['mode'] = 'dedicated_trace'
        profile['trace']['bindings']['transport_busy'] = project['bindings']['busy']
        with tempfile.TemporaryDirectory(prefix='kymo shared port ') as directory:
            root = Path(directory)
            for name, text in render_port_template(project).items():
                # The zero-valued callback stubs are sufficient for this linkage
                # check; remove only the intentional user-implementation barrier.
                lines = [line for line in text.splitlines() if not line.startswith('#error')]
                (root / name).write_text('\n'.join(lines) + '\n', encoding='utf-8')
            (root / 'main.cpp').write_text('''#include "kymo_port.h"
int main() {
    int result = 0;
#if KYMO_EFFECTIVE_RUNTIME || KYMO_EFFECTIVE_TRACE
    result = kymo_port_busy(nullptr);
#endif
    return result;
}
''', encoding='utf-8')
            for runtime, trace in ((1, 1), (0, 1), (1, 0), (0, 0)):
                with self.subTest(runtime=runtime, trace=trace):
                    command = [compiler, '-std=c++11', '-Wall', '-Wextra', '-Werror',
                               '-I' + str(CORE / 'src'),
                               f'-DKYMO_ENABLE_RUNTIME={runtime}', f'-DKYMO_ENABLE_TRACE={trace}',
                               str(root / 'main.cpp'), str(root / 'kymo_port.cpp'),
                               '-o', str(root / 'port-test.exe')]
                    result = subprocess.run(command, env=env, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
