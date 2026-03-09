#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>

namespace {

std::string ShellQuote(const std::string& input) {
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

int Run(const std::string& command) {
  const int code = std::system(command.c_str());
  if (code == -1) {
    return code;
  }
  if (WIFEXITED(code)) {
    return WEXITSTATUS(code);
  }
  if (WIFSIGNALED(code)) {
    return 128 + WTERMSIG(code);
  }
  return code;
}

bool Exists(const std::filesystem::path& path) { return std::filesystem::exists(path); }

bool FileContains(const std::filesystem::path& path, const std::string& needle) {
  std::ifstream in(path);
  if (!in.is_open()) {
    return false;
  }
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return content.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  const std::filesystem::path tmp = std::filesystem::temp_directory_path();
  const std::filesystem::path out_phase2 = tmp / "urbandrop_plot_smoke_phase2";
  const std::filesystem::path out_phase3 = tmp / "urbandrop_plot_smoke_phase3";

  std::filesystem::remove_all(out_phase2);
  std::filesystem::remove_all(out_phase3);

  const std::string phase1_fixture = "../tests/fixtures/phase1_plot_summary_fixture.csv";
  const std::string phase2_fixture = "../tests/fixtures/phase2_plot_summary_fixture.csv";
  const std::string phase3_fixture = "../tests/fixtures/phase3_plot_summary_fixture.csv";

  const std::string phase1_cmd =
      "python3 ../scripts/plot_phase1_dev.py --dry-run --summary-csv " + ShellQuote(phase1_fixture) +
      " --output-dir " + ShellQuote((tmp / "urbandrop_plot_smoke_phase1").string()) + " > /dev/null";
  if (Run(phase1_cmd) != 0) {
    std::cerr << "phase1 plot dry-run smoke failed\n";
    return EXIT_FAILURE;
  }

  const std::string phase2_cmd =
      "python3 ../scripts/plot_phase2_dev.py --summary-csv " + ShellQuote(phase2_fixture) +
      " --output-dir " + ShellQuote(out_phase2.string()) +
      " --parallel-thread 2 --parallel-threads 1,2 --allow-partial > /dev/null";
  if (Run(phase2_cmd) != 0) {
    std::cerr << "phase2 plot smoke failed\n";
    return EXIT_FAILURE;
  }

  const std::string phase3_cmd =
      "python3 ../scripts/plot_phase3_dev.py --summary-csv " + ShellQuote(phase3_fixture) +
      " --output-dir " + ShellQuote(out_phase3.string()) +
      " --parallel-thread 1 --optimized-thread 2 --allow-partial > /dev/null";
  if (Run(phase3_cmd) != 0) {
    std::cerr << "phase3 plot smoke failed\n";
    return EXIT_FAILURE;
  }

  if (!Exists(out_phase2 / "chart_manifest.csv") || !Exists(out_phase2 / "chart_index.md") ||
      !Exists(out_phase2 / "runtime_vs_threads_speed_below_15.svg")) {
    std::cerr << "phase2 expected graph artifacts missing\n";
    return EXIT_FAILURE;
  }

  if (!Exists(out_phase3 / "chart_manifest.csv") || !Exists(out_phase3 / "chart_index.md") ||
      !Exists(out_phase3 / "runtime_by_subset_speed_below_15.svg") ||
      !Exists(out_phase3 / "optimized_runtime_vs_threads_speed_below_15.svg")) {
    std::cerr << "phase3 expected graph artifacts missing\n";
    return EXIT_FAILURE;
  }

  if (!FileContains(out_phase2 / "runtime_vs_threads_speed_below_15.svg", "What this tests:") ||
      !FileContains(out_phase2 / "runtime_vs_threads_speed_below_15.svg", "Thread Count") ||
      !FileContains(out_phase2 / "runtime_vs_threads_speed_below_15.svg", "Mean Query Runtime (ms)")) {
    std::cerr << "phase2 svg missing expected annotation/axis labels\n";
    return EXIT_FAILURE;
  }

  if (!FileContains(out_phase3 / "runtime_by_subset_speed_below_15.svg", "What this tests:") ||
      !FileContains(out_phase3 / "runtime_by_subset_speed_below_15.svg", "Subset Size") ||
      !FileContains(out_phase3 / "runtime_by_subset_speed_below_15.svg", "Mean Query Runtime (ms)")) {
    std::cerr << "phase3 svg missing expected annotation/axis labels\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
