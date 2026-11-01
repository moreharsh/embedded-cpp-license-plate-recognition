# Validation notes — 2026-10-26

## Verified in the working environment

- The standalone C++17 core configured and compiled with `-DALPR_BUILD_APP=OFF`.
- `ctest --test-dir build --output-on-failure` **passed (16 C++ assertions)**.
- The added `--allow-numeric-only` core formatting checks pass.
- `scripts/fetch_reference_images.py` and `scripts/evaluate_reference.py` parse/compile as Python scripts.
- An independent **Python OpenCV + Tesseract CLI approximation**, not the compiled
  C++ executable, recognized synthetic `TEST123` in full-image, dim, bright,
  blurred, and pre-cropped conditions in at least one OCR pass. A mildly rotated
  sample was unreliable (`TEST125` / `TEST1235`), showing a genuine limitation.

## Not verified here

- The **complete OpenCV-linked C++ executable**: native OpenCV headers/libraries
  are unavailable in this environment, so no full native build was possible.
- The nine source JPEGs (`LicensePlate1.jpg` through `LicensePlate9.jpg`) from
  Luca Meschiari's GitHub repository: web access allowed listing them, but blocked
  downloading their binary image data into the test environment. No real image
  accuracy score has been measured or inferred.
- Raspberry Pi 5 live camera integration, Pi latency, and real-road video data.

## Next validation on Windows WSL2 or Raspberry Pi

```bash
sudo apt install -y build-essential cmake pkg-config libopencv-dev libtesseract-dev tesseract-ocr-eng
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/alpr --input samples/synthetic_plate.png --annotated results/synthetic_result.png
./build/alpr --input samples/synthetic_plate_crop.png --plate-crop
python3 scripts/fetch_reference_images.py
python3 scripts/evaluate_reference.py --binary ./build/alpr --mode both
```

For interpreting the report and optional verified ground-truth labels, see
[`reference-evaluation.md`](reference-evaluation.md). Do not label OCR confidence
as recognition accuracy, and do not commit external plate photos or generated logs.
