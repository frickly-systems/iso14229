#!/bin/bash

set -euxo pipefail

# Get workspace path
WORKSPACE=$(bazel info workspace)
FUZZ_ROOT=${WORKSPACE}/fuzz
CORPUS=${FUZZ_ROOT}/outputs/proto/corpus
SEED_BIN=${FUZZ_ROOT}/outputs/proto/seed_corpus/bin

# Create corpus directory if it doesn't exist
mkdir -p "${CORPUS}"
mkdir -p "${SEED_BIN}"

# Run coverage with hermetic toolchain, replaying both corpus and seed
bazel coverage -c opt --config=hermetic-coverage \
  --test_arg="${CORPUS}/" \
  --test_arg="${SEED_BIN}/" \
  //fuzz:fuzz_server_proto

# Copy and process the coverage report
cp -f "$(bazel info output_path)/_coverage/_coverage_report.dat" coverage_proto.lcov
sed -E -i 's#^SF:bazel-out/.*/iso14229.c#SF:iso14229.c#' coverage_proto.lcov
sed -E -i 's#^SF:bazel-out/.*/iso14229.h#SF:iso14229.h#' coverage_proto.lcov
ls -l coverage_proto.lcov
