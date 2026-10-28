# Historical Loss Risk in C++

A C++17 command-line tool for empirical Value at Risk (VaR), Expected Shortfall
(ES), mean loss and worst loss. It compares full sorting with a selection-based
implementation using `std::nth_element`. The statistics have explicit quantile
and fractional-tail definitions, including tied observations.

<!-- repository-links:start -->
**Public repository:** [https://github.com/MIN-JX/historical-risk-cpp](https://github.com/MIN-JX/historical-risk-cpp)  
**Documentation:** [https://MIN-JX.github.io/historical-risk-cpp/](https://MIN-JX.github.io/historical-risk-cpp/)
<!-- repository-links:end -->

## Run the project

Requirements: C++17 compiler, CMake 3.24 or later, a build tool, Python 3.9 or
later, and Doxygen. Bundled doctest and Google Benchmark source archives have
pinned SHA-256 hashes; building does not download those dependencies.

Linux/macOS:

```sh
bash scripts/verify.sh
./build/bin/risk_analytics --input data/example_losses.csv --confidence-bps 8000
```

Windows, in PowerShell with the Visual Studio C++ build tools available:

```powershell
./scripts/verify.ps1
./build/bin/risk_analytics.exe --input data/example_losses.csv --confidence-bps 8000
```

The verification script configures a Release build, compiles, runs CTest, prints
and validates benchmark measurements, generates Doxygen, and checks local
documentation links. A failing stage stops the script. An optional build-directory
argument keeps generated files outside the source folder.

The example contains ten synthetic daily losses. Positive numbers are losses;
negative numbers are profits. At 80% confidence the expected output is:

| Output | Value |
|---|---:|
| Sample count | 10 |
| Value at Risk | 60 |
| Expected Shortfall | 90 |
| Mean loss | 27 |
| Worst loss | 100 |

Confidence uses integer basis points: `8000` means 80%, `9500` means 95%, and
`9750` means 97.5%. The CLI accepts 1 to 9999. Its default is 9500. With this
small ten-observation example, 95% VaR and ES both equal the sample maximum;
that follows from the discrete sample, not from extra data.

Compare implementations or demonstrate a fractional tail:

```sh
./build/bin/risk_analytics --input data/example_losses.csv --confidence-bps 8000 --method sort
./build/bin/risk_analytics --input data/example_losses.csv --confidence-bps 8250 --method partition
```

At 82.5%, VaR is 80 and ES is approximately 91.42857143. Both methods should
agree within floating-point tolerance. CSV output goes to standard output;
errors go to standard error with a nonzero exit status. Use `--help` alone for
the accepted arguments and units.

## Input contract

The input is a single-column CSV with header `loss` and one decimal representable
as a finite double per line. A leading UTF-8 BOM, CRLF endings, and surrounding ASCII whitespace are
accepted. Empty data, empty observation rows, nonfinite numbers, multiple columns,
and quoted fields are rejected. A nonzero decimal that rounds to zero is also
rejected; representable subnormal values are accepted. This narrow format is intentional; the CLI is
not a general-purpose CSV parser. The API accepts 1 to 1000000 observations.

All observations must use the same currency and horizon. No return conversion,
annualization, scenario weighting, or multi-asset aggregation is inferred from
the file. Negative estimated risk is retained when the worst observed outcomes
are still profits.

## Individual checks and outputs

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target run_benchmarks
cmake --build build --config Release --target docs
```

Executables are in `build/bin/`; measurements are in `build/benchmark.json`.
Open `build/docs/html/index.html` for generated documentation. The delivered
`verification/` directory contains actual local logs, measurement JSON, source
checksums, a numerical example, and a portable HTML documentation archive.

| Path | Contents |
|---|---|
| `include/risk/analytics.hpp` | Public API and numerical contracts |
| `src/analytics.cpp` | Sorting and partition estimators |
| `src/main.cpp` | Validated loss CSV and command-line interface |
| `tests/` | C++ tests and independent Decimal checks |
| `benchmarks/` | Matched synthetic workloads |
| `docs/` | Definitions, algorithms, results and publishing |
| `scripts/` | Verification and evidence validators |
| `.github/workflows/` | Three-platform verification and Pages publication |
| `third_party/` | Fixed dependencies and original licenses |

Start with [the statistical model](docs/model.md), then the
[algorithm walkthrough](docs/algorithms.md), [benchmark report](docs/benchmark-results.md),
and [six-item acceptance checklist](docs/acceptance.md).

