#include "statistic.h"

BenchStatistic::BenchStatistic(int client_fd) : total_bytes_received_(0), total_transmit_time_ms_(0), client_fd_(client_fd) {
}

void BenchStatistic::Clear() {
  total_bytes_received_ = 0;
  total_transmit_time_ms_ = 0;
}
