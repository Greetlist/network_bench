#ifndef __STATISTIC_H_
#define __STATISTIC_H_

#include <glog/logging.h>
#include <vector>
#include <chrono>

using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

enum Direction {
  Receive = 0,
  Send = 1,
};

struct RateRecord {
  Direction direction;
  TimePoint record_start_time_point;
  long current_second_record;
  std::vector<long> total_records;
};

class BenchStatistic {
public:
  explicit BenchStatistic(int client_fd);
  ~BenchStatistic();

  long& GetTotalReceivedBytes() {return total_received_bytes;}
  long& GetTotalReceivedTime() {return total_received_time_ms_;}
  long& GetTotalSendBytes() {return total_send_bytes;}
  long& GetTotalSendTime() {return total_send_time_ms_;}
  long& GetClientSendRate() {return client_send_rate_bytes_;}

  int GetSocket() {return client_fd_;}
  void Clear();
  void RecordCurrentSecondRate(Direction, long);
private:
  long total_received_bytes;
  long total_received_time_ms_;
  long total_send_bytes;
  long total_send_time_ms_;
  long client_send_rate_bytes_;
  int client_fd_;
  std::vector<RateRecord> rate_records_;
};

#endif
