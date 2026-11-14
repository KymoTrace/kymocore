#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise every default-Core feature through C99, then inspect its wire output."""
from pathlib import Path
import binascii
import os
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
WARNINGS = ["-Wall", "-Wextra", "-Werror", "-pedantic"]
FEATURES = {"plot": "RUNTIME", "param": "PARAMETERS", "watch": "WATCH",
            "memory": "MEMORY", "trace": "TRACE"}
HOOKS = ["-DKYMO_ENABLE_TRACE_ISR=1", "-DKYMO_ENABLE_TRACE_SCHEDULER=1",
         "-DKYMO_ENABLE_TRACE_FLOW=1"]


def flags(enabled, core=True):
    return [f"-DKYMO_ENABLE_CORE={int(core)}", "-DKYMO_ENABLE_CRC=1",
            "-DKYMO_ENABLE_DECODER=1"] + [
        f"-DKYMO_ENABLE_{switch}={int(name in enabled)}"
        for name, switch in FEATURES.items()]


def crc8(data):
    value = 0
    for byte in data:
        value ^= byte
        for _ in range(8):
            value = ((value << 1) ^ (0x07 if value & 0x80 else 0)) & 0xff
    return value


def verify_wire(data, enabled, hooks=False):
    """Decode the documented framing independently from the C library."""
    offset = 0
    samples = {}
    controls = set()
    trace_kinds = []
    events = []
    while offset + 4 <= len(data):
        assert data[offset:offset + 2] == b"\xa5\x5a", offset
        descriptor = data[offset + 2]
        if descriptor == 0x80:
            if offset + 24 > len(data):
                break
            size = 24 + int.from_bytes(data[offset + 4:offset + 6], "little")
        elif descriptor == 0x60:
            size = 5 + data[offset + 3]
        else:
            assert descriptor & 0xc0 == 0x40
            size = 9 + 4 * sum(bool(descriptor & bit) for bit in (8, 4, 2))
        if offset + size > len(data):
            break  # Main has a bounded writer; a final frame may still be pending.
        frame = data[offset:offset + size]
        if descriptor == 0x80:
            assert binascii.crc_hqx(frame[2:20], 0xffff) == int.from_bytes(frame[20:22], "little")
            assert binascii.crc_hqx(frame[2:-2], 0xffff) == int.from_bytes(frame[-2:], "little")
            assert int.from_bytes(frame[8:16], "little") == 1
            trace_kinds.append(frame[3])
            if frame[3] == 3:
                payload = frame[22:-2]
                assert payload[0] == 1 and len(payload) == 4 + 32 * payload[1]
                for event_offset in range(4, len(payload), 32):
                    events.append(payload[event_offset + 24])
        else:
            assert crc8(frame[2:-1]) == frame[-1], frame.hex()
            if descriptor == 0x60:
                controls.add(frame[4])
            else:
                samples.setdefault(frame[3], []).append(struct.unpack_from("<f", frame, 4)[0])
        offset += size
    for name, channel, value in (("plot", 3, 12.5), ("param", 42, 200.0),
                                 ("watch", 5, 12.5), ("memory", 6, 512.0)):
        if name in enabled:
            assert value in samples.get(channel, []), (name, samples)
        else:
            assert channel not in samples, (name, samples)
    if "watch" in enabled:
        assert 6 in controls  # WATCH_ACK
    if "memory" in enabled:
        assert 7 in controls  # MEMORY_INFO
    if "trace" in enabled:
        assert trace_kinds[:2] == [1, 2], trace_kinds
        assert events[:3] == [1, 3, 2], events
        if hooks:
            assert {4, 5, 0x20, 0x21, 0x22, 0x23, 0x30, 0x31} <= set(events), events
    else:
        assert not trace_kinds


