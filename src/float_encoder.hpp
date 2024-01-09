#ifndef __FLOAT_ENCODER_H_INCLUDED__
#define __FLOAT_ENCODER_H_INCLUDED__

#include <cstdint>
#include <vector>

#include "compressed_buffer.hpp"
#include "slice_buffer.hpp"

class FloatEncoder {
 private:
 public:
  FloatEncoder(){};

  static CompressedBuffer encode(const std::vector<double>& values);
  static void decode(CompressedSlice& values, std::vector<double>& out, uint32_t size);
  static uint64_t getUint64Representation(double value);
  static double getDoubleRepresentation(uint64_t intRepresentation);
};

#endif
