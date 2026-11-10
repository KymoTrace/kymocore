#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Build/run the portable integration example and verify its actual wire output.

Only Python's standard library and a C++11 compiler are needed. Python is a host
runner, not part of the firmware. --check adds transport/error regressions.
"""
from __future__ import annotations

import argparse
import binascii
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

EXAMPLE = Path(__file__).resolve().parent
ROOT = EXAMPLE.parents[1]


def crc8(data: bytes) -> int:
    value = 0
    for byte in data:
        value ^= byte
        for _ in range(8):
            value = ((value << 1) ^ (0x07 if value & 0x80 else 0)) & 0xFF
    return value


def control(payload: bytes) -> bytes:
    body = bytes((0x60, len(payload))) + payload
    return b"\xa5\x5a" + body + bytes((crc8(body),))


def parameter(value: float) -> bytes:
    body = bytes((0x40, 100)) + struct.pack("<f", value)
    return b"\xa5\x5a" + body + bytes((crc8(body),))


def requests() -> bytes:
    corrupt = bytearray(parameter(9.0))
    corrupt[-1] ^= 0xFF
    return (b"noise" + parameter(2.5) + bytes(corrupt) + parameter(99.0)
            + control(b"\x01") + control(b"\x03")
            + control(struct.pack("<BBBIBH", 5, 0, 150, 0x20000000, 7, 100)))


def verify(capture: bytes, *, with_requests: bool = True) -> dict[str, int]:
    """Independent wire checks: no resync/skipping can hide interleaved frames."""
    offset = 0
    points: dict[int, list[float]] = {}
    controls: dict[int, list[bytes]] = {}
    records: dict[int, bytearray] = {}
    record_lengths: dict[int, int] = {}
    trace_sequences = []
    packet_sequences = []
    frame_count = 0
    while offset < len(capture):
        assert capture[offset:offset + 2] == b"\xa5\x5a", offset
        descriptor = capture[offset + 2]
        if descriptor == 0x80:
            payload_length = struct.unpack_from("<H", capture, offset + 4)[0]
            length = 24 + payload_length
            frame = capture[offset:offset + length]
            assert len(frame) == length <= 240
            assert binascii.crc_hqx(frame[2:20], 0xFFFF) == int.from_bytes(frame[20:22], "little")
            assert binascii.crc_hqx(frame[2:-2], 0xFFFF) == int.from_bytes(frame[-2:], "little")
            assert struct.unpack_from("<Q", frame, 8)[0] == 1
            packet_sequences.append(struct.unpack_from("<I", frame, 16)[0])
            kind, payload = frame[3], frame[22:-2]
            if kind == 3:
                assert payload[0] == 1 and payload[2:4] == b"\0\0"
                assert len(payload) == 4 + payload[1] * 32
                for event_offset in range(4, len(payload), 32):
                    event = struct.unpack_from("<QIIIHHBBHI", payload, event_offset)
                    ticks, seq, context, _, symbol, depth, event_type, core, _, _ = event
                    assert ticks > 0 and (context, symbol, depth, event_type, core) == (1, 1, 0, 3, 0)
                    trace_sequences.append(seq)
            else:
                assert kind in (1, 2, 6), kind
                schema, record_kind, generation, transfer, total, fragment_offset = struct.unpack_from("<BBIIII", payload)
                assert (schema, record_kind, generation) == (1, kind, 1)
                assert transfer > 0
                record = records.setdefault(kind, bytearray())
                record_lengths.setdefault(kind, total)
                assert record_lengths[kind] == total and fragment_offset == len(record)
                record.extend(payload[18:])
                assert len(record) <= total
        else:
            assert descriptor in (0x40, 0x60), descriptor
            length = 9 if descriptor == 0x40 else 5 + capture[offset + 3]
            frame = capture[offset:offset + length]
            assert len(frame) == length
            assert crc8(frame[2:-1]) == frame[-1]
            if descriptor == 0x40:
                points.setdefault(frame[3], []).append(struct.unpack_from("<f", frame, 4)[0])
            else:
                controls.setdefault(frame[4], []).append(frame[4:-1])
        frame_count += 1
        offset += length
    assert packet_sequences == list(range(len(packet_sequences)))
    assert set(records) == {1, 2, 6}, set(records)
    for kind, record in records.items():
        assert len(record) == record_lengths[kind]
        assert binascii.crc32(record[:-4]) == int.from_bytes(record[-4:], "little")
    metadata = json.loads(records[1][:-4])
    dictionary = json.loads(records[2][:-4])
    for kind, value in ((1, metadata), (2, dictionary)):
        assert bytes(records[kind][:-4]) == json.dumps(value, sort_keys=True, separators=(",", ":")).encode()
    assert metadata["dictionary_hash"] == hashlib.sha256(records[2][:-4]).hexdigest()
    assert metadata["clock"]["frequency_hz"] == "1000"
    assert dictionary["symbols"][0]["id"] == 1 and dictionary["contexts"][0]["id"] == 1
    assert trace_sequences and trace_sequences == list(range(len(trace_sequences)))
    end = records[6][:-4]
    assert end[:4] == bytes((1, 1, 0, 0))
    produced, settled, drops = struct.unpack_from("<III", end, 8)
    assert produced == settled == len(trace_sequences) and drops == 0
    assert len(points.get(0, [])) >= 5, "telemetry starved"
    assert len(points.get(220, [])) >= 5, "memory starved"
    assert all(value == 2048.0 for value in points[220])
    assert 7 in controls and b"demo free" in controls[7][0]
    if with_requests:
        assert points.get(100) and all(value == 2.5 for value in points[100])
        assert len(points.get(150, [])) >= 5 and all(value == 2.5 for value in points[150])
        assert max(points[0]) > 1.0 and max(points[0]) < 2.5
        assert 2 in controls and b"gain" in controls[2][0]
        assert 4 in controls and b"\x06\x00\x00" in controls.get(6, [])
    else:
        assert 100 not in points and 150 not in points
        assert max(points[0]) < 1.0
    return {"frames": frame_count, "trace_events": len(trace_sequences), "bytes": len(capture)}


def build(cxx: str, directory: Path, defines: dict[str, int], *, contract: str | None = None) -> Path:
    binary = directory / ("integration.exe" if os.name == "nt" else "integration")
    if contract == "tx":
        sources = [EXAMPLE / "tests/tx_test.cpp", EXAMPLE / "kymo_tx.cpp"]
    elif contract == "app":
        sources = [EXAMPLE / name for name in
                   ("tests/app_test.cpp", "kymo_app.cpp", "kymo_tx.cpp", "kymo_bindings.cpp")]
        sources.extend(sorted((ROOT / "src").rglob("*.cpp")))
    else:
        sources = [*sorted(EXAMPLE.glob("*.cpp")), *sorted((ROOT / "src").rglob("*.cpp"))]
    subprocess.run([
        cxx, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-fno-exceptions", "-fno-rtti", "-O1", "-I" + str(ROOT / "src"),
        "-I" + str(EXAMPLE), '-DKYMO_USER_CONFIG_HEADER="kymo_user_config.h"',
        *[f"-DEXAMPLE_NATIVE_{key}={value}" for key, value in defines.items()],
        *map(str, sources), "-o", str(binary),
    ], check=True)
    return binary


def run_case(cxx: str, output: Path, name: str, defines: dict[str, int], *, empty: bool = False) -> None:
    directory = output / name
    directory.mkdir(parents=True, exist_ok=True)
    binary = build(cxx, directory, defines)
    incoming, outgoing = directory / "requests.bin", directory / "capture.bin"
    incoming.write_bytes(b"" if empty else requests())
    completed = subprocess.run([str(binary), str(incoming), str(outgoing)], timeout=15)
    if defines.get("FAULT"):
        assert completed.returncode == 2, completed.returncode
        assert outgoing.read_bytes() == b"", "writes continued after a transport fault"
        print(f"{name}: PASS (driver violation stops transmission)", flush=True)
    else:
        assert completed.returncode == 0, completed.returncode
        stats = verify(outgoing.read_bytes(), with_requests=not empty)
        print(f"{name}: PASS {stats}", flush=True)


def verify_with_app(app: Path, output: Path) -> None:
    """Optionally check these exact captures with the sibling KymoStudio reader."""
    sys.path.insert(0, str(app.resolve() / "python"))
    from Core.parsing import parse_payload
    from Core.trace_model import TraceSessionStore
    from Receiver.binary_protocol import ProtocolStreamDecoder
    from Receiver.trace_protocol import TraceEnvelope

    for capture_path in sorted(output.glob("*/capture.bin")):
        if capture_path.parent.name == "driver-fault":
            continue
        wire = capture_path.read_bytes()
        for chunk_size in (1, 7, 64, 4096):
            decoder = ProtocolStreamDecoder()
            store = TraceSessionStore()
            frames = []
            for start in range(0, len(wire), chunk_size):
                frames.extend(decoder.feed(wire[start:start + chunk_size]))
            frames.extend(decoder.flush())
            assert b"".join(frames) == wire and decoder.take_losses() is None
            for frame in frames:
                message = parse_payload(frame)
                assert message is not None
                if isinstance(message, TraceEnvelope):
                    store.ingest(message)
            session = store.session(1)
            assert session.complete and not session.gaps, session.gaps
            assert session.dictionary["symbols"][0]["name"] == "main cycle"
        print(f"KymoStudio {capture_path.parent.name}: PASS (4 fragment sizes, complete trace session)", flush=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="g++")
    parser.add_argument("--out", type=Path, default=ROOT / ".pio/integration")
    parser.add_argument("--check", action="store_true", help="exercise additional transport conditions")
    parser.add_argument("--app", type=Path, help="also decode captures with a KymoStudio checkout")
    args = parser.parse_args()
    cxx = shutil.which(args.cxx)
    if cxx is None:
        parser.error("C++ compiler not found; add it to PATH or pass --cxx")
    # Native GCC's runtime DLLs must also be discoverable on Windows.
    os.environ["PATH"] = str(Path(cxx).parent) + os.pathsep + os.environ["PATH"]
    output = args.out.resolve()
    run_case(cxx, output, "partial-dma", {})
    if args.check:
        for contract in ("tx", "app"):
            directory = output / f"{contract}-contract"
            directory.mkdir(parents=True, exist_ok=True)
            subprocess.run([str(build(cxx, directory, {}, contract=contract))], check=True, timeout=15)
            print(f"{contract} contract: PASS", flush=True)
        run_case(cxx, output, "copying", {"CHUNK": 240, "HOLD": 0, "STALL": 1})
        run_case(cxx, output, "one-byte-dma", {"CHUNK": 1, "HOLD": 2, "STALL": 500})
        run_case(cxx, output, "no-requests", {}, empty=True)
        run_case(cxx, output, "driver-fault", {"FAULT": 1})
    if args.app:
        verify_with_app(args.app, output)
    print(f"Integration example passed. Captures: {output}", flush=True)


if __name__ == "__main__":
    main()
