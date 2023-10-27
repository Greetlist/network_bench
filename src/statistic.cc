#include "statistic.h"

BenchStatistic::BenchStatistic(int client_fd) : total_received_bytes(0), total_received_time_ms_(0), total_send_bytes(0), total_send_time_ms_(0), client_send_rate_bytes_(-1), client_fd_(client_fd) {
  rate_records_.push_back(RateRecord()); // for receive record
  rate_records_.push_back(RateRecord()); // for send record;
}

BenchStatistic::~BenchStatistic() {
  LOG(INFO)
    << "Total Receive Data: " << total_received_bytes
    << ", Total Send Data: " << total_send_bytes;

  int rate_record_len = rate_records_.size();
  for (int i = 0; i < rate_record_len; ++i) {
    RateRecord& record = rate_records_[i];
    int total_record_len = record.total_records.size();
    for (int j = 0; j < total_record_len; ++j) {
      LOG(INFO) << j << " s " << (i == 0 ? "Receive" : "Send") << " Rate is: " << record.total_records[j] / 1024.0 / 1024.0 << "MB/s";
    }
  }
}

void BenchStatistic::Clear() {
  total_received_bytes = 0;
  total_received_time_ms_ = 0;
  total_send_bytes = 0;
  total_send_time_ms_ = 0;
}

void BenchStatistic::RecordCurrentSecondRate(Direction direction, long bytes) {
  RateRecord& record = rate_records_[direction];
  auto time_elapse_ms = \
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - record.record_start_time_point).count();
  if (time_elapse_ms < 1000) {
    record.current_second_record += bytes;
    return;
  }
  record.record_start_time_point = std::chrono::system_clock::now();
  record.total_records.push_back(record.current_second_record);
  record.current_second_record = 0;
}
