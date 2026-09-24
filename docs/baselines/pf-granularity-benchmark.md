# PF Intra-TTI Granularity Benchmark

## Purpose

This benchmark evaluates the experimental
`pf_intra_tti_update: allocation_unit` mode against `none` across the two time
aggregation modes and the two frequency aggregation modes.

The PF EWMA is committed once per 1 ms TTI in both modes. Allocation-unit mode
uses nominal scheduled bits only to update provisional ordering within the
current TTI.

## Method

- Static homogeneous UEs at 200 m.
- Full-buffer UL and DL traffic.
- No stochastic fading or external interference.
- PF alpha 1 and 100 ms EWMA.
- Single-threaded emulator.
- 50 simulated TTIs per case.
- One timing repetition in the initial screening run.
- Logs disabled.

Grids:

- 20 MHz / μ1;
- 100 MHz / μ1;
- 400 MHz / μ3.

UE populations: 16 and 64.

Aggregation:

- localized + grouped;
- distributed + grouped;
- localized + per-RB;
- distributed + per-RB.

The complete machine-readable results are in
[`pf-granularity-benchmark.csv`](pf-granularity-benchmark.csv).

## Aggregate findings

| Frequency mode | Median runtime change | Runtime range | Mean DL Jain gain | Mean maximum-gap reduction |
|---|---:|---:|---:|---:|
| Grouped RBG | -1.0% | -8.5% to +4.7% | +0.261 | 27.5 TTIs |
| Per-RB | +11.2% | -5.7% to +92.3% | +0.135 | 27.9 TTIs |

The negative timing values in some short cases are measurement noise; the
screening run is sufficient to distinguish negligible grouped-mode overhead
from potentially large high-resolution overhead, but not to establish a stable
performance regression threshold.

## Representative results

### 100 MHz / μ1 / 64 UEs

| Time mode | Frequency mode | Decisions/TTI | Reranking | µs/TTI | DL Jain | Max DL gap |
|---|---|---:|---|---:|---:|---:|
| Localized | Grouped | 17 | none | 37,556 | 0.450 | 47 |
| Localized | Grouped | 17 | allocation unit | 37,638 | 0.995 | 11 |
| Distributed | Grouped | 34 | none | 37,520 | 0.449 | 47 |
| Distributed | Grouped | 34 | allocation unit | 37,438 | 0.998 | 9 |
| Localized | Per-RB | 273 | none | 39,309 | 0.450 | 47 |
| Localized | Per-RB | 273 | allocation unit | 44,552 | 0.600 | 16 |
| Distributed | Per-RB | 546 | none | 42,514 | 0.447 | 47 |
| Distributed | Per-RB | 546 | allocation unit | 44,338 | 0.538 | 26 |

### 400 MHz / μ3 / 64 UEs

| Time mode | Frequency mode | Decisions/TTI | Reranking | µs/TTI | DL Jain | Max DL gap |
|---|---|---:|---|---:|---:|---:|
| Localized | Grouped | 15 | none | 39,877 | 0.625 | 47 |
| Localized | Grouped | 15 | allocation unit | 37,492 | 0.987 | 24 |
| Distributed | Grouped | 120 | none | 41,201 | 0.625 | 47 |
| Distributed | Grouped | 120 | allocation unit | 39,219 | 0.995 | 10 |
| Localized | Per-RB | 250 | none | 40,201 | 0.625 | 47 |
| Localized | Per-RB | 250 | allocation unit | 43,808 | 0.752 | 0 |
| Distributed | Per-RB | 2,000 | none | 44,258 | 0.476 | 47 |
| Distributed | Per-RB | 2,000 | allocation unit | 61,346 | 0.492 | 1 |

## Interpretation

1. Allocation-unit reranking provides a large fairness and continuity benefit
   in grouped-RBG modes.
2. The grouped-mode runtime effect is below the noise floor of this short
   screening benchmark.
3. Per-RB reranking can be materially more expensive, especially for
   distributed 400 MHz / μ3.
4. At very high decision counts, fairness after only 50 TTIs remains sensitive
   to TDD phase and finite observation duration; longer functional runs are
   required before comparing final Jain values.
5. Aggregate throughput changes little in most DL cases; the primary benefit is
   service continuity and fairness.

## Proposed production policy

- Keep both modes configurable.
- Use `allocation_unit` in the canonical grouped-RBG PF profiles.
- Keep `none` as the conservative default for per-RB high-resolution
  experiments until a longer repeated benchmark establishes the acceptable
  runtime envelope.
- Run a repeated timing suite on the target real-time host before changing the
  global parser default.

This policy is pending owner approval.
