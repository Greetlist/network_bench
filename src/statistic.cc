#include "statistic.h"

BenchStatistic::BenchStatistic(int client_fd) : total_received_bytes(0), total_received_time_ms_(0), total_send_bytes(0), total_send_time_ms_(0), client_send_rate_bytes_(-1), current_second_received_bytes_(0), current_second_send_bytes_(0), client_fd_(client_fd), need_reset_start_time_(true) {
}

BenchStatistic::~BenchStatistic() {
  LOG(INFO)
    << "Total Receive Data: " << total_received_bytes
    << ", Total Send Data: " << total_send_bytes;
  int receive_record_len = receive_record_.size();
  for (int i = 0; i < receive_record_len; ++i) {
    LOG(INFO) << i << " s Receive Rate is: " << receive_record_[i] / 1024.0 / 1024.0 << "MB/s";
  }
  int send_record_len = send_record_.size();
  for (int i = 0; i < send_record_len; ++i) {
    LOG(INFO) << i << " s Send Rate is: " << send_record_[i] / 1024.0 / 1024.0 << "MB/s";
  }
}

void BenchStatistic::ResetCurrentSecondRecord() {
  current_second_received_bytes_ = 0;
  current_second_send_bytes_ = 0;
}

void BenchStatistic::Clear() {
  total_received_bytes = 0;
  total_received_time_ms_ = 0;
  total_send_bytes = 0;
  total_send_time_ms_ = 0;
}
