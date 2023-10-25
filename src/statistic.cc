#include "statistic.h"

BenchStatistic::BenchStatistic(int client_fd) : total_received_bytes(0), total_received_time_ms_(0), total_send_bytes(0), total_send_time_ms_(0), client_fd_(client_fd) {
}

BenchStatistic::~BenchStatistic() {
  LOG(INFO)
    << "Total Receive Data: " << total_received_bytes
    << ", Total Send Data: " << total_send_bytes;
}

void BenchStatistic::Clear() {
  total_received_bytes = 0;
  total_received_time_ms_ = 0;
  total_send_bytes = 0;
  total_send_time_ms_ = 0;
}
