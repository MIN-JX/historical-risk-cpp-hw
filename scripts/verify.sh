#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"
build_dir="${1:-build}"
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --config Release --parallel 2
ctest --test-dir "$build_dir" -C Release --output-on-failure
cmake --build "$build_dir" --config Release --target run_benchmarks
cmake --build "$build_dir" --config Release --target docs
printf '%s\n' 'Verification complete: build, tests, benchmark JSON and documentation passed.'
