#include "io/CSVReader.hpp"
#include "io/CsvParseUtils.hpp"

#include <chrono>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "data_model/TrafficRecord.hpp"
#include "data_model/CommonCodes.hpp"

// Parallel mmap loader — Linux/POSIX only
#if !defined(_WIN32)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(_OPENMP)
#include <omp.h>
#endif
#endif

namespace urbandrop {
namespace {

// ---------------------------------------------------------------------------
// Shared helpers for the parallel mmap loaders
// ---------------------------------------------------------------------------

struct ColIndices {
  std::size_t link_id    = std::numeric_limits<std::size_t>::max();
  std::size_t speed      = std::numeric_limits<std::size_t>::max();
  std::size_t travel_time = std::numeric_limits<std::size_t>::max();
  std::size_t timestamp  = std::numeric_limits<std::size_t>::max();
  std::size_t borough    = std::numeric_limits<std::size_t>::max();
  std::size_t link_name  = std::numeric_limits<std::size_t>::max();
};

// Return the start of the next line after `pos` (i.e. char after the '\n'),
// or `end` if no newline exists.
const char* NextLineStart(const char* pos, const char* end) {
  const char* nl = static_cast<const char*>(memchr(pos, '\n', end - pos));
  return nl ? nl + 1 : end;
}

// Parse column indices from a header line.
bool ParseColIndices(const std::string& header_line, ColIndices* cols, std::string* error) {
  const std::vector<std::string> header_fields = io::ParseCsvLine(header_line);
  std::unordered_map<std::string, std::size_t> columns;
  columns.reserve(header_fields.size());
  for (std::size_t i = 0; i < header_fields.size(); ++i) {
    columns[io::ToLower(header_fields[i])] = i;
  }
  const bool ok =
      io::ResolveColumn(columns, {"link_id", "link id"}, &cols->link_id) &&
      io::ResolveColumn(columns, {"speed"}, &cols->speed) &&
      io::ResolveColumn(columns, {"travel_time", "travel time"}, &cols->travel_time) &&
      io::ResolveColumn(columns, {"data_as_of", "timestamp", "datetime"}, &cols->timestamp);
  if (!ok) {
    if (error) {
      *error = "CSV missing one or more required columns: link_id, speed, travel_time, data_as_of";
    }
    return false;
  }
  io::ResolveColumn(columns, {"borough"}, &cols->borough);
  io::ResolveColumn(columns, {"link_name", "link name"}, &cols->link_name);
  return true;
}

// Raw parsed row — used as intermediate in the parallel SoA loader.
struct RawRow {
  std::int64_t link_id              = -1;
  double       speed_mph            = 0.0;
  double       travel_time_seconds  = 0.0;
  std::int64_t timestamp            = 0;
  std::int16_t borough_code         = -1;
  std::string  link_name;
};

// Parse one text line into a RawRow.  Returns false and updates counters if
// the line should be rejected.
bool ParseRow(const std::string& line,
              const ColIndices& cols,
              RawRow* row,
              IngestionCounters* cnt) {
  const std::vector<std::string> fields = io::ParseCsvLine(line);

  const auto has_col = [&fields](std::size_t idx) { return idx < fields.size(); };
  if (!has_col(cols.link_id) || !has_col(cols.speed) ||
      !has_col(cols.travel_time) || !has_col(cols.timestamp)) {
    ++cnt->rows_rejected;
    ++cnt->malformed_rows;
    return false;
  }

  if (!io::ParseInt64(fields[cols.link_id], &row->link_id) ||
      !io::ParseDouble(fields[cols.speed], &row->speed_mph) ||
      !io::ParseDouble(fields[cols.travel_time], &row->travel_time_seconds)) {
    ++cnt->rows_rejected;
    ++cnt->malformed_rows;
    return false;
  }

  if (!io::ParseTimestamp(fields[cols.timestamp], &row->timestamp)) {
    ++cnt->rows_rejected;
    ++cnt->missing_timestamp;
    return false;
  }

  if (has_col(cols.borough)) {
    row->borough_code = ToInt(ParseBoroughCode(fields[cols.borough]));
  }
  if (has_col(cols.link_name)) {
    row->link_name = fields[cols.link_name];
  }

  if (row->speed_mph < 0.0 || row->speed_mph > 120.0) {
    ++cnt->suspicious_speed;
  }
  if (row->travel_time_seconds < 0.0) {
    ++cnt->suspicious_travel_time;
  }

  // Core-field validity check (mirrors HasValidCoreFields):
  if (row->link_id < 0 || row->travel_time_seconds < 0.0 || row->timestamp <= 0) {
    ++cnt->rows_rejected;
    ++cnt->malformed_rows;
    return false;
  }

  return true;
}

#if !defined(_WIN32)
// ---------------------------------------------------------------------------
// Parallel AoS loader (Phase 2)
// ---------------------------------------------------------------------------
static bool LoadTrafficCSVParallelImpl(const std::string& csv_path,
                                       std::size_t num_threads,
                                       TrafficDataset* dataset,
                                       std::string* error) {
  // Open and mmap the file.
  const int fd = open(csv_path.c_str(), O_RDONLY);
  if (fd < 0) {
    if (error) *error = "parallel loader: cannot open file: " + csv_path;
    return false;
  }
  struct stat st {};
  if (fstat(fd, &st) != 0 || st.st_size == 0) {
    close(fd);
    if (error) *error = "parallel loader: cannot stat or empty file: " + csv_path;
    return false;
  }
  const std::size_t file_size = static_cast<std::size_t>(st.st_size);
  const char* file_data = static_cast<const char*>(
      mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
  close(fd);
  if (file_data == MAP_FAILED) {
    if (error) *error = "parallel loader: mmap failed for: " + csv_path;
    return false;
  }

  const char* file_end = file_data + file_size;

  // Parse the header.
  const char* data_start = NextLineStart(file_data, file_end);
  if (data_start == file_data) {
    munmap(const_cast<char*>(file_data), file_size);
    if (error) *error = "parallel loader: CSV file is empty (no header newline)";
    return false;
  }
  std::string header_line(file_data, data_start - 1);
  if (!header_line.empty() && header_line.back() == '\r') header_line.pop_back();

  ColIndices cols;
  if (!ParseColIndices(header_line, &cols, error)) {
    munmap(const_cast<char*>(file_data), file_size);
    return false;
  }

  // Compute chunk boundaries aligned to line starts.
  if (num_threads < 1) num_threads = 1;
  std::vector<const char*> chunk_starts(num_threads + 1);
  chunk_starts[0] = data_start;
  chunk_starts[num_threads] = file_end;
  const std::size_t data_size = static_cast<std::size_t>(file_end - data_start);
  for (std::size_t i = 1; i < num_threads; ++i) {
    const char* approx = data_start + data_size * i / num_threads;
    chunk_starts[i] = NextLineStart(approx, file_end);
  }

  // Thread-local datasets.
  std::vector<TrafficDataset> locals(num_threads);

#if defined(_OPENMP)
#pragma omp parallel for num_threads(static_cast<int>(num_threads)) schedule(static, 1)
#endif
  for (std::size_t t = 0; t < num_threads; ++t) {
    const char* p   = chunk_starts[t];
    const char* end = chunk_starts[t + 1];
    TrafficDataset& local = locals[t];
    IngestionCounters& cnt = local.Counters();

    while (p < end) {
      const char* nl      = static_cast<const char*>(memchr(p, '\n', end - p));
      const char* line_end = nl ? nl : end;

      std::string line(p, line_end);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      p = nl ? nl + 1 : end;

      if (line.empty()) continue;
      ++cnt.rows_read;

      RawRow row;
      if (!ParseRow(line, cols, &row, &cnt)) continue;

      TrafficRecord rec;
      rec.link_id                 = row.link_id;
      rec.speed_mph               = row.speed_mph;
      rec.travel_time_seconds     = row.travel_time_seconds;
      rec.timestamp_epoch_seconds = row.timestamp;
      rec.borough_code            = row.borough_code;
      rec.link_name               = row.link_name;
      local.AddRecord(rec);
    }
  }

  munmap(const_cast<char*>(file_data), file_size);

  // Merge thread-local results into the output dataset.
  std::size_t total_accepted = 0;
  for (std::size_t t = 0; t < num_threads; ++t) {
    total_accepted += locals[t].Counters().rows_accepted;
  }
  dataset->Reserve(total_accepted);

  IngestionCounters& dst = dataset->Counters();
  for (std::size_t t = 0; t < num_threads; ++t) {
    const IngestionCounters& src = locals[t].Counters();
    dst.rows_read             += src.rows_read;
    dst.rows_rejected         += src.rows_rejected;
    dst.malformed_rows        += src.malformed_rows;
    dst.missing_timestamp     += src.missing_timestamp;
    dst.suspicious_speed      += src.suspicious_speed;
    dst.suspicious_travel_time += src.suspicious_travel_time;

    for (const TrafficRecord& rec : locals[t].Records()) {
      dataset->AddRecord(rec);
    }
  }

  return true;
}

// ---------------------------------------------------------------------------
// Parallel SoA loader (Phase 3)
// ---------------------------------------------------------------------------
static bool LoadTrafficCSVOptimizedParallelImpl(const std::string& csv_path,
                                                std::size_t num_threads,
                                                TrafficDatasetOptimized* dataset,
                                                std::string* error) {
  const int fd = open(csv_path.c_str(), O_RDONLY);
  if (fd < 0) {
    if (error) *error = "parallel optimized loader: cannot open file: " + csv_path;
    return false;
  }
  struct stat st {};
  if (fstat(fd, &st) != 0 || st.st_size == 0) {
    close(fd);
    if (error) *error = "parallel optimized loader: cannot stat or empty file: " + csv_path;
    return false;
  }
  const std::size_t file_size = static_cast<std::size_t>(st.st_size);
  const char* file_data = static_cast<const char*>(
      mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
  close(fd);
  if (file_data == MAP_FAILED) {
    if (error) *error = "parallel optimized loader: mmap failed for: " + csv_path;
    return false;
  }

  const char* file_end = file_data + file_size;

  const char* data_start = NextLineStart(file_data, file_end);
  if (data_start == file_data) {
    munmap(const_cast<char*>(file_data), file_size);
    if (error) *error = "parallel optimized loader: CSV file is empty";
    return false;
  }
  std::string header_line(file_data, data_start - 1);
  if (!header_line.empty() && header_line.back() == '\r') header_line.pop_back();

  ColIndices cols;
  if (!ParseColIndices(header_line, &cols, error)) {
    munmap(const_cast<char*>(file_data), file_size);
    return false;
  }

  if (num_threads < 1) num_threads = 1;
  std::vector<const char*> chunk_starts(num_threads + 1);
  chunk_starts[0] = data_start;
  chunk_starts[num_threads] = file_end;
  const std::size_t data_size = static_cast<std::size_t>(file_end - data_start);
  for (std::size_t i = 1; i < num_threads; ++i) {
    const char* approx = data_start + data_size * i / num_threads;
    chunk_starts[i] = NextLineStart(approx, file_end);
  }

  // Thread-local raw row buffers.
  std::vector<std::vector<RawRow>> locals(num_threads);
  std::vector<IngestionCounters>   local_ctrs(num_threads);

#if defined(_OPENMP)
#pragma omp parallel for num_threads(static_cast<int>(num_threads)) schedule(static, 1)
#endif
  for (std::size_t t = 0; t < num_threads; ++t) {
    const char* p   = chunk_starts[t];
    const char* end = chunk_starts[t + 1];
    std::vector<RawRow>& rows = locals[t];
    IngestionCounters&   cnt  = local_ctrs[t];

    while (p < end) {
      const char* nl       = static_cast<const char*>(memchr(p, '\n', end - p));
      const char* line_end = nl ? nl : end;

      std::string line(p, line_end);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      p = nl ? nl + 1 : end;

      if (line.empty()) continue;
      ++cnt.rows_read;

      RawRow row;
      if (!ParseRow(line, cols, &row, &cnt)) continue;

      rows.push_back(std::move(row));
    }
  }

  munmap(const_cast<char*>(file_data), file_size);

  // Merge: build SoA columns sequentially (handles string interning correctly).
  std::size_t total_accepted = 0;
  for (std::size_t t = 0; t < num_threads; ++t) {
    total_accepted += locals[t].size();
  }
  dataset->Reserve(total_accepted);

  IngestionCounters& dst = dataset->Counters();
  for (std::size_t t = 0; t < num_threads; ++t) {
    const IngestionCounters& src = local_ctrs[t];
    dst.rows_read             += src.rows_read;
    dst.rows_rejected         += src.rows_rejected;
    dst.malformed_rows        += src.malformed_rows;
    dst.missing_timestamp     += src.missing_timestamp;
    dst.suspicious_speed      += src.suspicious_speed;
    dst.suspicious_travel_time += src.suspicious_travel_time;

    for (const RawRow& row : locals[t]) {
      dataset->AddRecord(row.link_id,
                         row.speed_mph,
                         row.travel_time_seconds,
                         row.timestamp,
                         row.borough_code,
                         row.link_name);
    }
  }

  return true;
}
#endif  // !defined(_WIN32)

}  // namespace

bool CSVReader::LoadTrafficCSV(const std::string& csv_path,
                               TrafficDataset* dataset,
                               const TrafficLoadOptions& options,
                               std::ostream* log_stream,
                               std::string* error) {
  if (dataset == nullptr) {
    if (error != nullptr) {
      *error = "dataset pointer cannot be null";
    }
    return false;
  }

  dataset->Clear();

#if !defined(_WIN32)
  if (options.num_threads > 1) {
    return LoadTrafficCSVParallelImpl(csv_path, options.num_threads, dataset, error);
  }
#endif

  std::ifstream input(csv_path);
  if (!input.is_open()) {
    if (error != nullptr) {
      *error = "unable to open CSV file: " + csv_path;
    }
    return false;
  }

  std::string header_line;
  if (!std::getline(input, header_line)) {
    if (error != nullptr) {
      *error = "CSV file is empty: " + csv_path;
    }
    return false;
  }

  const std::vector<std::string> header_fields = io::ParseCsvLine(header_line);
  std::unordered_map<std::string, std::size_t> columns;
  columns.reserve(header_fields.size());
  for (std::size_t i = 0; i < header_fields.size(); ++i) {
    columns[io::ToLower(header_fields[i])] = i;
  }

  std::size_t link_id_col = std::numeric_limits<std::size_t>::max();
  std::size_t speed_col = std::numeric_limits<std::size_t>::max();
  std::size_t travel_time_col = std::numeric_limits<std::size_t>::max();
  std::size_t timestamp_col = std::numeric_limits<std::size_t>::max();
  std::size_t borough_col = std::numeric_limits<std::size_t>::max();
  std::size_t link_name_col = std::numeric_limits<std::size_t>::max();

  const bool found_required =
      io::ResolveColumn(columns, {"link_id", "link id"}, &link_id_col) &&
      io::ResolveColumn(columns, {"speed"}, &speed_col) &&
      io::ResolveColumn(columns, {"travel_time", "travel time"}, &travel_time_col) &&
      io::ResolveColumn(columns, {"data_as_of", "timestamp", "datetime"}, &timestamp_col);

  io::ResolveColumn(columns, {"borough"}, &borough_col);
  io::ResolveColumn(columns, {"link_name", "link name"}, &link_name_col);

  if (!found_required) {
    if (error != nullptr) {
      *error = "CSV missing one or more required columns: link_id, speed, travel_time, data_as_of";
    }
    return false;
  }

  IngestionCounters& counters = dataset->Counters();
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }

    ++counters.rows_read;
    const std::vector<std::string> fields = io::ParseCsvLine(line);

    const auto has_col = [&fields](std::size_t idx) { return idx < fields.size(); };
    if (!has_col(link_id_col) || !has_col(speed_col) || !has_col(travel_time_col) || !has_col(timestamp_col)) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    TrafficRecord record;
    if (!io::ParseInt64(fields[link_id_col], &record.link_id) ||
        !io::ParseDouble(fields[speed_col], &record.speed_mph) ||
        !io::ParseDouble(fields[travel_time_col], &record.travel_time_seconds)) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    if (!io::ParseTimestamp(fields[timestamp_col], &record.timestamp_epoch_seconds)) {
      ++counters.rows_rejected;
      ++counters.missing_timestamp;
      continue;
    }

    if (has_col(borough_col)) {
      record.borough = fields[borough_col];
      record.borough_code = ToInt(ParseBoroughCode(record.borough));
    }
    if (has_col(link_name_col)) {
      record.link_name = fields[link_name_col];
    }

    if (record.speed_mph < 0.0 || record.speed_mph > 120.0) {
      ++counters.suspicious_speed;
    }
    if (record.travel_time_seconds < 0.0) {
      ++counters.suspicious_travel_time;
    }

    if (!record.HasValidCoreFields()) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    dataset->AddRecord(record);

    if (options.print_progress && options.progress_every_rows > 0 &&
        counters.rows_read % options.progress_every_rows == 0 && log_stream != nullptr) {
      (*log_stream) << "[ingest] rows_read=" << counters.rows_read
                    << " accepted=" << counters.rows_accepted
                    << " rejected=" << counters.rows_rejected << '\n';
    }
  }

