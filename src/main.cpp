#include "alpr/EventLogger.hpp"
#include "alpr/OCRProcessor.hpp"
#include "alpr/PlateDetector.hpp"
#include "alpr/PlateUtils.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Options {
    std::string input;
    std::optional<int> camera;
    std::string output = "results/events.csv";
    std::string annotated = "";
    int max_frames = 150;
    float min_confidence = 35.f;
    bool plate_crop = false; // Treat the input image as an already-localized plate.
    bool allow_numeric_only = false;
    bool debug = false;
    bool ocr_robust = false;
    std::string debug_dir;
};

void help() {
    std::cout << "Usage:\n"
              << "  alpr --input samples/synthetic_plate.png [--output results/events.csv] [--annotated results/result.png]\n"
              << "  alpr --input samples/video.mp4 [--max-frames 150]\n"
              << "  alpr --camera 0 [--max-frames 150]  (requires working OpenCV camera backend)\n"
              << "  alpr --input samples/plate_crop.png --plate-crop --allow-numeric-only\n"
              << "  Options: --min-confidence 35 --plate-crop --allow-numeric-only --ocr-robust --debug [--debug-dir results/debug] --help\n";
}

Options parse(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string flag(argv[i]);
        if (flag == "--help" || flag == "-h") { help(); std::exit(0); }
        if (flag == "--plate-crop") { opt.plate_crop = true; continue; }
        if (flag == "--allow-numeric-only") { opt.allow_numeric_only = true; continue; }
        if (flag == "--debug") { opt.debug = true; continue; }
        if (flag == "--ocr-robust") { opt.ocr_robust = true; continue; }
        if (i + 1 >= argc) throw std::invalid_argument("Missing value after " + flag);
        const std::string value(argv[++i]);
        if (flag == "--input") opt.input = value;
        else if (flag == "--camera") opt.camera = std::stoi(value);
        else if (flag == "--output") opt.output = value;
        else if (flag == "--annotated") opt.annotated = value;
        else if (flag == "--max-frames") opt.max_frames = std::stoi(value);
        else if (flag == "--min-confidence") opt.min_confidence = std::stof(value);
        else if (flag == "--debug-dir") { opt.debug_dir = value; opt.debug = true; }
        else throw std::invalid_argument("Unknown option: " + flag);
    }
    if (opt.camera.has_value() == !opt.input.empty()) {
        throw std::invalid_argument("Specify exactly one of --input or --camera");
    }
    if (opt.plate_crop && opt.camera.has_value()) {
        throw std::invalid_argument("--plate-crop cannot be used with --camera");
    }
    if (opt.max_frames < 1 || opt.min_confidence < 0 || opt.min_confidence > 100) {
        throw std::invalid_argument("Invalid max-frames or min-confidence");
    }
    return opt;
}

bool imageExtension(const std::string& p) {
    const std::string ext = std::filesystem::path(p).extension().string();
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" ||
           ext == ".JPG" || ext == ".PNG";
}

void saveAnnotated(const cv::Mat& img, const std::string& path) {
    if (path.empty()) return;
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    if (!cv::imwrite(path, img)) throw std::runtime_error("Cannot save " + path);
}

