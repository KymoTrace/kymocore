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

# Independent wire expectations: do not import C++ implementation constants.
SYNC = b"\xa5\x5a"
DATA_DESCRIPTOR, CONTROL_DESCRIPTOR, TRACE_DESCRIPTOR = 0x40, 0x60, 0x80
PARAM_LIST, PARAM_DESCRIPTION = 0x01, 0x02
WATCH_INFO, WATCH_INFO_REPLY, WATCH_SET, WATCH_ACK, MEMORY_INFO = 0x03, 0x04, 0x05, 0x06, 0x07
SESSION_META, DICTIONARY, TRACE_BATCH, SESSION_END = 0x01, 0x02, 0x03, 0x06
SCHEMA_VERSION = 1
RECORD_GENERATION = 1
TRACE_EVENT_MARK = 3
TRACE_FRAME_MAX_BYTES = 240
CRC8_POLYNOMIAL = 0x07
CRC16_INITIAL = 0xFFFF
BYTE_BITS = 8
BYTE_MASK = 0xFF
BYTE_HIGH_BIT = 0x80
CRC8_BYTES = 1
CRC16 = struct.Struct("<H")
CRC32 = struct.Struct("<I")
TRACE_HEADER = struct.Struct("<2sBBHHQIH")
TRACE_BATCH_HEADER = struct.Struct("<BBH")
TRACE_EVENT = struct.Struct("<QIIIHHBBHI")
ADMIN_HEADER = struct.Struct("<BBIIII")
SESSION_END_HEADER = struct.Struct("<BBH")
SESSION_END_CORE = struct.Struct("<B3xIIIQ")
DATA_FRAME = struct.Struct("<2sBBfB")
CONTROL_HEADER = struct.Struct("<2sBB")
WATCH_SET_REQUEST = struct.Struct("<BBBIBH")

MEASUREMENT_CHANNEL, GAIN_PARAMETER_ID, WATCH_CHANNEL, MEMORY_CHANNEL = 0, 100, 150, 220
WATCH_SLOT = 0
WATCH_GAIN_ADDRESS = 0x20000000
WATCH_TYPE_FLOAT32 = 7
WATCH_PERIOD_MS = 100
WATCH_STATUS_OK = 0
GAIN_INITIAL = 1.0
GAIN_REQUESTED = 2.5
GAIN_IN_CORRUPT_FRAME = 9.0
GAIN_OUTSIDE_ALLOWED_RANGE = 99.0
MEMORY_FREE_BYTES = 2048
TRACE_SESSION_ID, TRACE_CONTEXT_ID, TRACE_MARKER_ID = 1, 1, 1
TRACE_CORE_ID, TRACE_CLOCK_DOMAIN, TRACE_MARKER_DEPTH = 0, 0, 0
TRACE_CORE_COUNT = 1
TRACE_CLOCK_HZ = 1000
MIN_PROGRESS_SAMPLES = 5
PROCESS_TIMEOUT_SECONDS = 15
EXIT_TX_CONTRACT_FAILURE = 2
APP_FRAGMENT_SIZES = (1, 7, 64, 4096)


def crc8(data: bytes) -> int:
    value = 0
    for byte in data:
        value ^= byte
        for _ in range(BYTE_BITS):
            value = ((value << 1) ^ (CRC8_POLYNOMIAL if value & BYTE_HIGH_BIT else 0)) & BYTE_MASK
    return value


def control(payload: bytes) -> bytes:
    body = bytes((CONTROL_DESCRIPTOR, len(payload))) + payload
    return SYNC + body + bytes((crc8(body),))


def parameter(value: float) -> bytes:
    body = bytes((DATA_DESCRIPTOR, GAIN_PARAMETER_ID)) + struct.pack("<f", value)
    return SYNC + body + bytes((crc8(body),))


