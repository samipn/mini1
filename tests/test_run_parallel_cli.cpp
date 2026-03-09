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
  out << "5,22.0,45.0,ok,2024-01-01T00:04:00,300,Brooklyn,Link C\n";
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
  return WEXITSTATUS(code);
}

}  // namespace

int main() {
  const std::string csv = WriteFixtureCsv();
  const std::string qcsv = ShellQuote(csv);
  const std::string out_path =
      (std::filesystem::temp_directory_path() / "urbandrop_run_parallel_bench_output.csv").string();

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

  const std::string time_cmd =
      "./run_parallel --traffic " + qcsv +
      " --query time_window --start-epoch 1704067200 --end-epoch 1704067380 --threads 2 --validate-serial > /dev/null";
  if (Run(time_cmd) != 0) {
    std::cerr << "run_parallel time_window failed\n";
    return EXIT_FAILURE;
  }

  const std::string topn_cmd =
      "./run_parallel --traffic " + qcsv +
      " --query top_n_slowest --top-n 2 --min-link-samples 1 --threads 2 --validate-serial > /dev/null";
  if (Run(topn_cmd) != 0) {
    std::cerr << "run_parallel top_n_slowest failed\n";
    return EXIT_FAILURE;
  }

  const std::string summary_cmd =
      "./run_parallel --traffic " + qcsv +
      " --query summary --thread-list 1,2,4,8 --benchmark-runs 2 --validate-serial --benchmark-out " +
      ShellQuote(out_path) + " > /dev/null";
  if (Run(summary_cmd) != 0) {
    std::cerr << "run_parallel summary/thread-list failed\n";
    return EXIT_FAILURE;
  }
  if (!std::filesystem::exists(out_path) || CountLines(out_path) != 9) {
    std::cerr << "run_parallel benchmark output CSV should have header + 8 rows\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
