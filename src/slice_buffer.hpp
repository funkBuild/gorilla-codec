#ifndef __SLICE_BUFFER_H_INCLUDED__
#define __SLICE_BUFFER_H_INCLUDED__

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

class CompressedSlice {
private:
  size_t length_ = 0;
  size_t offset = 0;
  int bitOffset = 0;

public:
  const uint64_t *data = nullptr;

  CompressedSlice(const uint8_t *_data, size_t _length)
      : length_(_length / 8), data((uint64_t *)_data){};

  template <typename T> T read(const int bits) {
    if (bitOffset > 63) {
      offset++;
      bitOffset = 0;

      if (offset >= length_) {
        throw std::runtime_error("Out of bounds");
      }
    }

    const int leftover_bits = bits - (64 - bitOffset);
    const int bits_read = leftover_bits > 0 ? bits - leftover_bits : bits;
    const uint64_t mask =
        bits_read == 64 ? 0xffffffffffffffff : (1ull << bits_read) - 1;

    uint64_t value = data[offset] >> bitOffset;
    value &= mask;

    if (leftover_bits > 0) {
      offset++;
      bitOffset = leftover_bits;

      const uint64_t mask = leftover_bits == 64 ? 0xffffffffffffffff
                                                : (1ull << leftover_bits) - 1;
      value |= (data[offset] & mask) << bits_read;
    } else {
      bitOffset += bits;
    }

    return value;
  };

  template <typename T, int bits> T readFixed() {
    if (bitOffset > 63) {
      offset++;
      bitOffset = 0;

      if (offset >= length_) {
        throw std::runtime_error("Out of bounds");
      }
    }

    const int leftover_bits = bits - (64 - bitOffset);
    const int bits_read = leftover_bits > 0 ? bits - leftover_bits : bits;
    const uint64_t mask =
        bits_read == 64 ? 0xffffffffffffffff : (1ull << bits_read) - 1;

    uint64_t value = data[offset] >> bitOffset;
    value &= mask;

    if (leftover_bits > 0) {
      offset++;
      bitOffset = leftover_bits;

      const uint64_t mask = leftover_bits == 64 ? 0xffffffffffffffff
                                                : (1ull << leftover_bits) - 1;
      value |= (data[offset] & mask) << bits_read;
    } else {
      bitOffset += bits;
    }

    return value;
  };

  bool readBit() {
    if (bitOffset > 63) {
      offset++;
      bitOffset = 1;

      if (offset >= length_) {
        throw std::runtime_error("Out of bounds");
      }

      return (data[offset] & 1) == 1;
    }

    bool value = ((data[offset] >> bitOffset) & 1) == 1;
    bitOffset++;

    return value;
  }

  bool isAtEnd() {
    return offset >= length_ || (offset == length_ - 1 && bitOffset > 63);
  }
};

class Slice {
public:
  const uint8_t *data;
  size_t length_;
  size_t offset = 0;

  Slice(const uint8_t *_data, size_t _length) : data(_data), length_(_length){};

  template <class T> T read() {
    T value;

    std::memcpy(&value, data + offset, sizeof(T));
    offset += sizeof(T);

    return value;
  }

  template <class T> size_t length() { return length_ / sizeof(T); }

  std::string readString(size_t byteLength) {
    std::string thisString((char *)(data + offset), byteLength);

    offset += byteLength;

    return thisString;
  }

  size_t bytesLeft() { return length_ - offset; }

  Slice getSlice(size_t byteLength) {
    const uint8_t *ref = data + offset;
    Slice slice(ref, byteLength);

    offset += byteLength;

    return slice;
  }

  CompressedSlice getCompressedSlice(size_t byteLength) {
    const uint8_t *ref = data + offset;
    CompressedSlice slice(ref, byteLength);

    offset += byteLength;

    return slice;
  }

  template <class T> T read(size_t offset) {
    T value;

    std::memcpy(&value, data + offset, sizeof(T));
    offset += sizeof(T);

    return value;
  }
};

// TODO: Remove this in favour of a Slice.
/*
class SliceBuffer {
private:

public:
  const uint8_t *data;
  size_t length;
  size_t offset = 0; // TODO: Not used?

  SliceBuffer(const uint8_t *_data, size_t _length) : data(_data),
length(_length) {};

  Slice getSlice(size_t offset, size_t byteLength){
    const uint8_t* ref = data + offset;
    Slice slice(ref, byteLength);

    return std::move(slice);
  }

  CompressedSlice getCompressedSlice(size_t offset, size_t byteLength){
    const uint8_t* ref = data + offset;
    CompressedSlice slice(ref, byteLength);

    return std::move(slice);
  }

  template <class T>
  T read(size_t offset){
    T value;

    std::memcpy(&value, data + offset, sizeof(T));
    offset += sizeof(T);

    return value;
  }
};
*/
#endif
