#include "client.h"

BenchClient::BenchClient(const std::string& server_str, const std::string& send_rate, const int send_duration, bool is_parallel) : server_str_(server_str), send_rate_(send_rate), send_rate_bytes_(0), send_duration_(send_duration), is_parallel_(is_parallel)  {
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
  SplitServerAddress();
  CalculateRate();
  InitEpoll();
}

void BenchClient::InitEpoll() {
  epoll_fd_ = epoll_create1(0);
  if (epoll_fd_ <= 0) {
    LOG(INFO) << "Epoll Create Error: " << strerror(errno);
    exit(1);
  }
}

void BenchClient::Start() {
  StartSendThread();
  receive_thread_ = std::thread(&BenchClient::ReceiveThread, this);
  WaitSendThread();
  is_stop_ = true;
  WaitReceiveThread();
}

void BenchClient::MainProcess(const std::pair<std::string, int>& server_info) {
  Connect(server_info);
}

void BenchClient::Connect(const std::pair<std::string, int>& server_info) {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    LOG_INFO << "Create Socket Error: " << strerror(errno);
    return;
  }
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  inet_pton(AF_INET, server_info.first.c_str(), &addr.sin_addr);
  addr.sin_port = htons(server_info.second);
  int connect_status = connect(socket_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
  if (connect_status < 0) {
    LOG(ERROR) << "Connect to [" << server_info.first << ":" << server_info.second << "] Error:" << strerror(errno);
    return;
  }

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.fd = listen_fd;
  ev.events = EPOLLIN;
  ss = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd, &ev);
  if (ss < 0) {
    LOG(INFO) << "Epoll Add Error: " << strerror(errno);
    return;
  }
}

void BenchClient::StartSendThread() {
  if (is_parallel_) {
    for (const auto& server_info : server_addr_vec_) {
      std::thread t = std::thread(&BenchClient::MainProcess, this, server_addr_vec_);
      send_thread_vec_.push_back(t);
    }
  } else {

  }
}

void BenchClient::WaitSendThread() {
  for (auto& t : send_thread_vec_) {
    t.join();
  }
}

void BenchServer::SplitServerAddress() {
  std::string servers_delimiter{","};
  size_t pos_start = 0, pos_end = 0;
  std::vector<std::string> cur_vec;
  while ((pos_end = server_str_.find(servers_delimiter, pos_start)) != std::string::npos) {
    std::string single_server = server_str_.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + servers_delimiter.size();
    cur_vec.push_back(single_server);
  }
  cur_vec.push_back(server_str_.substr(pos_start));

  for (const auto& single_server : cur_vec) {
    std::string port_delimiter{":"};
    size_t port_start = 0;
    if ((port_start = single_server.find(port_delimiter, port_start)) != std::string::npos) {
      std::string interface = single_server.substr(0, port_start);
      int port = std::stoi(single_server.substr(port_start+1, single_server.size() - port_start));
      server_addr_vec_.push_back(std::make_pair(interface, port));
    }
  }
}

void BenchClient::CalculateRate() {
  std::string unit = send_rate_.substr(send_rate_.size() - 1);
  double value = std::stod(send_rate_.substr(0, send_rate_.size() - 1));
  if (unit == "K") {
    send_rate_bytes_ = static_cast<long>(value);
  } else if (unit == "M") {
    send_rate_bytes_ = static_cast<long>(value * 1024);
  } else {
    send_rate_bytes_ = static_cast<long>(value * 1024 * 1024);
  }
}

