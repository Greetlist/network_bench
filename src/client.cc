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
  LOG(INFO) << "Start As Client Mode";
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
  receive_thread_ = std::thread(&BenchClient::ReceiveProcess, this);
  CreateSendThread();
  WaitSendThread();
  LOG(INFO) << "Stop Send Threads";
  is_stop_ = true;
  receive_thread_.join();
}

void BenchClient::ReceiveProcess() {
  struct epoll_event events[1024];
  while (!is_stop_) {
    int nums = epoll_wait(epoll_fd_, events, 1024, 1000);
    for (int i = 0; i < nums; ++i) {
      BenchStatistic* s = static_cast<BenchStatistic*>(events[i].data.ptr);
      char buf[BUFF_SIZE * 2];
      struct iovec iov[2];
      iov[0].iov_base = buf;
      iov[0].iov_len = BUFF_SIZE;
      iov[1].iov_base = buf + BUFF_SIZE;
      iov[1].iov_len = BUFF_SIZE;
      auto start_time = std::chrono::system_clock::now();
      int n_read = readv(s->GetSocket(), iov, 2);
      auto end_time = std::chrono::system_clock::now();
      auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

      s->GetTotalReceivedTime() += duration_ms;
      s->GetTotalReceivedBytes() += n_read;
    }
  }
}

void BenchClient::SendProcess(std::unique_ptr<BenchStatistic>&& s) {
  int second_count = 0;
  long current_second_send_bytes = 0;
  auto per_second_start_time = std::chrono::system_clock::now();
  while (true) {
    char send_buf[BUFF_SIZE * 2];
    struct iovec iov[2];
    iov[0].iov_base = send_buf;
    iov[0].iov_len = BUFF_SIZE;
    iov[1].iov_base = send_buf + BUFF_SIZE;
    iov[1].iov_len = BUFF_SIZE;
    int n_write = writev(s->GetSocket(), iov, 2);

    //计算当前秒已经发送的数据,大于发送速率就sleep
    current_second_send_bytes += n_write;
    s->GetTotalSendBytes() += n_write;
    auto time_elapse = std::chrono::system_clock::now() - per_second_start_time;
    auto milli_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_elapse).count();
    if (milli_seconds <= 1000 && current_second_send_bytes >= send_rate_bytes_) {
      LOG(INFO) << "Reach Send Rate Limit, Sleep for a while.";
      std::this_thread::sleep_for(std::chrono::milliseconds(1000 - milli_seconds));
    }
    LOG(INFO) << "Current Sended Bytes: " << current_second_send_bytes << ", Send Rate: " << current_second_send_bytes / 1024.0 / 1024.0 << " MB/s";

    //如果发送已经超了一秒，重置当前秒的计数器
    auto current_elapse = \
      std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - per_second_start_time
      ).count();
    if (current_elapse >= 1000) {
      s->GetTotalSendTime() += milli_seconds;
      second_count += 1;
      current_second_send_bytes = 0;
      per_second_start_time = std::chrono::system_clock::now();
    }

    if (second_count >= send_duration_) {
      break;
    }
  }
  close(s->GetSocket());
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, s->GetSocket(), NULL);
}

std::unique_ptr<BenchStatistic> BenchClient::Connect(const std::pair<std::string, int>& server_info) {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    LOG(INFO) << "Create Socket Error: " << strerror(errno);
    return nullptr;
  }
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  inet_pton(AF_INET, server_info.first.c_str(), &addr.sin_addr);
  addr.sin_port = htons(server_info.second);
  int connect_status = connect(socket_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
  if (connect_status < 0) {
    LOG(ERROR) << "Connect to [" << server_info.first << ":" << server_info.second << "] Error:" << strerror(errno);
    return nullptr;
  }
  set_nonblock(socket_fd);

  struct epoll_event ev;
  std::unique_ptr<BenchStatistic> return_ptr = std::make_unique<BenchStatistic>(socket_fd);
  memset(&ev, 0, sizeof(ev));
  BenchStatistic* s = return_ptr.get();
  ev.data.ptr = static_cast<void*>(s);
  ev.events = EPOLLIN;
  int ss = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_fd, &ev);
  if (ss < 0) {
    LOG(INFO) << "Epoll Add Error: " << strerror(errno);
    return nullptr;
  }
  return return_ptr;
}

void BenchClient::CreateSendThread() {
  for (const auto& server_info : server_addr_vec_) {
    std::unique_ptr<BenchStatistic> s = Connect(server_info);
    if (s == nullptr) {
      return;
    }
    if (is_parallel_) {
      std::thread t = std::thread(&BenchClient::SendProcess, this, std::move(s));
      send_thread_vec_.emplace_back(std::move(t));
    } else {
      SendProcess(std::move(s));
    }
  }
}

void BenchClient::WaitSendThread() {
  for (auto& t : send_thread_vec_) {
    t.join();
  }
}

void BenchClient::SplitServerAddress() {
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
    send_rate_bytes_ = static_cast<long>(value) * 1024;
  } else if (unit == "M") {
    send_rate_bytes_ = static_cast<long>(value * 1024 * 1024);
  } else {
    send_rate_bytes_ = static_cast<long>(value * 1024 * 1024 * 1024);
  }
  LOG(INFO) << "Send To Server With Rate: " << send_rate_bytes_ << "B/s";
}

