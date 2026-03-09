#include "io/CsvParseUtils.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace urbandrop::io {

std::string Trim(const std::string& input) {
  std::size_t start = 0;
  while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start])) != 0) {
    ++start;
  }

  std::size_t end = input.size();
  while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
    --end;
  }

  return input.substr(start, end - start);
}

std::string ToLower(std::string input) {
  std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return input;
}

std::vector<std::string> ParseCsvLine(const std::string& line) {
  std::vector<std::string> fields;
  std::string current;
  bool in_quotes = false;

  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (c == '"') {
      if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
        current.push_back('"');
        ++i;
      } else {
        in_quotes = !in_quotes;
      }
      continue;
    }
    if (c == ',' && !in_quotes) {
      fields.push_back(Trim(current));
      current.clear();
      continue;
    }
    current.push_back(c);
  }

  fields.push_back(Trim(current));
  return fields;
}

bool ParseInt32(const std::string& value, std::int32_t* out) {
  if (value.empty()) {
    return false;
  }

  errno = 0;
  char* end = nullptr;
  const long parsed = std::strtol(value.c_str(), &end, 10);
  if (errno != 0 || end == value.c_str() || *end != '\0') {
    return false;
  }

  *out = static_cast<std::int32_t>(parsed);
  return true;
}

bool ParseInt64(const std::string& value, std::int64_t* out) {
  if (value.empty()) {
    return false;
  }

  errno = 0;
  char* end = nullptr;
  const long long parsed = std::strtoll(value.c_str(), &end, 10);
  if (errno != 0 || end == value.c_str() || *end != '\0') {
    return false;
  }

  *out = static_cast<std::int64_t>(parsed);
  return true;
}

bool ParseDouble(const std::string& value, double* out) {
  if (value.empty()) {
    return false;
  }

  errno = 0;
  char* end = nullptr;
  const double parsed = std::strtod(value.c_str(), &end);
  if (errno != 0 || end == value.c_str() || *end != '\0') {
    return false;
  }

  *out = parsed;
  return true;
}

std::string NormalizeTimestamp(const std::string& value) {
  std::string cleaned = Trim(value);
  if (cleaned.empty()) {
    return cleaned;
  }

  if (cleaned.back() == 'Z') {
    cleaned.pop_back();
  }

  const std::size_t dot_pos = cleaned.find('.');
  if (dot_pos != std::string::npos) {
    cleaned = cleaned.substr(0, dot_pos);
  }

  return cleaned;
}

bool ParseTimestamp(const std::string& value, std::int64_t* out_epoch_seconds) {
  const std::string normalized = NormalizeTimestamp(value);
  if (normalized.empty()) {
    return false;
  }

  std::tm tm = {};
  std::istringstream stream(normalized);
  stream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
  if (stream.fail()) {
    stream.clear();
    stream.str(normalized);
    stream >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (stream.fail()) {
      return false;
    }
  }

#if defined(_WIN32)
  const std::time_t parsed_time = _mkgmtime(&tm);
#else
  const std::time_t parsed_time = timegm(&tm);
#endif

  if (parsed_time < 0) {
    return false;
  }

  *out_epoch_seconds = static_cast<std::int64_t>(parsed_time);
  return true;
}

bool ResolveColumn(const std::unordered_map<std::string, std::size_t>& columns,
                   const std::vector<std::string>& aliases,
                   std::size_t* index) {
  for (const auto& alias : aliases) {
    const auto it = columns.find(alias);
    if (it != columns.end()) {
      *index = it->second;
      return true;
    }
  }
  return false;
}

}  // namespace urbandrop::io
