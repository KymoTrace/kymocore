# SPDX-License-Identifier: MIT
"""Build integrations pinned to the immutable directory containing this export.

No export consults the active selector during compilation. Include the selected
snapshot's export once at configure/build start; regeneration cannot mix sources.
"""
from __future__ import annotations


def render_exports(*, watch: bool = False) -> dict[str, str]:
    """Return deterministic build files; application callback files remain owned by users."""
    exports = {
        "kymo_build_export.cmake": r'''# Generated immutable Kymo configuration snapshot. Requires CMake 3.19+.
# include("<snapshot>/kymo_build_export.cmake")
# kymo_configure_targets(application_target core_target)
set(KYMO_SNAPSHOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
function(kymo_configure_targets application_target core_target)
    if(NOT TARGET "${application_target}" OR NOT TARGET "${core_target}")
        message(FATAL_ERROR "Define application and Core targets before configuring Kymo")
    endif()
    if(NOT TARGET kymo_project_configuration)
        add_library(kymo_project_configuration INTERFACE)
        target_include_directories(kymo_project_configuration INTERFACE "${KYMO_SNAPSHOT_DIR}")
        target_compile_definitions(kymo_project_configuration INTERFACE
            "KYMO_USER_CONFIG_HEADER=\"kymo_user_config.h\"")
    else()
        message(FATAL_ERROR "A Kymo configuration has already been bound to this build")
    endif()
    target_link_libraries("${application_target}" PRIVATE kymo_project_configuration)
    target_link_libraries("${core_target}" PUBLIC kymo_project_configuration)
    target_sources("${application_target}" PRIVATE "${KYMO_SNAPSHOT_DIR}/kymo_generated_config.cpp")
endfunction()
''',
        "kymo_build_export.py": '''# Generated PlatformIO pre-script; reference this immutable file in extra_scripts.
from pathlib import Path
Import("env")
snapshot = Path(Dir(".").srcnode().abspath)
env.Append(CPPPATH=[str(snapshot)])
# SCons ignores include paths when deciding what to rebuild and cannot see the
# macro-included config header; naming the snapshot forces a full rebuild on change.
env.Append(CPPDEFINES=[("KYMO_USER_CONFIG_HEADER", r'\\"kymo_user_config.h\\"'),
                       ("KYMO_CONFIG_SNAPSHOT", "kymo_" + snapshot.name.replace("-", "_"))])
env.Append(CXXFLAGS=["-std=c++11"])
''',
        "kymo_build_sources.py": '''# Generated PlatformIO post-script; run after library include resolution.
from pathlib import Path
Import("env", "projenv")
snapshot = Path(Dir(".").srcnode().abspath)
library = projenv.BuildLibrary(str(Path(env.subst("$BUILD_DIR")) / "kymo-generated"),
                           str(snapshot), src_filter=["+<kymo_generated_config.cpp>"])
env.Append(LIBS=[library])
''',
        "kymo_build_export.mk": r'''# Include this file by its immutable snapshot path. No Python/Qt is used.
# Set KYMO_SNAPSHOT_DIR before including this file if the include path has spaces.
ifndef KYMO_SNAPSHOT_DIR
KYMO_SNAPSHOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
endif
kymo_empty :=
kymo_space := $(kymo_empty) $(kymo_empty)
kymo_snapshot_make_path := $(subst $(kymo_space),\$(kymo_space),$(KYMO_SNAPSHOT_DIR))
CPPFLAGS += -I"$(KYMO_SNAPSHOT_DIR)" -DKYMO_USER_CONFIG_HEADER=\"kymo_user_config.h\"
KYMO_GENERATED_SOURCES := $(kymo_snapshot_make_path)/kymo_generated_config.cpp
# Use CPPFLAGS for EVERY Core and application translation unit.
# Add KYMO_GENERATED_SOURCES to application sources. Use distinct build directories per profile.
''',
        "kymo_build_export.platformio.ini": '''; In your PlatformIO environment, reference this snapshot's script:
; extra_scripts =
;     pre:<path-to-this-snapshot>/kymo_build_export.py
;     post:<path-to-this-snapshot>/kymo_build_sources.py
; The pre-script sets global definitions; the post-script builds the same snapshot.
; Never use build_src_flags: those omit library translation units.
''',
    }
    if watch:
        exports["kymo_build_export.cmake"] = exports["kymo_build_export.cmake"].replace(
            'PRIVATE "${KYMO_SNAPSHOT_DIR}/kymo_generated_config.cpp")',
            'PRIVATE "${KYMO_SNAPSHOT_DIR}/kymo_generated_config.cpp"\n'
            '        "${KYMO_SNAPSHOT_DIR}/kymo_generated_watch.cpp")')
        exports["kymo_build_sources.py"] = exports["kymo_build_sources.py"].replace(
            '"+<kymo_generated_config.cpp>"',
            '"+<kymo_generated_config.cpp>", "+<kymo_generated_watch.cpp>"')
        exports["kymo_build_export.mk"] = exports["kymo_build_export.mk"].replace(
            '$(kymo_snapshot_make_path)/kymo_generated_config.cpp\n',
            '$(kymo_snapshot_make_path)/kymo_generated_config.cpp $(kymo_snapshot_make_path)/kymo_generated_watch.cpp\n')
    return exports
