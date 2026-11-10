# SPDX-License-Identifier: MIT
#!/usr/bin/env python3
"""Host-native KymoCore test suite: build and run every test in several builds.

Covers the common utilities, protocol golden vectors, the telemetry runtime and
C++ sender, the C entry header in all profiles, the portable example, the trace
and compile-out suites and the single-return structural check. Needs gcc/g++.
"""
from pathlib import Path
import argparse
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
WARNINGS = ["-Wall", "-Wextra", "-Werror", "-pedantic"]
TELEMETRY_SOURCES = ["src/telemetry/kymo_protocol.cpp", "src/telemetry/kymo_runtime.cpp",
                     "src/telemetry/kymo_sender.cpp"]
FULL_TELEMETRY = ["-DKYMO_PROFILE=KYMO_PROFILE_XYZ", "-DKYMO_ENABLE_CPP=1",
                  "-DKYMO_ENABLE_DECODER=1"]
PARAMETERS = ["-DKYMO_ENABLE_DECODER=1", "-DKYMO_ENABLE_PARAMETERS=1"]
WATCH = ["-DKYMO_ENABLE_DECODER=1", "-DKYMO_ENABLE_WATCH=1"]
CONTROL_SOURCES = ["src/control/kymo_control_rx.cpp", "src/control/kymo_param.cpp",
                   "src/control/kymo_watch.cpp"]
PROFILES = ([],["-DKYMO_PROFILE=KYMO_PROFILE_XYZ"], ["-DKYMO_PROFILE=KYMO_PROFILE_TRACE"],
            ["-DKYMO_ENABLED=0"])


def run(command):
    subprocess.run([str(part) for part in command], cwd=ROOT, check=True)


def build_and_run(cxx, temp, name, source, flags, sources=()):
    binary = temp / name
    run([cxx, "-std=c++11", *WARNINGS, "-fno-exceptions", "-fno-rtti", "-O1", *flags,
         "-Isrc", source, *sources,
         "-o", binary])
    run([binary])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--cxx", default="g++")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="kymo-native-") as directory:
        temp = Path(directory)
        run([args.cc, "-std=c99", *WARNINGS, "-Isrc", "test/common_test.c",
             "-o", temp / "common_test"])
        run([temp / "common_test"])
        for crc in ("0", "1"):
            build_and_run(args.cxx, temp, "protocol_golden_" + crc,
                          "test/protocol_golden_test.cpp",
                          FULL_TELEMETRY + ["-DKYMO_ENABLE_CRC=" + crc], TELEMETRY_SOURCES)
        # Table CRCs must produce the same golden frames.
        build_and_run(args.cxx, temp, "protocol_golden_table", "test/protocol_golden_test.cpp",
                      FULL_TELEMETRY + ["-DKYMO_ENABLE_CRC=1", "-DKYMO_ENABLE_CRC_TABLE=1"],
                      TELEMETRY_SOURCES)
        for index, flags in enumerate(([], FULL_TELEMETRY,
                                       FULL_TELEMETRY + ["-DKYMO_ENABLE_CRC=1"],
                                       ["-DKYMO_MAX_CHANNELS=256"])):
            build_and_run(args.cxx, temp, f"runtime_{index}", "test/runtime_test.cpp",
                          flags, TELEMETRY_SOURCES)
        param_sources = TELEMETRY_SOURCES + CONTROL_SOURCES
        for index, flags in enumerate((PARAMETERS, PARAMETERS + ["-DKYMO_ENABLE_CRC=1"],
                                       PARAMETERS + ["-DKYMO_ENABLE_CRC_TABLE=1"],
                                       PARAMETERS + ["-DKYMO_PROFILE=KYMO_PROFILE_XYZ"],
                                       PARAMETERS + ["-DKYMO_MAX_PARAMETERS=256"])):
            build_and_run(args.cxx, temp, f"param_{index}", "test/param_test.cpp",
                          flags, param_sources)
        for index, flags in enumerate((WATCH, WATCH + ["-DKYMO_ENABLE_PARAMETERS=1"],
                                       WATCH + ["-DKYMO_ENABLE_CRC=1", "-DKYMO_MAX_WATCHES=64",
                                                "-DKYMO_WATCH_MIN_PERIOD_MS=1"],
                                       WATCH + ["-DKYMO_MAX_WATCHES=6",
                                                "-DKYMO_WATCH_MIN_PERIOD_MS=10"])):
            build_and_run(args.cxx, temp, f"watch_{index}", "test/watch_test.cpp",
                          flags, param_sources)
        for index, flags in enumerate((["-DKYMO_ENABLE_MEMORY=1"],
                                       ["-DKYMO_ENABLE_MEMORY=1", "-DKYMO_ENABLE_CRC=1",
                                        "-DKYMO_MAX_MEMORY_ENTRIES=32"])):
            build_and_run(args.cxx, temp, f"memory_{index}", "test/memory_test.cpp",
                          flags, TELEMETRY_SOURCES + ["src/telemetry/kymo_memory.cpp"])
        probe = temp / "entry.c"
        probe.write_text("#include <KymoCore.h>\nint main(void) { return 0; }\n",
                         encoding="utf-8")
        for flags in PROFILES + (PARAMETERS, WATCH, PARAMETERS + WATCH[1:], ["-DKYMO_ENABLE_MEMORY=1"]):
            run([args.cc, "-std=c99", *WARNINGS, *flags, "-Isrc", "-c", probe,
                 "-o", temp / "entry.o"])
        refused = subprocess.run([args.cc, "-std=c99", "-DKYMO_ENABLE_PARAMETERS=1", "-Isrc",
                                  "-c", str(probe), "-o", str(temp / "refused.o")],
                                 cwd=ROOT, capture_output=True, text=True)
        assert refused.returncode != 0 and "requires KYMO_ENABLE_DECODER" in refused.stderr
        refused = subprocess.run([args.cc, "-std=c99", "-DKYMO_ENABLE_WATCH=1", "-Isrc",
                                  "-c", str(probe), "-o", str(temp / "refused.o")],
                                 cwd=ROOT, capture_output=True, text=True)
        assert refused.returncode != 0 and "KYMO_ENABLE_WATCH requires" in refused.stderr
        build_and_run(args.cxx, temp, "basic_example", "examples/basic/main.cpp", [],
                      TELEMETRY_SOURCES)
        run([sys.executable, "examples/integration/run.py", "--check",
             "--cxx", args.cxx, "--out", temp / "integration"])
    for script in ("test/trace_core_test.py", "test/compile_out_test.py",
                   "test/style_check.py"):
        run([sys.executable, script])
    print("KymoCore native suite: PASS")


if __name__ == "__main__":
    main()
