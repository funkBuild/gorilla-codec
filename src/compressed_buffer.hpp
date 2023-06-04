#ifndef __COMPRESSED_BUFFER_H_INCLUDED__
#define __COMPRESSED_BUFFER_H_INCLUDED__

#include <cstdint>
#include <iostream>
#include <vector>

class CompressedBuffer {
private:
  size_t offset = 0;
  int bitOffset = 0;

public:
  std::vector<uint64_t> data;

  CompressedBuffer(unsigned int initialSize = 0) {
    if (initialSize > 0)
      data.resize(initialSize);
  };

  void write(uint64_t value, int bits);
  template <int bits> void write(uint64_t value);
  template <uint64_t value, int bits> void writeFixed();
  void rewind() {
    offset = 0;
    bitOffset = 0;
  };
  template <typename T> T read(int bits);
  template <typename T, int bits> T readFixed();
  bool isAtEnd() { return offset >= data.size(); };
  size_t size() { return dataByteSize(); };
  bool readBit();
  size_t dataByteSize() { return data.size() * sizeof(uint64_t); }

  void printOffsets() {
    std::cout << "buf offset=" << std::dec << offset
              << " bitOffset=" << bitOffset << std::endl;
  }

  void printCurrentValue() {
    std::cout << "buf value=" << std::hex << data[offset] << std::endl;
  }
};

#endif
