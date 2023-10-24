#include "server.h"

BenchServer::BenchServer(const std::string& server_str) : server_str_(server_str) {
}

void BenchServer::Init() {
  SplitServerAddress();
  InitEpoll();
  InitListenSockets();
}

void BenchServer::SplitServerAddress() {
  std::string servers_delimiter{","};
  size_t pos_start = 0, pos_end = 0;
  while ((pos_end = server_str_.find(servers_delimiter, pos_start)) != std::string::npos) {
    std::string single_server = server_str_.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + servers_delimiter.size();

    std::string port_delimiter{":"};
    size_t port_start = 0;
    if ((port_start) = single_server.find(port_delimiter, port_start) != std::string::npos) {
      std::string interface = single_server.substr(0, port_start);
      int port = std::stoi(single_server.substr(port_start+1, single_server.size() - port_start));
      server_addr_vec_.push_back(std::make_pair<std::string, int>(interface, port));
    }
  }
}

void BenchServer::InitEpoll() {
}

void BenchServer::InitListenSockets() {
}

void BenchServer::Start() {
}

