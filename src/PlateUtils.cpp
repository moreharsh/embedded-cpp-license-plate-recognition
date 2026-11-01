#include "alpr/PlateUtils.hpp"
#include <cctype>

namespace alpr {
std::string normalizePlate(const std::string& raw) {
    std::string result;
    result.reserve(raw.size());
    for (unsigned char ch : raw) {
        if (std::isalnum(ch) && ch < 128) {
            result.push_back(static_cast<char>(std::toupper(ch)));
        }
    }
    return result;
}

bool plausiblePlate(const std::string& normalized, bool allow_numeric_only) {
    if (normalized.size() < 4 || normalized.size() > 10) return false;
    bool has_letter = false;
    bool has_digit = false;
    for (unsigned char ch : normalized) {
        if (ch >= 'A' && ch <= 'Z') has_letter = true;
        else if (ch >= '0' && ch <= '9') has_digit = true;
        else return false;
    }
    // Some regions use all-numeric plates. Opt in explicitly to avoid
    // silently broadening the original default pattern heuristic.
    return has_digit && (has_letter || allow_numeric_only);
}
}
