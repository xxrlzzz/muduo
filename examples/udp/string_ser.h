#ifndef EXAMPLE_UDP_STRING_SER_H
#define EXAMPLE_UDP_STRING_SER_H

#include <string>

using std::string;

class StringSer {
public:
  explicit StringSer(string data);
  StringSer() = default;

  char ReadByte();
  void WriteByte(char data);
  // 读写整数
  int ReadInt32();
  void WriteInt32(int data);
  // 读写 string,  length + string
  string ReadString();
  void WriteString(const string &str);
  // 以固定长度读写 string
  string ReadStringWithLen(size_t len);
  void WriteStringWithoutLen(const string &str);

  string FinishBuild();

private:
  const char *CurOffset(int offset = 0) const;
  string data_;
  size_t read_offset_ = 0;
};

#endif // EXAMPLE_UDP_STRING_SER_H