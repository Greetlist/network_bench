#ifndef __STATISTIC_H_
#define __STATISTIC_H_

class BenchStatistic {
public:
  explicit BenchStatistic(int client_fd);
  ~BenchStatistic() = default;
  long& GetTotalBytes() {return total_bytes_received_;}
  void SetTotalBytes(long value) {total_bytes_received_ = value;}
  long& GetTotalTransmitTime() {return total_transmit_time_ms_;}
  void SetTotalTransmitTime(long value) {total_transmit_time_ms_ = value;}
  int GetClient() {return client_fd_;}
  void Clear();
private:
  long total_bytes_received_;
  long total_transmit_time_ms_;
  int client_fd_;
};

#endif
