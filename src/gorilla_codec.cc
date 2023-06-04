#include "float_encoder.hpp"
#include "integer_encoder.hpp"
#include <cstring>
#include <napi.h>
#include <snappy.h>
#include <stdint.h>
#include <variant>
#include <vector>

using namespace Napi;

enum CompressionType : uint8_t {
  INTEGER_ENCODER = 0,
  FLOAT_ENCODER = 1,
  BOOLEAN_ENCODER = 2,
  STRING_ENCODER = 3,
  SNAPPY = 10
};

struct CompressionCarrier {
  napi_deferred deferred;
  napi_async_work work;
  std::vector<uint64_t> timestamps;
  std::variant<std::vector<int64_t>, std::vector<double>, std::vector<bool>,
               std::vector<std::string>>
      values;
  std::vector<uint8_t> compressedData;
};

enum class VariantType { Int64, Double, Bool, String };

VariantType getVariantType(
    const std::variant<std::vector<int64_t>, std::vector<double>,
                       std::vector<bool>, std::vector<std::string>> &var) {
  if (std::holds_alternative<std::vector<int64_t>>(var))
    return VariantType::Int64;
  if (std::holds_alternative<std::vector<double>>(var))
    return VariantType::Double;
  if (std::holds_alternative<std::vector<bool>>(var))
    return VariantType::Bool;
  if (std::holds_alternative<std::vector<std::string>>(var))
    return VariantType::String;

  throw std::runtime_error("Unsupported type");
}

void CompressFloats(CompressionCarrier *carrier) {
  std::vector<double> &doubleVector =
      std::get<std::vector<double>>(carrier->values);

  // Use our custom FloatEncoder to compress the data
  CompressedBuffer encodeBuffer = FloatEncoder::encode(doubleVector);

  // Copy the encoded data into the compressedData vector
  const size_t prefixSize = sizeof(CompressionType);
  const size_t offset = carrier->compressedData.size();

  carrier->compressedData.resize(offset + prefixSize +
                                 encodeBuffer.data.size() * sizeof(uint64_t));
  carrier->compressedData[offset] = FLOAT_ENCODER;

  const uint8_t *encoded_uint8 =
      reinterpret_cast<const uint8_t *>(encodeBuffer.data.data());

  std::copy(encoded_uint8,
            encoded_uint8 + encodeBuffer.data.size() * sizeof(uint64_t),
            carrier->compressedData.begin() + prefixSize + offset);
}

void CompressStrings(CompressionCarrier *carrier) {
  std::vector<std::string> &strings =
      std::get<std::vector<std::string>>(carrier->values);

  std::string concatenatedData;

  // Concatenate the strings into a binary buffer with a 32-bit unsigned int
  // prefix for each string's length
  for (const auto &str : strings) {
    uint32_t length = static_cast<uint32_t>(str.size());
    concatenatedData.append(reinterpret_cast<char *>(&length), sizeof(length));
    concatenatedData.append(str);
  }

  // Compress the data with Snappy
  std::string compressedData;
  snappy::Compress(concatenatedData.data(), concatenatedData.size(),
                   &compressedData);

  // Copy the compressed data into the compressedData vector

  const size_t prefixSize = sizeof(CompressionType);
  const size_t offset = carrier->compressedData.size();

  carrier->compressedData.resize(offset + prefixSize + compressedData.size());

  carrier->compressedData[offset] = STRING_ENCODER;

  std::copy(compressedData.begin(), compressedData.end(),
            carrier->compressedData.begin() + prefixSize + offset);
}

void DecompressString(CompressionCarrier *carrier, int offset) {
  char *buffer_data = (char *)carrier->compressedData.data() + offset;
  size_t buffer_length = carrier->compressedData.size() - offset;

  // Decompress the data with Snappy
  std::string decompressedData;
  if (!snappy::Uncompress(buffer_data, buffer_length, &decompressedData)) {
    throw std::runtime_error("Invalid data format");
    return;
  }

  carrier->values = std::vector<std::string>{};

  std::vector<std::string> &strings =
      std::get<std::vector<std::string>>(carrier->values);
  size_t index = 0;
  while (index < decompressedData.size()) {
    uint32_t length =
        *(reinterpret_cast<uint32_t *>(decompressedData.data() + index));
    index += sizeof(uint32_t);
    if (index + length > decompressedData.size()) {
      return;
    }
    strings.push_back(decompressedData.substr(index, length));
    index += length;
  }
}

