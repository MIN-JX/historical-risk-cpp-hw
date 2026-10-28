# Sorting and partition selection

## Full sorting

Validate the data, copy it, and sort ascending. Read the VaR boundary by its
rank and accumulate the exact complete/fractional tail defined in the model.
This makes the reference estimator straightforward to inspect. Total work is
O(n log n), and the working copy occupies O(n) storage.

## Partition selection

Compute `k`, the number of complete observations in the upper tail. In zero-based
indexing the required boundary is `j=n-k-1`. Apply `std::nth_element` at `j`.
The boundary is the same value that full sorting would place there. The `k`
elements to its right are the worst complete tail; their internal order does not
matter to the mathematical sum. Add the fractional boundary contribution.

For the ten-value example at 80%, `k=2` and `j=7`. The boundary is 60. The two
values to its right are 80 and 100, in either order. ES is 90, while VaR is 60.

The non-parallel selection algorithm has average linear complexity in the
[C++ standard draft](https://eel.is/c++draft/alg.nth.element). This is an average
bound, not a worst-case linear guarantee. Validation, copying, mean calculation
and tail aggregation are linear too, so the complete estimator is O(n) on average
with O(n) working storage. Input generation is outside the benchmark; copying
and validation inside the public API remain inside its timing.

## Why the estimators are comparable

Both methods receive the same input and confidence, retain the original input,
and return the same four quantities. Benchmark setup checks their agreement
before recording timings. The tested convention uses exact integer tail weights;
neither method silently changes confidence or uses a rounded tail count.
Order-dependent floating-point summation can differ slightly, so comparisons
use a tolerance. An independent Decimal oracle and explicit hand calculations
check the definition rather than relying only on method agreement.

## Questions for code review

1. Is VaR the worst loss? It is a specified empirical quantile. It equals the
   maximum for some small samples or sufficiently high confidence levels.
2. Why does ES differ from the average of values above VaR? A discrete boundary
   may occupy only part of the required tail probability, or no part at all.
3. Why use basis points? Integer rank and tail arithmetic avoid ambiguous
   rounding of probabilities such as 0.8 times the sample count.
4. Why can VaR be negative? Negative losses are profits; a sample of profits can
   have a negative upper loss quantile. No truncation is imposed.
5. Does selection sort the right tail? No. It puts the boundary in the correct
   rank and partitions around it; tail summation does not require sorted order.
6. Does the faster method use constant memory? The public API preserves input,
   so its working copy makes auxiliary storage O(n).
7. Is a large speed ratio guaranteed? No. Small samples, allocation, compiler
   choices, and hardware can change the measured ratio.
8. Is this a forecast of tomorrow's loss? It is an empirical summary of the
   supplied observations. Forecast validation is outside the project.

