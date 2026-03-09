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

std::string WriteFixtureCsv() {
  const auto path = std::filesystem::temp_directory_path() / "urbandrop_run_optimized_fixture.csv";
  std::ofstream out(path);
  out << "id,speed,travel_time,status,data_as_of,link_id,borough,link_name\n";
  out << "1,10.0,30.0,ok,2024-01-01T00:00:00,100,Manhattan,Link A\n";
  out << "2,25.0,20.0,ok,2024-01-01T00:01:00,200,Queens,Link B\n";
  out << "3,12.0,40.0,ok,2024-01-01T00:02:00,100,Manhattan,Link A\n";
  return path.string();
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
  const std::string csv = WriteFixtureCsv();
  const std::string qcsv = ShellQuote(csv);

  const std::string direct_cmd =
      "./run_optimized --traffic " + qcsv +
      " --query speed_below --threshold 20 --execution serial --load-mode direct --benchmark-runs 2 --validate-serial > /dev/null";
  if (Run(direct_cmd) != 0) {
    std::cerr << "run_optimized direct mode failed\n";
    return EXIT_FAILURE;
  }

  const std::string parallel_cmd =
      "./run_optimized --traffic " + qcsv +
      " --query summary --execution parallel --threads 2 --load-mode direct --benchmark-runs 2 --validate-serial > /dev/null";
  if (Run(parallel_cmd) != 0) {
    std::cerr << "run_optimized parallel mode failed\n";
    return EXIT_FAILURE;
  }

  const std::string convert_cmd =
      "./run_optimized --traffic " + qcsv +
      " --query time_window --start-epoch 1704067200 --end-epoch 1704067380 --execution serial --load-mode row_convert > /dev/null";
  if (Run(convert_cmd) != 0) {
    std::cerr << "run_optimized row_convert mode failed\n";
    return EXIT_FAILURE;
  }

  const std::string invalid_cmd =
      "./run_optimized --traffic " + qcsv + " --benchmark-runs nope > /dev/null 2>&1";
  if (Run(invalid_cmd) != 2) {
    std::cerr << "run_optimized should return code 2 on invalid numeric argument\n";
    return EXIT_FAILURE;
  }

  const std::string unknown_query_cmd =
      "./run_optimized --traffic " + qcsv + " --query no_such_query --load-mode row_convert > /dev/null 2>&1";
  if (Run(unknown_query_cmd) != 2) {
    std::cerr << "run_optimized should reject unknown query types\n";
    return EXIT_FAILURE;
  }

  const std::string row_convert_benchmark_cmd =
      "./run_optimized --traffic " + qcsv +
      " --query summary --load-mode row_convert --benchmark-runs 2 > /dev/null 2>&1";
  if (Run(row_convert_benchmark_cmd) != 2) {
    std::cerr << "run_optimized should reject benchmark harness options in row_convert mode\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
