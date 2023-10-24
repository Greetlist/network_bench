#ifndef __CLIENT_H_
#define __CLIENT_H_

#include <string>
#include <stdlib.h>
#include <glog/logging.h>
#include <vector>
#include <map>
#include <atomic>

class BenchClient {
public:
  BenchClient(const std::string& server_str, const std::string& send_rate, const int send_duration, bool is_parallel_);
  ~BenchClient() = default;
  void CheckInput();
  void Init();
  void InitEpoll();
  void StartSendThread();
  void WaitSendThread();
  void Start();
  void MainProcess(const std::pair<std::string, int>&);
  void Connect(const std::pair<std::string, int>&);
private:
  void SplitServerAddress();
  void CalculateRate();
  std::string server_str_;
  std::string send_rate_;
  long send_rate_bytes_;
  int send_duration_;
  int epoll_fd_;
  bool is_parallel_;
  std::atomic<bool> is_stop_{false};
  std::vector<std::pair<std::string, int>> server_addr_vec_;
  std::vector<std::thread> send_thread_vec_;
  std::thread receive_thread_;
};

#endif
