#pragma once
#include <opencv2/core.hpp>
#include <tesseract/baseapi.h>
#include <string>

namespace alpr {
struct OCRResult {
    std::string text;
    float confidence = 0.f; // Tesseract MeanTextConf scale 0–100; NOT accuracy.
};

class OCRProcessor {
public:
    OCRProcessor();
    ~OCRProcessor();
    OCRProcessor(const OCRProcessor&) = delete;
    OCRProcessor& operator=(const OCRProcessor&) = delete;
    OCRResult recognize(const cv::Mat& crop, bool debug = false,
                        bool robust = false, const std::string& debug_dir = "");
private:
    tesseract::TessBaseAPI engine_;
};
}
