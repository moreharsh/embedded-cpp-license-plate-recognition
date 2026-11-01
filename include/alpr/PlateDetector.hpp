#pragma once
#include <opencv2/core.hpp>
#include <vector>

namespace alpr {
class PlateDetector {
public:
    // Returns ranked rectangular candidates, NOT verified plates.
    std::vector<cv::Rect> detect(const cv::Mat& frame) const;
private:
    static bool candidateShape(const cv::Rect& r, const cv::Size& frame_size);
};
}
