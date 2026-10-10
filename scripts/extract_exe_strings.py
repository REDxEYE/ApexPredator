#!/usr/bin/env python3
"""Extract null-terminated, printable ASCII and UTF-16LE strings from an executable."""

import argparse
import re
from pathlib import Path

ASCII_RUN = re.compile(rb"[\x20-\x7e\r\n\t]{4,}")
UTF16_RUN = re.compile(rb"(?:[\x20-\x7e\r\n\t]\x00){4,}")
STRUCTURAL = frozenset("_./\\- ")


def valid(text: str) -> bool:
    if len(text) < 4 or not text.isprintable():
        return False
    # Permit file extensions and rooted/relative paths, not other leading punctuation.
    first = text.lstrip("./\\")
    if not first or not first[0].isascii() or not (first[0].isalnum() or first[0] == "_"):
        return False
    return sum(char.isalnum() or char in STRUCTURAL for char in text) * 5 > len(text) * 4


def extract(data: bytes) -> list[str]:
    result: set[str] = set()
    for match in ASCII_RUN.finditer(data):
        if data[match.end():match.end() + 1] != b"\0":
            continue
        text = match.group().decode("ascii")
        if valid(text):
            result.add(text)
    for match in UTF16_RUN.finditer(data):
        if data[match.end():match.end() + 2] != b"\0\0":
            continue
        text = match.group().decode("utf-16le")
        if valid(text):
            result.add(text)
    return sorted(result)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output", type=Path, nargs="?", default=Path("strings/just_cause_2/just_cause_2_exe_strings.txt"))
    args = parser.parse_args()
    strings = extract(args.executable.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(strings) + "\n", encoding="utf-8")
    print(f"Wrote {len(strings)} strings to {args.output}")


if __name__ == "__main__":
    main()
