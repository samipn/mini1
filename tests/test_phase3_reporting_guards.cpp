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

void WriteText(const std::filesystem::path& path, const std::string& content) {
  std::ofstream out(path);
  out << content;
}

}  // namespace

int main() {
  const auto tmp = std::filesystem::temp_directory_path() / "urbandrop_phase3_reporting_guards";
  std::filesystem::remove_all(tmp);
  std::filesystem::create_directories(tmp / "raw");
  std::filesystem::create_directories(tmp / "tables");
  std::filesystem::create_directories(tmp / "graphs");

  const auto old_manifest = tmp / "raw" / "batch_20260101T000000Z_manifest.csv";
  const auto new_manifest = tmp / "raw" / "batch_20260102T000000Z_manifest.csv";
  const auto out_csv = tmp / "raw" / "dummy.csv";

  WriteText(out_csv,
            "thread_count,ingest_ms,query_ms,total_ms,validation_ms,validation_ingest_ms,rows_accepted,result_count,average_speed_mph,average_travel_time_seconds,serial_match,serial_validation_enabled\n"
            "1,1,1,2,0,0,10,5,10,20,true,false\n");

  const std::string manifest_row =
      "batch_utc,git_branch,git_commit,optimization_step,evidence_tier,subset_label,scenario_name,mode,thread_list,dataset_path,benchmark_runs,validation_enabled,output_csv,log_file\n"
      "20260101T000000Z,main,abc123,soa_encoded_hotloop,deliverable,small,summary,serial,1,/tmp/x.csv,1,0," +
      out_csv.string() + ",/tmp/x.log\n";
  WriteText(old_manifest, manifest_row);
  WriteText(new_manifest, manifest_row);

  const std::string stale_cmd =
      "python3 ../scripts/summarize_phase3_dev.py --manifest " + ShellQuote(old_manifest.string()) +
      " --output-dir " + ShellQuote((tmp / "tables").string()) + " > /dev/null 2>&1";
  if (Run(stale_cmd) == 0) {
    std::cerr << "summarize_phase3_dev should fail stale manifest without --allow-stale-manifest\n";
    return EXIT_FAILURE;
  }

  const auto memory_csv = tmp / "raw" / "memory.csv";
  WriteText(memory_csv,
            "batch_utc,git_branch,git_commit,optimization_step,mode,thread_count,query_type,max_rss_kb,elapsed_wall_clock,exit_code,status,log_file,command\n"
            "20260101T000000Z,main,abc123,soa_encoded_hotloop,serial,1,summary,100,0:00.01,0,ok,/tmp/a.log,cmd\n"
            "20260102T000000Z,main,abc123,soa_encoded_hotloop,serial,1,summary,120,0:00.01,0,ok,/tmp/b.log,cmd\n");

  const std::string plot_multi_batch_cmd =
      "python3 ../scripts/plot_phase3_dev.py --summary-csv ../tests/fixtures/phase3_plot_summary_fixture.csv "
      "--output-dir " +
      ShellQuote((tmp / "graphs").string()) + " --memory-csv " + ShellQuote(memory_csv.string()) +
      " > /dev/null 2>&1";
  if (Run(plot_multi_batch_cmd) == 0) {
    std::cerr << "plot_phase3_dev should fail memory plotting when memory CSV has multiple batches and no key\n";
    return EXIT_FAILURE;
  }

  const std::string plot_selected_batch_cmd =
      "python3 ../scripts/plot_phase3_dev.py --summary-csv ../tests/fixtures/phase3_plot_summary_fixture.csv "
      "--output-dir " +
      ShellQuote((tmp / "graphs").string()) + " --memory-csv " + ShellQuote(memory_csv.string()) +
      " --memory-batch-utc 20260102T000000Z --allow-partial > /dev/null 2>&1";
  if (Run(plot_selected_batch_cmd) != 0) {
    std::cerr << "plot_phase3_dev should accept explicit --memory-batch-utc\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
