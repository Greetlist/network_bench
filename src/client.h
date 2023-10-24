#ifndef __CLIENT_H_
#define __CLIENT_H_

#include <string>
#include <stdlib.h>
#include <glog/logging.h>
#include <vector>

class BenchClient {
public:
  BenchClient(const std::string& server_str, const std::string& send_rate, const int send_duration);
  ~BenchClient() = default;
  void CheckInput();
  void Init();
  void Start();
private:
  void CalculateRate();
  std::string server_str_;
  std::string send_rate_;
  long send_rate_bytes_;
  int send_duration_;
};

#endif
