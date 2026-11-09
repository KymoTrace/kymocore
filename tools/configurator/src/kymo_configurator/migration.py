# SPDX-License-Identifier: MIT
"""Explicit schema migration entry point; preserves unknown/user data."""
from __future__ import annotations
import copy
from typing import Any
from .model import CORE_CONFIG_API_VERSION, SCHEMA_VERSION


def _to_v1(result: dict[str, Any]) -> None:
    """Early development fixture without schema_version."""
    result["schema_version"] = 1
    profiles = result.get("profiles")
    if not isinstance(profiles, dict):
        raise ValueError("Migration requires profiles to be an object")
    result.setdefault("active_profile", next(iter(profiles), "development"))
    result.setdefault("core_config_api_version", 1)


def _v1_to_v2(result: dict[str, Any]) -> None:
    """KymoCore 7.0: configuration API 2 and use-case presets per profile."""
    from .presets import matching_preset
    result["schema_version"] = 2
    result["core_config_api_version"] = CORE_CONFIG_API_VERSION
    profiles = result.get("profiles")
    if isinstance(profiles, dict):
        for profile in profiles.values():
            if isinstance(profile, dict):
                profile.setdefault("preset", matching_preset(profile))


def needs_migration(project: dict[str, Any]) -> bool:
    """True when the document is an older, migratable schema."""
    version = project.get("schema_version")
    return (version is None and "profiles" in project) or (type(version) is int and version == 1)


def migrate(project: dict[str, Any], to_schema: int = SCHEMA_VERSION) -> dict[str, Any]:
    if to_schema not in (1, SCHEMA_VERSION):
        raise ValueError(f"Supported target schemas are 1 and {SCHEMA_VERSION}")
    result = copy.deepcopy(project)
    version = result.get("schema_version")
    if version is None and "profiles" in result:
        _to_v1(result)
        version = 1
    if type(version) is not int:
        raise ValueError(f"No migration path from schema {version!r} to {to_schema}")
    if version == to_schema:
        return result
    if version == 1 and to_schema == 2:
        _v1_to_v2(result)
        return result
    raise ValueError(f"No migration path from schema {version!r} to {to_schema}")
