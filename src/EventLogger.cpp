#include "alpr/EventLogger.hpp"
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <stdexcept>

namespace alpr {
DuplicateFilter::DuplicateFilter(std::chrono::milliseconds cooldown) : cooldown_(cooldown) {
    if (cooldown.count() < 0) throw std::invalid_argument("Cooldown cannot be negative");
}

bool DuplicateFilter::shouldEmit(const std::string& plate,
                                 std::chrono::steady_clock::time_point now) {
    const auto it = last_seen_.find(plate);
    if (it != last_seen_.end() && now >= it->second && now - it->second < cooldown_) {
        return false;
    }
    last_seen_[plate] = now;
    return true;
}

EventLogger::EventLogger(const std::string& csv_file) {
    const std::filesystem::path path(csv_file);
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    const bool needs_header = !std::filesystem::exists(path) || std::filesystem::file_size(path) == 0;
    out_.open(csv_file, std::ios::app);
    if (!out_) throw std::runtime_error("Cannot open CSV output: " + csv_file);
    if (needs_header) out_ << "timestamp_utc,plate,ocr_confidence,processing_ms\n";
}

void EventLogger::log(const std::string& plate, float confidence, double elapsed_ms) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
    const std::tm* utc = std::gmtime(&timestamp); // Single-threaded logging in this MVP.
    if (!utc) throw std::runtime_error("Unable to format UTC timestamp");
    out_ << std::put_time(utc, "%Y-%m-%dT%H:%M:%SZ") << ','
         << plate << ',' << std::fixed << std::setprecision(1) << confidence << ','
         << std::setprecision(2) << elapsed_ms << '\n';
    out_.flush();
    if (!out_) throw std::runtime_error("Unable to write CSV event");
}
}
