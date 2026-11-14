#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Public Core/Parameter contract, C linkage, header isolation and compile-out."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
WARNINGS = ["-Wall", "-Wextra", "-Werror", "-pedantic"]
BASE = ["-DKYMO_ENABLE_CORE=1", "-DKYMO_ENABLE_RUNTIME=0", "-DKYMO_ENABLE_CRC=1"]
PARAM = ["-DKYMO_ENABLE_PARAMETERS=1", "-DKYMO_ENABLE_DECODER=1"]


def main():
    fallback = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
    os.environ["PATH"] = str(fallback) + os.pathsep + os.environ.get("PATH", "")
    cc, cxx, nm = (shutil.which(name) for name in ("gcc", "g++", "nm"))
    assert cc and cxx and nm, "gcc/g++/nm required"
    sources = sorted((ROOT / "src").rglob("*.cpp"))

    def run(command):
        subprocess.run(list(map(str, command)), cwd=ROOT, check=True)

    with tempfile.TemporaryDirectory(prefix="kymo-core-entry-") as directory:
        temp = Path(directory)
        for name, flags, cases in (
            ("empty", BASE, ["empty", "idle", "retry"]),
            ("parameters", BASE + PARAM, ["empty", "idle", "retry", "normal",
             "before-init", "late", "invalid", "null", "duplicate", "small",
             "overflow", "crc", "range"]),
        ):
            caller = temp / (name + ".o")
            binary = temp / (name + ".exe")
            run([cc, "-std=c99", *WARNINGS, *flags, "-Isrc", "-c",
                 "test/core_entry_test.c", "-o", caller])
            run([cxx, "-std=c++11", *WARNINGS, "-fno-exceptions", "-fno-rtti",
                 *flags, "-Isrc", caller, *sources, "-o", binary])
            for case in cases:
                run([binary, case])
            print(f"Public Core {name}: {len(cases)} behavior cases PASS", flush=True)

        probe = temp / "probe.c"
        negative = [
            (BASE, "#include <kymo_core_param.h>\nKymoCoreParamConfig config;\n"),
            (BASE, "#include <kymo_core_param.h>\nint main(void) { return KymoCoreParam_init(0); }\n"),
            (PARAM, "#include <kymo_core.h>\nint main(void) { return KymoCore_init(0); }\n"),
            (BASE + PARAM, "#include <kymo_core.h>\nKymoCoreParamConfig leaked;\n"),
            (BASE + PARAM, "#include <kymo_core_param.h>\nKymoParamReceiver leaked;\n"),
            (BASE + PARAM, "#include <kymo_core.h>\n#include <kymo_core_param.h>\nKymoCoreEndpoint leaked;\n"),
            (BASE + PARAM + ["-DKYMO_ENABLED=0"],
             "#include <kymo_core_param.h>\nint main(void) { return KymoCoreParam_init(0); }\n"),
        ]
        for flags, code in negative:
            probe.write_text(code, encoding="utf-8")
            result = subprocess.run([cc, "-std=c99", *WARNINGS, *flags, "-Isrc",
                                     "-c", str(probe), "-o", str(temp / "bad.o")],
                                    cwd=ROOT, capture_output=True)
            assert result.returncode != 0, (flags, code)
        for path, flags, absent in (
            ("src/features/parameters/kymo_core_param.cpp", BASE, "defaultParameters"),
            ("src/core/kymo_core.cpp", [], "defaultConnection"),
            ("src/features/parameters/kymo_core_param.cpp", BASE, "KymoCoreParam_init"),
            ("src/features/parameters/kymo_param.cpp", BASE, "KymoParam_"),
        ):
            obj = temp / "off.o"
            run([cxx, "-std=c++11", *WARNINGS, "-O0", *flags, "-Isrc", "-c", path, "-o", obj])
            symbols = subprocess.check_output([nm, "-a", str(obj)], text=True)
            assert absent not in symbols, (path, symbols)
        # Public C++ consumer uses exactly the same declarations as the C test.
        probe.write_text("#include <kymo_core.h>\n#include <kymo_core_param.h>\n"
                         "int main() { KymoCoreConfig core = {}; KymoCoreParamConfig param = {}; "
                         "(void)core; (void)param; return 0; }\n", encoding="utf-8")
        run([cxx, "-std=c++11", *WARNINGS, *BASE, *PARAM, "-Isrc",
             "-x", "c++", "-c", probe, "-o", temp / "cpp.o"])
    print("Core public headers, disabled declarations and static storage: PASS")


if __name__ == "__main__":
    main()
