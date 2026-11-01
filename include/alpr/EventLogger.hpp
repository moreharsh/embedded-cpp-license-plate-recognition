#pragma once
#include <chrono>
#include <fstream>
#include <string>
#include <unordered_map>

namespace alpr {
class DuplicateFilter {
public:
    explicit DuplicateFilter(std::chrono::milliseconds cooldown);
    bool shouldEmit(const std::string& plate,
                    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
private:
    std::chrono::milliseconds cooldown_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_seen_;
};

class EventLogger {
public:
    explicit EventLogger(const std::string& csv_file);
    void log(const std::string& plate, float confidence, double elapsed_ms);
private:
    std::ofstream out_; // RAII closes the file automatically.
};
}
