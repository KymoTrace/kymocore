# SPDX-License-Identifier: MIT
from __future__ import annotations
import tempfile, unittest
from pathlib import Path
from kymo_configurator.generation import check, generate
from kymo_configurator.model import default_project
from kymo_configurator.port_template import create_port_template
from kymo_configurator.resolution import resolve_profile
from kymo_configurator.validation import validate

class EngineTests(unittest.TestCase):
    def test_trace_off_preserves_subfeature_requests_without_enabling_them(self):
        p=default_project()
        p['profiles']['development']['trace'].update(enabled=False,isr=True,scheduler=True,flow=True)
        resolved=resolve_profile(p,'development')
        self.assertTrue(resolved['requested_trace']['isr'])
        self.assertEqual(resolved['effective_trace'], {'enabled':False,'isr':False,'scheduler':False,'flow':False})

    def test_profile_off_preserves_requested_values_but_disables_effective(self):
        p=default_project(); p["profiles"]["production"]["features"]["x"]=True
        r=resolve_profile(p,"production")
        self.assertTrue(r["requested_features"]["x"]); self.assertFalse(r["effective_features"]["x"])

    def test_channel_errors_are_located(self):
        p=default_project(); p["channels"].append(dict(p["channels"][0]))
        self.assertIn("channels[1].id",[d.path for d in validate(p)])

    def test_unverified_scheduler_rejected_but_trace_warned(self):
        p=default_project(); t=p["profiles"]["development"]["trace"]; t.update(enabled=True,isr=True,scheduler=True)
        codes=[d.code for d in validate(p)]
        self.assertIn("port.scheduler_unverified",codes)

    def test_port_scaffold_never_overwrites(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/"port"
            create_port_template(default_project(),path)
            self.assertTrue((path/"kymo_port.cpp").exists())
            with self.assertRaises(FileExistsError): create_port_template(default_project(),path)

    def test_disabled_cpp_profile_generates_without_an_unused_stream_binding(self):
        project = default_project()
        project["profiles"]["production"]["features"]["cpp"] = True
        with tempfile.TemporaryDirectory() as directory:
            generate(project, "production", directory)
            self.assertTrue(check(project, "production", directory)[0])

    def test_generation_snapshot_deterministic_and_checkable(self):
        p=default_project()
        with tempfile.TemporaryDirectory() as d:
            a=generate(p,"development",d); b=generate(p,"development",d)
            self.assertEqual(a,b); self.assertEqual(check(p,"development",d)[0],True)
            (a/"kymo_generated_config.cpp").write_text("manual edit")
            self.assertFalse(check(p,"development",d)[0])
            with self.assertRaisesRegex(ValueError,"edited"):
                generate(p,"development",d)
            repaired=generate(p,"development",d,force=True)
            self.assertTrue(repaired.name.endswith("-repair"))
            self.assertTrue(check(p,"development",d)[0])
            self.assertEqual(repaired,generate(p,"development",d))

if __name__=="__main__": unittest.main()
