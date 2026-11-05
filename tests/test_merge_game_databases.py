#!/usr/bin/env python3
"""Isolated SQLite fixtures for the staged five-game database merger."""

from contextlib import closing
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "merge_game_databases.py"
SOURCES = {
    "generation_zero": "hashes.db",
    "rage2": "rage2_hashes.db",
    "second_extinction": "second_extinction_hashes.db",
    "just_cause_2": "jc2_hashes.db",
    "mad_max": "mad_max_hashes.db",
}
KNOWN_LOOKUP3 = 2035976115  # hashlittle("AnimationSet", 0)
KNOWN_MURMUR = 0x8453EE3581F31F39 - 2**64  # Murmur x64_128("text/master_eng.stringlookup", 0)


def dual(path, kv_rows, file_rows, version=1, file_primary_key=True):
    primary = ", PRIMARY KEY(lookup3,murmur)" if file_primary_key else ""
    with closing(sqlite3.connect(path)) as db, db:
        db.execute("CREATE TABLE kv (lookup3 INTEGER, murmur INTEGER, v TEXT, PRIMARY KEY(lookup3,murmur))")
        db.execute("CREATE TABLE files (lookup3 INTEGER, murmur INTEGER, name TEXT, size INTEGER, parent INTEGER"
                   + primary + ")")
        db.executemany("INSERT INTO kv VALUES (?,?,?)", kv_rows)
        db.executemany("INSERT INTO files VALUES (?,?,?,?,?)", file_rows)
        db.execute(f"PRAGMA user_version={version}")


def legacy(path, kv_rows, file_rows):
    with closing(sqlite3.connect(path)) as db, db:
        db.execute("CREATE TABLE kv (k INTEGER PRIMARY KEY, v TEXT)")
        db.execute("CREATE TABLE files (hash INTEGER PRIMARY KEY, name TEXT, size INTEGER, parent INTEGER)")
        db.executemany("INSERT INTO kv VALUES (?,?)", kv_rows)
        db.executemany("INSERT INTO files VALUES (?,?,?,?)", file_rows)


class DatabaseMergeTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="apex-db-merge-test-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        dual(self.root / SOURCES["generation_zero"],
             [(42, -13, "shared/name"), (71, 901, "gz-only")],
             [(42, -13, "gz/archive", 100, -8), (100, 0, None, 0, 0)])
        dual(self.root / SOURCES["rage2"],
             [(42, -13, "shared/name"), (81, 0, "text/master_eng.stringlookup")],
             [(42, -13, "rage2/archive", 200, 812), (81, 0, "text/master_eng.stringlookup", 9, -1)],
             version=0)
        legacy(self.root / SOURCES["second_extinction"],
               [(KNOWN_MURMUR, "text/master_eng.stringlookup")],
               [(KNOWN_MURMUR, "text/master_eng.stringlookup", 300, 73), (-10, None, 0, -11)])
        legacy(self.root / SOURCES["just_cause_2"],
               [(KNOWN_LOOKUP3, "AnimationSet")],
               [(KNOWN_LOOKUP3, "AnimationSet", 400, 74), (101, None, 1, 0)])
        dual(self.root / SOURCES["mad_max"],
             [(93, -(1 << 63), "mad-only")],
             [(42, -13, "mad/archive", 500, 75), (93, -(1 << 63), None, 0, 0)])

    def run_merge(self, *args, succeeds=True):
        result = subprocess.run(
            [sys.executable, str(SCRIPT), "--source-dir", str(self.root), *map(str, args)],
            capture_output=True, text=True,
        )
        if succeeds:
            self.assertEqual(result.returncode, 0, result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, result.stdout)
        return result

    def test_merge_dual_legacy_and_live_wal(self):
        output = self.root / "separate-output.db"
        rage2 = self.root / SOURCES["rage2"]
        with closing(sqlite3.connect(rage2)) as writer:
            self.assertEqual(writer.execute("PRAGMA journal_mode=WAL").fetchone(), ("wal",))
            writer.execute("INSERT INTO kv VALUES (?,?,?)", (87, 0, "committed/in/wal"))
            writer.execute("INSERT INTO files VALUES (?,?,?,?,?)", (87, 0, "committed/in/wal", 777, -5))
            writer.commit()  # Keep the source connection open, forcing the backup to read its WAL.
            self.run_merge("--output", output)
            self.assertTrue((self.root / SOURCES["generation_zero"]).is_file())
            self.assertFalse((self.root / "generation_zero_hashes.db").exists())
            with closing(sqlite3.connect(output)) as db:
                self.assertEqual(db.execute("PRAGMA user_version").fetchone(), (2,))
                self.assertEqual(db.execute("PRAGMA integrity_check").fetchone(), ("ok",))
                tables = {row[0] for row in db.execute("SELECT name FROM sqlite_master WHERE type='table'")}
                self.assertEqual(tables, {"kv", *("files_" + game for game in SOURCES)})
                indexes = {row[0] for row in db.execute("SELECT name FROM sqlite_master WHERE type='index'")}
                for game in SOURCES:
                    self.assertIn(f"idx_files_{game}_murmur", indexes)
                    self.assertIn(f"idx_files_{game}_parent", indexes)
                self.assertIn("idx_kv_murmur", indexes)
                self.assertEqual(db.execute("SELECT count(*) FROM kv").fetchone(), (7,))
                self.assertEqual(db.execute("SELECT v FROM kv WHERE lookup3=42 AND murmur=-13").fetchone(),
                                 ("shared/name",))
                self.assertEqual(db.execute("SELECT name,size,parent FROM files_generation_zero "
                                            "WHERE lookup3=42 AND murmur=-13").fetchone(),
                                 ("gz/archive", 100, -8))
                self.assertEqual(db.execute("SELECT name,size,parent FROM files_rage2 "
                                            "WHERE lookup3=42 AND murmur=-13").fetchone(),
                                 ("rage2/archive", 200, 812))
                self.assertEqual(db.execute("SELECT name,size,parent FROM files_mad_max "
                                            "WHERE lookup3=42 AND murmur=-13").fetchone(),
                                 ("mad/archive", 500, 75))
                self.assertEqual(db.execute("SELECT size,parent FROM files_rage2 WHERE lookup3=87").fetchone(),
                                 (777, -5))
                self.assertEqual(db.execute("SELECT murmur FROM kv WHERE lookup3=87").fetchone() != (0,), True)
                self.assertEqual(db.execute("SELECT murmur FROM kv WHERE lookup3=81").fetchone(),
                                 (KNOWN_MURMUR,))
                self.assertEqual(db.execute("SELECT lookup3 FROM kv WHERE murmur=?", (KNOWN_MURMUR,)).fetchall().__len__(),
                                 2)  # Legacy Murmur-domain row and the v0 Rage 2 row retain distinct lookup3s.
                self.assertEqual(db.execute("SELECT lookup3,murmur,name,size,parent FROM files_second_extinction "
                                            "WHERE murmur=?", (KNOWN_MURMUR,)).fetchone()[1:],
                                 (KNOWN_MURMUR, "text/master_eng.stringlookup", 300, 73))
                self.assertEqual(db.execute("SELECT lookup3,murmur,name,size,parent FROM files_just_cause_2 "
                                            "WHERE lookup3=?", (KNOWN_LOOKUP3,)).fetchone()[2:],
                                 ("AnimationSet", 400, 74))
                self.assertEqual(db.execute("SELECT lookup3,murmur,name,size,parent FROM files_just_cause_2 "
                                            "WHERE lookup3=101").fetchone(), (101, 0, None, 1, 0))
                self.assertEqual(db.execute("SELECT murmur,name FROM files_generation_zero WHERE lookup3=100")
                                 .fetchone(), (0, None))
                self.assertEqual(db.execute("SELECT murmur FROM files_rage2 WHERE lookup3=87").fetchone() != (0,),
                                 True)
                self.assertEqual(db.execute("SELECT murmur FROM kv WHERE lookup3=93").fetchone(), (-(1 << 63),))

    def test_dry_run_does_not_replace_output(self):
        output = self.root / "output.db"
        output.write_bytes(b"not a database")
        result = self.run_merge("--output", output, "--dry-run")
        self.assertIn("Dry run", result.stdout)
        self.assertEqual(output.read_bytes(), b"not a database")
        self.assertFalse((self.root / "generation_zero_hashes.db").exists())

    def test_in_place_preserves_generation_zero_snapshot(self):
        self.run_merge()
        backup = self.root / "generation_zero_hashes.db"
        self.assertTrue(backup.is_file())
        with closing(sqlite3.connect(backup)) as original:
            self.assertEqual(original.execute("SELECT count(*) FROM kv").fetchone(), (2,))
            self.assertEqual(original.execute("SELECT count(*) FROM files").fetchone(), (2,))
        with closing(sqlite3.connect(self.root / "hashes.db")) as combined:
            self.assertEqual(combined.execute("PRAGMA user_version").fetchone(), (2,))
            self.assertEqual(combined.execute("SELECT count(*) FROM files_mad_max").fetchone(), (2,))

    def test_conflicting_global_kv_leaves_output_unchanged(self):
        with closing(sqlite3.connect(self.root / SOURCES["rage2"])) as db, db:
            db.execute("UPDATE kv SET v='different/name' WHERE lookup3=42 AND murmur=-13")
        output = self.root / "existing.db"
        output.write_bytes(b"unchanged")
        result = self.run_merge("--output", output, succeeds=False)
        self.assertIn("conflicting global KV names", result.stderr)
        self.assertEqual(output.read_bytes(), b"unchanged")

    def test_conflicting_file_metadata_leaves_source_intact(self):
        rage2 = self.root / SOURCES["rage2"]
        with closing(sqlite3.connect(rage2)) as db, db:
            db.execute("DROP TABLE files")
            db.execute("CREATE TABLE files (lookup3 INTEGER, murmur INTEGER, name TEXT, size INTEGER, parent INTEGER)")
            db.executemany("INSERT INTO files VALUES (?,?,?,?,?)",
                           [(42, -13, "same/path", 10, 1), (42, -13, "same/path", 11, 1)])
        output = self.root / "new.db"
        result = self.run_merge("--output", output, succeeds=False)
        self.assertIn("conflicting metadata in incoming_files", result.stderr)
        self.assertFalse(output.exists())
        with closing(sqlite3.connect(rage2)) as db:
            self.assertEqual(db.execute("SELECT count(*) FROM files").fetchone(), (2,))

    def test_v0_backfill_deduplicates_identical_and_rejects_conflicting_rows(self):
        rage2 = self.root / SOURCES["rage2"]
        with closing(sqlite3.connect(rage2)) as db, db:
            db.execute("INSERT INTO kv VALUES (?,?,?)", (81, KNOWN_MURMUR, "text/master_eng.stringlookup"))
            db.execute("INSERT INTO files VALUES (?,?,?,?,?)",
                       (81, KNOWN_MURMUR, "text/master_eng.stringlookup", 9, -1))
        output = self.root / "deduplicated.db"
        self.run_merge("--output", output)
        with closing(sqlite3.connect(output)) as db:
            self.assertEqual(db.execute("SELECT count(*) FROM kv WHERE lookup3=81").fetchone(), (1,))
            self.assertEqual(db.execute("SELECT count(*) FROM files_rage2 WHERE lookup3=81").fetchone(), (1,))
        with closing(sqlite3.connect(rage2)) as db, db:
            db.execute("UPDATE files SET size=10 WHERE lookup3=81 AND murmur=?", (KNOWN_MURMUR,))
        result = self.run_merge("--output", output, succeeds=False)
        self.assertIn("conflicting metadata in incoming_files", result.stderr)
        with closing(sqlite3.connect(output)) as db:
            self.assertEqual(db.execute("SELECT size FROM files_rage2 WHERE lookup3=81").fetchone(), (9,))


if __name__ == "__main__":
    unittest.main()
