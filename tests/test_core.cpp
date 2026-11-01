#include "alpr/EventLogger.hpp"
#include "alpr/PlateUtils.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int failures = 0;
void check(bool ok, const std::string& case_name) {
    if (!ok) { std::cerr << "FAIL: " << case_name << '\n'; ++failures; }
}
}

int main() {
    using namespace std::chrono_literals;
    check(alpr::normalizePlate(" ab-c 1234\n") == "ABC1234", "uppercase and remove separators");
    check(alpr::normalizePlate("a_b9") == "AB9", "underscore removal");
    check(alpr::normalizePlate("").empty(), "empty normalization");
    check(alpr::plausiblePlate("TEST123"), "plausible plate");
    check(!alpr::plausiblePlate("ABC"), "reject short plate");
    check(!alpr::plausiblePlate("1234567"), "reject no-letter plate by default");
    check(alpr::plausiblePlate("1234567", true), "allow numeric-only plates explicitly");
    check(!alpr::plausiblePlate("123", true), "reject short numeric-only plate");
    check(!alpr::plausiblePlate("TEST#12"), "reject punctuation");
    alpr::DuplicateFilter deduper(5000ms);
    const std::chrono::steady_clock::time_point t0{};
    check(deduper.shouldEmit("ABC1234", t0), "first sighting emitted");
    check(!deduper.shouldEmit("ABC1234", t0 + 1500ms), "duplicate suppressed");
    check(deduper.shouldEmit("XYZ9999", t0 + 1500ms), "different plate emitted");
    check(deduper.shouldEmit("ABC1234", t0 + 5000ms), "expired cooldown");
    bool caught = false;
    try { alpr::DuplicateFilter invalid(-1ms); } catch (const std::invalid_argument&) { caught = true; }
    check(caught, "negative cooldown rejected");
    const auto tmp = std::filesystem::temp_directory_path() / "alpr_event_logger_test.csv";
    std::filesystem::remove(tmp);
    {
        alpr::EventLogger logger(tmp.string());
        logger.log("TEST123", 85.5f, 12.34);
    }
    std::ifstream csv(tmp);
    std::stringstream buffer;
    buffer << csv.rdbuf();
    check(buffer.str().find("timestamp_utc,plate,ocr_confidence,processing_ms") != std::string::npos,
          "CSV header written");
    check(buffer.str().find(",TEST123,85.5,12.34") != std::string::npos,
          "CSV event written");
    std::filesystem::remove(tmp);
    if (failures) { std::cerr << failures << " unit tests failed\n"; return 1; }
    std::cout << "PASS: all 16 unit checks\n";
    return 0;
}
