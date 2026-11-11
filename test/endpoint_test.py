#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Feature isolation, C linkage, compile gates and behavioral Endpoint matrix."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FALLBACK = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
BASE = ["-DKYMO_ENABLE_ENDPOINT=1", "-DKYMO_ENABLE_TX=1"]
FEATURES = {
    "plot": ["-DKYMO_ENABLE_RUNTIME=1"],
    "param": ["-DKYMO_ENABLE_RUNTIME=0", "-DKYMO_ENABLE_PARAMETERS=1", "-DKYMO_ENABLE_DECODER=1"],
    "watch": ["-DKYMO_ENABLE_RUNTIME=0", "-DKYMO_ENABLE_WATCH=1", "-DKYMO_ENABLE_DECODER=1"],
    "memory": ["-DKYMO_ENABLE_RUNTIME=0", "-DKYMO_ENABLE_MEMORY=1"],
    "trace": ["-DKYMO_ENABLE_RUNTIME=0", "-DKYMO_ENABLE_TRACE=1"],
}
ALL = ["-DKYMO_ENABLE_RUNTIME=1", "-DKYMO_ENABLE_PARAMETERS=1", "-DKYMO_ENABLE_WATCH=1",
       "-DKYMO_ENABLE_MEMORY=1", "-DKYMO_ENABLE_TRACE=1", "-DKYMO_ENABLE_DECODER=1"]
WARNINGS = ["-Wall", "-Wextra", "-Werror", "-pedantic"]


def main():
    os.environ["PATH"] = str(FALLBACK) + os.pathsep + os.environ.get("PATH", "")
    cxx, cc = shutil.which("g++"), shutil.which("gcc")
    assert cxx and cc, "gcc/g++ required"
    sources = sorted((ROOT / "src").rglob("*.cpp"))

    def run(command):
        subprocess.run(list(map(str, command)), cwd=ROOT, check=True)

    with tempfile.TemporaryDirectory(prefix="kymo-endpoint-") as directory:
        temp = Path(directory)
        profiles = {**FEATURES, "xy": ["-DKYMO_ENABLE_X=1"], "all": ALL, "timed": ALL + ["-DKYMO_ENABLE_TIMESTAMP=1"],
                    "trace-hooks": FEATURES["trace"] + ["-DKYMO_ENABLE_TRACE_ISR=1",
                        "-DKYMO_ENABLE_TRACE_SCHEDULER=1", "-DKYMO_ENABLE_TRACE_FLOW=1"]}
        for name, flags in profiles.items():
            binary = temp / (name + ".exe")
            run([cxx, "-std=c++11", *WARNINGS, "-fno-exceptions", "-fno-rtti", "-O1",
                 *BASE, *flags, "-Isrc", "test/endpoint_test.cpp", *sources, "-o", binary])
            run([binary])
            print(f"Endpoint {name}: PASS", flush=True)
        for name, flags in FEATURES.items():
            probe = temp / (name + ".c")
            probe.write_text(f"#include <endpoint/features/kymo_{name}.h>\n"
                             "int main(void) { return 0; }\n", encoding="utf-8")
            run([cc, "-std=c99", *WARNINGS, *BASE, *flags, "-Isrc", "-c", probe,
                 "-o", temp / "header.o"])
            # Even when all features are compiled, one feature header must not
            # expose another feature's facade via an umbrella include.
            unrelated = "Plot" if name == "trace" else "Trace"
            probe.write_text(f"#include <endpoint/features/kymo_{name}.h>\n"
                             f"KymoEndpoint{unrelated} unrelated;\n", encoding="utf-8")
            refused = subprocess.run([cc, "-std=c99", *BASE, *ALL, "-Isrc", "-c",
                                      str(probe), "-o", str(temp / "leak.o")],
                                     cwd=ROOT, capture_output=True)
            assert refused.returncode != 0, f"{name} leaks unrelated feature types"
        run([cc, "-std=c99", *WARNINGS, *BASE, "-Isrc", "-c", "test/endpoint_c_test.c",
             "-o", temp / "caller.o"])
        run([cxx, "-std=c++11", *WARNINGS, *BASE, "-Isrc", temp / "caller.o",
             *sources, "-o", temp / "caller.exe"])
        run([temp / "caller.exe"])
        disabled = temp / "disabled.c"
        disabled.write_text('''#include <endpoint/features/kymo_trace.h>
int main(void) {
    int side_effect = 0;
    if (1) KYMO_ENDPOINT_TRACE_MARK(missing, ++side_effect, symbol, value);
    else side_effect = 1;
    KYMO_ENDPOINT_TRACE_ENTER(missing, ++side_effect, symbol);
    KYMO_ENDPOINT_TRACE_EXIT(missing, ++side_effect, symbol);
    KYMO_ENDPOINT_TRACE_SCOPE(missing, ++side_effect, symbol);
    KYMO_ENDPOINT_TRACE_ISR_ENTER(missing, ++side_effect);
    KYMO_ENDPOINT_TRACE_ISR_EXIT(missing, ++side_effect);
    KYMO_ENDPOINT_TRACE_SCHEDULER(missing, ++side_effect, task);
    KYMO_ENDPOINT_TRACE_FLOW_SEND(missing, ++side_effect, symbol, id);
    KYMO_ENDPOINT_TRACE_FLOW_RECEIVE(missing, ++side_effect, symbol, id);
    return side_effect;
}
''', encoding="utf-8")
        for index, flags in enumerate(([], ["-DKYMO_ENABLED=0", *BASE, *ALL], BASE)):
            binary = temp / f"disabled-{index}.exe"
            run([cc, "-std=c99", *WARNINGS, *flags, "-Isrc", disabled,
                 "-o", binary])
            run([binary])
        probe = temp / "disabled-api.c"
        probe.write_text('#include <endpoint/kymo_endpoint.h>\n'
                         'int main(void) { return KymoEndpoint_Main(0); }\n', encoding="utf-8")
        for flags in ([], ["-DKYMO_ENABLE_ENDPOINT=1"], ["-DKYMO_ENABLED=0", *BASE]):
            refused = subprocess.run([cc, "-std=c99", "-Werror=implicit-function-declaration",
                                      *flags, "-Isrc", "-c", str(probe), "-o", str(temp / "off.o")],
                                     cwd=ROOT, capture_output=True)
            assert refused.returncode != 0, flags
    print("Endpoint C linkage, isolated headers, compile-out and macro contracts: PASS")


if __name__ == "__main__":
    main()
