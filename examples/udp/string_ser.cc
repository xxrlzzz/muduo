
#include "examples/udp/string_ser.h"

#include <cstring>

StringSer::StringSer(string data) : data_(data) {}

const char *StringSer::CurOffset(int offset) const {
  return data_.c_str() + read_offset_ + offset;
}
char StringSer::ReadByte() {
  char ret = data_[read_offset_];
  read_offset_++;
  return ret;
}

void StringSer::WriteByte(char data) {
  data_ += data;
}

int StringSer::ReadInt32() {
  char buf[8];
  memcpy(buf, CurOffset(), 8);
  int ret = static_cast<int>(strtol(buf, nullptr, 16));
  read_offset_ += 8;
  return ret;
}

void StringSer::WriteInt32(int data) {
  char cdata[9];
  sprintf(cdata, "%X", data);
  for (int i = 0; i < 8; ++i) {
    data_.push_back(cdata[i]);
  }
}

string StringSer::ReadString() {
  int len = ReadInt32();
  string ret = data_.substr(read_offset_, len);
  read_offset_ += len;
  return ret;
}

void StringSer::WriteString(const string &str) {
  WriteInt32(static_cast<int>(str.size()));
  data_ += str;
}

string StringSer::ReadStringWithLen(size_t len) {
  string ret = data_.substr(read_offset_, len);
  read_offset_ += len;
  return ret;
}

void StringSer::WriteStringWithoutLen(const string &str) { data_ += str; }

string StringSer::FinishBuild() {
  string ret = std::move(data_);
  read_offset_ = 0;
  return ret;
}

// test code
bool testSer() {
  StringSer ser;
  ser.WriteInt32(3);
  ser.WriteInt32(0x3f3f3f3f);
  string str = "hahaha";
  ser.WriteString(str);
  ser.WriteStringWithoutLen(str);
  string res = ser.FinishBuild();
  StringSer deser(res);
  int i = deser.ReadInt32();
  if (i != 3) {
    return false;
  }
  i = deser.ReadInt32();
  if (i != 0x3f3f3f3f) {
    return false;
  }
  str = deser.ReadString();
  if (str != "hahaha") {
    return false;
  }
  str = deser.ReadStringWithLen(6);

  if (str != "hahaha") {
    return false;
  }
  return true;
}
