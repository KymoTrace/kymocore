# SPDX-License-Identifier: MIT
"""Build generated CPP-only wiring against KymoCore's actual C++ API."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from kymo_configurator.generation import generate
from kymo_configurator.model import default_project

CORE = Path(__file__).resolve().parents[3]


class CppSenderGenerationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fallback = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
        cls.env = os.environ.copy()
        cls.env["PATH"] = str(fallback) + os.pathsep + cls.env.get("PATH", "")
        cls.cxx = shutil.which("g++", path=cls.env["PATH"])
        cls.nm = shutil.which("nm", path=cls.env["PATH"])
        if not cls.cxx or not cls.nm:
            raise unittest.SkipTest("C++11 compiler and nm required")

    def run_command(self, *args, expected=0):
        result = subprocess.run(args, env=self.env, capture_output=True, text=True)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result.stdout

    def project(self, *, timestamp, timestamp_default=None):
        project = default_project("CppOnly")
        features = project["profiles"]["development"]["features"]
        features["runtime"] = False
        features["cpp"] = True
        features["timestamp"] = timestamp
        project["bindings"]["cpp_stream"] = "kymo_application_stream"
        if timestamp_default is not None:
            project["bindings"]["cpp_timestamp_default"] = timestamp_default
        if timestamp:
            project["bindings"]["cpp_clock"] = "kymo_application_clock"
        return project

    def build_case(self, directory, *, timestamp, timestamp_default=None, expected_timestamp):
        root = Path(directory)
        snapshot = generate(
            self.project(timestamp=timestamp, timestamp_default=timestamp_default),
            "development", root / "generated",
        )
        source = root / "main.cpp"
        source.write_text(r'''#include <kymo_generated_config.h>
#include <KymoCore.h>
class TestStream : public KymoStream {
public:
    size_t write(const uint8_t *, size_t length) override { bytes += length; return length; }
    bool busy() const override { return false; }
    size_t bytes = 0;
};
static TestStream test_stream;
KymoStream &kymo_application_stream = test_stream;
static uint32_t clock_calls = 0;
uint32_t kymo_application_clock(void) { ++clock_calls; return 100U + clock_calls; }
int main() {
    kymo_generated_init_cpp_sender();
    if (kymo_generated_sender.isTimestampEnabled() != EXPECTED_TIMESTAMP) return 1;
#if KYMO_EFFECTIVE_TIMESTAMP
    if (clock_calls != 1U) return 2;
    kymo_generated_sender.setTimestampEnabled(true);
    if (kymo_generated_sender.send(3U, 1.0f, true) != KYMO_OK) return 3;
    if (clock_calls != 2U || test_stream.bytes != 13U) return 4;
#else
    if (clock_calls != 0U) return 5;
    if (kymo_generated_sender.send(3U, 1.0f) != KYMO_OK) return 6;
    if (test_stream.bytes != 9U) return 7;
#endif
    return 0;
}
'''.replace("EXPECTED_TIMESTAMP", "true" if expected_timestamp else "false"), encoding="utf-8")
        flags = [
            "-std=c++11", "-Wall", "-Wextra", "-Werror", "-O0",
            "-I" + str(CORE / "src"), "-I" + str(snapshot),
            '-DKYMO_USER_CONFIG_HEADER="kymo_user_config.h"',
        ]
        objects = []
        for source_path in sorted((CORE / "src").rglob("*.cpp")) + [snapshot / "kymo_generated_config.cpp"]:
            obj = root / (str(len(objects)) + "-" + source_path.stem + ".o")
            self.run_command(self.cxx, *flags, "-c", str(source_path), "-o", str(obj))
            objects.append(str(obj))
        client_obj = root / "main.o"
        self.run_command(self.cxx, *flags, "-c", str(source), "-o", str(client_obj))
        executable = root / "cpp-only.exe"
        self.run_command(self.cxx, str(client_obj), *objects, "-o", str(executable))
        self.run_command(str(executable))
        generated = (snapshot / "kymo_generated_config.cpp").read_text(encoding="utf-8")
        self.assertIn("extern KymoStream &kymo_application_stream;", generated)
        self.assertIn("kymo_generated_sender.begin(kymo_application_stream, " +
                      ("true" if expected_timestamp else "false") + ");", generated)

    def test_cpp_only_runtime_off_timestamp_off_forces_default_off(self):
        with tempfile.TemporaryDirectory(prefix="kymo-cpp-only-off-") as directory:
            # A saved true preference is ineffective when the profile has no timestamp feature.
            self.build_case(directory, timestamp=False, expected_timestamp=False)

    def test_cpp_only_runtime_off_timestamp_binding_can_be_enabled_or_disabled(self):
        for default_enabled in (True, False):
            with self.subTest(default_enabled=default_enabled), tempfile.TemporaryDirectory(
                prefix="kymo-cpp-only-timestamp-"
            ) as directory:
                self.build_case(directory, timestamp=True, timestamp_default=default_enabled,
                                expected_timestamp=default_enabled)

    def test_global_off_override_removes_generated_cpp_sender(self):
        with tempfile.TemporaryDirectory(prefix="kymo-cpp-only-global-off-") as directory:
            root = Path(directory)
            snapshot = generate(self.project(timestamp=False),
                                "development", root / "generated")
            source = root / "off.cpp"
            source.write_text('#include <kymo_generated_config.h>\n'
                              '#if KYMO_EFFECTIVE_CPP\n#error sender should be globally off\n#endif\n'
                              'int main() { return 0; }\n', encoding="utf-8")
            flags = ["-std=c++11", "-O0", "-DKYMO_ENABLED=0", "-I" + str(CORE / "src"),
                     "-I" + str(snapshot), '-DKYMO_USER_CONFIG_HEADER="kymo_user_config.h"']
            objects = []
            for source_path in sorted((CORE / "src").rglob("*.cpp")) + [snapshot / "kymo_generated_config.cpp"]:
                obj = root / (str(len(objects)) + "-" + source_path.stem + ".o")
                self.run_command(self.cxx, *flags, "-c", str(source_path), "-o", str(obj))
                objects.append(str(obj))
            client_obj = root / "off.o"
            self.run_command(self.cxx, *flags, "-c", str(source), "-o", str(client_obj))
            executable = root / "off.exe"
            self.run_command(self.cxx, str(client_obj), *objects, "-o", str(executable))
            self.run_command(str(executable))
            symbol_output = self.run_command(self.nm, "--defined-only", *objects)
            symbols = "\n".join(line for line in symbol_output.splitlines() if not line.endswith(":"))
            self.assertNotRegex(symbols, r"\b[Kk]ymo\w*")


if __name__ == "__main__":
    unittest.main()
