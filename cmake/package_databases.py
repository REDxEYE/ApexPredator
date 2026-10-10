"""Create portable LZMA-compressed snapshots of the game hash databases."""

import lzma
import sqlite3
import sys
import tarfile
from contextlib import closing
from pathlib import Path
from tempfile import NamedTemporaryFile, TemporaryDirectory


def main() -> None:
    source_dir = Path(sys.argv[1])
    with TemporaryDirectory(prefix="apex-databases-") as temporary:
        for name in source_dir.glob("*.db"):
            snapshot = Path(temporary) / name.name
            # SQLite's backup API includes committed WAL pages without modifying the source.
            with closing(sqlite3.connect(f"{name.resolve().as_uri()}?mode=ro", uri=True)) as database:
                with closing(sqlite3.connect(snapshot)) as copy:
                    database.backup(copy)
            with NamedTemporaryFile(dir=source_dir, prefix=f".{name.name}.", suffix=".tmp", delete=False) as output:
                staged_archive = Path(output.name)
            try:
                with tarfile.open(staged_archive, "w:xz") as archive:
                    archive.add(snapshot, arcname=name.name)
                # Read through the stream checksum before replacing a usable archive.
                with lzma.open(staged_archive, "rb") as compressed:
                    while compressed.read(1024 * 1024):
                        pass
                staged_archive.replace(source_dir / f"{name.name}.tar.xz")
            finally:
                staged_archive.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
