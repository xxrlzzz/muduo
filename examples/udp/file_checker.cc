
#include "examples/udp/file_checker.h"

#include <openssl/sha.h>

string GenSha256(int fd) {
  lseek(fd, 0, SEEK_SET);
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  ssize_t nread, accread = 0;
  char buf[10240];
  while ((nread = read(fd, buf, 10240)) > 0) {
    SHA256_Update(&ctx, data, nread);
  }
  SHA256_Final(buf, &ctx);
  return string(buf, 32);
}