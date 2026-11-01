#!/usr/bin/env python3
"""Download Luca Meschiari's 9 GitHub ALPR reference images for LOCAL testing.

The files are deliberately not committed or included in redistributed ZIPs.
See: https://github.com/LucaMeschiari/LicensePlateRecognition
"""
from pathlib import Path
from urllib.request import Request, urlopen
from urllib.error import URLError
import argparse

URL = "https://raw.githubusercontent.com/LucaMeschiari/LicensePlateRecognition/master/LicensePlate{}.jpg"
DEFAULT_DIR = Path(__file__).resolve().parents[1] / "tests" / "reference_images"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_DIR)
    parser.add_argument("--force", action="store_true", help="Redownload existing files")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    failures = []
    for i in range(1, 10):
        target = args.output / f"LicensePlate{i}.jpg"
        if target.is_file() and not args.force:
            print(f"SKIP {target.name}: already exists")
            continue
        try:
            with urlopen(Request(URL.format(i), headers={"User-Agent": "ALPR-Evaluation/1.0"}), timeout=15) as response:
                raw = response.read(5_000_001)
            if len(raw) > 5_000_000 or not raw.startswith(b"\xff\xd8\xff"):
                raise ValueError("unexpected response (not a small JPEG image)")
            target.write_bytes(raw)
            print(f"OK   {target.name}: {len(raw)} bytes")
        except (URLError, OSError, ValueError) as exc:
            failures.append(target.name)
            print(f"FAIL {target.name}: {exc}")
    print(f"Downloaded/available: {9 - len(failures)}/9")
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
