#include "server.h"

BenchServer::BenchServer(const std::string& server_str, int read_block_size) : server_str_(server_str), read_block_size_(read_block_size) {
}

void BenchServer::Init() {
  SplitServerAddress();
  InitEpoll();
  InitListenSockets();
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

void BenchServer::InitEpoll() {
  epoll_fd_ = epoll_create1(0);
  if (epoll_fd_ <= 0) {
    LOG(INFO) << "Epoll Create Error: " << strerror(errno);
    exit(1);
  }
}

void BenchServer::InitListenSockets() {
  LOG(INFO) << "Start As Server Mode";
  for (const auto& server_info : server_addr_vec_) {
    InitSingleListenSocket(server_info);
    LOG(INFO) << "Listen On: " << server_info.first << ":" << server_info.second;
  }
}

bool BenchServer::InitSingleListenSocket(const std::pair<std::string, int>& server_info) {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    LOG(INFO) << "Socket Error: " << strerror(errno);
    return false;
  }

  struct sockaddr_in server;
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(server_info.second);
  server.sin_addr.s_addr = inet_addr(server_info.first.c_str());

  int flag = 1;
  int l = sizeof(flag);
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &flag, l);
  int ss = bind(listen_fd, (struct sockaddr*)&server, sizeof(server));
  if (ss < 0) {
    LOG(INFO) << "Bind Error";
    close(listen_fd);
    return false;
  }
  ss = set_nonblock(listen_fd);
  if (ss < 0) {
    LOG(INFO) << "SetNonblock Error";
    close(listen_fd);
    return false;
  }
  ss = listen(listen_fd, 128);
  if (ss < 0) {
    LOG(INFO) << "Listen Error";
    close(listen_fd);
    return false;
  }

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.fd = listen_fd;
  ev.events = EPOLLIN | EPOLLET;
  ss = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd, &ev);
  if (ss < 0) {
    LOG(INFO) << "Epoll Add Error: " << strerror(errno);
    return false;
  }
  listen_fd_vec_.push_back(listen_fd);
  return true;
}

void BenchServer::Start() {
  struct epoll_event events[1024];
  while (!stop_) {
    int nums = epoll_wait(epoll_fd_, events, 1024, -1);
    for (int i = 0; i < nums; ++i) {
      BenchStatistic* s = static_cast<BenchStatistic*>(events[i].data.ptr);
      if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        LOG(ERROR) << "Client Quit With Error Event: [" << events[i].events << "], error is: " << strerror(errno);
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, s->GetSocket(), NULL);
        close(s->GetSocket());
        delete s;
        continue;
      }
      bool is_listen_fd = false;
      for (const auto& fd : listen_fd_vec_) {
        if (events[i].data.fd == fd) {
          is_listen_fd = true;
          break;
        }
      }
      if (is_listen_fd) {
        AcceptClient(events[i].data.fd);
        continue;
      }
      if (s->GetClientSendRate() == -1) {
        ReadClientSendRate(&events[i]);
      }
      int n_read = ReceiveBytes(&events[i]);
      if (n_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        continue;
      }
      s->RecordCurrentSecondRate(Direction::Receive, n_read);
      if (n_read == 0) {
        LOG(INFO) << "Client Normal Quit";
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, s->GetSocket(), NULL);
        close(s->GetSocket());
        delete s;
      }
    }
  }
}

int BenchServer::AcceptClient(int listen_fd) {
  int client_fd = 0;
  struct sockaddr_in cli;
  socklen_t len = sizeof(struct sockaddr_in);
  while ((client_fd = accept(listen_fd, (struct sockaddr*)&cli, &len)) > 0) {
    set_nonblock(client_fd);

    struct epoll_event new_ev;
    memset(&new_ev, 0, sizeof(new_ev));
    BenchStatistic* s = new BenchStatistic(client_fd);
    new_ev.data.ptr = static_cast<void*>(s);
    new_ev.events = EPOLLIN; //default with LT mode
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &new_ev) < 0) {
      LOG(INFO) << "Accept Error, error is : " << strerror(errno);
      return -1;
    }
  }
  return 0;
}

void BenchServer::ReadClientSendRate(struct epoll_event* event) {
  int read_bytes = 0;
  BenchStatistic* s = static_cast<BenchStatistic*>(event->data.ptr);
  uint32_t rate_str_len_network_byte;
  read_bytes = read(s->GetSocket(), &rate_str_len_network_byte, sizeof(uint32_t));
  uint32_t rate_str_len = ntohl(rate_str_len_network_byte);
  char buf[rate_str_len];
  read_bytes += read(s->GetSocket(), buf, rate_str_len);
  s->GetClientSendRate() = std::stol(std::string(buf));
  LOG(INFO)
    << "Read Rate info Bytes count: " << read_bytes
    << ", Client Send Rate is: " << s->GetClientSendRate() / 1024.0 / 1024.0 << "MB/s.";
}

int BenchServer::ReceiveBytes(struct epoll_event* event) {
  BenchStatistic* s = static_cast<BenchStatistic*>(event->data.ptr);
  char buf[read_block_size_];
  struct iovec iov;
  iov.iov_base = buf;
  iov.iov_len = read_block_size_;
  int n_read = readv(s->GetSocket(), &iov, 1);
  if (n_read < 0) {
    return n_read;
  }
  //LOG(INFO) << "Read " << n_read << " Bytes";
  s->GetTotalReceivedBytes() += n_read;

  char send_buf[n_read];
  int left_data_len = n_read;
  int total_write_bytes = 0;
  while (left_data_len > 0) {
    struct iovec iov;
    iov.iov_base = send_buf + n_read - left_data_len;
    iov.iov_len = left_data_len;
    int n_write = writev(s->GetSocket(), &iov, 1);
    if (n_write < 0) {
      continue;
    }
    left_data_len -= n_write;
    total_write_bytes += n_write;
  }
  s->GetTotalSendBytes() += total_write_bytes;
  s->RecordCurrentSecondRate(Direction::Send, total_write_bytes);
  //LOG(INFO) << "Total Send Bytes is: " << s->GetTotalSendBytes();
  return n_read;
}

