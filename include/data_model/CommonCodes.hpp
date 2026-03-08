#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

namespace urbandrop {

enum class BoroughCode : std::int16_t {
  kUnknown = -1,
  kManhattan = 1,
  kBronx = 2,
  kBrooklyn = 3,
  kQueens = 4,
  kStatenIsland = 5,
};

inline std::string ToLowerAscii(std::string input) {
  std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return input;
}

inline BoroughCode ParseBoroughCode(const std::string& value) {
  const std::string normalized = ToLowerAscii(value);
  if (normalized == "manhattan" || normalized == "mn") {
    return BoroughCode::kManhattan;
  }
  if (normalized == "bronx" || normalized == "bx") {
    return BoroughCode::kBronx;
  }
  if (normalized == "brooklyn" || normalized == "bk" || normalized == "k") {
    return BoroughCode::kBrooklyn;
  }
  if (normalized == "queens" || normalized == "qn" || normalized == "q") {
    return BoroughCode::kQueens;
  }
  if (normalized == "staten island" || normalized == "si") {
    return BoroughCode::kStatenIsland;
  }
  return BoroughCode::kUnknown;
}

inline std::int16_t ToInt(BoroughCode code) { return static_cast<std::int16_t>(code); }

}  // namespace urbandrop
