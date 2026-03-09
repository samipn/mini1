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

}  // namespace

int main() {
  const std::string dry_run_cmd =
      "bash ../scripts/run_phase3_step_matrix.sh --dry-run --runs 3 > /dev/null 2>&1";
  if (Run(dry_run_cmd) != 0) {
    std::cerr << "run_phase3_step_matrix dry-run should succeed\n";
    return EXIT_FAILURE;
  }

  const auto tmp = std::filesystem::temp_directory_path() / "urbandrop_phase3_workflow_cli";
  std::filesystem::remove_all(tmp);
  std::filesystem::create_directories(tmp);
  const auto manifest = tmp / "fake_manifest.csv";
  std::ofstream out(manifest);
  out << "batch_utc,git_branch,git_commit,optimization_step,evidence_tier,subset_label,scenario_name,mode,thread_list,dataset_path,benchmark_runs,validation_enabled,output_csv,log_file\n";
  out << "20260101T000000Z,main,abc123,phase3_baseline,deliverable,small,summary,serial,1,/tmp/x.csv,1,0,/tmp/y.csv,/tmp/z.log\n";
  out.close();

  const std::string baseline_guard_cmd =
      "bash ../scripts/run_phase3_baseline_reporting.sh --manifest " + ShellQuote(manifest.string()) +
      " > /dev/null 2>&1";
  if (Run(baseline_guard_cmd) != 2) {
    std::cerr << "run_phase3_baseline_reporting should enforce baseline path guard\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
