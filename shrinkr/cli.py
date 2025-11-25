import questionary
import argparse
import shrinkr as sh
import sys


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=["compress", "decompress"])
    parser.add_argument("files", nargs="+")
    args = parser.parse_args()

    if args.mode == "compress":
        algorithm = questionary.select(
            "What compression algorithm do you wish to use?",
            choices=sh.list_ctypes(),
        ).ask()

        if algorithm is None:
            return 1

        comp = sh.CompressorFactory.create_by_name(algorithm)
        comp.compress(args.files)
        print(f"Compressed {len(args.files)} file(s) using {algorithm}")

    else:
        if len(args.files) > 1:
            print("\033[93mWarning:\033[0m only the first file will be decompressed")

        compressed_file = args.files[0]

        with open(compressed_file, "rb") as f:
            first_byte = f.read(1)[0]

        comp = sh.CompressorFactory.create_by_id(first_byte)
        comp.decompress([compressed_file])
        print(f"Decompressed {compressed_file}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
