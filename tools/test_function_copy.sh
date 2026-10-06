#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_test_dir="$(mktemp -d)"
trap 'rm -f "$task_test_dir/function-copy"; rmdir "$task_test_dir"' EXIT
for architecture in 32 64; do
    compiler_flags=()
    archive="libs/libudis86.a"
    if [[ "$architecture" == 64 ]]; then
        compiler_flags+=(-DPLATFORM_64BITS)
        archive="libs/libudis86x64.a"
    fi
    "${CXX:-g++}" "-m$architecture" -std=c++17 -O2 -fPIC \
        "${compiler_flags[@]}" -Isrc -Ilibs/udis86 \
        tests/function_copy.cpp src/mem/func_copy.cpp "$archive" \
        -o "$task_test_dir/function-copy"
    "$task_test_dir/function-copy"
done