def requests() -> bytes:
    corrupt = bytearray(parameter(GAIN_IN_CORRUPT_FRAME))
    corrupt[-CRC8_BYTES] ^= BYTE_MASK
    return (b"noise" + parameter(GAIN_REQUESTED) + bytes(corrupt)
            + parameter(GAIN_OUTSIDE_ALLOWED_RANGE)
            + control(bytes((PARAM_LIST,))) + control(bytes((WATCH_INFO,)))
            + control(WATCH_SET_REQUEST.pack(WATCH_SET, WATCH_SLOT, WATCH_CHANNEL,
                      WATCH_GAIN_ADDRESS, WATCH_TYPE_FLOAT32, WATCH_PERIOD_MS)))


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
        assert capture[offset:offset + len(SYNC)] == SYNC, offset
        descriptor = capture[offset + len(SYNC)]
        if descriptor == TRACE_DESCRIPTOR:
            _, _, kind, payload_length, flags, session, packet, header_crc = TRACE_HEADER.unpack_from(capture, offset)
            length = TRACE_HEADER.size + payload_length + CRC16.size
            frame = capture[offset:offset + length]
            assert len(frame) == length <= TRACE_FRAME_MAX_BYTES
            header_body = frame[len(SYNC):TRACE_HEADER.size - CRC16.size]
            assert binascii.crc_hqx(header_body, CRC16_INITIAL) == header_crc
            assert binascii.crc_hqx(frame[len(SYNC):-CRC16.size], CRC16_INITIAL) == CRC16.unpack(frame[-CRC16.size:])[0]
            assert flags == 0 and session == TRACE_SESSION_ID
            packet_sequences.append(packet)
            payload = frame[TRACE_HEADER.size:-CRC16.size]
            if kind == TRACE_BATCH:
                schema, count, clock_domain = TRACE_BATCH_HEADER.unpack_from(payload)
                assert schema == SCHEMA_VERSION and clock_domain == TRACE_CLOCK_DOMAIN
                assert len(payload) == TRACE_BATCH_HEADER.size + count * TRACE_EVENT.size
                for event_offset in range(TRACE_BATCH_HEADER.size, len(payload), TRACE_EVENT.size):
                    event = TRACE_EVENT.unpack_from(payload, event_offset)
                    ticks, seq, context, _, symbol, depth, event_type, core, _, _ = event
                    assert ticks > 0
                    assert (context, symbol, depth, event_type, core) == (
                        TRACE_CONTEXT_ID, TRACE_MARKER_ID, TRACE_MARKER_DEPTH, TRACE_EVENT_MARK, TRACE_CORE_ID)
                    trace_sequences.append(seq)
            else:
                assert kind in (SESSION_META, DICTIONARY, SESSION_END), kind
                schema, record_kind, generation, transfer, total, fragment_offset = ADMIN_HEADER.unpack_from(payload)
                assert (schema, record_kind, generation) == (SCHEMA_VERSION, kind, RECORD_GENERATION)
                assert transfer > 0
                record = records.setdefault(kind, bytearray())
                record_lengths.setdefault(kind, total)
                assert record_lengths[kind] == total and fragment_offset == len(record)
                record.extend(payload[ADMIN_HEADER.size:])
                assert len(record) <= total
        else:
            assert descriptor in (DATA_DESCRIPTOR, CONTROL_DESCRIPTOR), descriptor
            if descriptor == DATA_DESCRIPTOR:
                length = DATA_FRAME.size
            else:
                _, _, payload_length = CONTROL_HEADER.unpack_from(capture, offset)
                length = CONTROL_HEADER.size + payload_length + CRC8_BYTES
            frame = capture[offset:offset + length]
            assert len(frame) == length
            assert crc8(frame[len(SYNC):-CRC8_BYTES]) == frame[-CRC8_BYTES]
            if descriptor == DATA_DESCRIPTOR:
                _, _, channel, value, _ = DATA_FRAME.unpack(frame)
                points.setdefault(channel, []).append(value)
            else:
                payload = frame[CONTROL_HEADER.size:-CRC8_BYTES]
                controls.setdefault(payload[0], []).append(payload)
        frame_count += 1
        offset += length
    assert packet_sequences == list(range(len(packet_sequences)))
    assert set(records) == {SESSION_META, DICTIONARY, SESSION_END}, set(records)
    for kind, record in records.items():
        assert len(record) == record_lengths[kind]
        assert binascii.crc32(record[:-CRC32.size]) == CRC32.unpack(record[-CRC32.size:])[0]
    metadata = json.loads(records[SESSION_META][:-CRC32.size])
    dictionary = json.loads(records[DICTIONARY][:-CRC32.size])
    for kind, value in ((SESSION_META, metadata), (DICTIONARY, dictionary)):
        assert bytes(records[kind][:-CRC32.size]) == json.dumps(value, sort_keys=True, separators=(",", ":")).encode()
    assert metadata["dictionary_hash"] == hashlib.sha256(records[DICTIONARY][:-CRC32.size]).hexdigest()
    assert metadata["clock"]["frequency_hz"] == str(TRACE_CLOCK_HZ)
    assert metadata["clock"]["clock_domain_id"] == TRACE_CLOCK_DOMAIN
    assert metadata["core_count"] == TRACE_CORE_COUNT
    assert dictionary["symbols"][0]["id"] == TRACE_MARKER_ID
    assert dictionary["contexts"][0]["id"] == TRACE_CONTEXT_ID
    assert dictionary["generation"] == RECORD_GENERATION
    assert trace_sequences and trace_sequences == list(range(len(trace_sequences)))
    end = records[SESSION_END][:-CRC32.size]
    assert SESSION_END_HEADER.unpack_from(end) == (SCHEMA_VERSION, TRACE_CORE_COUNT, 0)
    assert len(end) == SESSION_END_HEADER.size + TRACE_CORE_COUNT * SESSION_END_CORE.size
    core, produced, settled, drops, _ = SESSION_END_CORE.unpack_from(end, SESSION_END_HEADER.size)
    assert core == TRACE_CORE_ID
    assert produced == settled == len(trace_sequences) and drops == 0
    assert len(points.get(MEASUREMENT_CHANNEL, [])) >= MIN_PROGRESS_SAMPLES, "telemetry starved"
    assert len(points.get(MEMORY_CHANNEL, [])) >= MIN_PROGRESS_SAMPLES, "memory starved"
    assert all(value == MEMORY_FREE_BYTES for value in points[MEMORY_CHANNEL])
    assert MEMORY_INFO in controls and b"demo free" in controls[MEMORY_INFO][0]
    if with_requests:
        assert points.get(GAIN_PARAMETER_ID) and all(value == GAIN_REQUESTED for value in points[GAIN_PARAMETER_ID])
        assert len(points.get(WATCH_CHANNEL, [])) >= MIN_PROGRESS_SAMPLES
        assert all(value == GAIN_REQUESTED for value in points[WATCH_CHANNEL])
        assert max(points[MEASUREMENT_CHANNEL]) > GAIN_INITIAL
        assert max(points[MEASUREMENT_CHANNEL]) < GAIN_REQUESTED
        assert PARAM_DESCRIPTION in controls and b"gain" in controls[PARAM_DESCRIPTION][0]
        expected_ack = bytes((WATCH_ACK, WATCH_SLOT, WATCH_STATUS_OK))
        assert WATCH_INFO_REPLY in controls and expected_ack in controls.get(WATCH_ACK, [])
    else:
        assert GAIN_PARAMETER_ID not in points and WATCH_CHANNEL not in points
        assert max(points[MEASUREMENT_CHANNEL]) < GAIN_INITIAL
    return {"frames": frame_count, "trace_events": len(trace_sequences), "bytes": len(capture)}


