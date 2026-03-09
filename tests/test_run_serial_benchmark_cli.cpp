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
  const auto path = std::filesystem::temp_directory_path() / "urbandrop_run_serial_bench_fixture.csv";
  std::ofstream out(path);
  out << "id,speed,travel_time,status,data_as_of,link_id,borough,link_name\n";
  out << "1,10.0,30.0,ok,2024-01-01T00:00:00,100,Manhattan,Link A\n";
  out << "2,20.0,20.0,ok,2024-01-01T00:01:00,200,Queens,Link B\n";
  out << "3,30.0,10.0,ok,2024-01-01T00:02:00,300,Brooklyn,Link C\n";
  return path.string();
}

std::size_t CountLines(const std::string& path) {
  std::ifstream in(path);
  std::size_t lines = 0;
  std::string line;
  while (std::getline(in, line)) {
    ++lines;
  }
  return lines;
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
  const std::string csv_path = WriteFixtureCsv();
  const std::string out_path =
      (std::filesystem::temp_directory_path() / "urbandrop_run_serial_bench_output.csv").string();

  const std::string cmd = "./run_serial --traffic " + ShellQuote(csv_path) +
                          " --query summary --benchmark-runs 2 --benchmark-out " +
                          ShellQuote(out_path) + " --expect-accepted 3 > /dev/null";
  if (Run(cmd) != 0) {
    std::cerr << "run_serial benchmark mode failed\n";
    return EXIT_FAILURE;
  }

  if (!std::filesystem::exists(out_path) || CountLines(out_path) != 3) {
    std::cerr << "run_serial benchmark output CSV should have header + 2 rows\n";
    return EXIT_FAILURE;
  }

  const std::string invalid_cmd =
      "./run_serial --traffic " + ShellQuote(csv_path) + " --benchmark-runs nope > /dev/null 2>&1";
  if (Run(invalid_cmd) != 2) {
    std::cerr << "run_serial should return code 2 on invalid numeric argument\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
