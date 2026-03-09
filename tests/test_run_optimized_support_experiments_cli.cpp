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
  const std::string invalid_repeats =
      "./run_optimized_support_experiments --traffic /tmp/does_not_matter.csv --repeats nope > /dev/null 2>&1";
  if (Run(invalid_repeats) != 2) {
    std::cerr << "run_optimized_support_experiments should return code 2 for invalid --repeats\n";
    return EXIT_FAILURE;
  }

  const std::string zero_repeats =
      "./run_optimized_support_experiments --traffic /tmp/does_not_matter.csv --repeats 0 > /dev/null 2>&1";
  if (Run(zero_repeats) != 2) {
    std::cerr << "run_optimized_support_experiments should reject --repeats 0\n";
    return EXIT_FAILURE;
  }

  const std::string unknown_arg =
      "./run_optimized_support_experiments --traffic /tmp/does_not_matter.csv --nope > /dev/null 2>&1";
  if (Run(unknown_arg) != 2) {
    std::cerr << "run_optimized_support_experiments should reject unknown args\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