  return true;
}

bool CSVReader::LoadTrafficCSVOptimized(const std::string& csv_path,
                                        TrafficDatasetOptimized* dataset,
                                        const TrafficLoadOptions& options,
                                        std::ostream* log_stream,
                                        std::string* error) {
  if (dataset == nullptr) {
    if (error != nullptr) {
      *error = "optimized dataset pointer cannot be null";
    }
    return false;
  }

  dataset->Clear();

#if !defined(_WIN32)
  if (options.num_threads > 1) {
    return LoadTrafficCSVOptimizedParallelImpl(csv_path, options.num_threads, dataset, error);
  }
#endif

  std::ifstream input(csv_path);
  if (!input.is_open()) {
    if (error != nullptr) {
      *error = "unable to open CSV file: " + csv_path;
    }
    return false;
  }

  std::string header_line;
  if (!std::getline(input, header_line)) {
    if (error != nullptr) {
      *error = "CSV file is empty: " + csv_path;
    }
    return false;
  }

  const std::vector<std::string> header_fields = io::ParseCsvLine(header_line);
  std::unordered_map<std::string, std::size_t> columns;
  columns.reserve(header_fields.size());
  for (std::size_t i = 0; i < header_fields.size(); ++i) {
    columns[io::ToLower(header_fields[i])] = i;
  }

  std::size_t link_id_col = std::numeric_limits<std::size_t>::max();
  std::size_t speed_col = std::numeric_limits<std::size_t>::max();
  std::size_t travel_time_col = std::numeric_limits<std::size_t>::max();
  std::size_t timestamp_col = std::numeric_limits<std::size_t>::max();
  std::size_t borough_col = std::numeric_limits<std::size_t>::max();
  std::size_t link_name_col = std::numeric_limits<std::size_t>::max();

  const bool found_required =
      io::ResolveColumn(columns, {"link_id", "link id"}, &link_id_col) &&
      io::ResolveColumn(columns, {"speed"}, &speed_col) &&
      io::ResolveColumn(columns, {"travel_time", "travel time"}, &travel_time_col) &&
      io::ResolveColumn(columns, {"data_as_of", "timestamp", "datetime"}, &timestamp_col);

  io::ResolveColumn(columns, {"borough"}, &borough_col);
  io::ResolveColumn(columns, {"link_name", "link name"}, &link_name_col);

  if (!found_required) {
    if (error != nullptr) {
      *error = "CSV missing one or more required columns: link_id, speed, travel_time, data_as_of";
    }
    return false;
  }

  IngestionCounters& counters = dataset->Counters();
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }

