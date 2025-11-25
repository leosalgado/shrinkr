from ._core import CompressorFactory, Compressor


def compress(files: list[str], ctype: str = "rle") -> None:
    """
    Compresses a list of files using the specified algorithm.

    Params:
    - files: List of file paths to be compressed.
    - ctype: Name of the compression algorithm to be used.
      Could be 'rle', 'huffman', etc.
      If not specified, the default algorithm is 'rle'.
    """
    if not files:
        raise ValueError("No files provided")

    comp = CompressorFactory.create_by_name(ctype)
    comp.compress(files)


def decompress(files: list[str]) -> None:
    if not files:
        raise ValueError("No files provided")

    with open(files[0], "rb") as f:
        first_byte = f.read(1)[0]

    comp = CompressorFactory.create_by_id(first_byte)
    comp.decompress(files)


def list_ctypes() -> list[str]:
    return CompressorFactory.get_available_algorithms()


__all__ = ["compress", "decompress", "list_ctypes", "CompressorFactory", "Compressor"]
