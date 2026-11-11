# SPDX-License-Identifier: MIT
"""Compile and execute generated shared-v2 trace wiring against KymoCore."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from kymo_configurator.generation import generate
from kymo_configurator.model import default_project

CORE = Path(__file__).resolve().parents[3]


class GeneratedTraceRoundTripTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fallback = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
        cls.env = os.environ.copy()
        cls.env["PATH"] = str(fallback) + os.pathsep + cls.env.get("PATH", "")
        cls.cxx = shutil.which("g++", path=cls.env["PATH"])
        if not cls.cxx:
            raise unittest.SkipTest("C++11 compiler required")

    def run_command(self, *args):
        result = subprocess.run(args, env=self.env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout

    def test_shared_v2_generated_service_emits_trace_and_data_frames(self):
        project = default_project("GeneratedTraceRoundTrip")
        trace = project["profiles"]["development"]["trace"]
        trace["enabled"] = True
        trace["transport"].update({
            "mode": "shared_v2",
            "max_frame_bytes": 240,
            "drain_events": 12,
            "drain_bytes": 512,
        })
        trace["bindings"].update({
            "clock": "trace_clock",
            "core_id": "trace_core_id",
            "current_context": "trace_context",
            "session_id": "trace_session",
            "trace_write": "trace_write",
            "transport_busy": "trace_busy",
        })

        with tempfile.TemporaryDirectory(prefix="kymo-trace-roundtrip-") as directory:
            root = Path(directory)
            snapshot = generate(project, "development", root / "generated")
            source = root / "roundtrip.cpp"
            source.write_text(r'''#include <kymo_generated_config.h>
#include <KymoCore.h>
#include <cassert>
#include <cstdint>
#include <vector>

static std::vector<std::vector<uint8_t> > frames;

extern "C" uint64_t trace_clock(void *)
{
    static uint64_t ticks = 0u;
    ++ticks;
    return ticks;
}

extern "C" uint8_t trace_core_id(void *) { return 0u; }
extern "C" uint32_t trace_context(void *) { return 1u; }
extern "C" uint64_t trace_session(void) { return 0x123456789abcdef0ULL; }

extern "C" size_t trace_write(void *, const uint8_t *bytes, size_t length)
{
    frames.push_back(std::vector<uint8_t>(bytes, bytes + length));
    return length;
}

extern "C" uint8_t trace_busy(void *) { return 0u; }
extern "C" uint32_t kymo_port_clock_ms(void *) { return 1u; }
extern "C" uint8_t kymo_port_sample(void *, uint8_t, KymoSample *) { return 0u; }
extern "C" uint8_t kymo_port_write(void *, const uint8_t *, uint8_t) { return 0u; }
extern "C" uint8_t kymo_port_busy(void *) { return 0u; }
extern "C" void kymo_port_service(void *) {}

int main()
{
    assert(kymo_generated_trace_init(&frames) == KYMO_OK);
    KymoTrace_Enter(&kymo_generated_trace_recorder, 1u, 1u);
    KymoTrace_Exit(&kymo_generated_trace_recorder, 1u, 1u);

    const uint8_t legacy_frame[9] = {
        0xa5u, 0x5au, 0x40u, 7u, 0u, 0u, 0x80u, 0x3fu, 0u
    };
    assert(KymoMuxDataSlot_Write(
        &kymo_generated_trace_data_adapter, legacy_frame, 9u) == 9u);

    for (uint16_t cycle = 0u; cycle < 32u; ++cycle) {
        kymo_generated_trace_service();
    }

    bool saw_metadata = false;
    bool saw_dictionary = false;
    bool saw_batch = false;
    bool saw_data = false;
    for (size_t frame_index = 0u; frame_index < frames.size(); ++frame_index) {
        const std::vector<uint8_t> &frame = frames[frame_index];
        assert(frame.size() >= KYMO_TRACE_ENVELOPE_SIZE);
        assert(frame[0] == 0xa5u && frame[1] == 0x5au);
        const uint16_t payload_length = static_cast<uint16_t>(frame[4]) |
            (static_cast<uint16_t>(frame[5]) << 8);
        assert(frame.size() == KYMO_TRACE_ENVELOPE_SIZE + payload_length);
        const uint16_t header_crc = static_cast<uint16_t>(frame[20]) |
            (static_cast<uint16_t>(frame[21]) << 8);
        const uint16_t frame_crc = static_cast<uint16_t>(frame[22u + payload_length]) |
            (static_cast<uint16_t>(frame[23u + payload_length]) << 8);
        assert(header_crc == KymoTrace_Crc16(frame.data() + 2u, 18u));
        assert(frame_crc == KymoTrace_Crc16(frame.data() + 2u, 20u + payload_length));
        saw_metadata = saw_metadata || frame[3] == KYMO_TRACE_PAYLOAD_SESSION_META;
        saw_dictionary = saw_dictionary || frame[3] == KYMO_TRACE_PAYLOAD_DICTIONARY;
        saw_batch = saw_batch || frame[3] == KYMO_TRACE_PAYLOAD_BATCH;
        saw_data = saw_data || frame[3] == KYMO_TRACE_PAYLOAD_DATA_V1;
    }
    assert(saw_metadata && saw_dictionary && saw_batch && saw_data);
    assert(KymoMuxDataSlot_IsBusy(
        &kymo_generated_trace_data_adapter) == 0u);
    return 0;
}
''', encoding="utf-8")

            flags = [
                "-std=c++11", "-Wall", "-Wextra", "-Werror", "-O0",
                "-I" + str(CORE / "src"), "-I" + str(snapshot),
                '-DKYMO_USER_CONFIG_HEADER="kymo_user_config.h"',
            ]
            objects = []
            sources = sorted((CORE / "src").rglob("*.cpp")) + [
                snapshot / "kymo_generated_config.cpp"
            ]
            for source_path in sources:
                object_path = root / (str(len(objects)) + "-" + source_path.stem + ".o")
                self.run_command(self.cxx, *flags, "-c", str(source_path),
                                 "-o", str(object_path))
                objects.append(str(object_path))

            client_object = root / "roundtrip.o"
            self.run_command(self.cxx, *flags, "-c", str(source),
                             "-o", str(client_object))
            executable = root / "roundtrip.exe"
            self.run_command(self.cxx, str(client_object), *objects,
                             "-o", str(executable))
            self.run_command(str(executable))


if __name__ == "__main__":
    unittest.main()
