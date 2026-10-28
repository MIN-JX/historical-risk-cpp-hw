# Repository instructions

## Purpose and structure
This C++17 project estimates empirical loss Value at Risk and Expected Shortfall.
`include/risk/` defines contracts; `src/` contains estimators and the CSV CLI;
`tests/` contains doctest cases and independent numerical checks; `benchmarks/`
compares full sorting with partition selection; `docs/` explains the statistics;
`scripts/` runs reproducible verification; `third_party/` holds pinned dependencies.

## Commands
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- Build: `cmake --build build --config Release --parallel 2`
- Tests: `ctest --test-dir build -C Release --output-on-failure`
- Measurements: `cmake --build build --config Release --target run_benchmarks`
- Documentation: `cmake --build build --config Release --target docs`
- Complete Linux/macOS check: `bash scripts/verify.sh`
- Example: `./build/bin/risk_analytics --input data/example_losses.csv --confidence-bps 8000`

## Conventions and constraints
Use C++17, explicit units and short functions. Public APIs need Doxygen comments.
Confidence is an integer number of basis points: 9500 means 95%, not 0.95%.
Positive input is a loss; negative input is a profit. Do not silently clamp
negative VaR or change the quantile or fractional-tail definitions.
Verify changed numerical code against hand cases and the Decimal oracle.
Build and run tests before finishing changes. Preserve dependency licenses and
keep modifications within this repository. Do not invent measurements or claim
public CI/Pages success from local results. Workflow permission changes, remote
publication and release changes require a request.

