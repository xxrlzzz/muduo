
#include <condition_variable>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unistd.h>

#include "examples/udp/string_ser.h"
#include "examples/udp/udp_conn.h"
#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/Socket.h"
#include "muduo/net/SocketsOps.h"
#include "muduo/net/TcpClient.h"
#include "muduo/net/TcpServer.h"

using namespace muduo;
using namespace muduo::net;
using std::string;

using SendCallback = std::function<void(string)>;

UdpClient client;

std::condition_variable cv;
std::mutex mu;

void clientReadCallback(int sockfd, muduo::Timestamp receiveTime) {
  char buf[1024];
  ssize_t n = sockets::read(sockfd, buf, 1024);
  if (n == -1) {
    LOG_WARN << "failed to read " << errno;
  } else {
    LOG_INFO << "Read " << n;
    StringSer ser(string(buf, n));
    operation op = static_cast<operation>(ser.ReadByte());
    int connid;
    string msg;
    switch (op) {
    case conn:
      connid = ser.ReadInt32();
      client.set_connid(connid);
      cv.notify_one();
      LOG_INFO << "recved connid " << connid << '\n';
      break;
    case data:
      msg = ser.ReadString();
      LOG_INFO << "recved msg " << msg << '\n';
      break;
    default:
      break;
    }
  }
}

void sendFile(int sockfd, string filename) {
  int fd = open(filename.c_str(), O_RDONLY);
  if (fd == -1) {
    LOG_WARN << "failed to open " << filename;
    return;
  }
  off_t end = lseek(fd, 0, SEEK_END);
  string sha256 = GenSha256();
  lseek(fd, 0, SEEK_SET);
  StringSer ser;
  LOG_INFO << "sending file " << filename << " " << fd << " " << end;

  sha256.resize(32);
  ser.WriteByte(operation::data);
  ser.WriteInt32(client.connid());
  ser.WriteString(filename);
  ser.WriteInt32(static_cast<int>(end));
  ser.WriteStringWithoutLen(sha256);
  string msg = ser.FinishBuild();
  sockets::write(sockfd, msg.c_str(), msg.size());

  // TODO avoid copy
  ssize_t nread, accread = 0;
  char buf[10240];
  while ((nread = read(fd, buf, 10240)) > 0) {
    msg = string(buf);
    ser.WriteByte(operation::data);
    ser.WriteInt32(client.connid());
    ser.WriteInt32(accread);
    ser.WriteString(msg);
    msg = ser.FinishBuild();
    sockets::write(sockfd, msg.c_str(), msg.size());
    accread += nread;
  }
  LOG_INFO << "sending file end " << filename;
}

void sendData(int sockfd, string msg) {
  LOG_INFO << "Get " << msg;
  if (msg.substr(0, 4) == "send" && msg.size() > 4) {
    sendFile(sockfd, msg.substr(4));
    return;
  }
}

void inputThread(SendCallback sc) {
  string input;
  std::unique_lock<std::mutex> lk(mu);
  cv.wait(lk);
  std::cout << "Ready for read input: ";
  while (std::cin >> input) {
    sc(input);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage:\n%s ip port\n", argv[0]);
    return -1;
  }
  uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
  const char *ip = argv[1];
  InetAddress serverAddr(ip, port);
  Socket sock(sockets::createUdpNonblockingOrDie());

  int ret = sockets::connect(sock.fd(), serverAddr.getSockAddr());
  if (ret < 0) {
    LOG_SYSFATAL << "::connect";
  }

  EventLoop loop;
  Channel channel(&loop, sock.fd());
  channel.setReadCallback(std::bind(&clientReadCallback, sock.fd(), _1));
  channel.enableReading();

  std::thread thread(inputThread, [&](string msg) {
    loop.queueInLoop(std::bind(sendData, sock.fd(), msg));
  });
  loop.queueInLoop([&]() {
    // connect with server.
    char op = operation::conn;
    sockets::write(sock.fd(), &op, 1);
  });
  loop.loop();
  thread.join();
  return 0;
}