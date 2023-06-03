#ifndef __ALIGNED_BUFFER_H_INCLUDED__
#define __ALIGNED_BUFFER_H_INCLUDED__

#include <cstdint>
#include <iostream>
#include <vector>

#include "compressed_buffer.hpp"

class AlignedBuffer {
private:
public:
  std::vector<uint8_t> data;

  AlignedBuffer(unsigned int initialSize = 0) {
    if (initialSize > 0)
      data.resize(initialSize);
  };

  template <class T> void write(T value);
  void write(std::string &value);
  void write(CompressedBuffer &value);
  void write(AlignedBuffer &value);

  uint64_t read64(unsigned int offset) {
    uint64_t *data64 = (uint64_t *)&data[0] + offset;
    return *data64;
  }

  uint64_t read8(unsigned int offset) {
    uint8_t *data8 = &data[0] + offset;
    return *data8;
  }

  size_t size() { return data.size(); }
};

std::ofstream &operator<<(std::ofstream &os, const AlignedBuffer &buf);

#endif
