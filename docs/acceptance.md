# Six-item acceptance checklist

The source includes the requested automation. Execution status is recorded
separately: the delivered `verification/summary.json` describes the local run,
and the eventual owner's GitHub account must provide the public CI evidence.

| Criterion | Points | Implemented evidence | Public step still required |
|---|---:|---|---|
| Public repository, structure and README | 2 | Separate code, tests, benchmarks, and build/run instructions | Publish in the owner's account |
| CMake and automatic CI build | 4 | CMake library/CLI targets and three-platform Actions workflow | Successful build jobs on GitHub |
| C++ testing framework and CI tests | 4 | doctest cases, Decimal CLI oracle, workflow-evidence checks | Successful test jobs on GitHub |
| Automatic benchmark with log output | 4 | Google Benchmark runs both methods and validates measurement JSON | Successful benchmark jobs and artifacts |
| Automatic Doxygen publication, README link | 4 | Checked HTML, Pages workflow, real-link configuration script | Enable Pages, deploy, and verify actual README link |
| Root agent configuration | 2 | `AGENTS.md` gives purpose, layout, commands, style and constraints | Include the file in the pushed repository |

Criteria 2-5 receive full credit only with successful CI/CD execution under the
assignment rubric. This folder has not been published on another person's
account. Follow [Publishing](publishing.md), then keep the final commit, successful
Actions run and documentation link with the submission.

Local reproduction: run `bash scripts/verify.sh` or `./scripts/verify.ps1` with the
required compiler and tools installed. The source archives for test/benchmark
dependencies are bundled. The output includes CTest results, benchmark validation,
and Doxygen file/anchor validation.

