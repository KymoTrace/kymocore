# SPDX-License-Identifier: MIT
#!/usr/bin/env python3
"""Compile-time contract tests for KymoCore's global gate and macro facade."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FALLBACK = Path(r"C:\Users\NoName\.platformio\packages\toolchain-gccmingw32\bin")
NM = shutil.which("nm") or str(FALLBACK / "nm.exe")


def run(command, expected=0):
    env = os.environ.copy()
    env["PATH"] = str(FALLBACK) + os.pathsep + env.get("PATH", "")
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, env=env)
    if (result.returncode == 0) != (expected == 0):
        raise AssertionError(
            f"command failed ({result.returncode}): {' '.join(map(str, command))}\n"
            f"{result.stdout}{result.stderr}"
        )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default=shutil.which("gcc") or str(FALLBACK / "gcc.exe"))
    parser.add_argument("--cxx", default=shutil.which("g++") or str(FALLBACK / "g++.exe"))
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="kymo-compile-out-") as temp_name:
        temp = Path(temp_name)
        c_off = temp / "off.c"
        c_off.write_text(r'''#include "trace/kymo_instrumentation.h"
int main(void) {
    int side_effect = 0;
    if (1) KYMO_TRACE_ENTER(undefined_recorder, ++side_effect, missing_symbol);
    else side_effect = 99;
    KYMO_TRACE_EXIT(missing_recorder, ++side_effect, missing_symbol);
    KYMO_TRACE_MARK(missing_recorder, ++side_effect, missing_marker, missing_value);
    KYMO_TRACE_ISR_ENTER(missing_recorder, ++side_effect);
    KYMO_TRACE_ISR_EXIT(missing_recorder, ++side_effect);
    KYMO_TRACE_SCHEDULER(missing_recorder, ++side_effect, missing_task);
    KYMO_TRACE_FLOW(missing_recorder, ++side_effect, missing_symbol, missing_correlation);
    KYMO_TRACE_SCOPE(missing_recorder, ++side_effect, missing_symbol);
    return side_effect;
}
''', encoding="utf-8")
        run([args.cc, "-std=c99", "-Wall", "-Wextra", "-Werror", "-O0",
             "-DKYMO_ENABLED=0", "-Isrc", str(c_off), "-o", str(temp / "off-c")])
        run([str(temp / "off-c")])
        cpp_off = temp / "off.cpp"
        cpp_off.write_text(c_off.read_text(encoding="utf-8"), encoding="utf-8")
        run([args.cxx, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-O0",
             "-DKYMO_ENABLED=0", "-Isrc", str(cpp_off), "-o", str(temp / "off-cpp")])
        run([str(temp / "off-cpp")])

        off_flags = ["-DKYMO_ENABLE_TX=1", "-DKYMO_ENABLED=0", "-DKYMO_ENABLE_RUNTIME=1", "-DKYMO_ENABLE_X=1",
                     "-DKYMO_ENABLE_Z=1", "-DKYMO_ENABLE_TIMESTAMP=1", "-DKYMO_ENABLE_CRC=1",
                     "-DKYMO_ENABLE_DECODER=1", "-DKYMO_ENABLE_CPP=1", "-DKYMO_ENABLE_TRACE=1",
                     "-DKYMO_ENABLE_TRACE_ISR=1", "-DKYMO_ENABLE_TRACE_SCHEDULER=1",
                     "-DKYMO_ENABLE_TRACE_FLOW=1"]
        objects = []
        for source_path in sorted((ROOT / "src").rglob("*.cpp")):
            obj = temp / (source_path.stem + ".o")
            run([args.cxx, "-std=c++11", "-O0", *off_flags, "-Isrc", "-c",
                 str(source_path), "-o", str(obj)])
            objects.append(obj)
        for obj in objects:
            result = subprocess.run([NM, "-g", str(obj)], cwd=ROOT, text=True,
                                    capture_output=True, env={**os.environ, "PATH": str(FALLBACK) + os.pathsep + os.environ.get("PATH", "")})
            if result.returncode != 0 or "kymo" in result.stdout.lower():
                raise AssertionError(f"global-off object contains symbols: {obj}\n{result.stdout}{result.stderr}")

        project_config = temp / "project_config.h"
        project_config.write_text(
            "#ifndef KYMO_ENABLE_RUNTIME\n#define KYMO_ENABLE_RUNTIME 0\n#endif\n"
            "#ifndef KYMO_ENABLE_TX\n#define KYMO_ENABLE_TX 0\n#endif\n",
            encoding="utf-8",
        )
        config_probe = temp / "config_probe.c"
        config_probe.write_text(r'''#include "kymo_config.h"
#if KYMO_ENABLED != 1 || KYMO_ENABLE_RUNTIME != 1 || KYMO_EFFECTIVE_RUNTIME != 1 || !KYMO_EFFECTIVE_TX
#error "compiler definitions must override project configuration"
#endif
int main(void) { return 0; }
''', encoding="utf-8")
        include_macro = '-DKYMO_USER_CONFIG_HEADER="project_config.h"'
        run([args.cc, "-std=c99", "-Werror", "-DKYMO_ENABLE_RUNTIME=1", "-DKYMO_ENABLE_TX=1", include_macro,
             "-I", str(temp), "-Isrc", str(config_probe), "-o", str(temp / "config-probe")])
        run([str(temp / "config-probe")])

        direct = temp / "direct.c"
        direct.write_text(
            '#include "telemetry/kymo_protocol.h"\n'
            'int main(void) { return (int)KymoProtocol_Encode(0, 0, 0); }\n',
            encoding="utf-8",
        )
        run([args.cc, "-std=c99", "-Werror=implicit-function-declaration",
             "-DKYMO_ENABLED=0", "-Isrc", str(direct), "-o", str(temp / "direct")], 1)
        direct_runtime = temp / "direct-runtime.c"
        direct_runtime.write_text(
            '#include "telemetry/kymo_runtime.h"\n'
            'int main(void) { KymoContext c = {0}; return (int)Kymo_Main(&c); }\n',
            encoding="utf-8",
        )
        run([args.cc, "-std=c99", "-Werror=implicit-function-declaration", "-DKYMO_ENABLED=0",
             "-DKYMO_ENABLE_RUNTIME=1", "-Isrc", str(direct_runtime), "-o", str(temp / "direct-runtime")], 1)
        direct_tx = temp / "direct-tx.c"
        direct_tx.write_text('#include <KymoCore.h>\n'
                             'int main(void) { return KymoTx_Service(0); }\n', encoding="utf-8")
        for flags in ([], ["-DKYMO_ENABLE_TX=0"], ["-DKYMO_ENABLED=0", "-DKYMO_ENABLE_TX=1"]):
            run([args.cc, "-std=c99", "-Werror=implicit-function-declaration", *flags,
                 "-Isrc", "-c", str(direct_tx), "-o", str(temp / "direct-tx.o")], 1)
        run([args.cxx, "-std=c++11", "-O0", "-Isrc", "-c", "src/transport/kymo_tx.cpp",
             "-o", str(temp / "tx-off.o")])
        symbols = subprocess.run([NM, "-g", str(temp / "tx-off.o")], check=True,
                                 capture_output=True, text=True)
        assert "kymotx" not in symbols.stdout.lower(), symbols.stdout
        direct_cpp = temp / "direct-cpp.cpp"
        direct_cpp.write_text(
            '#include "telemetry/kymo_sender.h"\nint main() { KymoSender sender; return (int)sender.flush(); }\n',
            encoding="utf-8",
        )
        run([args.cxx, "-std=c++11", "-DKYMO_ENABLED=0", "-DKYMO_ENABLE_CPP=1",
             "-Isrc", str(direct_cpp), "-o", str(temp / "direct-cpp")], 1)

        fake = temp / "fake"
        fake.mkdir()
        (fake / "trace").mkdir()
        shutil.copy2(ROOT / "src" / "kymo_config.h", fake / "kymo_config.h")
        shutil.copy2(ROOT / "src" / "trace" / "kymo_instrumentation.h",
                     fake / "trace" / "kymo_instrumentation.h")
        (fake / "trace" / "kymo_trace.h").write_text(r'''#ifndef TEST_KYMO_TRACE_H
#define TEST_KYMO_TRACE_H
static int trace_calls;
static int trace_scope_constructed;
static int trace_scope_destroyed;
static inline int KymoTrace_Enter(int *r, int c, int s) { ++trace_calls; return *r + c + s; }
static inline int KymoTrace_Exit(int *r, int c, int s) { ++trace_calls; return *r + c + s; }
static inline int KymoTrace_Mark(int *r, int c, int m, int v) { ++trace_calls; return *r + c + m + v; }
static inline int KymoTrace_IsrEnter(int *r, int c) { ++trace_calls; return *r + c; }
static inline int KymoTrace_IsrExit(int *r, int c) { ++trace_calls; return *r + c; }
static inline int KymoTrace_Scheduler(int *r, int c, int t) { ++trace_calls; return *r + c + t; }
static inline int KymoTrace_FlowSend(int *r, int c, int s, int correlation) { ++trace_calls; return *r + c + s + correlation; }
static inline int KymoTrace_FlowReceive(int *r, int c, int s, int correlation) { ++trace_calls; return *r + c + s + correlation; }
class KymoTraceScope { public: KymoTraceScope(int *, int, int) { ++trace_scope_constructed; } ~KymoTraceScope() { ++trace_scope_destroyed; } };
#endif
''', encoding="utf-8")
        active = temp / "active.cpp"
        active.write_text(r'''#include "trace/kymo_instrumentation.h"
static int eval_counts[27];
static int eval(int slot) { ++eval_counts[slot]; return slot; }
static int *eval_recorder(int slot, int *recorder) { ++eval_counts[slot]; return recorder; }
int main() {
    int recorder = 7;
    KYMO_TRACE_ENTER(eval_recorder(0, &recorder), eval(1), eval(2));
    KYMO_TRACE_EXIT(eval_recorder(3, &recorder), eval(4), eval(5));
    KYMO_TRACE_MARK(eval_recorder(6, &recorder), eval(7), eval(8), eval(9));
    KYMO_TRACE_ISR_ENTER(eval_recorder(10, &recorder), eval(11));
    KYMO_TRACE_ISR_EXIT(eval_recorder(12, &recorder), eval(13));
    KYMO_TRACE_SCHEDULER(eval_recorder(14, &recorder), eval(15), eval(16));
    KYMO_TRACE_FLOW(eval_recorder(17, &recorder), eval(18), eval(19), eval(20));
    { KYMO_TRACE_SCOPE(eval_recorder(21, &recorder), eval(22), eval(23)); KYMO_TRACE_SCOPE(eval_recorder(24, &recorder), eval(25), eval(26)); KYMO_TRACE_SCOPE(&recorder, 1, 2); }
    int each_argument_once = 1;
    for (int i = 0; i < 27; ++i) { each_argument_once = each_argument_once && (eval_counts[i] == 1); }
    return (trace_calls == 7 && each_argument_once && trace_scope_constructed == 3 && trace_scope_destroyed == 3) ? 0 : 1;
}
''', encoding="utf-8")
        defines = ["-DKYMO_ENABLED=1", "-DKYMO_ENABLE_TRACE=1",
                   "-DKYMO_ENABLE_TRACE_ISR=1", "-DKYMO_ENABLE_TRACE_SCHEDULER=1",
                   "-DKYMO_ENABLE_TRACE_FLOW=1", "-DKYMO_ENABLE_CPP=0"]
        run([args.cxx, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-O0",
             *defines, "-I", str(fake), "-Isrc", str(active), "-o", str(temp / "active")])
        run([str(temp / "active")])

        for bad in ("2", "-1", "UNKNOWN_BOOL", ""):
            proc = subprocess.run(
                [args.cc, "-std=c99", "-DKYMO_ENABLED=" + bad, "-Isrc", "-x", "c",
                 "-c", "-o", str(temp / "bad.o"), "-"],
                input='#include "kymo_config.h"\n', cwd=ROOT, text=True, capture_output=True,
            )
            if proc.returncode == 0:
                raise AssertionError(f"invalid KYMO_ENABLED token accepted: {bad!r}")

        boolean_flags = ("KYMO_ENABLE_TX", "KYMO_ENABLED", "KYMO_ENABLE_RUNTIME", "KYMO_ENABLE_X", "KYMO_ENABLE_Z",
                         "KYMO_ENABLE_TIMESTAMP", "KYMO_ENABLE_CRC", "KYMO_ENABLE_DECODER",
                         "KYMO_ENABLE_CPP", "KYMO_ENABLE_TRACE", "KYMO_ENABLE_TRACE_ISR",
                         "KYMO_ENABLE_TRACE_SCHEDULER", "KYMO_ENABLE_TRACE_FLOW")
        for flag in boolean_flags:
            proc = subprocess.run(
                [args.cc, "-std=c99", "-DKYMO_ENABLED=0", f"-D{flag}=2", "-Isrc", "-x", "c",
                 "-c", "-o", str(temp / "bad-flag.o"), "-"],
                input='#include "kymo_config.h"\n', cwd=ROOT, text=True, capture_output=True,
            )
            if proc.returncode == 0:
                raise AssertionError(f"invalid boolean setting accepted while globally off: {flag}")

        valid = subprocess.run(
            [args.cc, "-std=c99", "-DKYMO_ENABLED=0", "-DKYMO_ENABLE_RUNTIME=1", "-Isrc",
             "-x", "c", "-c", "-o", str(temp / "valid.o"), "-"],
            input='#include "kymo_config.h"\n', cwd=ROOT, text=True, capture_output=True,
        )
        if valid.returncode != 0:
            raise AssertionError(f"valid off settings were rejected:\n{valid.stdout}{valid.stderr}")

        for bad_flags in (
            ["-DKYMO_ENABLED=1", "-DKYMO_ENABLE_TRACE_ISR=1"],
            ["-DKYMO_ENABLED=1", "-DKYMO_ENABLE_TRACE_FLOW=1"],
            ["-DKYMO_ENABLED=1", "-DKYMO_ENABLE_TRACE=1", "-DKYMO_ENABLE_TRACE_SCHEDULER=1"],
        ):
            proc = subprocess.run(
                [args.cc, "-std=c99", *bad_flags, "-Isrc", "-x", "c", "-c", "-o",
                 str(temp / "dependency.o")],
                input='#include "kymo_config.h"\n', cwd=ROOT, text=True, capture_output=True,
            )
            if proc.returncode == 0:
                raise AssertionError(f"invalid active feature dependency accepted: {bad_flags}")

        def preprocess(flags, probe):
            return subprocess.run(
                [args.cc, "-std=c99", *flags, "-Isrc", "-x", "c", "-c", "-o",
                 str(temp / "probe.o"), "-"],
                input='#include "kymo_config.h"\n' + probe, cwd=ROOT, text=True,
                capture_output=True,
            )

        for bad_profile in ("3", "UNKNOWN_PROFILE", "-1"):
            if preprocess(["-DKYMO_PROFILE=" + bad_profile], "").returncode == 0:
                raise AssertionError(f"invalid KYMO_PROFILE accepted: {bad_profile!r}")
        profile_checks = (
            ([], "#if KYMO_ENABLE_TX || KYMO_ENABLE_X || KYMO_ENABLE_TRACE\n#error minimal\n#endif\n"),
            (["-DKYMO_PROFILE=KYMO_PROFILE_XYZ"],
             "#if !KYMO_ENABLE_X || !KYMO_ENABLE_Z || !KYMO_ENABLE_TIMESTAMP || KYMO_ENABLE_TRACE\n#error xyz\n#endif\n"),
            (["-DKYMO_PROFILE=KYMO_PROFILE_TRACE"],
             "#if !KYMO_EFFECTIVE_TRACE_SCHEDULER || !KYMO_EFFECTIVE_TRACE_FLOW || !KYMO_ENABLE_X\n#error trace\n#endif\n"),
            (["-DKYMO_PROFILE=KYMO_PROFILE_XYZ", "-DKYMO_ENABLE_Z=0"],
             "#if KYMO_ENABLE_Z || !KYMO_ENABLE_X\n#error override\n#endif\n"),
            (["-DKYMO_PROFILE=1"], "#if !KYMO_ENABLE_X\n#error numeric\n#endif\n"),
        )
        for flags, probe in profile_checks:
            proc = preprocess(flags, probe)
            if proc.returncode != 0:
                raise AssertionError(f"profile defaults wrong for {flags}:\n{proc.stderr}")
        for bad_channels in ("0", "257"):
            if preprocess(["-DKYMO_MAX_CHANNELS=" + bad_channels], "").returncode == 0:
                raise AssertionError(f"invalid KYMO_MAX_CHANNELS accepted: {bad_channels}")

    print("compile-out configuration, profiles and instrumentation facade: PASS")


if __name__ == "__main__":
    main()



