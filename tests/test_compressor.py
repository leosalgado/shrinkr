import shrinkr as sh
import pytest
from pathlib import Path
import tempfile


@pytest.mark.parametrize("algorithm", sh.list_ctypes())
def test_compress(algorithm):
    with tempfile.TemporaryDirectory() as tmpdir:
        file = Path(tmpdir) / "test.txt"
        file.write_bytes(b"aaaa" * 100)

        sh.compress([str(file)], ctype=algorithm)

        compressed_file = Path(tmpdir) / "test.leo"
        assert compressed_file.exists()
        assert compressed_file.stat().st_size < file.stat().st_size


@pytest.mark.parametrize("algorithm", sh.list_ctypes())
def test_compress_decompress(algorithm):
    with tempfile.TemporaryDirectory() as tmpdir:
        file = Path(tmpdir) / "test.txt"
        file.write_bytes(b"aaaa" * 100)

        sh.compress([str(file)], ctype=algorithm)

        compressed_file = Path(tmpdir) / "test.leo"
        sh.decompress([str(compressed_file)])

        decompressed_file = Path(tmpdir) / "test" / "test.txt"
        assert decompressed_file.exists()
        assert decompressed_file.read_bytes() == file.read_bytes()


@pytest.mark.parametrize("algorithm", sh.list_ctypes())
def test_multiple_files(algorithm):
    with tempfile.TemporaryDirectory() as tmpdir:
        files = []
        contents = [b"aaaa" * 100, b"bbbb" * 50, b"cccc" * 75]
        for i, content in enumerate(contents, start=1):
            f = Path(tmpdir) / f"file{i}.txt"
            f.write_bytes(content)
            files.append(f)

        sh.compress([str(f) for f in files], ctype=algorithm)

        compressed_file = Path(tmpdir) / "file1.leo"
        sh.decompress([str(compressed_file)])

        decompressed_dir = Path(tmpdir) / "file1"
        for i, original_file in enumerate(files, start=1):
            decompressed_file = decompressed_dir / original_file.name
            assert decompressed_file.exists()
            assert decompressed_file.read_bytes() == original_file.read_bytes()


@pytest.mark.parametrize("algorithm", sh.list_ctypes())
def test_empty_file(algorithm):
    with tempfile.TemporaryDirectory() as tmpdir:
        file = Path(tmpdir) / "empty.txt"
        file.write_bytes(b"")

        sh.compress([str(file)], ctype=algorithm)

        compressed_file = Path(tmpdir) / "empty.leo"
        assert compressed_file.exists()
        assert compressed_file.stat().st_size > 0

        sh.decompress([str(compressed_file)])


@pytest.mark.parametrize("algorithm", sh.list_ctypes())
def test_one_or_more_empty_files(algorithm):
    with tempfile.TemporaryDirectory() as tmpdir:
        files = []
        contents = [b"", b"bbbb" * 50, b"", b"dddd" * 90]
        for i, content in enumerate(contents, start=1):
            f = Path(tmpdir) / f"file{i}.txt"
            f.write_bytes(content)
            files.append(f)

        sh.compress([str(f) for f in files], ctype=algorithm)

        compressed_file = Path(tmpdir) / "file1.leo"
        sh.decompress([str(compressed_file)])

        decompressed_dir = Path(tmpdir) / "file1"

        for original_file, content in zip(files, contents):
            decompressed_file = decompressed_dir / original_file.name
            assert decompressed_file.exists()
            assert decompressed_file.read_bytes() == content