def main():
    fallback = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
    os.environ["PATH"] = str(fallback) + os.pathsep + os.environ.get("PATH", "")
    cc, cxx, nm = (shutil.which(name) for name in ("gcc", "g++", "nm"))
    assert cc and cxx and nm
    sources = sorted((ROOT / "src").rglob("*.cpp"))

    def run(command, **kwargs):
        return subprocess.run(list(map(str, command)), cwd=ROOT, check=True, **kwargs)

    with tempfile.TemporaryDirectory(prefix="kymo-core-features-") as directory:
        temp = Path(directory)
        profiles = [(name, [name], []) for name in FEATURES]
        profiles += [("all", list(FEATURES), []), ("trace-hooks", ["trace"], HOOKS)]
        for name, enabled, extras in profiles:
            defines = flags(enabled) + extras
            caller, binary, wire = temp / "caller.o", temp / (name + ".exe"), temp / "wire.bin"
            run([cc, "-std=c99", *WARNINGS, *defines, "-Isrc", "-c",
                 "test/core_features_test.c", "-o", caller])
            run([cxx, "-std=c++11", *WARNINGS, "-fno-exceptions", "-fno-rtti",
                 *defines, "-Isrc", caller, *sources, "-o", binary])
            for scenario in ("inactive", "invalid", "late", "normal"):
                run([binary, scenario, wire])
                if scenario == "normal":
                    verify_wire(wire.read_bytes(), enabled, bool(extras))
                else:
                    assert wire.read_bytes() == b""
            print(f"Public Core {name}: activation, invalid/late setup and wire verification PASS", flush=True)

        probe, obj = temp / "probe.c", temp / "probe.o"
        for name in FEATURES:
            module = "KymoCore" + name.capitalize()
            include = f"#include <kymo_core_{name}.h>\n"
            # Every public header is sufficient on its own in C and C++.
            probe.write_text(include + f"{module}Config config;\n", encoding="utf-8")
            for compiler, standard in ((cc, "c99"), (cxx, "c++11")):
                run([compiler, f"-std={standard}", *WARNINGS, *flags([name]),
                     "-Isrc", "-c", probe, "-o", obj])
            # Disabled feature interfaces are unavailable, including Init.
            for code, defines in (
                (f"{module}Config config;", flags([])),
                (f"int main(void) {{ return {module}_init(0); }}", flags([])),
                (f"int main(void) {{ return {module}_init(0); }}", flags([name], False)),
                ("KymoCoreEndpoint leaked;", flags(FEATURES)),
                ("KymoTraceRecorder leaked;", flags(FEATURES)),
                (("KymoCoreParamConfig" if name != "param" else "KymoCoreTraceConfig") + " leaked;", flags(FEATURES)),
            ):
                probe.write_text(include + code, encoding="utf-8")
                refused = subprocess.run([cc, "-std=c99", *WARNINGS, *defines,
                                          "-Isrc", "-c", str(probe), "-o", str(obj)],
                                         cwd=ROOT, capture_output=True)
                assert refused.returncode != 0, (name, code)
            folder = "parameters" if name == "param" else name
            source = ROOT / f"src/features/{folder}/kymo_core_{name}.cpp"
            for defines, is_present in ((flags([]), False), (flags([name]), True),
                                        (flags([name], False), False)):
                run([cxx, "-std=c++11", *WARNINGS, *defines, "-Isrc", "-c", source, "-o", obj])
                symbols = subprocess.check_output([nm, "-a", str(obj)], text=True)
                assert (module + "_init" in symbols) == is_present
                assert ("default" in symbols) == is_present
        probe.write_text("#include <kymo_core_trace.h>\n"
                         "int main() { KymoCoreTraceScope scope(1u, 1u); return 0; }\n",
                         encoding="utf-8")
        run([cxx, "-x", "c++", "-std=c++11", *WARNINGS, *flags(["trace"]),
             "-Isrc", probe, *sources, "-o", temp / "scope.exe"])
        run([temp / "scope.exe"])
    print("All feature APIs: C/C++ isolation, disabled APIs/storage and scope pairing PASS")


if __name__ == "__main__":
    main()
