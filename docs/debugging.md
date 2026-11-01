# Understanding reference image failures

In the C++ executable, exit code **2** means an image was read successfully but no acceptable plate string was produced. It is not a build error.

## Debug one image

```bash
./build/alpr --input tests/reference_images/LicensePlate1.jpg --plate-crop --allow-numeric-only --min-confidence 0 --debug --debug-dir results/debug/plate1
```

The terminal prints each OCR pass, recognized candidate string and confidence, and whether it failed the format gate. Candidate crops are saved under `results/debug/plate1/`.

## Debug all nine

```bash
python3 scripts/evaluate_reference.py --mode both --min-confidence 0 --debug > results/debug_report.txt 2>&1
```

Open `results/reference_report.csv` or `results/debug_report.txt`. If the C++ output says `candidates=0`, automatic localization found nothing; if `ocr_pass` has empty text, OCR failed; if a text is printed but format=FAIL, the format validator rejected the recognition. A low confidence can be a separate reason for rejection.

Note: the nine third-party reference images are not redistributed with this project, and their plate labels have not been verified. OCR confidence is not recognition accuracy.


## Robust crop OCR (experimental)

```bash
./build/alpr --input tests/reference_images/LicensePlate1.jpg \
  --plate-crop --allow-numeric-only --ocr-robust --min-confidence 20 \
  --debug --debug-dir results/debug/plate1
```

Inspect `results/debug/plate1/candidate_0/ocr_*.png` to compare the grayscale,
Otsu, CLAHE, CLAHE/Otsu, inverted and adaptive variants. `--ocr-robust` runs
additional preprocessing and OCR calls and can be slower than default mode.
If text has confidence 0, do not claim a verified successful recognition.
These samples are already cropped plates; `auto` mode seeks plate-shaped
regions in **larger vehicle images** and returning zero candidates here is normal.
