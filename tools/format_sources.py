#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Apply the repository indentation/braces and declaration-comment spacing rules."""
import argparse
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang-format", default="clang-format")
    args = parser.parse_args()
    paths = sorted(path for folder in ("src", "examples", "test")
                   for path in (ROOT / folder).rglob("*")
                   if path.suffix in (".c", ".cpp", ".h", ".ino")
                   and not any(part in {".pio", "__pycache__"} for part in path.parts))
    # Long trailing field documentation must not split the type from its name.
    # Move that documentation above the field before applying indentation.
    field_doc = re.compile(
        r"(?m)^([ \t]*)([\w:*]+(?:[ \t]+[\w:*]+)*[ \t\n]+\**\w+"
        r"(?:\[[^\]\n]+\])?;)[ \t]*/\*\*<[ \t]*(.*?)[ \t]*\*/")
    for path in paths:
        if path.suffix == ".h":
            source = path.read_text(encoding="utf-8")
            source = field_doc.sub(
                lambda match: match[1] + "/** @brief " + match[3] + " */\n" +
                match[1] + " ".join(match[2].split()), source)
            path.write_text(source, encoding="utf-8")
    subprocess.run([args.clang_format, "-i", *map(str, paths)], cwd=ROOT, check=True)
    for path in paths:
        output = []
        is_block_comment = False
        comment_indent = ""
        for line in path.read_text(encoding="utf-8").splitlines():
            stripped = line.lstrip()
            if is_block_comment and stripped.startswith("*"):
                line = comment_indent + " " + stripped
            if stripped.startswith(("/*", "//")) and not is_block_comment and output:
                previous = output[-1].strip()
                if previous and previous != "{" and not previous.startswith(("/*", "*", "//")) and not previous.endswith("\\"):
                    output.append("")
            output.append(line)
            if stripped.startswith("/*") and "*/" not in line:
                comment_indent = line[:len(line) - len(line.lstrip())]
                is_block_comment = True
            if "*/" in line:
                is_block_comment = False
        path.write_text("\n".join(output) + "\n", encoding="utf-8")
    print(f"Formatted {len(paths)} C/C++/Arduino files.")


if __name__ == "__main__":
    main()
