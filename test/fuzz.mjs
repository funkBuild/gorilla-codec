import { createRequire } from "module";
const require = createRequire(import.meta.url);
const GorillaCodec = require("../lib/binding.js");

let successCount = 0;
let maxLength = 10000;

async function fuzzTest() {
  const arrayLength = Math.floor(Math.random() * maxLength) + 1;

  const timestamps = Array.from(
    { length: arrayLength },
    () => Math.floor(Math.random() * Number.MAX_SAFE_INTEGER) + 1
  );

  const types = ["number", "boolean", "string", "float"];
  const randomType = types[Math.floor(Math.random() * types.length)];

  const values = Array.from({ length: arrayLength }, () => {
    switch (randomType) {
      case "number":
        return Math.random() * Number.MAX_SAFE_INTEGER;
      case "boolean":
        return Math.random() < 0.5;
      case "string":
        return Math.random().toString(36).substring(2, 15); // random string of length up to 13
      case "float":
        return 1000000 * (Math.random() - 0.5);
      default:
        return null;
    }
  });

  const encodeResult = await GorillaCodec.encode({ timestamps, values });
  const decodeResult = await GorillaCodec.decode(encodeResult);

  if (JSON.stringify(decodeResult) !== JSON.stringify({ timestamps, values })) {
    console.log("Error");
    console.log({ timestamps, values });
    console.log(decodeResult);
    process.exit(1);
  } else {
    successCount++;

    if (successCount % 1000 === 0) {
      maxLength = maxLength * 2;
    }

    console.log("Success. count=" + successCount);
  }
}

while (true) {
  await fuzzTest();
}