void CompressBoolean(CompressionCarrier *carrier) {
  std::vector<bool> &boolVector = std::get<std::vector<bool>>(carrier->values);
  uint32_t numBooleans = boolVector.size();

  std::vector<uint8_t> data;
  data.resize(sizeof(uint32_t) + (numBooleans + 7) / 8,
              0); // Account for size prefix and padding bits

  // Store the size prefix in the data
  uint32_t *sizePrefixPtr = reinterpret_cast<uint32_t *>(data.data());
  *sizePrefixPtr = numBooleans;

  uint8_t *bitsDataStart = data.data() + sizeof(uint32_t);
  uint8_t currentByte = 0;
  unsigned int bitsSet = 0;

  // Read the array elements from JavaScript and store them as bits
  for (unsigned int i = 0; i < numBooleans; i++) {
    // Get the current array element
    bool value = boolVector[i];
    // If the boolean value is true, set the bit in the current byte
    if (value) {
      currentByte |= 1 << (7 - (bitsSet % 8));
    }

    if (++bitsSet % 8 == 0) {
      // If we have set 8 bits, store the current byte in the data and reset
      *(bitsDataStart++) = currentByte;
      currentByte = 0;
    }
  }

  // If there are remaining bits that have not been stored in the data, do so
  // now
  if (bitsSet % 8 != 0) {
    *bitsDataStart = currentByte;
  }

  // Copy the encoded data into the compressedData vector
  const size_t prefixSize = sizeof(CompressionType);
  const size_t offset = carrier->compressedData.size();

  carrier->compressedData.resize(offset + prefixSize + data.size());
  carrier->compressedData[offset] = BOOLEAN_ENCODER;

  std::copy(data.begin(), data.end(),
            carrier->compressedData.begin() + prefixSize + offset);
}

void DecompressBoolean(CompressionCarrier *carrier, int offset) {
  uint8_t *buffer_data = (uint8_t *)carrier->compressedData.data() + offset;
  size_t buffer_length = carrier->compressedData.size() - offset;

  // Read the size prefix from the data
  uint32_t numBooleans = *(reinterpret_cast<uint32_t *>(buffer_data));
  buffer_data += sizeof(uint32_t);
  buffer_length -= sizeof(uint32_t);

  // Create a vector to store the decompressed data
  carrier->values = std::vector<bool>{};
  std::vector<bool> &boolVector = std::get<std::vector<bool>>(carrier->values);
  boolVector.resize(numBooleans);

  uint8_t currentByte = 0;
  unsigned int bitsRead = 0;

  // Read the data and store it as booleans
  for (unsigned int i = 0; i < numBooleans; i++) {
    // If we have read 8 bits, read the next byte
    if (bitsRead % 8 == 0) {
      currentByte = *(buffer_data++);
      buffer_length--;
    }

    // Get the bit at the current position
    boolVector[i] = (currentByte & (1 << (7 - (bitsRead % 8)))) != 0;
    bitsRead++;
  }
}

void ExecuteCompression(napi_env env, void *data) {
  CompressionCarrier *carrier = static_cast<CompressionCarrier *>(data);

  const size_t prefixSize =
      sizeof(CompressionType) + sizeof(uint32_t) + sizeof(uint32_t);
  carrier->compressedData.resize(prefixSize);

  int offset = 0;

  // Encode timestamps

  const uint8_t timestampTypePrefix = static_cast<uint8_t>(INTEGER_ENCODER);
  carrier->compressedData[offset] = timestampTypePrefix;
  offset += sizeof(uint8_t);

  // Add the total number of items to the compressed data

  uint32_t *sizePtr = (uint32_t *)&carrier->compressedData[offset];
  *sizePtr = carrier->timestamps.size();
  offset += sizeof(uint32_t);

  AlignedBuffer timestampsBuffer = IntegerEncoder::encode(carrier->timestamps);

  // Add the total number of items to the compressed data
  uint32_t *timestampeLengthPtr = (uint32_t *)&carrier->compressedData[offset];
  *timestampeLengthPtr = timestampsBuffer.data.size();
  offset += sizeof(uint32_t);

  carrier->compressedData.resize(offset + timestampsBuffer.data.size());

  std::copy(timestampsBuffer.data.data(),
            timestampsBuffer.data.data() + timestampsBuffer.data.size(),
            carrier->compressedData.begin() + offset);

  offset += timestampsBuffer.data.size();

  // Encode values

  switch (getVariantType(carrier->values)) {
  case VariantType::Int64:
    // Handle int64_t
    break;
  case VariantType::Double:
    CompressFloats(carrier);
    break;
  case VariantType::Bool:
    CompressBoolean(carrier);
    break;
  case VariantType::String:
    CompressStrings(carrier);
    break;
  default:
    // Handle error
    break;
  }

  // Try compressing the buffer with Snappy

  std::string snappyOutput;
  snappy::Compress(
      reinterpret_cast<const char *>(carrier->compressedData.data()),
      carrier->compressedData.size(), &snappyOutput);

  // If the Snappy compressed buffer is smaller, use it instead

  if (false && snappyOutput.size() < carrier->compressedData.size()) {
    const int outputSize = sizeof(CompressionType) + snappyOutput.size();

    carrier->compressedData.resize(outputSize);
    carrier->compressedData[0] = static_cast<char>(SNAPPY);

    std::copy(snappyOutput.begin(), snappyOutput.end(),
              carrier->compressedData.begin() + sizeof(CompressionType));
  }
}

