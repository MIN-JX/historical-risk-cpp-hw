# Empirical risk definitions

## Loss convention and confidence

Observations are equally weighted losses in one currency and on one horizon.
A loss of 100 means losing 100 currency units; -20 means making a profit of 20.
The supplied data are synthetic examples. The estimator does not predict that
the same distribution will hold in the future.

Write the ascending observations as `L[1] <= ... <= L[n]`. Confidence is
`alpha=b/10000`, where `b` is an integer from 1 to 9999. This avoids computing
the rank by rounding an inexact floating-point probability.

## Value at Risk

The empirical CDF assigns probability `1/n` to each observation. Its inverse
at confidence `alpha` is the nearest-rank quantile:

```text
r = ceil(n * b / 10000)          # one-based rank
VaR = L[r]
```

This matches the inverse-empirical-CDF convention described as type 1 in the
[R statistics documentation](https://stat.ethz.ch/R-manual/R-devel/library/stats/html/quantile.html).
Other quantile conventions can interpolate and give different answers. This
project implements the displayed convention rather than a software default.

## Expected Shortfall

ES averages the worst `1-alpha` probability mass, not simply every observation
whose value is greater than or equal to VaR. The quantile-integral interpretation
handles probability atoms consistently. See Acerbi and Tasche,
[On the coherence of Expected Shortfall](https://arxiv.org/abs/cond-mat/0104295),
for the distinction from naive tail conditional averages. Their paper uses a
return/lower-tail convention; this implementation uses losses and the upper tail.

For this equally weighted empirical distribution, define:

```text
tail_units = n * (10000 - b)     # exact integer arithmetic
k = tail_units // 10000         # number of complete observations in the tail
f = tail_units % 10000          # fractional observation weight, in basis points
boundary = L[n-k]               # also the one-based VaR rank
ES = (10000 * sum(worst k losses) + f * boundary) / tail_units
```

An empty complete tail has sum zero. A zero fraction contributes nothing, even
though the boundary still defines VaR. Ties do not change the weighted result.
ES is at least VaR up to numerical rounding; neither value is clamped to zero.

## Worked examples

The sample losses in ascending order are
`[-50,-20,0,10,20,30,40,60,80,100]`, with mean 27 and maximum 100.

At 80%, `tail_units=20000`, so `k=2`, `f=0`. VaR is the eighth observation,
60. ES is the average of the worst two observations: `(80+100)/2=90`.
A filter `loss >= VaR` would also include 60 and give the wrong ES of 80.

At 82.5%, `tail_units=17500`, so `k=1`, `f=7500`. VaR is 80 and
`ES=(100+0.75*80)/1.75`, approximately 91.42857143. Rounding the tail count to
two would change the definition and give 90 instead.

At 95%, the tail occupies half an observation. Both VaR and ES are 100 in this
ten-value sample. More decimal output cannot compensate for limited sample size.

## Scope and numerical interpretation

The API accepts up to one million finite signed losses. Confidence is an integer
with basis-point resolution. The estimators copy the input and do not mutate it.
Mean and tail calculations use scaled, compensated arithmetic to limit overflow
and cancellation; outputs remain floating-point approximations.

The project calculates empirical summaries. It does not estimate confidence
intervals, backtest a forecasting model, add regulatory stress/liquidity-horizon
adjustments, or combine unaligned assets. A financial risk framework would need
those choices separately; the included 97.5% benchmark is only a parameter value.

