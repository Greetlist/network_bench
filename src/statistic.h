#ifndef __STATISTIC_H_
#define __STATISTIC_H_

class BenchStatistic {
public:
  explicit BenchStatistic(int client_fd);
  ~BenchStatistic() = default;

  long& GetTotalReceivedBytes() {return total_received_bytes;}
  long& GetTotalReceivedTime() {return total_received_time_ms_;}
  long& GetTotalSendBytes() {return total_send_bytes;}
  long& GetTotalSendTime() {return total_send_time_ms_;}

  int GetClient() {return client_fd_;}
  void Clear();
private:
  long total_received_bytes;
  long total_received_time_ms_;
  long total_send_bytes;
  long total_send_time_ms_;
  int client_fd_;
};

#endif
