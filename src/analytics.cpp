#include "risk/analytics.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace risk {
namespace {

// Neumaier summation retains small terms when a much larger partial sum is
// later cancelled. The arithmetic can be double-width on some platforms.
class CompensatedSum {
public:
    void add(long double value) {
        if (!valid_) return;
        const long double next = sum_ + value;
        if (!std::isfinite(value) || !std::isfinite(next)) {
            valid_ = false;
            return;
        }
        correction_ += std::abs(sum_) >= std::abs(value)
            ? (sum_ - next) + value : (value - next) + sum_;
        sum_ = next;
        valid_ = std::isfinite(correction_);
    }

    bool valid() const { return valid_ && std::isfinite(sum_ + correction_); }
    long double value() const { return sum_ + correction_; }

private:
    long double sum_{0.0L};
    long double correction_{0.0L};
    bool valid_{true};
};

// The visitor supplies (loss, integer mass) pairs. Summing weighted terms
// avoids overflowing an unweighted sum that would later be divided. A scaled
// fallback also handles a collection of subnormal values whose individual
// weighted terms would round to zero, and means at the finite range boundary.
template <class Visitor>
double weighted_average(const Visitor& visit, std::uint64_t total_mass) {
    const long double denominator = static_cast<long double>(total_mass);
    CompensatedSum ordinary;
    bool underflowed_term = false;
    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();
    visit([&](double value, std::uint64_t mass) {
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
        const long double weight = static_cast<long double>(mass) / denominator;
        const long double term = static_cast<long double>(value) * weight;
        underflowed_term = underflowed_term || (value != 0.0 && term == 0.0L);
        ordinary.add(term);
    });

    long double result = ordinary.value();
    if (!ordinary.valid() || underflowed_term) {
        const long double scale = std::max(std::abs(static_cast<long double>(minimum)),
                                           std::abs(static_cast<long double>(maximum)));
        if (scale == 0.0L) return 0.0;
        CompensatedSum normalized;
        visit([&](double value, std::uint64_t mass) {
            normalized.add((static_cast<long double>(value) / scale) *
                           static_cast<long double>(mass));
        });
        if (!normalized.valid()) {
            throw std::overflow_error("weighted loss calculation exceeded numeric range");
        }
        long double fraction = normalized.value() / denominator;
        // A weighted mean is bounded by its observations. This also prevents a
        // final rounding overshoot at max_double after the scaled calculation.
        fraction = std::clamp(fraction,
                              static_cast<long double>(minimum) / scale,
                              static_cast<long double>(maximum) / scale);
        result = fraction * scale;
    }
    if (!std::isfinite(result)) {
        throw std::overflow_error("weighted loss calculation is not finite");
    }
    result = std::clamp(result, static_cast<long double>(minimum),
                       static_cast<long double>(maximum));
    const double converted = static_cast<double>(result);
    if (!std::isfinite(converted)) {
        throw std::overflow_error("risk statistic is not representable as a finite double");
    }
    return converted;
}

Metrics finish(const std::vector<double>& original,
               const std::vector<double>& arranged, std::size_t boundary,
               std::uint64_t tail_mass, std::uint64_t fractional_mass) {
    const double mean = weighted_average([&](const auto& add) {
        for (double loss : original) add(loss, 1);
    }, static_cast<std::uint64_t>(original.size()));

    const double shortfall = weighted_average([&](const auto& add) {
        for (std::size_t i = boundary + 1; i < arranged.size(); ++i) {
            add(arranged[i], 10000);
        }
        if (fractional_mass != 0) add(arranged[boundary], fractional_mass);
    }, tail_mass);
    return {arranged[boundary], shortfall, mean,
            *std::max_element(original.begin(), original.end())};
}

Metrics estimate(const std::vector<double>& losses, unsigned confidence_bps,
                 bool full_sort) {
    validate(losses, confidence_bps);
    const std::uint64_t tail_mass = static_cast<std::uint64_t>(losses.size()) *
                                   (10000U - confidence_bps);
    const std::size_t whole_tail = static_cast<std::size_t>(tail_mass / 10000U);
    const std::uint64_t fractional_mass = tail_mass % 10000U;
    const std::size_t boundary = losses.size() - whole_tail - 1;
    std::vector<double> arranged(losses);
    if (full_sort) {
        std::sort(arranged.begin(), arranged.end());
    } else {
        std::nth_element(arranged.begin(), arranged.begin() +
                         static_cast<std::ptrdiff_t>(boundary), arranged.end());
    }
    return finish(losses, arranged, boundary, tail_mass, fractional_mass);
}

} // namespace

void validate(const std::vector<double>& losses, unsigned confidence_bps) {
    if (losses.empty() || losses.size() > max_observations) {
        throw std::invalid_argument("sample must contain between 1 and 1000000 losses");
    }
    if (confidence_bps == 0 || confidence_bps >= 10000) {
        throw std::invalid_argument("confidence must be between 1 and 9999 basis points");
    }
    for (double loss : losses) {
        if (!std::isfinite(loss)) {
            throw std::invalid_argument("all loss observations must be finite");
        }
    }
}

Metrics estimate_sort(const std::vector<double>& losses, unsigned confidence_bps) {
    return estimate(losses, confidence_bps, true);
}

Metrics estimate_partition(const std::vector<double>& losses, unsigned confidence_bps) {
    return estimate(losses, confidence_bps, false);
}

} // namespace risk
