#include "alpr/OCRProcessor.hpp"
#include "alpr/PlateUtils.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
struct Variant {
    std::string name;
    cv::Mat image;
};

// Whitespace prevents glyphs at the image boundary being interpreted as noise.
cv::Mat pad(const cv::Mat& src) {
    cv::Mat out;
    cv::copyMakeBorder(src, out, 24, 24, 40, 40,
                       cv::BORDER_CONSTANT, cv::Scalar(255));
    return out;
}

std::vector<Variant> preprocess(const cv::Mat& crop, bool robust) {
    cv::Mat gray;
    if (crop.channels() == 3) cv::cvtColor(crop, gray, cv::COLOR_BGR2GRAY);
    else if (crop.channels() == 4) cv::cvtColor(crop, gray, cv::COLOR_BGRA2GRAY);
    else gray = crop;

    // 200x50 reference plate crops are enlarged before OCR.
    const double scale = gray.rows < 80 ? 4.0 : 2.0;
    cv::Mat enlarged;
    cv::resize(gray, enlarged, cv::Size(), scale, scale, cv::INTER_CUBIC);

    cv::Mat otsu;
    cv::threshold(enlarged, otsu, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    std::vector<Variant> v;
    v.push_back({"grayscale", pad(enlarged)});
    v.push_back({"otsu", pad(otsu)});
    if (!robust) return v;

    cv::Mat clahe, clahe_otsu;
    cv::Ptr<cv::CLAHE> contrast = cv::createCLAHE(2.0, cv::Size(8, 8));
    contrast->apply(enlarged, clahe);
    cv::threshold(clahe, clahe_otsu, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    v.push_back({"clahe", pad(clahe)});
    v.push_back({"clahe_otsu", pad(clahe_otsu)});
    cv::Mat inverted;
    cv::bitwise_not(otsu, inverted);
    // Tesseract generally prefers dark text on light background. This variant
    // can help negative-polarity plates; padding remains white.
    v.push_back({"inverted", pad(inverted)});
    cv::Mat adaptive;
    cv::adaptiveThreshold(enlarged, adaptive, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY, 31, 10);
    v.push_back({"adaptive", pad(adaptive)});
    return v;
}
}

namespace alpr {
OCRProcessor::OCRProcessor() {
    if (engine_.Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY) != 0) {
        throw std::runtime_error("Failed to initialize Tesseract (check eng.traineddata)");
    }
    engine_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
    engine_.SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
}

OCRProcessor::~OCRProcessor() { engine_.End(); }

OCRResult OCRProcessor::recognize(const cv::Mat& crop, bool debug,
                                  bool robust, const std::string& debug_dir) {
    if (crop.empty()) return {};
    const auto variants = preprocess(crop, robust);
    OCRResult best;
    for (const auto& v : variants) {
        if (!debug_dir.empty()) {
            std::filesystem::create_directories(debug_dir);
            cv::imwrite((std::filesystem::path(debug_dir) /
                         ("ocr_" + v.name + ".png")).string(), v.image);
        }
        engine_.Clear();
        engine_.SetImage(v.image.data, v.image.cols, v.image.rows, 1,
                         static_cast<int>(v.image.step));
        std::unique_ptr<char[]> raw(engine_.GetUTF8Text());
        const std::string text = normalizePlate(raw ? raw.get() : "");
        // This is the OCR model's reported mean confidence, NOT accuracy.
        const float confidence = static_cast<float>(engine_.MeanTextConf());
        if (debug) {
            std::cerr << "[DEBUG] ocr_variant=" << v.name << " text='" << text
                      << "' confidence=" << confidence << '\n';
        }
        if (!text.empty() && (best.text.empty() || confidence > best.confidence)) {
            best = {text, confidence};
        }
        if (!robust && best.confidence >= 80.f && plausiblePlate(best.text, true)) break;
    }
    if (debug && !best.text.empty() && best.confidence == 0.f) {
        std::cerr << "[WARN] OCR text has zero reported confidence; treat as unverified output\n";
    }
    return best;
}
}
