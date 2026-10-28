# Pinned dependencies

CMake reads the included upstream release archives and checks their SHA-256 hashes.
A normal build does not fetch test or benchmark dependencies from the Internet.
`dependencies.json` records source URLs, versions and checksums. The adjacent
license files are the original upstream licenses. Dependency sources are
extracted only into the build directory.

- doctest 2.5.3: MIT license, `doctest-LICENSE.txt`.
- Google Benchmark 1.9.5: Apache-2.0 license, `benchmark-LICENSE.txt`.

The generic CMake/CI verification layout and HTML/benchmark validators were
adapted from the preceding course project infrastructure. The risk estimators,
CLI, test cases, input data and measurements are separate implementations.
