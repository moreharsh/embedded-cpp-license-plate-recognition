#pragma once
#include <string>

namespace alpr {
// Uppercase A-Z and digits only. This is normalization, not accuracy validation.
std::string normalizePlate(const std::string& raw);
bool plausiblePlate(const std::string& normalized, bool allow_numeric_only = false);
}
