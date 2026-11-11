# SPDX-License-Identifier: MIT
"""Read-only GCC/Clang configuration audit of compile_commands argument arrays.

This reproduces preprocessing, not the provenance of an existing object. Always
perform a clean integration build after a successful audit.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

FEATURES = ('RUNTIME', 'TX', 'ENDPOINT', 'X', 'Z', 'TIMESTAMP', 'CRC', 'DECODER', 'CPP',
            'PARAMETERS', 'WATCH', 'MEMORY', 'TRACE', 'TRACE_ISR', 'TRACE_SCHEDULER',
            'TRACE_FLOW', 'TRACE_STACK_POINTER')
BOOLEAN_MACROS = ('KYMO_ENABLED', 'KYMO_ENABLE_CRC_TABLE') + tuple('KYMO_ENABLE_' + key for key in FEATURES) + tuple(
    'KYMO_EFFECTIVE_' + key for key in FEATURES)
SIZE_MACROS = ('KYMO_MAX_CHANNELS', 'KYMO_TRACE_CORE_COUNT', 'KYMO_TRACE_EVENTS_PER_CORE',
               'KYMO_TRACE_MAX_CONTEXTS', 'KYMO_TRACE_MAX_DEPTH', 'KYMO_TRACE_MAX_ISR_DEPTH',
               'KYMO_MUX_ADMIN_RECORD_MAX', 'KYMO_MAX_PARAMETERS', 'KYMO_MAX_WATCHES',
               'KYMO_WATCH_MIN_PERIOD_MS', 'KYMO_MAX_MEMORY_ENTRIES')
MACROS = BOOLEAN_MACROS + SIZE_MACROS


def inspect_entry(entry: dict) -> dict:
    """Evaluate requested/effective booleans after the actual source's includes."""
    if not isinstance(entry, dict):
        raise ValueError("Compile database entries must be objects")
    directory = Path(entry['directory']).resolve()
    source = Path(entry['file'])
    source = source.resolve() if source.is_absolute() else (directory / source).resolve()
    arguments = entry.get('arguments')
    if not isinstance(arguments, list) or not arguments or not all(isinstance(x, str) for x in arguments):
        raise ValueError('Compile database entries must provide an arguments array; shell command strings are unsupported')
    filtered = [arguments[0]]
    skip = False
    for arg in arguments[1:]:
        if skip:
            skip = False
            continue
        if arg.startswith(('@', '-Wp,', '-save-temps', '-fdump-', '-fplugin')):
            raise ValueError(f'Unsupported compiler argument for a read-only audit: {arg}')
        if arg in ('-o', '-MF', '-MT', '-MQ', '-MJ'):
            skip = True
            continue
        if arg in ('-c', '-MD', '-MMD', '-MP', '-M', '-MM'):
            continue
        if arg.startswith(('-o', '-MF', '-MT', '-MQ', '-MJ')):
            continue
        path = Path(arg)
        if not arg.startswith('-') and (directory / path).resolve() == source:
            continue
        filtered.append(arg)
    with tempfile.TemporaryDirectory(prefix='kymo-config-audit-') as temp:
        probe = Path(temp) / ('audit.c' if source.suffix == '.c' else 'audit.cpp')
        lines = [f'#include "{source.as_posix()}"', '#ifdef KYMO_CONFIG_H']
        for name in BOOLEAN_MACROS:
            lines.extend((f'#if {name}', f'KYMO_AUDIT "{name}" 1', '#else', f'KYMO_AUDIT "{name}" 0', '#endif'))
        # Ask the target preprocessor to evaluate integer expressions as bits.
        # Comparing their expanded text would treat 16, 0x10 and (8 + 8) as
        # different configurations. Resource settings fit in uint32_t.
        for name in SIZE_MACROS:
            lines.append(f'KYMO_AUDIT "{name}" 0')
            for bit in range(32):
                lines.extend((f'#if (({name}) & (1ULL << {bit}))',
                              f'KYMO_AUDIT_BIT "{name}" {1 << bit}', '#endif'))
        lines.extend(('#ifdef KYMO_USER_CONFIG_HEADER', 'KYMO_AUDIT_HEADER KYMO_USER_CONFIG_HEADER',
                      '#endif', '#endif'))
        probe.write_text('\n'.join(lines) + '\n', encoding='utf-8')
        result = subprocess.run([*filtered, '-E', '-P', '-H', str(probe)], cwd=directory,
                                capture_output=True, encoding="utf-8", errors="replace", timeout=120)
        if result.returncode:
            raise ValueError(f'{source}: preprocessing failed\n{result.stderr}')
        values = {}
        user_header = None
        for line in result.stdout.splitlines():
            fields = line.split(maxsplit=2)
            if len(fields) == 3 and fields[0] == 'KYMO_AUDIT' and fields[1].strip('"') in MACROS:
                values[fields[1].strip('"')] = int(fields[2])
            elif len(fields) == 3 and fields[0] == 'KYMO_AUDIT_BIT' and fields[1].strip('"') in SIZE_MACROS:
                values[fields[1].strip('"')] |= int(fields[2])
            elif line.startswith("KYMO_AUDIT_HEADER "):
                user_header = line.removeprefix("KYMO_AUDIT_HEADER ").strip().strip('<>"')
        if values and len(values) != len(MACROS):
            raise ValueError(f'{source}: incomplete macro audit')
        header = None
        if user_header is not None:
            wanted = Path(user_header)
            config_depth = None
            for line in result.stderr.splitlines():
                match = re.fullmatch(r'(\.+) (.+)', line)
                if not match:
                    continue
                depth = len(match[1])
                path = Path(match[2])
                if config_depth is not None and depth <= config_depth:
                    config_depth = None
                if path.name == 'kymo_config.h':
                    config_depth = depth
                elif (config_depth is not None and depth == config_depth + 1
                      and path.parts[-len(wanted.parts):] == wanted.parts):
                    header = str((directory / path).resolve())
                    break
            if header is None:
                raise ValueError(f'{source}: cannot identify the included user config header {user_header!r}')
        return {'file': str(source), 'values': values, 'user_config_header': header}


def check_commands(entries: list[dict]) -> list[dict]:
    """Raise on differences; return the compiler-resolved settings for every TU."""
    if not isinstance(entries, list) or not entries:
        raise ValueError('No translation units selected')
    reports = [report for entry in entries if (report := inspect_entry(entry))['values']]
    if not reports:
        raise ValueError('No translation unit includes kymo_config.h')
    reference = reports[0]
    for report in reports[1:]:
        differences = [name for name in MACROS if report['values'][name] != reference['values'][name]]
        if report['user_config_header'] != reference['user_config_header']:
            differences.append('KYMO_USER_CONFIG_HEADER')
        if differences:
            raise ValueError(f"Configuration mismatch: {reference['file']} versus {report['file']}: " + ', '.join(differences))
    return reports


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('compile_commands', type=Path)
    args = parser.parse_args(argv)
    try:
        reports = check_commands(json.loads(args.compile_commands.read_text(encoding='utf-8')))
    except (ValueError, KeyError, TypeError, OSError, subprocess.TimeoutExpired) as exc:
        parser.exit(1, str(exc) + '\n')
    print(json.dumps({'translation_units': reports,
                      'evidence': 'preprocessor audit; existing object provenance is not verified'}, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
