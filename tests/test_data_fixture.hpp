#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/wait.h>

namespace urbandrop::testutil {

inline std::string GetTrafficSourceCsvPath() {
  const char* env_path = std::getenv("URBANDROP_TRAFFIC_CSV");
  if (env_path != nullptr && std::string(env_path).size() > 0) {
    return std::string(env_path);
  }
  return "/datasets/i4gi-tjb9.csv";
}

inline std::filesystem::path GetSampleCsvPath(std::size_t max_data_lines) {
  return std::filesystem::temp_directory_path() /
         ("urbandrop_traffic_first_" + std::to_string(max_data_lines) + ".csv");
}

inline bool ExistingSampleLooksUsable(const std::filesystem::path& sample_path) {
  std::ifstream in(sample_path);
  if (!in.is_open()) {
    return false;
  }

  std::string header;
  std::string first_data_row;
  return static_cast<bool>(std::getline(in, header)) && !header.empty() &&
         static_cast<bool>(std::getline(in, first_data_row)) && !first_data_row.empty();
}

inline bool EnsureFirstNLinesSample(const std::string& source_csv,
                                    std::size_t max_data_lines,
                                    std::string* sample_csv_out,
                                    std::size_t* copied_data_lines_out,
                                    bool* reused_existing_sample_out) {
  if (sample_csv_out == nullptr || copied_data_lines_out == nullptr ||
      reused_existing_sample_out == nullptr) {
    return false;
  }

  const auto sample_path = GetSampleCsvPath(max_data_lines);
  if (ExistingSampleLooksUsable(sample_path)) {
    *sample_csv_out = sample_path.string();
    *copied_data_lines_out = max_data_lines;
    *reused_existing_sample_out = true;
    return true;
  }

  std::ifstream in(source_csv);
  if (!in.is_open()) {
    return false;
  }

  std::ofstream out(sample_path);
  if (!out.is_open()) {
    return false;
  }

  std::string header;
  if (!std::getline(in, header)) {
    return false;
  }
  out << header << '\n';

  std::string line;
  std::size_t copied = 0;
  while (copied < max_data_lines && std::getline(in, line)) {
    out << line << '\n';
    ++copied;
  }

  *sample_csv_out = sample_path.string();
  *copied_data_lines_out = copied;
  *reused_existing_sample_out = false;
  return copied > 0;
}

inline std::string ShellQuote(const std::string& input) {
  std::string out = "'";
  for (char c : input) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out.push_back(c);
    }
  }
  out += "'";
  return out;
}

inline int RunShell(const std::string& command) {
  const int code = std::system(command.c_str());
  if (code == -1) {
    return code;
  }
  return WEXITSTATUS(code);
}

}  // namespace urbandrop::testutil
