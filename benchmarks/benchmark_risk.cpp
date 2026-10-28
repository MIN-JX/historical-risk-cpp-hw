#include "risk/analytics.hpp"
#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
constexpr int counts[] = {100, 1000, 10000, 50000, 100000};
constexpr int confidences[] = {9500, 9750};

const std::vector<double>& observations(int count) {
    // Populate all inputs before benchmark timing begins. Integer-based sampling
    // keeps this synthetic workload reproducible across standard libraries.
    static const std::map<int, std::vector<double>> samples = [] {
        std::map<int, std::vector<double>> result;
        for (int size : counts) {
            std::mt19937_64 engine(20261026);
            std::vector<double> losses;
            losses.reserve(static_cast<std::size_t>(size));
            for (int i = 0; i < size; ++i) {
                const auto draw = static_cast<long long>(engine() % 200001) - 50000;
                losses.push_back(static_cast<double>(draw) / 10.0);
            }
            result.emplace(size, std::move(losses));
        }
        return result;
    }();
    return samples.at(count);
}

void workloads(benchmark::internal::Benchmark* experiment) {
    for (int count : counts) {
        for (int confidence : confidences) experiment->Args({count, confidence});
    }
    experiment->ArgNames({"count", "confidence_bps"});
}

void consume(const risk::Metrics& metrics) {
    benchmark::DoNotOptimize(metrics.value_at_risk);
    benchmark::DoNotOptimize(metrics.expected_shortfall);
    benchmark::DoNotOptimize(metrics.mean_loss);
    benchmark::DoNotOptimize(metrics.worst_loss);
}

void BM_Sort(benchmark::State& state) {
    const auto& losses = observations(static_cast<int>(state.range(0)));
    const auto confidence = static_cast<unsigned>(state.range(1));
    for (auto iteration : state) {
        (void)iteration;
        consume(risk::estimate_sort(losses, confidence));
    }
}

void BM_Partition(benchmark::State& state) {
    const auto& losses = observations(static_cast<int>(state.range(0)));
    const auto confidence = static_cast<unsigned>(state.range(1));
    for (auto iteration : state) {
        (void)iteration;
        consume(risk::estimate_partition(losses, confidence));
    }
}

bool close(double left, double right) {
    return std::abs(left - right) <= 1e-10 * std::max({1.0, std::abs(left), std::abs(right)});
}

BENCHMARK(BM_Sort)->Apply(workloads);
BENCHMARK(BM_Partition)->Apply(workloads);
} // namespace

int main(int argc, char** argv) {
    try {
        for (int count : counts) {
            const auto& losses = observations(count);
            for (int confidence : confidences) {
                const auto sorted = risk::estimate_sort(losses, static_cast<unsigned>(confidence));
                const auto partitioned = risk::estimate_partition(losses, static_cast<unsigned>(confidence));
                if (!close(sorted.value_at_risk, partitioned.value_at_risk) ||
                    !close(sorted.expected_shortfall, partitioned.expected_shortfall) ||
                    !close(sorted.mean_loss, partitioned.mean_loss) ||
                    !close(sorted.worst_loss, partitioned.worst_loss)) {
                    throw std::runtime_error("risk methods disagree before measurement");
                }
            }
        }
        benchmark::Initialize(&argc, argv);
        if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
        benchmark::RunSpecifiedBenchmarks();
        benchmark::Shutdown();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