bool processFrame(const cv::Mat& frame, alpr::PlateDetector& detector,
                  alpr::OCRProcessor& ocr, alpr::EventLogger& logger,
                  alpr::DuplicateFilter& duplicate, cv::Mat& annotated,
                  float min_confidence, bool plate_crop, bool allow_numeric_only,
                  bool debug, bool ocr_robust, const std::string& debug_dir) {
    const auto start = std::chrono::steady_clock::now();
    annotated = frame.clone();
    const auto candidates = plate_crop
        ? std::vector<cv::Rect>{cv::Rect(0, 0, frame.cols, frame.rows)}
        : detector.detect(frame);
    bool found = false;
    if (debug) {
        std::cerr << "[DEBUG] input=" << frame.cols << 'x' << frame.rows
                  << " mode=" << (plate_crop ? "crop" : "auto")
                  << " candidates=" << candidates.size() << '\n';
    }
    if (!debug_dir.empty()) std::filesystem::create_directories(debug_dir);
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const auto& rect = candidates[i];
        if (!debug_dir.empty()) {
            saveAnnotated(frame(rect), (std::filesystem::path(debug_dir) /
                ("candidate_" + std::to_string(i) + ".png")).string());
        }
        const std::string candidate_debug_dir = debug_dir.empty() ? "" :
            (std::filesystem::path(debug_dir) / ("candidate_" + std::to_string(i))).string();
        const auto prediction = ocr.recognize(frame(rect), debug, ocr_robust, candidate_debug_dir);
        const bool confidence_ok = prediction.confidence >= min_confidence;
        const bool format_ok = alpr::plausiblePlate(prediction.text, allow_numeric_only);
        if (debug) {
            std::cerr << "[DEBUG] candidate=" << i << " rect=(" << rect.x << ',' << rect.y
                      << ',' << rect.width << ',' << rect.height << ") text='"
                      << prediction.text << "' confidence=" << prediction.confidence
                      << " format=" << (format_ok ? "pass" : "FAIL")
                      << " threshold=" << (confidence_ok ? "pass" : "FAIL") << '\n';
        }
        if (!confidence_ok || !format_ok) continue;
        const double elapsed_ms =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        cv::rectangle(annotated, rect, cv::Scalar(0, 200, 0), 2);
        cv::putText(annotated, prediction.text, cv::Point(rect.x, std::max(20, rect.y - 10)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 230, 0), 2);
        if (duplicate.shouldEmit(prediction.text)) {
            logger.log(prediction.text, prediction.confidence, elapsed_ms);
            std::cout << "[ALERT] plate=" << prediction.text
                      << " ocr_confidence=" << prediction.confidence
                      << " processing_ms=" << elapsed_ms << '\n';
        }
        found = true;
        break; // Single-plate MVP. Extend to multi-plate support later.
    }
    if (!found) std::cout << "[INFO] No valid plate recognized in this frame\n";
    return found;
}
}

int main(int argc, char** argv) {
    try {
        const Options opt = parse(argc, argv);
        if (opt.plate_crop && !imageExtension(opt.input)) {
            throw std::invalid_argument("--plate-crop requires an image file input");
        }
        alpr::PlateDetector detector;
        alpr::OCRProcessor ocr;
        alpr::EventLogger logger(opt.output);
        alpr::DuplicateFilter duplicate(std::chrono::seconds(5));
        cv::Mat annotated;
        if (!opt.input.empty() && imageExtension(opt.input)) {
            cv::Mat image = cv::imread(opt.input, cv::IMREAD_COLOR);
            if (image.empty()) throw std::runtime_error("Cannot read image: " + opt.input);
            const bool ok = processFrame(image, detector, ocr, logger, duplicate,
                                         annotated, opt.min_confidence,
                                         opt.plate_crop, opt.allow_numeric_only,
                                         opt.debug, opt.ocr_robust, opt.debug_dir);
            saveAnnotated(annotated, opt.annotated);
            return ok ? 0 : 2; // Exit 2 means program ran but no plate was recognized.
        }
        cv::VideoCapture capture;
        if (opt.camera.has_value()) capture.open(*opt.camera);
        else capture.open(opt.input);
        if (!capture.isOpened()) throw std::runtime_error("Cannot open video or camera source");
        int recognized_frames = 0;
        int processed_frames = 0;
        while (processed_frames < opt.max_frames) {
            cv::Mat frame;
            if (!capture.read(frame) || frame.empty()) break;
            ++processed_frames;
            if (processFrame(frame, detector, ocr, logger, duplicate,
                             annotated, opt.min_confidence, false,
                             opt.allow_numeric_only, opt.debug, opt.ocr_robust, opt.debug_dir)) ++recognized_frames;
        }
        saveAnnotated(annotated, opt.annotated);
        std::cout << "[SUMMARY] frames=" << processed_frames
                  << " frames_with_plate=" << recognized_frames << '\n';
        return processed_frames ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        help();
        return 1;
    }
}
