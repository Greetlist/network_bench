#ifndef __CLIENT_H_
#define __CLIENT_H_

#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <glog/logging.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <memory>
#include <thread>

#include "statistic.h"
#include "util.h"
#include "static_vars.h"

class BenchClient {
public:
  BenchClient(const std::string& server_str, const std::string& send_rate, const int send_duration, int read_block_size, int write_block_size, bool is_parallel);
  ~BenchClient() = default;
  void CheckInput();
  void Init();
  void InitEpoll();
  void CreateSendThread();
  void WaitSendThread();
  void Start();
  void SendProcess(BenchStatistic*);
  void ReceiveProcess();
  BenchStatistic* Connect(const std::pair<std::string, int>&);
  void NotifyServerSendRate(BenchStatistic*);
private:
  void SplitServerAddress();
  void CalculateRate();
  std::string server_str_;
  std::string send_rate_;
  long send_rate_bytes_;
  int send_duration_;
  int read_block_size_;
  int write_block_size_;
  int epoll_fd_;
  std::atomic<int> alive_client_count_{0};
  bool is_parallel_;
  std::vector<std::pair<std::string, int>> server_addr_vec_;
  std::vector<std::thread> send_thread_vec_;
  std::thread receive_thread_;
};

#endif
