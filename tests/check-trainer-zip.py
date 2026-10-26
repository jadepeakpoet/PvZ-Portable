#!/usr/bin/env python3
"""Verify the artifact produced by trainer_files_test (no third-party modules)."""
import sys
from pathlib import Path
from zipfile import ZipFile

manifest = Path(sys.argv[1])
archive = Path(manifest.read_text(encoding="utf-8"))
with ZipFile(archive) as zipped:
    assert zipped.testzip() is None, "ZIP CRC validation failed"
    assert zipped.read("目录/中文.txt") == b"Chinese UTF8 payload"
    assert zipped.read("plain.bin") == b"A\0B"
    assert zipped.getinfo("目录/empty/").is_dir()
    assert all(entry.flag_bits & 0x800 for entry in zipped.infolist())
    assert all(not entry.filename.startswith(("/", "\\")) for entry in zipped.infolist())
    assert all(".." not in Path(entry.filename).parts for entry in zipped.infolist())
print("PASS ZIP CRC, UTF8 names, binary payload, empty directories and relative paths")
