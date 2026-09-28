"""Create portable LZMA-compressed snapshots of the game hash databases."""

import sqlite3
import subprocess
import sys
from contextlib import closing
from pathlib import Path
from tempfile import TemporaryDirectory


def main() -> None:
    cmake = sys.argv[1]
    source_dir = Path(sys.argv[2])
    with TemporaryDirectory(prefix="apex-databases-") as temporary:
        for name in ("hashes.db", "rage2_hashes.db"):
            source = source_dir / name
            snapshot = Path(temporary) / name
            # SQLite's backup API includes committed WAL pages without modifying the source.
            with closing(sqlite3.connect(f"{source.resolve().as_uri()}?mode=ro", uri=True)) as database:
                with closing(sqlite3.connect(snapshot)) as copy:
                    database.backup(copy)
            subprocess.run(
                [cmake, "-E", "tar", "cJf", str(source_dir / f"{name}.tar.xz"), name],
                cwd=temporary,
                check=True,
            )


if __name__ == "__main__":
    main()
