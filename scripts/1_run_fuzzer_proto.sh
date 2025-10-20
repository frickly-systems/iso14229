#!/bin/bash

# Get workspace path
WORKSPACE=$(bazel info workspace)
FUZZ_ROOT=${WORKSPACE}/fuzz
PROTO_FILE=${FUZZ_ROOT}/uds_input.proto
CORPUS=${WORKSPACE}/fuzz/outputs/proto/corpus
ARTIFACT_OUTPUT=${WORKSPACE}/fuzz/outputs/proto/
SEED_TXT=${WORKSPACE}/fuzz/outputs/proto/seed_corpus/text
SEED_BIN=${WORKSPACE}/fuzz/outputs/proto/seed_corpus/bin


rm -r "${SEED_BIN:?}/*"

for f in ${SEED_TXT}/*.textproto; do
  b="$(basename "${f%.textproto}")"
  protoc -I ${FUZZ_ROOT} \
    --encode=udsfuzz.Testcase ${PROTO_FILE} \
    < "$f" > "${SEED_BIN}/$b.pb"
done


# Create output corpus directory if it doesn't exist
mkdir -p ${WORKSPACE}/fuzz/outputs/proto/corpus


bazel run -c opt --config=hermetic-fuzz \
  --repo_env=CC=clang --repo_env=CXX=clang++ \
  //fuzz:fuzz_server_proto_bin -- -runs=0 "${SEED_BIN}/"

bazel run -c opt --config=hermetic-fuzz \
  --repo_env=CC=clang --repo_env=CXX=clang++ \
  //fuzz:fuzz_server_proto_bin -- \
  -artifact_prefix="${ARTIFACT_OUTPUT}" \
  -max_total_time=60 \
  -jobs=$(nproc) \
  -print_final_stats=1 \
  "${WORKSPACE}/fuzz/outputs/proto/corpus/" \
  "${SEED_BIN}/"