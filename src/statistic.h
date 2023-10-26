#ifndef __STATISTIC_H_
#define __STATISTIC_H_

#include <glog/logging.h>
#include <vector>
#include <chrono>

using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

class BenchStatistic {
public:
  explicit BenchStatistic(int client_fd);
  ~BenchStatistic();

  long& GetTotalReceivedBytes() {return total_received_bytes;}
  long& GetTotalReceivedTime() {return total_received_time_ms_;}
  long& GetTotalSendBytes() {return total_send_bytes;}
  long& GetTotalSendTime() {return total_send_time_ms_;}
  long& GetClientSendRate() {return client_send_rate_bytes_;}
  long& GetCurrentSecondReceiveRecord() {return current_second_received_bytes_;}
  long& GetCurrentSecondSendRecord() {return current_second_send_bytes_;}
  TimePoint& GetStartRecordTime() {return record_start_time_spot_;}
  bool& GetResetFlag() {return need_reset_start_time_;}
  void ResetRecordStartTime() {record_start_time_spot_ = std::chrono::system_clock::now();}
  std::vector<long>& GetSendRecord() {return send_record_;}
  std::vector<long>& GetReceiveRecord() {return receive_record_;}

  int GetSocket() {return client_fd_;}
  void Clear();
  void ResetCurrentSecondRecord();
private:
  long total_received_bytes;
  long total_received_time_ms_;
  long total_send_bytes;
  long total_send_time_ms_;
  long client_send_rate_bytes_;

  long current_second_received_bytes_;
  long current_second_send_bytes_;

  int client_fd_;

  bool need_reset_start_time_;
  TimePoint record_start_time_spot_;
  std::vector<long> send_record_;
  std::vector<long> receive_record_;
};

#endif
