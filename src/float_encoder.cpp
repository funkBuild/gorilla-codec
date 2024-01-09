#include "float_encoder.hpp"
#include "util.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

CompressedBuffer FloatEncoder::encode(const std::vector<double>& values) {
  CompressedBuffer buffer;

  uint64_t last_value = getUint64Representation(values[0]);
  int data_bits = 0;
  int prev_lzb = -1;
  int prev_tzb = -1;

  buffer.write(last_value, 64);

  for (size_t i = 1; i < values.size(); i++) {
    const uint64_t current_value = getUint64Representation(values[i]);
    const uint64_t xor_value = current_value ^ last_value;

    if (xor_value == 0) {
      buffer.writeFixed<0b0, 1>();
    } else {
      int lzb = getLeadingZeroBits(xor_value);
      const int tzb = getTrailingZeroBits(xor_value);

      if (data_bits != 0 && prev_lzb <= lzb && prev_tzb <= tzb) {
        buffer.writeFixed<0b01, 2>();
      } else {
        if (lzb > 31) lzb = 31;

        data_bits = 8 * sizeof(uint64_t) - lzb - tzb;

        buffer.writeFixed<0b11, 2>();
        buffer.write<5>(lzb);
        buffer.write<6>(data_bits != 64 ? data_bits : 0);

        prev_lzb = lzb;
        prev_tzb = tzb;
      }

      buffer.write(xor_value >> prev_tzb, data_bits);
    }

    last_value = current_value;
  }

  return buffer;
}

void FloatEncoder::decode(CompressedSlice& values, std::vector<double>& out, uint32_t size) {
  uint64_t last_value = values.readFixed<uint64_t, 64>();
  int tzb = 0;
  int data_bits = 0;
  double value;

  value = getDoubleRepresentation(last_value);
  out.push_back(value);

  while (!values.isAtEnd() && out.size() < size) {
    if (values.readBit()) {
      if (values.readBit()) {
        const int lzb = values.readFixed<uint64_t, 5>();
        data_bits = values.readFixed<uint64_t, 6>();

        if (data_bits == 0) {
          data_bits = 64;
        }

        tzb = 64 - lzb - data_bits;
      }

      const uint64_t decoded_value = values.read<uint64_t>(data_bits) << tzb;
      last_value = last_value ^ decoded_value;
    }

    value = getDoubleRepresentation(last_value);
    out.push_back(value);
  }
}

// Helper function to convert uint64_t back to double
double FloatEncoder::getDoubleRepresentation(uint64_t intRepresentation) {
  double doubleValue;
  std::memcpy(&doubleValue, &intRepresentation, sizeof(uint64_t));
  return doubleValue;
}

uint64_t FloatEncoder::getUint64Representation(double value) {
  uint64_t intRepresentation;
  std::memcpy(&intRepresentation, &value, sizeof(double));
  return intRepresentation;
}