#ifndef __SERVER_H_
#define __SERVER_H_

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glog/logging.h>
#include <iostream>
#include <sys/uio.h>
#include <errno.h>
#include <string.h>

#include <vector>
#include <atomic>
#include <map>
#include <chrono>

#include "statistic.h"
#include "util.h"
#include "static_vars.h"

class BenchServer {
public:
  BenchServer(const std::string& server_str, int read_block_size);
  ~BenchServer() = default;
  void Init();
  void Start();
  void Stop();
private:
  void SplitServerAddress();
  void InitEpoll();
  void InitListenSockets();
  bool InitSingleListenSocket(const std::pair<std::string, int>& server_info);
  int AcceptClient(int);
  void ReadClientSendRate(struct epoll_event*);
  int ReceiveBytes(struct epoll_event*);
  std::string server_str_;
  std::vector<std::pair<std::string, int>> server_addr_vec_;
  std::vector<int> listen_fd_vec_;
  int read_block_size_;
  int epoll_fd_;
  std::atomic<bool> stop_{false};
};

#endif