void ExecuteDecompression(napi_env env, void *data) {
  CompressionCarrier *carrier = static_cast<CompressionCarrier *>(data);

  if (carrier->compressedData[0] == SNAPPY) {
    // Get the size of the data
    const char *data = (const char *)carrier->compressedData.data() + 1;

    std::string decompressedData;
    snappy::Uncompress(data, carrier->compressedData.size() - 1,
                       &decompressedData);

    carrier->compressedData.resize(decompressedData.size());
    std::copy(decompressedData.begin(), decompressedData.end(),
              carrier->compressedData.begin());
  }

  int offset = 0;

  offset += sizeof(uint8_t);

  const uint32_t *itemCount =
      reinterpret_cast<const uint32_t *>(&carrier->compressedData[offset]);
  offset += sizeof(uint32_t);

  // Decode the timestamps

  const uint32_t *sizePtr =
      reinterpret_cast<const uint32_t *>(&carrier->compressedData[offset]);
  offset += sizeof(uint32_t);

  Slice timestampsSlice(carrier->compressedData.data() + offset, *sizePtr);
  IntegerEncoder::decode(timestampsSlice, carrier->timestamps, *itemCount);

  offset += *sizePtr;

  // Get the compression type
  const uint8_t compressionType = carrier->compressedData[offset];
  offset += sizeof(uint8_t);

  switch (compressionType) {
  case FLOAT_ENCODER: {
    CompressedSlice buffer(carrier->compressedData.data() + offset,
                           carrier->compressedData.size() - offset);

    carrier->values = std::vector<double>{};
    std::vector<double> &doubleVector =
        std::get<std::vector<double>>(carrier->values);

    doubleVector.reserve(*itemCount);

    // Decode the data
    FloatEncoder::decode(buffer, doubleVector, *itemCount);

    break;
  }
  case STRING_ENCODER: {
    DecompressString(carrier, offset);
    break;
  }
  case BOOLEAN_ENCODER: {
    DecompressBoolean(carrier, offset);
    break;
  }
  default: {
    break;
  }
  }
}

void CompressionComplete(napi_env env, napi_status status, void *data) {
  CompressionCarrier *carrier = static_cast<CompressionCarrier *>(data);

  napi_value result;

  napi_create_buffer_copy(env, carrier->compressedData.size(),
                          carrier->compressedData.data(), nullptr, &result);

  napi_resolve_deferred(env, carrier->deferred, result);
  napi_delete_async_work(env, carrier->work);
  delete carrier;
}

