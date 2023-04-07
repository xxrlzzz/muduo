#include "examples/udp/udp_conn.h"

#include "muduo/base/Logging.h"
#include "muduo/net/SocketsOps.h"

#include <fcntl.h>
#include <iostream>

using namespace muduo::net;

UdpServer::UdpServer(EventLoop *loop) : loop_(loop) {}

void UdpServer::handleMessage(int sockfd, string msg, struct sockaddr peerAddr,
                              socklen_t addrLen) {
  StringSer ser(msg);
  char op = ser.ReadByte();
  LOG_INFO << "Got op " << static_cast<int>(op) << " ";
  if (op == operation::conn) {
    newClient(sockfd, peerAddr, addrLen);
    return;
  }

  int iconnid = ser.ReadInt32();
  clientData(iconnid, std::move(ser));
}

void UdpServer::newClient(int sockfd, struct sockaddr peerAddr,
                          socklen_t addrLen) {
  int id = cur_id_;
  cur_id_++;
  // TODO: to much client id;

  id2sock_[id] = {peerAddr, addrLen};
  stateMap_[id] = state::idle;
  StringSer ser;
  ser.WriteByte(operation::conn);
  ser.WriteInt32(id);
  string data = ser.FinishBuild();
  ::sendto(sockfd, data.c_str(), data.size(), 0, &peerAddr, addrLen);
  LOG_INFO << "Set connid " << id;
}

void UdpServer::clientData(int connid, StringSer msg) {
  state cstate = stateMap_[connid];
  switch (cstate) {
  case idle:
    handleFileHeader(connid, std::move(msg));
    break;
  case sending:
    auto it = progressMap_.find(connid);
    if (it == progressMap_.end()) {
      LOG_WARN << "Progress for " << connid << " not exist";
      break;
    }
    FileStat &progress = progressMap_[connid];
    if (progress.failed) {
      break;
    }
    handleFileContent(connid, std::move(msg), progress);
    break;
  }
}

void UdpServer::handleFileHeader(int connid, StringSer msg) {
  // filename len, filename
  // filesize
  // sha256

  string file_name = msg.ReadString();
  int file_size = msg.ReadInt32();
  string sha256 = msg.ReadStringWithLen(32);
  stateMap_[connid] = state::sending;
  string out_file_name = "out/" + file_name;
  int fd = open(out_file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, O_RDWR);
  FileStat stat;
  stat.name = file_name;
  stat.tot_len = file_size;
  stat.sha256 = sha256;
  stat.cur_write = 0;
  stat.fd = fd;
  progressMap_[connid] = stat;

  LOG_INFO << "Recving file from " << connid << " " << file_name << " "
           << file_size;
}

void UdpServer::handleFileContent(int connid, StringSer msg,
                                  FileStat &progress) {
  int offset = msg.ReadInt32();
  string content = msg.ReadString();
  if (progress.cur_write != offset) {
    // TODO 解决乱序
  }

  ssize_t nwrite = write(progress.fd, content.c_str(), content.size());
  if (nwrite != static_cast<ssize_t>(content.size())) {
    LOG_WARN << "failed to write " << progress.name;
    progress.failed = true;
  }
  progress.cur_write += static_cast<int>(nwrite);
  if (progress.cur_write == progress.tot_len) {
    string cur_sha = GenSha256(progress.fd);
    if (cur_sha != progress.sha256) {
      LOG_WARN << connid << " " << progress.name << " sha miss match";
    } else {
      LOG_INFO << connid << " finish write";
    }
    progressMap_.erase(it);
    stateMap_[connid] = state::idle;
  }
}