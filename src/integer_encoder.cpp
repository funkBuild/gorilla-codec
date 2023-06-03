#include "integer_encoder.hpp"
#include "simple8b.hpp"
#include "slice_buffer.hpp"
#include "zigzag.hpp"

#include <iostream>
#include <limits>

// Timestamp encoding - http://www.vldb.org/pvldb/vol8/p1816-teller.pdf

AlignedBuffer IntegerEncoder::encode(const std::vector<uint64_t> &values) {
  std::vector<uint64_t> encoded;
  encoded.reserve(values.size());

  uint64_t start_value = values[0];
  encoded.push_back(start_value);

  int64_t delta = values[1] - values[0];
  uint64_t first_delta = ZigZag::zigzagEncode(delta);

  encoded.push_back(first_delta);

  for (unsigned int i = 2; i < values.size(); i++) {
    int64_t D = (values[i] - values[i - 1]) - (values[i - 1] - values[i - 2]);
    uint64_t encD = ZigZag::zigzagEncode(D);
    encoded.push_back(encD);
  }

  return Simple8B::encode(encoded);
}

void IntegerEncoder::decode(Slice &encoded, std::vector<uint64_t> &values,
                            size_t size) {
  if (size == 0)
    return;

  std::vector<uint64_t> deltaValues = Simple8B::decode(encoded);

  uint64_t last_decoded = deltaValues[0];

  values.push_back(last_decoded);

  if (size == 1)
    return;

  int64_t delta = ZigZag::zigzagDecode(deltaValues[1]);
  last_decoded += delta;

  values.push_back(last_decoded);

  if (size == 2)
    return;

  for (unsigned int i = 2; i < deltaValues.size(); i++) {
    int64_t encD = ZigZag::zigzagDecode(deltaValues[i]);

    delta += encD;
    last_decoded += delta;

    values.push_back(last_decoded);

    if (values.size() == size) {
      break;
    }
  }
}
