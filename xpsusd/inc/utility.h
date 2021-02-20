#pragma once
#include <memory>
#include <vector>

enum Result { SUCCESS = 0, EXTERNAL_ERROR, IO_ERROR, INVALID_DATA };

template <typename T>
using Ptr = std::shared_ptr<T>;

template <typename T>
size_t vectorsize(const std::vector<T>& v) {
  return v.size() * sizeof(std::vector<T>::value_type);
}
