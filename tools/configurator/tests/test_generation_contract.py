# SPDX-License-Identifier: MIT
"""Behavioral checks for publication, validation and damaged snapshot recovery."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from kymo_configurator.model import default_project, load_project
from kymo_configurator.generation import check, generate
from kymo_configurator.validation import validate


class GenerationContractTests(unittest.TestCase):
    def test_force_repairs_without_mutating_old_snapshot(self):
        project = default_project()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = generate(project, 'development', root)
            header = first / 'kymo_user_config.h'
            header.write_text('/* manual edit */\n')
            with self.assertRaises(ValueError):
                generate(project, 'development', root)
            repaired = generate(project, 'development', root, force=True)
            self.assertNotEqual(first, repaired)
            self.assertEqual(header.read_text(), '/* manual edit */\n')
            self.assertTrue(check(project, 'development', root)[0])
            self.assertEqual(generate(project, 'development', root), repaired)

    def test_interrupted_publication_preserves_previous_selection(self):
        project = default_project()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            generate(project, 'development', root)
            selected = (root / 'active_generation.json').read_bytes()
            changed = copy.deepcopy(project)
            changed['channels'][0]['period_ms'] = 40
            with patch('kymo_configurator.generation.os.replace', side_effect=OSError('simulated disk error')):
                with self.assertRaises(OSError):
                    generate(changed, 'development', root)
            self.assertEqual((root / 'active_generation.json').read_bytes(), selected)
            self.assertTrue(check(project, 'development', root)[0])

    def test_malformed_nested_objects_report_diagnostics(self):
        for field in ('profiles', 'bindings', 'port', 'build', 'resources', 'channels', 'symbols', 'contexts'):
            for bad in (None, 1, 'bad'):
                with self.subTest(field=field, bad=bad):
                    project = default_project()
                    project[field] = bad
                    self.assertTrue(any(d.severity == 'error' for d in validate(project, 'development')))

    def test_selector_cannot_escape_generation_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            generate(default_project(), 'development', root)
            (root / 'active_generation.json').write_text(json.dumps({'generation': '../outside'}))
            self.assertFalse(check(default_project(), 'development', root)[0])

    def test_rejects_missing_or_unowned_snapshot_files(self):
        project = default_project()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            snapshot = generate(project, 'development', root)
            (snapshot / 'unexpected.txt').touch()
            self.assertFalse(check(project, 'development', root)[0])
            repaired = generate(project, 'development', root, force=True)
            self.assertNotEqual(snapshot, repaired)
            (repaired / 'kymo_generation_manifest.json').unlink()
            self.assertFalse(check(project, 'development', root)[0])
            with self.assertRaisesRegex(ValueError, '--force'):
                generate(project, 'development', root)

    def test_validation_catches_values_that_cannot_be_rendered(self):
        project = default_project()
        project['watch'] = {'symbols': []}
        project['port']['kind'] = []
        project['build']['output_dir'] = r'C:outside'
        project['profiles']['development']['preset'] = []
        project['profiles']['development']['trace']['buffer']['events_per_core'] = None
        project['bindings']['clock'] = 'class'
        project['symbols'] = [{'id': 1, 'name': 'read', 'kind': 'function'},
                              {'id': 2, 'name': 'READ', 'kind': []}]
        codes = {d.code for d in validate(project)}
        self.assertTrue({'watch.symbols', 'port.unknown', 'build.path', 'profile.preset',
                         'trace.buffer_limit', 'binding.required', 'registry.kind',
                         'registry.duplicate_name'} <= codes)

    def test_trace_record_limit_is_a_validation_error(self):
        from kymo_configurator.presets import apply_preset
        project = default_project()
        apply_preset(project['profiles']['development'], 'trace')
        project['symbols'] = [{'id': n, 'name': f'symbol_{n}_' + 'x' * 100, 'kind': 'function'}
                              for n in range(1, 100)]
        self.assertIn('trace.admin_limit', {d.code for d in validate(project)})

    def test_project_reader_accepts_bom_and_rejects_ambiguous_json(self):
        with tempfile.TemporaryDirectory() as directory:
            project = Path(directory) / 'project.json'
            project.write_text('{"project_name": "example"}', encoding='utf-8-sig')
            self.assertEqual(load_project(project), {'project_name': 'example'})
            for contents in ('{"profiles": {}, "profiles": []}', '{"period": NaN}'):
                project.write_text(contents, encoding='utf-8')
                with self.assertRaises(ValueError):
                    load_project(project)
