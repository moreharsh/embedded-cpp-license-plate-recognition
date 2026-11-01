#include "alpr/PlateDetector.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

namespace {
struct Candidate {
    cv::Rect rect;
    double score;
};

double overlapIou(const cv::Rect& a, const cv::Rect& b) {
    const double overlap = static_cast<double>((a & b).area());
    const double uni = static_cast<double>(a.area()) + b.area() - overlap;
    return uni > 0.0 ? overlap / uni : 0.0;
}
}

namespace alpr {
bool PlateDetector::candidateShape(const cv::Rect& r, const cv::Size& s) {
    if (r.width < 70 || r.height < 18) return false;
    const double aspect = static_cast<double>(r.width) / r.height;
    const double fraction = static_cast<double>(r.area()) / (s.width * s.height);
    return aspect >= 2.2 && aspect <= 6.5 && fraction >= 0.002 && fraction <= 0.16;
}

std::vector<cv::Rect> PlateDetector::detect(const cv::Mat& frame) const {
    if (frame.empty()) return {};
    cv::Mat gray;
    if (frame.channels() == 3) cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    else if (frame.channels() == 4) cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
    else gray = frame;

    std::vector<Candidate> all;
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
    cv::Mat edges;
    cv::Canny(blurred, edges, 60, 160);
    cv::morphologyEx(edges, edges, cv::MORPH_CLOSE,
                     cv::getStructuringElement(cv::MORPH_RECT, cv::Size(9, 3)));

    // Complement the edge detector with bright-region proposals, useful for
    // clear white-on-dark plates. Not a substitute for a trained detector.
    cv::Mat light;
    cv::threshold(gray, light, 185, 255, cv::THRESH_BINARY);
    cv::morphologyEx(light, light, cv::MORPH_CLOSE,
                     cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 3)));

    for (const cv::Mat& mask : {edges, light}) {
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask.clone(), contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
        for (const auto& contour : contours) {
            const cv::Rect r = cv::boundingRect(contour);
            if (!candidateShape(r, frame.size())) continue;
            const double aspect = static_cast<double>(r.width) / r.height;
            const double brightness = cv::mean(gray(r))[0] / 255.0;
            const double shape_score = std::max(0.0, 1.0 - std::abs(aspect - 4.0) / 4.0);
            const double score = shape_score * 3.0 + brightness + std::min(1.0, r.area() / 20000.0);
            all.push_back({r, score});
        }
    }
    std::sort(all.begin(), all.end(), [](const Candidate& a, const Candidate& b) {
        return a.score > b.score;
    });

    std::vector<cv::Rect> result;
    for (const auto& candidate : all) {
        bool duplicate = false;
        for (const cv::Rect& kept : result) {
            if (overlapIou(kept, candidate.rect) > 0.40) { duplicate = true; break; }
        }
        if (!duplicate) result.push_back(candidate.rect);
        if (result.size() >= 8) break;
    }
    return result;
}
}
