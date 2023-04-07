#ifndef EXAMPLE_UDP_UDP_CONN_H
#define EXAMPLE_UDP_UDP_CONN_H

#include <map>

#include "examples/udp/string_ser.h"
#include "muduo/net/EventLoop.h"

using namespace muduo::net;

enum operation { conn, data };

enum state {
  idle,
  // headerRecved,
  sending
};

struct FileStat {
  string name;
  int tot_len;
  string sha256;
  int cur_write;
  int fd;
  bool failed = false;
};

class UdpServer {
public:
  void handleMessage(int sockfd, string msg, struct sockaddr peerAddr,
                     socklen_t addrLen);
  explicit UdpServer(EventLoop *loop);

private:
  void newClient(int sockfd, struct sockaddr peerAddr, socklen_t addrLen);
  void clientData(int connid, StringSer msg);
  void handleFileHeader(int connid, StringSer msg, FileStat &progress);
  void handleFileContent(int connid, StringSer msg);
  int cur_id_ = 0;
  std::map<int, std::pair<struct sockaddr, socklen_t>> id2sock_;
  std::map<int, state> stateMap_;
  std::map<int, FileStat> progressMap_;

  // TODO use eventloop;
  EventLoop *loop_;
};

class UdpClient {
public:
  int connid() const { return connid_; }
  void set_connid(int connid) { connid_ = connid; }

public:
  int connid_;
};

#endif // EXAMPLE_UDP_UDP_CONN_H
