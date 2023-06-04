#include "float_encoder.hpp"
#include "util.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

CompressedBuffer FloatEncoder::encode(const std::vector<double> &values) {
  CompressedBuffer buffer;

  uint64_t last_value = *((uint64_t *)&values[0]);
  int data_bits = 0;
  int prev_lzb = -1;
  int prev_tzb = -1;

  buffer.write(last_value, 64);

  for (size_t i = 1; i < values.size(); i++) {
    const uint64_t current_value = *((uint64_t *)&values[i]);
    const uint64_t xor_value = current_value ^ last_value;

    if (xor_value == 0) {
      buffer.writeFixed<0b0, 1>();
    } else {
      int lzb = getLeadingZeroBits(xor_value);
      const int tzb = getTrailingZeroBits(xor_value);

      if (data_bits != 0 && prev_lzb <= lzb && prev_tzb <= tzb) {
        buffer.writeFixed<0b01, 2>();
      } else {
        if (lzb > 31)
          lzb = 31;

        data_bits = 8 * sizeof(uint64_t) - lzb - tzb;

        buffer.writeFixed<0b11, 2>();
        buffer.write<5>(lzb);
        buffer.write<6>(data_bits);

        prev_lzb = lzb;
        prev_tzb = tzb;
      }

      buffer.write(xor_value >> prev_tzb, data_bits);
    }

    last_value = current_value;
  }

  return buffer;
};

void FloatEncoder::decode(CompressedSlice &values, std::vector<double> &out,
                          uint32_t size) {
  uint64_t last_value = values.readFixed<uint64_t, 64>();
  uint64_t tzb = 0;
  uint64_t data_bits = 0;
  double value;

  std::memcpy(&value, &last_value, sizeof(double));
  out.push_back(value);

  while (!values.isAtEnd() && out.size() < size) {
    if (values.readBit()) {
      if (values.readBit()) {
        const auto lzb = values.readFixed<uint64_t, 5>();
        data_bits = values.readFixed<uint64_t, 6>();
        tzb = 64 - lzb - data_bits;
      }

      const uint64_t decoded_value = values.read<uint64_t>(data_bits) << tzb;
      last_value = last_value ^ decoded_value;
    }

    std::memcpy(&value, &last_value, sizeof(double));
    out.push_back(value);
  }
}