    ++counters.rows_read;
    const std::vector<std::string> fields = io::ParseCsvLine(line);

    const auto has_col = [&fields](std::size_t idx) { return idx < fields.size(); };
    if (!has_col(link_id_col) || !has_col(speed_col) || !has_col(travel_time_col) ||
        !has_col(timestamp_col)) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    std::int64_t link_id = -1;
    double speed_mph = 0.0;
    double travel_time_seconds = 0.0;
    std::int64_t timestamp_epoch_seconds = 0;

    if (!io::ParseInt64(fields[link_id_col], &link_id) || !io::ParseDouble(fields[speed_col], &speed_mph) ||
        !io::ParseDouble(fields[travel_time_col], &travel_time_seconds)) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    if (!io::ParseTimestamp(fields[timestamp_col], &timestamp_epoch_seconds)) {
      ++counters.rows_rejected;
      ++counters.missing_timestamp;
      continue;
    }

    std::int16_t borough_code = -1;
    std::string link_name;
    if (has_col(borough_col)) {
      borough_code = ToInt(ParseBoroughCode(fields[borough_col]));
    }
    if (has_col(link_name_col)) {
      link_name = fields[link_name_col];
    }

    if (speed_mph < 0.0 || speed_mph > 120.0) {
      ++counters.suspicious_speed;
    }
    if (travel_time_seconds < 0.0) {
      ++counters.suspicious_travel_time;
    }
    if (link_id < 0 || speed_mph < 0.0 || travel_time_seconds < 0.0 || timestamp_epoch_seconds <= 0) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    dataset->AddRecord(link_id,
                       speed_mph,
                       travel_time_seconds,
                       timestamp_epoch_seconds,
                       borough_code,
                       link_name);

    if (options.print_progress && options.progress_every_rows > 0 &&
        counters.rows_read % options.progress_every_rows == 0 && log_stream != nullptr) {
      (*log_stream) << "[ingest_optimized] rows_read=" << counters.rows_read
                    << " accepted=" << counters.rows_accepted
                    << " rejected=" << counters.rows_rejected << '\n';
    }
  }

  return true;
}

}  // namespace urbandrop