void DecompressionComplete(napi_env env, napi_status status, void *data) {
  CompressionCarrier *carrier = static_cast<CompressionCarrier *>(data);

  napi_value result, timestampsArray, valuesArray;

  // Create the result object
  napi_create_object(env, &result);

  // Assuming timestamps are stored in carrier->timestamps, create the
  // timestamps array
  napi_create_array_with_length(env, carrier->timestamps.size(),
                                &timestampsArray);
  for (uint32_t i = 0; i < carrier->timestamps.size(); i++) {
    napi_value num;
    napi_create_double(env, carrier->timestamps[i], &num);
    napi_set_element(env, timestampsArray, i, num);
  }

  // Set the timestamps property on the result object
  napi_set_named_property(env, result, "timestamps", timestampsArray);

  // Create the values array based on the variant type
  switch (getVariantType(carrier->values)) {
  case VariantType::Int64:
    // Handle int64_t
    break;
  case VariantType::Double: {
    std::vector<double> &doubleVector =
        std::get<std::vector<double>>(carrier->values);
    napi_create_array_with_length(env, doubleVector.size(), &valuesArray);

    for (uint32_t i = 0; i < doubleVector.size(); i++) {
      napi_value num;
      napi_create_double(env, doubleVector[i], &num);
      napi_set_element(env, valuesArray, i, num);
    }

    break;
  }
  case VariantType::Bool: {
    std::vector<bool> &boolVector =
        std::get<std::vector<bool>>(carrier->values);

    napi_create_array_with_length(env, boolVector.size(), &valuesArray);

    for (uint32_t i = 0; i < boolVector.size(); i++) {
      napi_value boolean;
      napi_get_boolean(env, boolVector[i], &boolean);
      napi_set_element(env, valuesArray, i, boolean);
    }
    break;
  }
  case VariantType::String: {
    std::vector<std::string> &stringVector =
        std::get<std::vector<std::string>>(carrier->values);
    napi_create_array_with_length(env, stringVector.size(), &valuesArray);

    for (uint32_t i = 0; i < stringVector.size(); i++) {
      napi_value str;
      napi_create_string_utf8(env, stringVector[i].c_str(), NAPI_AUTO_LENGTH,
                              &str);

      napi_set_element(env, valuesArray, i, str);
    }

    break;
  }
  default:
    // Handle error
    break;
  }

  // Set the values property on the result object
  napi_set_named_property(env, result, "values", valuesArray);

  napi_resolve_deferred(env, carrier->deferred, result);
  napi_delete_async_work(env, carrier->work);

  delete carrier;
}

