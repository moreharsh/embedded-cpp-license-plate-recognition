# Start here — first local test

You can learn and run this project before connecting your Raspberry Pi camera.

1. On Windows, install **WSL2 Ubuntu** (if you do not already have it).
2. Extract the ZIP to a folder you can access from WSL.
3. Open an Ubuntu terminal and change into the extracted project folder.
4. Install dependencies:

   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake pkg-config git \
       libopencv-dev libtesseract-dev tesseract-ocr-eng
   ```

5. Configure and build:

   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j2
   ```

6. Run C++ unit tests:

   ```bash
   ctest --test-dir build --output-on-failure
   ```

7. Try the synthetic image (included in the ZIP):

   ```bash
   ./build/alpr --input samples/synthetic_plate.png \
     --output results/events.csv --annotated results/annotated.png
   ```

8. Inspect the saved `results/events.csv` and `results/annotated.png`.
   The expected dummy plate text is `TEST123`.

9. Push to GitHub once you have confirmed the build and tests. The precise
   commands are at the bottom of [README.md](README.md).

10. **Test the nine external reference images** from the GitHub repository:

    ```bash
    python3 scripts/fetch_reference_images.py
    python3 scripts/evaluate_reference.py --binary ./build/alpr --mode both
    cat results/reference_report.csv
    ```

    The resulting recognition strings are *predictions*, not proof of accuracy.
    See `docs/reference-evaluation.md` for optional manual ground truth labels.

11. After local success, build/run on Raspberry Pi OS and test a Pi camera photo.

**Important:** If the compiler reports missing OpenCV C++ headers, install
`libopencv-dev`; installing the Python `opencv-python` package alone does
not satisfy this project's C++ dependencies.


## Debugging reference images

See [`docs/debugging.md`](docs/debugging.md). The `--debug` and `--debug-dir` options print OCR decisions and save candidate crops.


### V3 update — debug results on historical 200x50 plate crops

`auto` mode is for full-car photos, not these 200x50 crops. Run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
python3 scripts/evaluate_reference.py --mode crop --ocr-robust --min-confidence 20 --debug > results/robust_debug_report.txt 2>&1
```

Use `results/reference_report.csv` for OCR outputs and `results/debug/` for
saved OCR preprocessing images. All-zero confidence in previous runs was *not*
recognition accuracy. Full v3 integration requires a local WSL compile.
