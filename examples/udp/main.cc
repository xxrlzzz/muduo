
#include <unistd.h>

#include "examples/udp/string_ser.h"
#include "examples/udp/udp_conn.h"
#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/Socket.h"
#include "muduo/net/SocketsOps.h"
#include "muduo/net/TcpServer.h"

using namespace muduo;
using namespace muduo::net;

void serverReadCallback(UdpServer *server, int sockfd,
                        muduo::Timestamp receiveTime) {

  struct sockaddr peerAddr;
  memZero(&peerAddr, sizeof peerAddr);
  socklen_t addrLen = sizeof peerAddr;
  char buf[1024];
  while (true) {
    ssize_t n = ::recvfrom(sockfd, buf, 1024, 0, &peerAddr, &addrLen);
    if (n > 0) {
      LOG_INFO << "recved " << n << '\n';
      // ::sendto(sockfd, buf, n, 0, &peerAddr, addrLen);
      server->handleMessage(sockfd, string(buf, n), peerAddr, addrLen);
    } else {
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage:\n%s port", argv[0]);
    return 0;
  }
  uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
  InetAddress listenAddr(port);
  Socket sock(sockets::createUdpNonblockingOrDie());
  sock.bindAddress(listenAddr);

  EventLoop loop;
  UdpServer server(&loop);
  Channel channel(&loop, sock.fd());
  channel.setReadCallback(
      std::bind(&serverReadCallback, &server, sock.fd(), _1));
  channel.enableReading();
  loop.loop();

  return 0;
}