napi_value Encode(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  // Check if the first argument is an object
  // Check the type of the argument
  napi_valuetype argType;
  napi_typeof(env, args[0], &argType);

  if (argType != napi_object) {
    napi_throw_type_error(env, nullptr, "Argument must be an object");
    return nullptr;
  }

  napi_value timestampsValue, valuesValue;
  napi_get_named_property(env, args[0], "timestamps", &timestampsValue);
  napi_get_named_property(env, args[0], "values", &valuesValue);

  // Check if the timestamps and values properties are arrays
  bool isTimestampsArray, isValuesArray;
  napi_is_array(env, timestampsValue, &isTimestampsArray);
  napi_is_array(env, valuesValue, &isValuesArray);
  if (!isTimestampsArray || !isValuesArray) {
    napi_throw_type_error(env, nullptr,
                          "Both timestamps and values must be arrays");
    return nullptr;
  }

  CompressionCarrier *carrier = new CompressionCarrier;

  // Read the array elements from JavaScript and store them in a vector

  uint32_t numTimestampValues;
  napi_get_array_length(env, timestampsValue, &numTimestampValues);

  uint32_t numValues;
  napi_get_array_length(env, valuesValue, &numValues);

  if (numTimestampValues != numValues) {
    napi_throw_type_error(
        env, nullptr,
        "Both timestamps and values must be arrays of the same length");
    return nullptr;
  }

  carrier->timestamps.reserve(numTimestampValues);

  for (uint32_t i = 0; i < numTimestampValues; i++) {
    napi_value element;
    napi_get_element(env, timestampsValue, i, &element);

    double num_double;
    napi_get_value_double(env, element, &num_double);
    uint64_t num = static_cast<uint64_t>(num_double);
    carrier->timestamps.push_back(num);
  }

  // Read the array elements from JavaScript and store them in a vector

  napi_valuetype valuetype;
  napi_value firstElement;

  if (numValues > 0) {
    napi_get_element(env, valuesValue, 0, &firstElement);
    napi_typeof(env, firstElement, &valuetype);
  } else {
    // Dummy type for encoding empty arrays
    valuetype = napi_boolean;
  }

  switch (valuetype) {
  case napi_number: {
    carrier->values = std::vector<double>{};

    std::vector<double> &numbers =
        std::get<std::vector<double>>(carrier->values);
    numbers.reserve(numValues);

    for (uint32_t i = 0; i < numValues; i++) {
      napi_value element;
      napi_get_element(env, valuesValue, i, &element);

      napi_valuetype itemType;
      napi_typeof(env, element, &itemType);

      // Throw if not a number
      if (itemType != napi_number) {
        napi_throw_type_error(env, nullptr, "Values must all be numbers");
        return nullptr;
      }

      double num;
      napi_get_value_double(env, element, &num);
      numbers.push_back(num);
    }
    break;
  }
  case napi_bigint: {
    std::vector<int64_t> &numbers =
        std::get<std::vector<int64_t>>(carrier->values);
    numbers.reserve(numValues);

    for (uint32_t i = 0; i < numValues; i++) {
      napi_value element;
      napi_get_element(env, valuesValue, i, &element);

      // Throw if not a napi_bigint
      napi_valuetype itemType;
      napi_typeof(env, element, &itemType);

      if (itemType != napi_bigint) {
        napi_throw_type_error(env, nullptr, "Values must all be bigints");
        return nullptr;
      }

      int64_t num;
      napi_get_value_bigint_int64(env, element, &num,
                                  nullptr); // this might require bigint support
      numbers.push_back(num);
    }
    break;
  }
  case napi_boolean: {
    carrier->values = std::vector<bool>{};

    std::vector<bool> &numbers = std::get<std::vector<bool>>(carrier->values);
    numbers.reserve(numValues);

    for (uint32_t i = 0; i < numValues; i++) {
      napi_value element;
      napi_get_element(env, valuesValue, i, &element);

      // Throw if not a napi_boolean
      napi_valuetype itemType;
      napi_typeof(env, element, &itemType);

      if (itemType != napi_boolean) {
        napi_throw_type_error(env, nullptr, "Values must all be boolean");
        return nullptr;
      }

      bool num;
      napi_get_value_bool(env, element, &num);
      numbers.push_back(num);
    }
    break;
  }
  case napi_string: {
    carrier->values = std::vector<std::string>{};

    std::vector<std::string> &strings =
        std::get<std::vector<std::string>>(carrier->values);
    strings.reserve(numValues);

    for (uint32_t i = 0; i < numValues; i++) {
      napi_value element;
      napi_get_element(env, valuesValue, i, &element);

      // Throw if not a napi_boolean
      napi_valuetype itemType;
      napi_typeof(env, element, &itemType);

      if (itemType != napi_string) {
        napi_throw_type_error(env, nullptr, "Values must all be strings");
        return nullptr;
      }

      size_t str_length;
      napi_get_value_string_utf8(env, element, nullptr, 0, &str_length);

      std::string str(str_length, '\0');
      napi_get_value_string_utf8(env, element, &str[0], str_length + 1,
                                 &str_length);

      strings.push_back(str);
    }
    break;
  }
  default:
    napi_throw_type_error(env, nullptr, "Unsupported data type in the array");
    return nullptr;
  }

  napi_value promise;
  napi_value asyncNameString;

  napi_create_string_utf8(env, "encode", NAPI_AUTO_LENGTH, &asyncNameString);

  napi_create_promise(env, &carrier->deferred, &promise);
  napi_create_async_work(env, nullptr, asyncNameString, ExecuteCompression,
                         CompressionComplete, carrier, &carrier->work);
  napi_queue_async_work(env, carrier->work);

  return promise;
}

napi_value Decode(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  // Check if the first argument is a buffer
  bool isBuffer;
  napi_is_buffer(env, args[0], &isBuffer);
  if (!isBuffer) {
    napi_throw_type_error(env, nullptr, "First argument must be a buffer");
    return nullptr;
  }

  // Get buffer data
  CompressionCarrier *carrier = new CompressionCarrier;
  size_t bufferLength;
  uint8_t *data = NULL;

  napi_get_buffer_info(env, args[0], (void **)&data, &bufferLength);

  carrier->compressedData.resize(bufferLength);
  std::copy(data, data + bufferLength, carrier->compressedData.begin());

  napi_value promise;
  napi_value asyncNameString;

  napi_create_string_utf8(env, "decode", NAPI_AUTO_LENGTH, &asyncNameString);

  napi_create_promise(env, &carrier->deferred, &promise);
  napi_create_async_work(env, nullptr, asyncNameString, ExecuteDecompression,
                         DecompressionComplete, carrier, &carrier->work);
  napi_queue_async_work(env, carrier->work);

  return promise;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  napi_property_descriptor desc[] = {
      {"encode", 0, Encode, 0, 0, 0, napi_default, 0},
      {"decode", 0, Decode, 0, 0, 0, napi_default, 0}};
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
  return exports;
}

NODE_API_MODULE(addon, Init)
