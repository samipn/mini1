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
  const auto path = std::filesystem::temp_directory_path() / "urbandrop_run_parallel_fixture.csv";
  std::ofstream out(path);
  out << "id,speed,travel_time,status,data_as_of,link_id,borough,link_name\n";
  out << "1,10.0,30.0,ok,2024-01-01T00:00:00,100,Manhattan,Link A\n";
  out << "2,25.0,20.0,ok,2024-01-01T00:01:00,200,Queens,Link B\n";
  out << "3,12.0,40.0,ok,2024-01-01T00:02:00,100,Manhattan,Link A\n";
  out << "4,18.0,50.0,ok,2024-01-01T00:03:00,300,Brooklyn,Link C\n";
  return path.string();
}

int Run(const std::string& command) {
  const int code = std::system(command.c_str());
  if (code == -1) {
    return code;
  }
  return WEXITSTATUS(code);
}

}  // namespace

int main() {
  const std::string csv = WriteFixtureCsv();
  const std::string qcsv = ShellQuote(csv);

  const std::string speed_cmd =
      "./run_parallel --traffic " + qcsv +
      " --query speed_below --threshold 20 --threads 4 --benchmark-runs 2 --validate-serial > /dev/null";
  if (Run(speed_cmd) != 0) {
    std::cerr << "run_parallel speed_below failed\n";
    return EXIT_FAILURE;
  }

  const std::string borough_cmd =
      "./run_parallel --traffic " + qcsv +
      " --query borough_speed_below --borough Manhattan --threshold 20 --threads 2 --validate-serial > /dev/null";
  if (Run(borough_cmd) != 0) {
    std::cerr << "run_parallel borough_speed_below failed\n";
    return EXIT_FAILURE;
  }

  const std::string summary_cmd =
      "./run_parallel --traffic " + qcsv + " --query summary --threads 2 --validate-serial > /dev/null";
  if (Run(summary_cmd) != 0) {
    std::cerr << "run_parallel summary failed\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
