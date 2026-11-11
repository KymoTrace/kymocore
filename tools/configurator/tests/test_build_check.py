# SPDX-License-Identifier: MIT
import os
from pathlib import Path
import shutil
import tempfile
import unittest
from kymo_configurator.build_check import check_commands

CORE = Path(__file__).resolve().parents[3]


class CompilerAuditTests(unittest.TestCase):
    def test_actual_translation_units_and_mismatch(self):
        compiler = shutil.which('g++')
        fallback = Path.home() / '.platformio/packages/toolchain-gccmingw32/bin/g++.exe'
        compiler = compiler or (str(fallback) if fallback.exists() else None)
        if not compiler:
            self.skipTest('g++ required')
        old_path = os.environ['PATH']
        os.environ['PATH'] = str(Path(compiler).parent) + os.pathsep + old_path
        try:
            with tempfile.TemporaryDirectory(prefix='audit spaces ') as directory:
                source = Path(directory) / 'app.cpp'
                source.write_text('#include <kymo_config.h>\nint app() { return 0; }\n')
                entries = [{'directory': directory, 'file': str(path),
                            'arguments': [compiler, '-std=c++11', '-I' + str(CORE / 'src'),
                                          '-DKYMO_ENABLED=0', '-c', str(path), '-o', str(Path(directory) / 'out.o')]}
                           for path in (source, CORE / 'src/telemetry/kymo_protocol.cpp')]
                reports = check_commands(entries)
                self.assertEqual(reports[0]['values']['KYMO_ENABLED'], 0)
                entries[1]['arguments'][3] = '-DKYMO_ENABLED=1'
                with self.assertRaisesRegex(ValueError, 'Configuration mismatch'):
                    check_commands(entries)
                entries[1]['arguments'][3] = '-DKYMO_MAX_CHANNELS=2'
                entries[0]['arguments'][3] = '-DKYMO_MAX_CHANNELS=1'
                with self.assertRaisesRegex(ValueError, 'KYMO_MAX_CHANNELS'):
                    check_commands(entries)
                entries[0]['arguments'][3] = '-DKYMO_ENABLE_TX=0'
                entries[1]['arguments'][3] = '-DKYMO_ENABLE_TX=1'
                with self.assertRaisesRegex(ValueError, 'KYMO_(ENABLE|EFFECTIVE)_TX'):
                    check_commands(entries)
                for index, entry in enumerate(entries):
                    entry['arguments'].append('-DKYMO_ENABLE_TX=1')
                    entry['arguments'][3] = f'-DKYMO_ENABLE_ENDPOINT={index}'
                with self.assertRaisesRegex(ValueError, 'KYMO_(ENABLE|EFFECTIVE)_ENDPOINT'):
                    check_commands(entries)
                entries[0]['arguments'][3] = '-DKYMO_MAX_CHANNELS=16'
                for expression in ('16u', '0x10', '(8+8)', '(1u << 4)', '(32/2)'):
                    with self.subTest(size_expression=expression):
                        entries[1]['arguments'][3] = '-DKYMO_MAX_CHANNELS=' + expression
                        reports = check_commands(entries)
                        self.assertEqual(reports[1]['values']['KYMO_MAX_CHANNELS'], 16)
                for index, entry in enumerate(entries):
                    header_dir = Path(directory) / str(index)
                    header_dir.mkdir()
                    (header_dir / 'kymo_user_config.h').write_text('#define KYMO_ENABLED 1\n')
                    entry['arguments'][3:4] = ['-I' + str(header_dir),
                                                '-DKYMO_USER_CONFIG_HEADER="kymo_user_config.h"']
                with self.assertRaisesRegex(ValueError, 'KYMO_USER_CONFIG_HEADER'):
                    check_commands(entries)
        finally:
            os.environ['PATH'] = old_path
