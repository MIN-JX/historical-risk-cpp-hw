# Historical risk benchmark results

Measured locally on 26 October 2026 with Apple Clang 21.0.0, a CMake Release
build, and Google Benchmark 1.9.5. Raw machine context and every repetition are
retained in `verification/benchmark-report.json`. These are local measurements,
not executed GitHub CI results.

Each table entry is the median CPU time of three repetitions. Both methods get
the same deterministic synthetic loss sample and confidence. Input generation
is outside timing; validation, copying, mean/tail calculation and working-memory
allocation inside the estimator are included. Setup checks agreement before timing.

| Observations | Confidence | Sort (us) | Partition (us) | Sort / Partition |
|---:|---:|---:|---:|---:|
| 100 | 95.00% | 0.545 | 0.299 | 1.82x |
| 100 | 97.50% | 0.545 | 0.293 | 1.86x |
| 1000 | 95.00% | 6.973 | 3.460 | 2.02x |
| 1000 | 97.50% | 6.956 | 3.357 | 2.07x |
| 10000 | 95.00% | 87.208 | 31.181 | 2.80x |
| 10000 | 97.50% | 85.850 | 30.135 | 2.85x |
| 50000 | 95.00% | 751.771 | 274.988 | 2.73x |
| 50000 | 97.50% | 735.385 | 228.375 | 3.22x |
| 100000 | 95.00% | 1650.176 | 490.308 | 3.37x |
| 100000 | 97.50% | 1620.588 | 484.418 | 3.35x |

Selection avoids sorting observations whose internal order is not needed. The
reference method has O(n log n) work; selection is O(n) on average. Both preserve
the caller's input through an O(n) working copy. Average complexity does not
promise a speed ratio for every input, library, compiler, or machine. Fixed costs
matter particularly at small sample sizes; use matched workloads to compare.

The run has 20 configurations, each repeated three times: 60 raw measurement
rows and 80 aggregate rows (mean, median, standard deviation and coefficient of
variation), for 140 JSON entries. The validator checks exact names, repetition
indices, aggregate completeness, positive measured times, and finite values.

Reproduce with `cmake --build build --config Release --target run_benchmarks`.
New measurements go to `build/benchmark.json` and the console. Keep each machine's
measurements separate. `verification/benchmark-reproduction.json` is a second
independent run from the final source-transfer check, not a replacement for
the report snapshot above.
