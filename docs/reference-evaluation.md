# Testing against the nine LucaMeschiari reference images

Source: <https://github.com/LucaMeschiari/LicensePlateRecognition> (2012 demonstration).

The source repository includes `LicensePlate1.jpg`–`LicensePlate9.jpg` and digit
reference images. These are external, third-party images. The project's own source
and code are **not copied** from that repository; only its general contour/character
recognition ideas are discussed. Sample JPEGs are ignored by Git and not bundled.

## Reproduce on Linux or Windows WSL2

```bash
# First build the native C++ binary following README.md.
python3 scripts/fetch_reference_images.py
python3 scripts/evaluate_reference.py --binary ./build/alpr --mode both
cat results/reference_report.csv
```

`--mode crop` applies OCR to the entire input, for an **already cropped plate**;
`--mode auto` independently searches the image for rectangular plate candidates.
This is an important difference: crop-only recognition does not demonstrate vehicle
image plate localization. The evaluator runs both by default.

If you inspect each sample and can confidently transcribe the **true plate string**,
create a `results/ground_truth.csv` file with these headings:

```csv
filename,expected
LicensePlate1.jpg,
LicensePlate2.jpg,
```

Fill labels only for images you have manually verified, then rerun:

```bash
python3 scripts/evaluate_reference.py --mode both --truth results/ground_truth.csv
```

The evaluator reports exact-match counts **only for labeled images**. It always
reports raw predictions, OCR confidence, C++ program exit status, and wall time.
An empty `recognized` column is a failure to recognize; a nonempty value is **not**
necessarily correct. Tesseract confidence is not an accuracy percentage.

## Interpreting failures

- `auto` fails and `crop` succeeds: the plate localization heuristic is limiting.
- Both fail: poor character recognition, unsupported plate formatting, or bad input.
- `auto` succeeds with the wrong plate: false-positive bounding box or OCR error.
- Many digit-only plates: use `--allow-numeric-only` (already set by evaluator).

An additional run on **full vehicle photos** is still required to support real-world
license plate detection claims. A handful of cropped plates is not an accuracy study.
