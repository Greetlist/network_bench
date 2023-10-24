#include <iostream>
#include <gflags/gflags.h>
#include <csignal>
#include <functional>

#include "server.h"
#include "client.h"

DEFINE_string(listen_addr, "", "Run As Serer, server listen addresses");
DEFINE_string(server_addr, "", "Run As Client, server address connect to");
DEFINE_string(send_rate, "1M", "Exp: 1024K/10M/1G [Unit: (B/s)]");
DEFINE_int32(send_duration, 10, "client send duration [Unit: (second)]");

using namespace std;

int main(int argc, char** argv)
{
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_listen_addr != "" && FLAGS_server_addr != "") {
    std::cout << "Cannot Run As Server and Client at same time. Quit" << std::endl;
    exit(1);
  }
  if (FLAGS_listen_addr != "") {
    BenchServer* server = new BenchServer(FLAGS_listen_addr);
    server->Init();
    server->Start();
    return 0;
  }
  if (FLAGS_server_addr != "") {
    BenchClient* client = new BenchClient(FLAGS_server_addr, FLAGS_send_rate, FLAGS_send_duration);
    client->CheckInput();
    client->Init();
    client->Start();
  }
  return 0;
}
