#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace urbandrop::io {

std::string Trim(const std::string& input);
std::string ToLower(std::string input);
std::vector<std::string> ParseCsvLine(const std::string& line);

bool ParseInt32(const std::string& value, std::int32_t* out);
bool ParseInt64(const std::string& value, std::int64_t* out);
bool ParseDouble(const std::string& value, double* out);

std::string NormalizeTimestamp(const std::string& value);
bool ParseTimestamp(const std::string& value, std::int64_t* out_epoch_seconds);

bool ResolveColumn(const std::unordered_map<std::string, std::size_t>& columns,
                   const std::vector<std::string>& aliases,
                   std::size_t* index);

}  // namespace urbandrop::io
