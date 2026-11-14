#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile and execute the public Core/Parameter example using a C99 caller."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
EXAMPLE = Path(__file__).resolve().parent


def main():
    fallback = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
    os.environ["PATH"] = str(fallback) + os.pathsep + os.environ.get("PATH", "")
    cc, cxx = shutil.which("gcc"), shutil.which("g++")
    assert cc and cxx, "gcc/g++ required"
    flags = ["-Wall", "-Wextra", "-Werror", "-pedantic", "-Isrc",
             "-I" + str(EXAMPLE), '-DKYMO_USER_CONFIG_HEADER="kymo_example_config.h"']
    with tempfile.TemporaryDirectory(prefix="kymo-core-example-") as directory:
        temp = Path(directory)
        subprocess.run([cc, "-std=c99", *flags, "-c", str(EXAMPLE / "main.c"),
                        "-o", str(temp / "main.o")], cwd=ROOT, check=True)
        subprocess.run([cxx, "-std=c++11", "-fno-exceptions", "-fno-rtti", *flags,
                        str(temp / "main.o"), *map(str, sorted((ROOT / "src").rglob("*.cpp"))),
                        "-o", str(temp / "demo.exe")], cwd=ROOT, check=True)
        subprocess.run([str(temp / "demo.exe")], check=True)


if __name__ == "__main__":
    main()
