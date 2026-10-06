#!/usr/bin/env bash
set -euo pipefail
task_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
task_build=$(mktemp -d)
trap 'rm -f -- "$task_build/layout"; rmdir -- "$task_build"' EXIT
for task_arch in -m32 -m64; do
    for task_engine in tf2 legacy; do
        task_defines=()
        if [[ "$task_engine" == tf2 ]]; then
            task_defines=(-DSE_IS_TF2)
        fi
        "${CXX:-g++}" "$task_arch" -std=c++17 -Wall -Wextra -Werror \
            "${task_defines[@]}" "$task_root/tests/frame_snapshot_layout.cpp" \
            -o "$task_build/layout"
        "$task_build/layout"
    done
done
