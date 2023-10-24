#ifndef __CLIENT_H_
#define __CLIENT_H_

#include <string>

class BenchClient {
public:
  BenchClient(const std::string& server_str);
  ~BenchClient() = default;
  void Init();
  void Start();
private:
  std::string server_str_;
};

#endif
