/** @file analytics.hpp
 * @brief Empirical loss quantiles and fractional-tail Expected Shortfall.
 */
#pragma once

#include <cstddef>
#include <vector>

namespace risk {

/** Maximum accepted observation count; bounds allocation and integer products. */
inline constexpr std::size_t max_observations = 1000000;

/** Empirical loss statistics in the same currency units as the input. */
struct Metrics {
    double value_at_risk; ///< Nearest-rank quantile at the specified confidence.
    double expected_shortfall; ///< Average of the worst tail, including fractional mass.
    double mean_loss; ///< Arithmetic mean of all observations.
    double worst_loss; ///< Largest loss in the sample.
};

/** Validate observations and confidence.
 * @param losses Between 1 and max_observations finite signed losses.
 * @param confidence_bps Confidence from 1 to 9999 basis points.
 * @throws std::invalid_argument For invalid sizes, nonfinite data or confidence.
 */
void validate(const std::vector<double>& losses, unsigned confidence_bps);

/** Evaluate empirical risk by completely sorting a copy of the input.
 * @param losses Finite signed loss observations; input is not modified.
 * @param confidence_bps Integer confidence in basis points, from 1 to 9999.
 * @return Nearest-rank VaR, fractional-tail ES, mean and worst loss.
 * @throws std::invalid_argument For invalid data or confidence.
 * @throws std::overflow_error If numerical arithmetic cannot produce finite results.
 * @note Time O(n log n); auxiliary storage O(n), including the working copy.
 */
Metrics estimate_sort(const std::vector<double>& losses, unsigned confidence_bps);

/** Evaluate the same statistics using a partition at the VaR boundary.
 * @param losses Finite signed loss observations; input is not modified.
 * @param confidence_bps Integer confidence in basis points, from 1 to 9999.
 * @return Nearest-rank VaR, fractional-tail ES, mean and worst loss.
 * @throws std::invalid_argument For invalid data or confidence.
 * @throws std::overflow_error If numerical arithmetic cannot produce finite results.
 * @note Average time O(n); auxiliary storage O(n), including the working copy.
 *       No worst-case linear-time guarantee is made for std::nth_element.
 */
Metrics estimate_partition(const std::vector<double>& losses,
                           unsigned confidence_bps);

} // namespace risk
