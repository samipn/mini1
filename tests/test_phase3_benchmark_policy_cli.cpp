#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/wait.h>

namespace {

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
  const std::string low_runs_no_override =
      "bash ../scripts/run_phase3_dev_benchmarks.sh --runs 1 > /dev/null 2>&1";
  if (Run(low_runs_no_override) != 2) {
    std::cerr << "run_phase3_dev_benchmarks should reject --runs < 3 unless --allow-low-runs is set\n";
    return EXIT_FAILURE;
  }

  const std::string bad_runs =
      "bash ../scripts/run_phase3_dev_benchmarks.sh --runs nope > /dev/null 2>&1";
  if (Run(bad_runs) != 2) {
    std::cerr << "run_phase3_dev_benchmarks should reject non-numeric --runs\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
