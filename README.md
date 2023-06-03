# Gorilla Codec

Gorilla Codec is a Node.js package that implements the Facebook Gorilla compression algorithm as a N-API module. It provides an efficient way to compress and decompress time series data, reducing memory usage and improving processing speed.

## Installation

Use npm to install the Gorilla Codec:

```bash
npm install gorilla-codec
```

## Usage

The module provides two main functions, encode and decode.

#`encode`
The encode function accepts a hash containing two arrays: timestamps and values. The timestamps array should contain integers, representing moments in time. The values array can contain integers, strings, floats or booleans, representing the data associated with each timestamp.

```
const GorillaCodec = require('gorilla-codec');
const data = {
  timestamps: [1, 2, 3],
  values: [10, 20, 30]
};
const encodedBuffer = GorillaCodec.encode(data);
```

The `encode` function returns a Node.js Buffer containing the compressed data.

`decode`
The decode function accepts a Buffer, which it decodes to return the original timestamps and values.

```
const decodedData = GorillaCodec.decode(encodedBuffer);
console.log(decodedData);  // Outputs: { timestamps: [1, 2, 3], values: [10, 20, 30] }
```

Notes
Please ensure your timestamps array only contains integers, and your values array only contains integers, strings, floats or booleans. Inconsistent or incorrect data types can lead to errors or unexpected behavior.

License
MIT
You can modify the Installation, Usage, Notes, and License sections to suit your specific needs and requirements. You may also want to add sections for contributing, a changelog, and more detailed information about the algorithm and its usage.
