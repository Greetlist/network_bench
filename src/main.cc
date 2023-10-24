#include <iostream>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <csignal>
#include <functional>

#include "server.h"
#include "client.h"

DEFINE_string(listen_addr, "", "server listen addresses");
DEFINE_string(server_addr, "", "client connect to server address");
DEFINE_string(send_rate, "1M", "Exp: 1024K/10M/1G (B/s)");
DEFINE_bool(, false, "");

using namespace std;

int main(int argc, char** argv)
{
  if (FLAGS_listen_addr != "" && FLAGS_server_addr != "") {
    LOG(ERROR) << "Cannot Run As Server and Client at same time. Quit";
    exit(1)
  }
  if (FLAGS_listen_addr != "") {
    BenchServer* server = new BenchServer(FLAGS_listen_addr);
    server->Init();
    server->Start();
    return 0;
  }
  if (FLAGS_server_addr != "") {
    BenchClient* client = new BenchClient(FLAGS_server_addr);
    client->Init();
    client->Start();
  }
  return 0;
}
