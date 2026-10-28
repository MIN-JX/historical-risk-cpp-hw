#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "risk/analytics.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
using Estimator = risk::Metrics (*)(const std::vector<double>&, unsigned);
constexpr std::array<Estimator, 2> methods{risk::estimate_sort, risk::estimate_partition};

void check_near(double actual, double expected) {
    CHECK(std::isfinite(actual));
    if (expected == 0.0) {
        CHECK(actual == doctest::Approx(0.0).epsilon(1e-12));
    } else {
        CHECK(actual == doctest::Approx(expected).epsilon(2e-12).scale(0.0));
    }
}

void check_values(const risk::Metrics& value, double var, double es,
                  double mean, double worst) {
    CHECK(value.value_at_risk == var);
    check_near(value.expected_shortfall, es);
    check_near(value.mean_loss, mean);
    CHECK(value.worst_loss == worst);
}

const std::vector<double> example{-50, -20, 0, 10, 20, 30, 40, 60, 80, 100};
} // namespace

TEST_CASE("integer tail gives the mean of exactly the worst observations") {
    for (auto estimate : methods) {
        check_values(estimate(example, 8000), 60, 90, 27, 100);
    }
}

TEST_CASE("fractional tail includes only its prescribed boundary mass") {
    for (auto estimate : methods) {
        check_values(estimate(example, 8250), 80, 640.0 / 7.0, 27, 100);
    }
}

TEST_CASE("basis points avoid floating point rank rounding at eighty percent") {
    const std::vector<double> losses{-20, 0, 10, 30, 40};
    for (auto estimate : methods) {
        check_values(estimate(losses, 8000), 30, 40, 12, 40);
    }
}

TEST_CASE("ties do not expand the integer tail to every value at or above VaR") {
    const std::vector<double> losses{0, 10, 10, 10, 100};
    for (auto estimate : methods) {
        check_values(estimate(losses, 6000), 10, 55, 26, 100);
    }
}

TEST_CASE("fractional tail treats repeated boundary values consistently") {
    const std::vector<double> losses{0, 10, 10, 10, 100};
    for (auto estimate : methods) {
        check_values(estimate(losses, 5000), 10, 46, 26, 100);
    }
}

TEST_CASE("an entirely profitable sample retains negative risk values") {
    const std::vector<double> losses{-9, -5, -2};
    for (auto estimate : methods) {
        check_values(estimate(losses, 5000), -5, -3, -16.0 / 3.0, -2);
    }
}

TEST_CASE("all zero losses give exactly zero statistics") {
    for (auto estimate : methods) {
        check_values(estimate({0, 0, 0, 0}, 9750), 0, 0, 0, 0);
    }
}

TEST_CASE("identical losses have identical quantile tail mean and full mean") {
    for (auto estimate : methods) {
        check_values(estimate({17, 17, 17, 17, 17}, 8250), 17, 17, 17, 17);
    }
}

TEST_CASE("a single observation defines every statistic") {
    for (double loss : {-13.0, 0.0, 19.0}) {
        for (unsigned confidence : {1U, 5000U, 9999U}) {
            for (auto estimate : methods) {
                check_values(estimate({loss}, confidence), loss, loss, loss, loss);
            }
        }
    }
}

TEST_CASE("one basis point confidence uses nearly the full sample tail") {
    for (auto estimate : methods) {
        check_values(estimate(example, 1), -50, 2700500.0 / 99990.0, 27, 100);
    }
}

TEST_CASE("very high confidence retains the worst loss even for a fractional tail") {
    for (auto estimate : methods) {
        check_values(estimate(example, 9999), 100, 100, 27, 100);
    }
}

TEST_CASE("ninety seven point five percent uses a half observation when needed") {
    std::vector<double> losses(100);
    for (std::size_t i = 0; i < losses.size(); ++i) losses[i] = static_cast<double>(i);
    for (auto estimate : methods) {
        check_values(estimate(losses, 9750), 97, 98.2, 49.5, 99);
    }
}

TEST_CASE("ordinary decimal observations preserve their mean") {
    for (auto estimate : methods) {
        check_values(estimate({-0.75, 0.25, 0.5, 1.5}, 5000), 0.25, 1.0, 0.375, 1.5);
    }
}

TEST_CASE("permuting observations leaves every statistic unchanged") {
    auto shuffled = example;
    std::reverse(shuffled.begin(), shuffled.end());
    for (auto estimate : methods) {
        const auto a = estimate(example, 8250);
        const auto b = estimate(shuffled, 8250);
        check_values(b, a.value_at_risk, a.expected_shortfall, a.mean_loss, a.worst_loss);
    }
}

TEST_CASE("adding a constant translates every loss statistic") {
    auto shifted = example;
    for (double& loss : shifted) loss += 125;
    for (auto estimate : methods) {
        const auto a = estimate(example, 8250);
        check_values(estimate(shifted, 8250), a.value_at_risk + 125,
                     a.expected_shortfall + 125, a.mean_loss + 125, a.worst_loss + 125);
    }
}

TEST_CASE("multiplying by a positive constant scales every statistic") {
    auto scaled = example;
    for (double& loss : scaled) loss *= 3.5;
    for (auto estimate : methods) {
        const auto a = estimate(example, 8250);
        check_values(estimate(scaled, 8250), a.value_at_risk * 3.5,
                     a.expected_shortfall * 3.5, a.mean_loss * 3.5, a.worst_loss * 3.5);
    }
}

