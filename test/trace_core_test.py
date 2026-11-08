# SPDX-License-Identifier: MIT
#!/usr/bin/env python3
"""Build C99/C++11 trace API, recorder, mux and resource-gating tests."""
import argparse
import json
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRACE_SOURCES = ["src/trace/kymo_trace.cpp", "src/trace/kymo_trace_protocol.cpp"]
MUX_SOURCE = "src/transport/kymo_mux.cpp"
TRACE_FLAGS = ["-DKYMO_ENABLE_TRACE=1", "-DKYMO_ENABLE_TRACE_ISR=1",
               "-DKYMO_ENABLE_TRACE_SCHEDULER=1", "-DKYMO_ENABLE_TRACE_FLOW=1"]

def run(command, env):
    subprocess.run(command, cwd=ROOT, check=True, env=env)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cxx", default="g++")
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="kymo-trace-") as directory:
        temp = Path(directory)
        tool_bin = Path(args.cxx).resolve().parent
        env = os.environ.copy()
        env["PATH"] = str(tool_bin) + os.pathsep + env.get("PATH", "")
        for test_name, source, flags, extra_sources in (
            ("trace_core_test", "test/trace_core_test.cpp",
             TRACE_FLAGS + ["-DKYMO_TRACE_MAX_DEPTH=2"], TRACE_SOURCES),
            ("trace_mux_test", "test/trace_mux_test.cpp",
             TRACE_FLAGS + ["-DKYMO_TRACE_MAX_DEPTH=2"], TRACE_SOURCES + [MUX_SOURCE]),
            ("trace_transport_review", "test/trace_transport_review.cpp",
             ["-DKYMO_ENABLE_TRACE=1"], TRACE_SOURCES + [MUX_SOURCE]),
            ("trace_instrumentation_review", "test/trace_instrumentation_review.cpp",
             TRACE_FLAGS, TRACE_SOURCES),
            ("trace_mux_table_test", "test/trace_mux_test.cpp",
             TRACE_FLAGS + ["-DKYMO_TRACE_MAX_DEPTH=2", "-DKYMO_ENABLE_CRC_TABLE=1"],
             TRACE_SOURCES + [MUX_SOURCE]),
            ("crc_table_test", "test/crc_table_test.cpp",
             ["-DKYMO_ENABLE_TRACE=1", "-DKYMO_ENABLE_CRC=1", "-DKYMO_ENABLE_CRC_TABLE=1"],
             TRACE_SOURCES + ["src/telemetry/kymo_protocol.cpp"]),
            ("trace_stack_test", "test/trace_stack_test.cpp",
             ["-DKYMO_ENABLE_TRACE=1", "-DKYMO_ENABLE_TRACE_STACK_POINTER=1"], TRACE_SOURCES),
        ):
            binary = temp / test_name
            command = [args.cxx, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-O0",
                       *flags, "-Isrc", source, *extra_sources, "-o", str(binary)]
            run(command, env)
            run([str(binary)], env)
        c_object = temp / "trace_c_api.o"
        c_binary = temp / "trace_c_api_test"
        run([args.cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
             "-DKYMO_ENABLE_TRACE=1", "-Isrc", "-c", "test/trace_c_api_test.c",
             "-o", str(c_object)], env)
        c_link = [args.cxx, "-std=c++11", "-DKYMO_ENABLE_TRACE=1", "-Isrc",
                  str(c_object), *TRACE_SOURCES, MUX_SOURCE, "-o", str(c_binary)]
        run(c_link, env)
        run([str(c_binary)], env)
        sizes = []
        for isr_enabled in ("0", "1"):
            binary = temp / ("size_" + isr_enabled)
            size_command = [args.cxx, "-std=c++11", "-DKYMO_ENABLE_TRACE=1",
                            "-DKYMO_ENABLE_TRACE_ISR=" + isr_enabled, "-Isrc",
                            "test/trace_state_size.cpp", "-o", str(binary)]
            run(size_command, env)
            output = subprocess.run([str(binary)], cwd=ROOT, check=True, env=env,
                                    text=True, capture_output=True).stdout.strip()
            sizes.append(int(output))
        assert sizes[1] > sizes[0], "ISR-off recorder retained ISR storage"
        vector_binary = temp / "trace_vector_dump"
        vector_command = [args.cxx, "-std=c++11", "-Wall", "-Wextra", "-Werror",
                          "-DKYMO_ENABLE_TRACE=1", "-Isrc",
                          "test/trace_vector_dump.cpp", "src/trace/kymo_trace_protocol.cpp",
                          "-o", str(vector_binary)]
        run(vector_command, env)
        encoded = subprocess.run([str(vector_binary)], cwd=ROOT, check=True,
                                 env=env, text=True, capture_output=True).stdout.strip()
        fixture = json.loads((ROOT / "test/fixtures/trace_protocol_vectors.json")
                             .read_text(encoding="utf-8"))
        golden = next(item["frame_hex"] for item in fixture["vectors"]
                      if item["name"] == "trace_enter")
        assert encoded == golden, "C++ envelope differs from shared wire vector"
    print("trace APIs, recorder, protocol, mux and resource gating: PASS")

if __name__ == "__main__":
    main()
