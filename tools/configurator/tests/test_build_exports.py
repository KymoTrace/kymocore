# SPDX-License-Identifier: MIT
"""Compile generated snapshots with the real C/C++ toolchain, including overrides."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from kymo_configurator.build_exports import render_exports
from kymo_configurator.generation import generate
from kymo_configurator.model import default_project

CORE = Path(__file__).resolve().parents[3]


class ExportBuildTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.env = os.environ.copy()
        fallback = Path.home() / '.platformio/packages/toolchain-gccmingw32/bin'
        cls.env['PATH'] = str(fallback) + os.pathsep + cls.env.get('PATH', '')
        cls.cxx = shutil.which('g++', path=cls.env['PATH'])
        cls.cc = shutil.which('gcc', path=cls.env['PATH'])
        cls.nm = shutil.which('nm', path=cls.env['PATH'])
        if not all((cls.cxx, cls.cc, cls.nm)):
            raise unittest.SkipTest('GCC C99/C++11 and nm required')

    def run_command(self, *args):
        result = subprocess.run(args, env=self.env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout

    def test_fixed_snapshot_c99_client_and_global_override(self):
        with tempfile.TemporaryDirectory(prefix='kymo snapshot ') as directory:
            root = Path(directory)
            project = default_project()
            snapshot = generate(project, 'development', root / 'generated')
            flags = ['-O0', '-Wall', '-Wextra', '-Werror', '-I' + str(CORE / 'src'),
                     '-I' + str(snapshot), '-DKYMO_USER_CONFIG_HEADER="kymo_user_config.h"']
            client = root / 'client.c'
            client.write_text('''#include <kymo_generated_config.h>
#include <kymo_config.h>
#if KYMO_EFFECTIVE_RUNTIME
uint32_t kymo_port_clock_ms(void *u) { (void)u; return 0; }
uint8_t kymo_port_sample(void *u, uint8_t id, KymoSample *s) {
    (void)u; (void)id; s->value = 1.0f; return 1;
}
uint8_t kymo_port_write(void *u, const uint8_t *p, uint8_t n) {
    (void)u; (void)p; return n;
}
uint8_t kymo_port_busy(void *u) { (void)u; return 0; }
void kymo_port_service(void *u) { (void)u; }
#endif
int main(void) {
    int result = 0;
#if KYMO_EFFECTIVE_RUNTIME
    KymoContext context;
    result = (int)Kymo_Init(&context, &kymo_generated_config);
#endif
    return result;
}
''', encoding='utf-8')
            for enabled in (1, 0, 1):
                build = root / ('on' if enabled else 'off')
                build.mkdir(exist_ok=True)
                selected = flags + [f'-DKYMO_ENABLED={enabled}']
                objects = []
                for source in [*sorted((CORE / 'src').rglob('*.cpp')),
                               snapshot / 'kymo_generated_config.cpp']:
                    target = build / (str(len(objects)) + '-' + source.stem + '.o')
                    self.run_command(self.cxx, '-std=c++11', *selected, '-c', str(source), '-o', str(target))
                    objects.append(str(target))
                client_obj = build / 'client.o'
                self.run_command(self.cc, '-std=c99', *selected, '-c', str(client), '-o', str(client_obj))
                executable = build / 'fixture.exe'
                self.run_command(self.cxx, str(client_obj), *objects, '-o', str(executable))
                self.run_command(str(executable))
                if not enabled:
                    symbols = self.run_command(self.nm, '--defined-only', *objects)
                    for line in symbols.splitlines():
                        if not line.endswith(':'):
                            self.assertNotRegex(line, r'\b[Kk]ymo\w*')

    def test_platformio_export_binds_one_snapshot(self):
        class Environment(dict):
            def Append(self, **values):
                self.update(values)
            def subst(self, value):
                return 'build/production'
            def BuildLibrary(self, output, source, src_filter):
                self['sources'] = (output, source, src_filter)
                return 'generated-library'
        env = Environment()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'kymo_build_export.py'
            from types import SimpleNamespace
            node = SimpleNamespace(abspath=str(path.parent.resolve()))
            scope = {'env': env, 'projenv': env, 'Import': lambda *names: None,
                     'Dir': lambda value: SimpleNamespace(srcnode=lambda: node)}
            exec(compile(render_exports()[path.name], str(path), 'exec'), scope)
            exec(compile(render_exports()['kymo_build_sources.py'], str(path), 'exec'), scope)
            self.assertEqual(env['CPPPATH'], [str(path.parent.resolve())])
            self.assertEqual(env['sources'][1], str(path.parent.resolve()))
            self.assertEqual(env['CPPDEFINES'][0][0], 'KYMO_USER_CONFIG_HEADER')
            self.assertEqual(env['CPPDEFINES'][1], ('KYMO_CONFIG_SNAPSHOT', 'kymo_' + path.parent.resolve().name.replace('-', '_')))

    def test_cmake_export_builds_application_and_core(self):
        cmake = shutil.which('cmake') or shutil.which('cmake', path=str(Path(__import__('sys').executable).parent))
        if not cmake:
            self.skipTest('CMake required')
        with tempfile.TemporaryDirectory(prefix='kymo cmake ') as directory:
            root = Path(directory)
            project = default_project()
            project['profiles']['development']['features'].update(runtime=False, decoder=True, watch=True)
            project['watch'] = {'range_start': 'test_start', 'range_end': 'test_end'}
            snapshot = generate(project, 'development', root / 'generated')
            (root / 'main.cpp').write_text('''#include <kymo_generated_config.h>
#if !KYMO_EFFECTIVE_WATCH
#error wrong profile
#endif
extern "C" {
uint8_t test_start;
uint8_t test_end;
}
int main() { return (int)kymo_generated_watch_init(); }
''')
            sources = ' '.join('"' + p.as_posix() + '"' for p in sorted((CORE / 'src').rglob('*.cpp')))
            (root / 'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.19)
project(KymoSnapshot LANGUAGES CXX)
add_library(core STATIC {sources})
target_include_directories(core PUBLIC "{(CORE / 'src').as_posix()}")
target_compile_features(core PUBLIC cxx_std_11)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE core)
include("{(snapshot / 'kymo_build_export.cmake').as_posix()}")
kymo_configure_targets(app core)
''')
            generator = 'MinGW Makefiles' if os.name == 'nt' else 'Unix Makefiles'
            self.run_command(cmake, '-S', str(root), '-B', str(root / 'build'), '-G', generator,
                             '-DCMAKE_CXX_COMPILER=' + self.cxx)
            self.run_command(cmake, '--build', str(root / 'build'))


if __name__ == '__main__':
    unittest.main()