TEST_CASE("both methods leave the caller observations unchanged") {
    const std::vector<double> original{9, -2, 17, 0, 9, 1};
    for (auto estimate : methods) {
        auto input = original;
        (void)estimate(input, 7500);
        CHECK(input == original);
    }
}

TEST_CASE("increasing confidence cannot decrease the empirical upper tail") {
    for (auto estimate : methods) {
        double previous_var = -std::numeric_limits<double>::infinity();
        double previous_es = previous_var;
        for (unsigned bps : {1U, 2000U, 5000U, 8000U, 8250U, 9500U, 9999U}) {
            const auto value = estimate(example, bps);
            CHECK(value.value_at_risk >= previous_var);
            CHECK(value.expected_shortfall >= previous_es - 1e-11);
            CHECK(value.expected_shortfall >= value.value_at_risk - 1e-11);
            CHECK(value.expected_shortfall >= value.mean_loss - 1e-11);
            previous_var = value.value_at_risk;
            previous_es = value.expected_shortfall;
        }
    }
}

TEST_CASE("deterministic varied samples agree with quantile ranks and each other") {
    std::mt19937 generator(271828);
    std::uniform_int_distribution<int> loss(-2000, 8000);
    for (std::size_t count : {2U, 7U, 37U, 100U, 257U}) {
        std::vector<double> sample(count);
        for (double& value : sample) value = loss(generator) / 8.0;
        auto ordered = sample;
        std::sort(ordered.begin(), ordered.end());
        for (unsigned bps : {1U, 3333U, 8000U, 9750U, 9999U}) {
            const std::size_t rank = (count * bps + 9999) / 10000;
            const auto sorted = risk::estimate_sort(sample, bps);
            const auto selected = risk::estimate_partition(sample, bps);
            CHECK(sorted.value_at_risk == ordered[rank - 1]);
            check_values(selected, sorted.value_at_risk, sorted.expected_shortfall,
                         sorted.mean_loss, sorted.worst_loss);
        }
    }
}

TEST_CASE("empty samples are rejected") {
    CHECK_THROWS_AS(risk::validate({}, 9500), std::invalid_argument);
    for (auto estimate : methods) CHECK_THROWS_AS(estimate({}, 9500), std::invalid_argument);
}

TEST_CASE("confidence must be an integer basis point value strictly within endpoints") {
    for (unsigned bps : {0U, 10000U, std::numeric_limits<unsigned>::max()}) {
        CHECK_THROWS_AS(risk::validate({1, 2}, bps), std::invalid_argument);
        for (auto estimate : methods) CHECK_THROWS_AS(estimate({1, 2}, bps), std::invalid_argument);
    }
}

TEST_CASE("nonfinite observations are rejected") {
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()}) {
        CHECK_THROWS_AS(risk::validate({1, bad, 2}, 9500), std::invalid_argument);
        for (auto estimate : methods) CHECK_THROWS_AS(estimate({1, bad, 2}, 9500), std::invalid_argument);
    }
}

TEST_CASE("samples exceeding the documented observation limit are rejected") {
    const std::vector<double> oversized(risk::max_observations + 1, 1.0);
    CHECK_THROWS_AS(risk::validate(oversized, 9500), std::invalid_argument);
    for (auto estimate : methods) CHECK_THROWS_AS(estimate(oversized, 9500), std::invalid_argument);
}

TEST_CASE("the documented maximum sample count is accepted") {
    const std::vector<double> sample(risk::max_observations, 3.0);
    for (auto estimate : methods) check_values(estimate(sample, 9750), 3, 3, 3, 3);
}

TEST_CASE("finite extreme loss magnitudes do not overflow weighted means") {
    const double limit = std::numeric_limits<double>::max();
    for (auto estimate : methods) {
        check_values(estimate({limit, limit, limit}, 5000), limit, limit, limit, limit);
        check_values(estimate({-limit, -limit, -limit}, 5000), -limit, -limit, -limit, -limit);
        check_values(estimate({limit, limit, -limit, -limit}, 5000), -limit, limit, 0, limit);
    }
}

TEST_CASE("tiny finite nonzero samples retain nonzero statistics") {
    const double tiny = std::numeric_limits<double>::denorm_min();
    for (auto estimate : methods) {
        const auto value = estimate({tiny, 2 * tiny, 3 * tiny}, 9500);
        CHECK(value.value_at_risk == 3 * tiny);
        CHECK(value.expected_shortfall == 3 * tiny);
        CHECK(value.mean_loss == 2 * tiny);
        CHECK(value.worst_loss == 3 * tiny);
    }
}

TEST_CASE("compensation preserves a small mean between cancelling extreme losses") {
    const double large = 1e300;
    for (auto estimate : methods) {
        for (const auto& sample : {std::vector<double>{large, 1, -large},
                                   std::vector<double>{-large, 1, large}}) {
            const auto value = estimate(sample, 5000);
            check_near(value.mean_loss, 1.0 / 3.0);
            CHECK(std::isfinite(value.expected_shortfall));
        }
    }
}
