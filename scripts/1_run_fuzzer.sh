#!/bin/bash

# Get workspace path
WORKSPACE=$(bazel info workspace)

bazel run -c opt --config=hermetic-fuzz //fuzz:fuzz_server_bin -- \
-max_total_time=2 \
-artifact_prefix=${WORKSPACE}/fuzz/outputs/server \
-jobs=$(nproc) \
${WORKSPACE}/fuzz/outputs/server/corpus
