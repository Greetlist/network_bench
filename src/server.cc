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

  bool flag = true;
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
        LOG(ERROR) << "Client Quit With Error Event: [" << events[i].events << "]";
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, s->GetSocket(), NULL);
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
      int n_read = ReceiveBytes(&events[i]);
      if (n_read == 0) {
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, s->GetSocket(), NULL);
        LOG(INFO) << "Client Normal Quit";
      } else {
        LOG(INFO) << "Receive " << s->GetTotalReceivedBytes();
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

int BenchServer::ReceiveBytes(struct epoll_event* event) {
  BenchStatistic* s = static_cast<BenchStatistic*>(event->data.ptr);
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

  char send_buf[BUFF_SIZE * 2];
  struct iovec write_iov[2];
  write_iov[0].iov_base = send_buf;
  write_iov[0].iov_len = BUFF_SIZE;
  write_iov[1].iov_base = send_buf + BUFF_SIZE;
  write_iov[1].iov_len = BUFF_SIZE;
  int n_write = writev(s->GetSocket(), write_iov, 2);
  s->GetTotalSendTime() += duration_ms;
  s->GetTotalSendBytes() += n_write;
  return n_read;
}

