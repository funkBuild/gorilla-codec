#ifndef __INTEGER_ENCODER_H_INCLUDED__
#define __INTEGER_ENCODER_H_INCLUDED__

#include <cstdint>
#include <vector>

#include "aligned_buffer.hpp"
#include "slice_buffer.hpp"

class IntegerEncoder {
private:
public:
  IntegerEncoder();

  static AlignedBuffer encode(const std::vector<uint64_t> &values);
  static void decode(Slice &encoded, std::vector<uint64_t> &values,
                     size_t size);
};

#endif
