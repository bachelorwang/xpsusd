#pragma once
#include "utility.h"
class XpsReaderImp;
struct XpsModel;

class XpsReader {
 public:
  XpsReader();
  virtual ~XpsReader() = default;

  Ptr<XpsModel> readBinaryXps(const char* file_path);

 private:
  Ptr<XpsReaderImp> _pimp;
};
