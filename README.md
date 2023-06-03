# Gorilla Codec

Gorilla Codec is a Node.js package that implements the Facebook Gorilla compression algorithm as a N-API module. It provides an efficient way to compress and decompress time series data, reducing memory usage and improving processing speed.

## Installation

Use npm to install the Gorilla Codec:

```bash
npm install gorilla-codec
```

## Usage

The module provides two main functions, encode and decode.

`encode`
The encode function accepts a hash containing two arrays: timestamps and values. The timestamps array should contain integers, representing moments in time. The values array can contain integers, strings, floats or booleans, representing the data associated with each timestamp.
