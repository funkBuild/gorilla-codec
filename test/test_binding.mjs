import { describe, it } from "node:test";
import assert from "node:assert/strict";

import { createRequire } from "module";
const require = createRequire(import.meta.url);
const GorillaCodec = require("../lib/binding.js");

describe("Errors", () => {
  it("No values", async () => {
    const timestamps = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

    try {
      const encodeResult = await GorillaCodec.encode({ timestamps });
      assert.deepStrictEqual(true, false);
    } catch (e) {
      assert.deepStrictEqual(true, true);
    }
  });

  it("No timestamps", async () => {
    const values = [1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.1];

    try {
      const encodeResult = await GorillaCodec.encode({ values });
      assert.deepStrictEqual(true, false);
    } catch (e) {
      assert.deepStrictEqual(true, true);
    }
  });

  it("Wrong timestamps types", async () => {
    const timestamps = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    const values = [1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.1];

    try {
      const encodeResult = await GorillaCodec.encode({
        timestamps: timestamps.map((x) => x.toString()),
        values,
      });
      assert.deepStrictEqual(true, false);
    } catch (e) {
      assert.deepStrictEqual(true, true);
    }
  });

  it("Wrong values types", async () => {
    const timestamps = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    const values = [1.1, 2.2, 3.3, 4.4, "5.5", 6.6, 7.7, 8.8, 9.9, 10.1];

    try {
      const encodeResult = await GorillaCodec.encode({ timestamps, values });
      assert.deepStrictEqual(true, false);
    } catch (e) {
      assert.deepStrictEqual(true, true);
    }
  });

  it("Different lengths", async () => {
    const timestamps = [1, 2, 3, 4, 5];
    const values = [1.1, 2.2, 3.3, 4.4, 5.5, 6.6];

    try {
      const encodeResult = await GorillaCodec.encode({ timestamps, values });
      assert.deepStrictEqual(true, false);
    } catch (e) {
      assert.deepStrictEqual(true, true);
    }
  });
});

describe("Empty", () => {
  it("Encodes an empty array", async () => {
    const timestamps = [];
    const values = [];

    const encodeResult = await GorillaCodec.encode({ timestamps, values });
    const decodeResult = await GorillaCodec.decode(encodeResult);

    assert.deepStrictEqual(decodeResult, { timestamps, values });
  });
});

describe("Integer", () => {
  it("Encodes an integer array", async () => {
    const timestamps = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    const values = [1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.1];

    const encodeResult = await GorillaCodec.encode({ timestamps, values });
    const decodeResult = await GorillaCodec.decode(encodeResult);

    assert.deepStrictEqual(decodeResult, { timestamps, values });
  });

  it("Encodes a large integer array", async () => {
    const timestamps = [];
    const values = [];

    for (let i = 0; i < 1000000; i++) {
      timestamps.push(i);
      values.push(i);
    }

    const encodeResult = await GorillaCodec.encode({ timestamps, values });
    const decodeResult = await GorillaCodec.decode(encodeResult);

    assert.deepStrictEqual(decodeResult, { timestamps, values });
  });

  it("Encodes a large integer array with a large gap", async () => {
    const timestamps = [];
    const values = [];

    for (let i = 0; i < 1000000; i++) {
      timestamps.push(i * 3333);
      values.push(i * 1.2333);
    }

    const encodeResult = await GorillaCodec.encode({ timestamps, values });
    const decodeResult = await GorillaCodec.decode(encodeResult);

    assert.deepStrictEqual(decodeResult, { timestamps, values });
  });
});

describe("String", () => {
  it("Encodes a string array", async () => {
    const timestamps = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    const values = [
      "one",
      "two",
      "three",
      "four",
      "five",
      "six",
      "seven",
      "eight",
      "nine",
      "ten",
    ];

    const encodeResult = await GorillaCodec.encode({ timestamps, values });
    const decodeResult = await GorillaCodec.decode(encodeResult);

    assert.deepStrictEqual(decodeResult, { timestamps, values });
  });

  it("Encodes a large string array", async () => {
    const timestamps = [];
    const values = [];

    for (let i = 0; i < 1000000; i++) {
      timestamps.push(i);
      values.push(i.toString());
    }

    const encodeResult = await GorillaCodec.encode({ timestamps, values });
    const decodeResult = await GorillaCodec.decode(encodeResult);

    assert.deepStrictEqual(decodeResult, { timestamps, values });
  });
});

describe("Boolean", () => {
  it("Encodes a boolean array", async () => {
    const timestamps = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    const values = [
      true,
      false,
      true,
      false,
      true,
      false,
      true,
      false,
      true,
      false,
    ];

    const encodeResult = await GorillaCodec.encode({ timestamps, values });
    const decodeResult = await GorillaCodec.decode(encodeResult);

    assert.deepStrictEqual(decodeResult, { timestamps, values });
  });

  it("Encodes a large boolean array", async () => {
    const timestamps = [];
    const values = [];

    for (let i = 0; i < 1000000; i++) {
      timestamps.push(i);
      values.push(i % 5 === 0);
    }

    const encodeResult = await GorillaCodec.encode({ timestamps, values });
    const decodeResult = await GorillaCodec.decode(encodeResult);

    assert.deepStrictEqual(decodeResult, { timestamps, values });
  });
});
