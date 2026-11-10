# SPDX-License-Identifier: MIT
"""Validate, pack and build consumers from the exported KymoCore archive.

Requires PlatformIO and g++; --skip-firmware runs just package/host checks.
No publishing, credentials or hardware access. Outputs stay under .pio/.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
LIB = ROOT


def run(*command, cwd=ROOT):
    subprocess.run([str(part) for part in command], cwd=cwd, check=True)


def executable(name, fallback):
    found = shutil.which(name)
    if found:
        return found
    if fallback.is_file():
        return str(fallback)
    raise RuntimeError(f"{name} is required; install it or add it to PATH")


def check_archive(archive, destination):
    with tarfile.open(archive) as package:
        members = package.getmembers()
        for member in members:
            target = (destination / member.name).resolve()
            if not target.is_relative_to(destination.resolve()) or not (member.isfile() or member.isdir()):
                raise RuntimeError(f"Unsafe archive entry: {member.name}")
        for member in members:
            target = destination / member.name
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with package.extractfile(member) as source, target.open("wb") as output:
                    shutil.copyfileobj(source, output)
    manifest = json.loads((destination / "library.json").read_text(encoding="utf-8"))
    properties = dict(line.split("=", 1) for line in
                      (destination / "library.properties").read_text(encoding="utf-8").splitlines()
                      if "=" in line)
    assert manifest["name"] == properties["name"] == "KymoCore"
    assert manifest["version"] == properties["version"]
    idf = re.search(r'^version: "([^"]+)"$', (LIB / "idf_component.yml").read_text(encoding="utf-8"), re.M)
    assert idf and idf.group(1) == manifest["version"], "idf_component.yml version differs from library.json"
    assert len(manifest["description"]) <= 255
    required = ["LICENSE", "docs/CONTRIBUTING.md", "docs/CONTRIBUTING.de.md", "CHANGELOG.md", "docs/PUBLISHING.md", "README.md", "README.de.md", "docs/PROTOCOL.md", "docs/README.md", "docs/GUIDE.md", "docs/GUIDE.de.md",
                "examples/basic/main.cpp", "examples/basic/README.md",
                "docs/images/architecture.png", "docs/images/transmission.png",
                "docs/images/protocol.png"]
    required += [str(path.relative_to(LIB)).replace("\\", "/") for path in (LIB / "src").rglob("*") if path.is_file()]
    required += [str(path.relative_to(LIB)).replace("\\", "/")
                 for path in (LIB / "examples/integration").rglob("*")
                 if path.is_file() and "__pycache__" not in path.parts and path.suffix != ".pyc"]
    # Every standalone example must survive export, including shared adapters
    # and the immutable generated profiles needed for firmware builds.
    required += [path.relative_to(LIB).as_posix()
                 for path in (LIB / "examples").rglob("*")
                 if path.is_file()
                 and not any(part in {".pio", ".vscode", "__pycache__"} for part in path.relative_to(LIB).parts)
                 and path.name != "network_secrets.h"]
    for name in required:
        assert (destination / name).is_file(), f"Missing package file: {name}"
    for path in destination.rglob("*"):
        relative = path.relative_to(destination)
        assert not any(part in {".pio", ".git", "legacy", "Events", "__pycache__"} for part in relative.parts)
        if path.is_file():
            assert path.name != "network_secrets.h", relative
            assert path.suffix not in {".o", ".exe", ".elf", ".pyc"}, relative
            if path.name != "library.json":
                assert path.read_bytes() == (LIB / relative).read_bytes(), relative
    # Registry consumers must not need files outside the package for local docs.
    for path in destination.rglob("*.md"):
        for target in re.findall(r"!?\[[^\]]*\]\(([^)]+)\)", path.read_text(encoding="utf-8")):
            if "://" not in target and not target.startswith("#"):
                linked = (path.parent / target.split("#", 1)[0]).resolve()
                assert linked.is_relative_to(destination.resolve()) and linked.exists(), (path, target)
    print("Archive contents, versions and local documentation links passed", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-firmware", action="store_true")
    args = parser.parse_args()
    pio = executable("pio", Path.home() / ".platformio/penv/Scripts/pio.exe")
    compiler = executable("g++", Path.home() / ".platformio/packages/toolchain-gccmingw32/bin/g++.exe")
    os.environ["PATH"] = str(Path(compiler).parent) + os.pathsep + os.environ["PATH"]
    version = json.loads((LIB / "library.json").read_text(encoding="utf-8"))["version"]
    output = ROOT / ".pio/package"
    output.mkdir(parents=True, exist_ok=True)
    archive = output / f"KymoCore-{version}.tar.gz"
    run(pio, "pkg", "pack", LIB, "-o", archive)
    workspace = Path(tempfile.mkdtemp(prefix="consumer-", dir=output))
    unpacked = workspace / "unpacked"
    unpacked.mkdir()
    check_archive(archive, unpacked)
    program = workspace / ("basic.exe" if os.name == "nt" else "basic")
    run(compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-fno-exceptions", "-fno-rtti", "-I" + str(unpacked / "src"),
        unpacked / "examples/basic/main.cpp", *sorted((unpacked / "src").rglob("*.cpp")),
        "-o", program)
    run(program, cwd=workspace)
    print("Packaged portable example passed", flush=True)
    run(sys.executable, unpacked / "examples/integration/run.py", "--cxx", compiler,
        "--out", workspace / "integration", cwd=workspace)
    print("Packaged integration example passed", flush=True)
    if not args.skip_firmware:
        consumer = workspace / "firmware"
        (consumer / "src").mkdir(parents=True)
        readme = (unpacked / "README.md").read_text(encoding="utf-8")
        source = re.search(r"```cpp\n(#include <Arduino.h>[\s\S]*?)\n```", readme)
        assert source, "Complete README quick-start example missing"
        (consumer / "src/main.cpp").write_text(source.group(1) + "\n", encoding="utf-8")
        (consumer / "platformio.ini").write_text(
            "[env]\nframework = arduino\nlib_deps =\n    file://" + archive.as_posix() +
            "\n\n[env:uno]\nplatform = atmelavr@5.3.0\nboard = uno\n"
            "\n[env:esp32dev]\nplatform = espressif32@6.12.0\nboard = esp32dev\n",
            encoding="utf-8")
        # Use only the archive dependency, never the repository's symlink library.
        run(pio, "run", "-d", consumer, cwd=consumer)
        print("README example builds from archive: Uno and ESP32 passed", flush=True)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (output / (archive.name + ".sha256")).write_text(f"{digest}  {archive.name}\n", encoding="ascii")
    print(f"Package checks passed: {archive}\nSHA256: {digest}")


if __name__ == "__main__":
    main()
