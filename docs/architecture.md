# Architecture

```
JPG/PNG or MP4 or camera source
            |
            v
       OpenCV capture
            |
            v
   PlateDetector (rectangular candidate proposals; heuristic MVP)
            |
            v
   OCRProcessor (Tesseract OCR; normalize and validate text)
            |
            v
   DuplicateFilter (5s cooldown per string)
            |
            v
   EventLogger (timestamp, plate text, OCR confidence, processing ms)
```

The detector uses edge-based and bright-region proposals and is **not** a trained
ALPR model. It will fail on many real-world scenes. There is no persistent
watchlist, network upload, storage of source imagery, or identity lookup.

## C++ design notes
- `PlateDetector`: single-responsibility candidate discovery.
- `OCRProcessor`: wraps one Tesseract API instance with RAII and non-copyable ownership.
- `DuplicateFilter`: testable logic using an injectable steady-clock timestamp.
- `EventLogger`: `std::ofstream` RAII, creates parent directory, writes a CSV header once.
- `PlateUtils`: pure, unit-tested normalization and simple plate-format heuristic.

## Next improvements (not implemented)
- Raspberry Pi CSI camera integration/benchmark.
- Reliable plate detector using an ONNX model rather than contour heuristics.
- Unit tests for detector and OCR with annotated gold examples.
- Multithreaded capture/processing for video throughput.
- Better multi-plate tracking and jurisdiction-aware format validation.
