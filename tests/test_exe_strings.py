#!/usr/bin/env python3
"""Check executable string extraction with ASCII and UTF-16LE samples."""

from pathlib import Path
import subprocess
import sys
import tempfile


script = Path(__file__).resolve().parents[1] / "scripts/extract_exe_strings.py"
with tempfile.TemporaryDirectory(prefix="apex-exe-strings-") as temp:
    root = Path(temp)
    executable = root / "sample.exe"
    output = root / "strings.txt"
    ascii_strings = [
        "ValidName", "/units/path.ddsc", "\\game\\folder\\asset.rbm", ".file_extension",
        "-valid-option", "Hello\nWorld", "Carriage\rReturn", "Tab\tField",
        "-&+B", "/@ v", ".?AVbad_alloc",
        *(character + "Invalid" for character in ";[]@#$%^&*()"),
    ]
    wide_strings = ["WideAsset", "ValidName", "Wide\nBroken", "[WideInvalid"]
    executable.write_bytes(
        b"MZ\x00" + b"\x00".join(text.encode("ascii") for text in ascii_strings)
        + b"\x00\x00" + b"\x00\x00".join(text.encode("utf-16le") for text in wide_strings)
        + b"\x00\x00" + b"NoNullTerminator\xff\x00"
    )
    subprocess.run([sys.executable, str(script), str(executable), str(output)], check=True)
    assert output.read_text(encoding="utf-8").splitlines() == sorted([
        "ValidName", "/units/path.ddsc", "\\game\\folder\\asset.rbm", ".file_extension",
        "WideAsset",
    ])
print("Executable string filtering passed")
