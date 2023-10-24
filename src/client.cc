#include "client.h"

BenchClient::BenchClient(const std::string& server_str, const std::string& send_rate, const int send_duration) : server_str_(server_str), send_rate_(send_rate), send_rate_bytes_(0), send_duration_(send_duration) {
}

void BenchClient::CheckInput() {
  if (send_rate_.size() < 2) {
    LOG(ERROR) << "send_rate args string length is less than 2, Quit";
    exit(1);
  }

  std::string unit = send_rate_.substr(send_rate_.size() - 1);
  std::vector<std::string> valid_unit_vec{"K", "M", "G"};
  bool valid = false;
  for (const auto& u : valid_unit_vec) {
    if (unit == u) {
      valid = true;
      break;
    }
  }
  if (!valid) {
    LOG(ERROR) << "send_rate unit is invalid, valid_list: ['K', 'M', 'G']";
    exit(1);
  }
}

void BenchClient::Init() {
  CalculateRate();
}

void BenchClient::Start() {

}

void BenchClient::CalculateRate() {
}
