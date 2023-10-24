#ifndef __SERVER_H_
#define __SERVER_H_

#include <vector>
#include <atomic>
#include <pair>

class BenchServer {
public:
  BenchServer(const std::string& server_str);
  ~BenchServer() = default;
  void Init();
  void Start();
  void Stop();
private:
  void SplitServerAddress();
  void InitEpoll();
  void InitListenSockets();
  std::string server_str_;
  std::vector<std::pair<std::string, int>> server_addr_vec_;
  int epoll_fd_;
  atomic<bool> stop_{false};
};

#endif
