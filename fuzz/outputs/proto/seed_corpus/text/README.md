# Example Seeds for UDS Fuzzer

## To create binary seed files from text proto:

1. Install protobuf compiler tools
2. Use this command to convert text to binary:

```bash
protoc --encode=udsfuzz.Testcase uds_input.proto < seed.textproto > seed.bin
```

## Seed Ideas:

- Different diagnostic session types (0x01, 0x02, 0x03)
- Edge cases (max values, zero values)
- Different timing scenarios
- Various source/target addresses
- Different message types

## Directory Structure:

- `seed_corpus/`: Your handcrafted seed inputs (version controlled, read-only)
- `outputs/proto/corpus/`: Generated corpus (gitignored, read-write)
