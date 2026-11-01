#!/usr/bin/env python3
"""Run the compiled C++ ALPR executable on local reference JPEGs.

This script never invents labels. Optional CSV: filename,expected
Any file without an expected string is reported as unscored.
"""
import argparse
import csv
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
ALERT = re.compile(r"\[ALERT\] plate=([A-Z0-9]+) ocr_confidence=([0-9.]+)")


def load_truth(path):
    if not path:
        return {}
    with path.open(newline="", encoding="utf-8") as handle:
        rows = csv.DictReader(handle)
        if not {"filename", "expected"} <= set(rows.fieldnames or []):
            raise ValueError("Truth CSV must have filename,expected columns")
        return {r["filename"]: r["expected"].replace(" ", "").replace("-", "").upper()
                for r in rows if r["expected"].strip()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "build" / "alpr")
    parser.add_argument("--images", type=Path, default=ROOT / "tests" / "reference_images")
    parser.add_argument("--mode", choices=["auto", "crop", "both"], default="both")
    parser.add_argument("--debug", action="store_true", help="Save per-image debug diagnostics in report")
    parser.add_argument("--ocr-robust", action="store_true", help="Try extra OCR preprocessing variants")
    parser.add_argument("--min-confidence", type=float, default=20.0)
    parser.add_argument("--truth", type=Path, help="Optional filename,expected CSV with manually verified labels")
    parser.add_argument("--output", type=Path, default=ROOT / "results" / "reference_report.csv")
    args = parser.parse_args()
    binary = args.binary.resolve()
    if not binary.is_file():
        parser.error(f"C++ executable not found: {binary}. Build with cmake first.")
    images = sorted(args.images.glob("LicensePlate*.jpg"))
    if not images:
        parser.error(f"No JPEGs under {args.images}. Run scripts/fetch_reference_images.py first.")
    truth = load_truth(args.truth)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    rows = []
    for image in images:
        for mode in (["auto", "crop"] if args.mode == "both" else [args.mode]):
            command = [str(binary), "--input", str(image), "--min-confidence", str(args.min_confidence),
                       "--allow-numeric-only", "--output", str(ROOT / "results" / "reference_events.csv")]
            if args.ocr_robust:
                command.append("--ocr-robust")
            if mode == "crop":
                command.append("--plate-crop")
            if args.debug:
                command.extend(["--debug", "--debug-dir",
                                str(ROOT / "results" / "debug" / f"{image.stem}_{mode}")])
            start = time.monotonic()
            try:
                proc = subprocess.run(command, capture_output=True, text=True, timeout=45)
                elapsed = round((time.monotonic() - start) * 1000)
                match = ALERT.search(proc.stdout)
                value, confidence = (match.group(1), match.group(2)) if match else ("", "")
                expected = truth.get(image.name, "")
                correct = str(value == expected) if expected else "unscored"
                row = dict(filename=image.name, mode=mode, exit_code=proc.returncode,
                           recognized=value, ocr_confidence=confidence,
                           zero_confidence=(confidence != "" and float(confidence) <= 0), expected=expected,
                           exact_match=correct, wall_ms=elapsed,
                           error=(proc.stderr or proc.stdout).strip()[:4000] if args.debug or proc.returncode not in (0, 2) else "")
            except subprocess.TimeoutExpired:
                row = dict(filename=image.name, mode=mode, exit_code="timeout", recognized="",
                           ocr_confidence="", zero_confidence=False, expected=truth.get(image.name, ""),
                           exact_match="unscored", wall_ms="", error="45-second timeout")
            rows.append(row)
            print(f"{image.name:18s} {mode:4s} recognized={row['recognized'] or '-':12s} "
                  f"confidence={row['ocr_confidence'] or '-':>5s} "
                  f"exit={row['exit_code']} expected={row['expected'] or '?'}")
            if row["zero_confidence"]:
                print("   WARNING: OCR returned text but 0 confidence; this is NOT verified recognition")
            if args.debug and row['error']:
                for debug_line in row['error'].splitlines():
                    print('   ' + debug_line)
    with args.output.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(f"\nSaved report: {args.output}")
    scored = [r for r in rows if r["exact_match"] != "unscored"]
    if scored:
        print(f"Exact match vs supplied labels: {sum(r['exact_match'] == 'True' for r in scored)}/{len(scored)}")
    else:
        print("Accuracy NOT computed: no verified labels were supplied. OCR confidence is not accuracy.")


if __name__ == "__main__":
    main()