def build(cxx: str, directory: Path, defines: dict[str, int], *, contract: str | None = None) -> Path:
    binary = directory / ("integration.exe" if os.name == "nt" else "integration")
    if contract == "tx":
        sources = [EXAMPLE / "tests/tx_test.cpp", ROOT / "src/transport/kymo_tx.cpp"]
    elif contract == "app":
        sources = [EXAMPLE / name for name in
                   ("tests/app_test.cpp", "kymo_app.cpp", "kymo_bindings.cpp")]
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
    completed = subprocess.run([str(binary), str(incoming), str(outgoing)], timeout=PROCESS_TIMEOUT_SECONDS)
    if defines.get("FAULT"):
        assert completed.returncode == EXIT_TX_CONTRACT_FAILURE, completed.returncode
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
        for chunk_size in APP_FRAGMENT_SIZES:
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
            session = store.session(TRACE_SESSION_ID)
            assert session.complete and not session.gaps, session.gaps
            assert session.dictionary["symbols"][0]["name"] == "main cycle"
        print(f"KymoStudio {capture_path.parent.name}: PASS ({len(APP_FRAGMENT_SIZES)} fragment sizes, complete trace session)", flush=True)


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
            subprocess.run([str(build(cxx, directory, {}, contract=contract))], check=True, timeout=PROCESS_TIMEOUT_SECONDS)
            print(f"{contract} contract: PASS", flush=True)
        run_case(cxx, output, "copying", {"CHUNK": TRACE_FRAME_MAX_BYTES, "HOLD": 0, "STALL": 1})
        run_case(cxx, output, "one-byte-dma", {"CHUNK": 1, "HOLD": 2, "STALL": 500})
        run_case(cxx, output, "no-requests", {}, empty=True)
        run_case(cxx, output, "driver-fault", {"FAULT": 1})
    if args.app:
        verify_with_app(args.app, output)
    print(f"Integration example passed. Captures: {output}", flush=True)


if __name__ == "__main__":
    main()
