#pragma once
#include "utility.h"

template <typename Stream>
concept STDStyleStream =
    requires(Stream stream, char* buff, size_t size, char c) {
  stream.read(buff, size);
  stream.get(c);
};

template <STDStyleStream Stream, typename T>
size_t read_block(Stream& stream, T* buffer, size_t size) {
  stream.read((char*)buffer, size);
  return size;
}

template <STDStyleStream Stream, typename T>
size_t read_vector(Stream& stream, std ::vector<T>& v) {
  return read_block(stream, v.data(), vectorsize(v));
}

template <STDStyleStream Stream, typename T>
size_t read_value(Stream& stream, T& value) {
  stream.read((char*)&value, sizeof(value));
  return sizeof(value);
}

template <STDStyleStream Stream>
size_t read_string(Stream& stream, std::string& str) {
  str.clear();
  const uint8_t MAX_SIZE = 128;
  uint8_t length[2] = {0};
  read_value(stream, length[0]);
  if (length[0] >= MAX_SIZE)
    read_value(stream, length[1]);
  const auto len = (length[0] % MAX_SIZE) + length[1] * MAX_SIZE;
  if (len > 0) {
    str.resize(len);
    stream.read(str.data(), len);
  }
  return str.size();
}

template <typename T>
concept PxrVector = requires(T v, size_t index) {
  std::is_reference_v<decltype(v[index])>;
  std::is_same_v<decltype(T::dimension), const size_t>;
};

template <STDStyleStream Stream, PxrVector V>
size_t read_vector(Stream& stream, V& v) {
  size_t read = 0;
  for (size_t i = 0; i < V::dimension; ++i) {
    read += read_value(stream, v[i]);
  }
  return read;
}

template <typename Stream>
requires STDStyleStream<Stream> size_t read_line(Stream& stream,
                                                 std::string& str) {
  str.clear();
  while (true) {
    char c = 0;
    stream.get(c);
    if (c == EOF) {
      break;
    }
    str += c;
    if (c == '\n') {
      break;
    }
  }
  return str.size();
}
