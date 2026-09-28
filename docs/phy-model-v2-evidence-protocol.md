# PHY Model V2 Evidence Protocol

## Purpose

This protocol defines the claims, controls, statistics, and artifact
requirements for the FikoRE PHY Model V2 paper. It was frozen before production
map activation and before the final profile runs to prevent results-driven
changes to the evaluation method.

The protocol baseline is commit
`dc81da62f59ca93e36a02c36bdcecd45ec6cd2bd` on
`feature/phy-model-v2`. The model baseline inherited from `dev` is commit
`e995563fd3e8fa300f4b0accca3b494f7177101c`.

## Falsifiable claims

| ID | Claim | Required evidence | Failure condition |
|---|---|---|---|
| C1 | Signal, noise, interference, and capacity use the same allocation bandwidth. | Equation audit, power-model unit tests, and an RBG-grouping invariance experiment. | Equivalent PSD and PRBs produce a material SINR or aggregate-capacity change solely because of grouping. |
| C2 | Two-stage UL scheduling respects the UE total-power cap while exposing a deterministic nominal rate to the scheduler. | Unit tests for nominal and finalized powers plus a power-conservation sweep. | Final grants exceed the configured UE power or depend on allocation order under an equivalent grant set. |
| C3 | PF service history is updated once per TTI and is independent of CQI report cadence. | State and scheduler tests, a CQI-period invariance test, and service-window metrics. | The EWMA update count follows CQI reports or an eligible backlogged UE can remain unserved because its history is not aged. |
| C4 | Allocation-unit reranking improves short-window service continuity in grouped grids at acceptable measured cost. | Common-case PF benchmark with `none` and `allocation_unit`, Jain fairness, service-gap quantiles, and repeated runtime samples. | No continuity benefit is observed or measured cost exceeds the stated operating envelope. |
| C5 | V2 maps are deterministic, origin-consistent, and statistically compatible with their declared LOS and shadow models. | Byte reproduction, interpolation tests, and 30 independent realizations per catalog entry. | Equal inputs produce different bytes, runtime origin differs from generator origin, or ensemble confidence intervals miss declared targets beyond documented finite-grid tolerances. |
| C6 | Activating V2 maps changes only propagation inputs and produces explainable deltas in the five canonical profiles. | Common-seed, one-factor-at-a-time legacy/V2 comparisons. | Non-map configuration differs, candidate and promoted bytes differ, or output deltas cannot be traced to changed link gain. |

Passing these checks establishes internal consistency and reproducibility. It
does not establish deployment-wide predictive validity.

## Experimental controls

- Use deterministic RNG seeds and record every derived map seed.
- Use common random numbers for paired legacy/V2 and scheduler comparisons.
- Change one modeled factor at a time whenever the implementation permits it.
- Keep traffic, mobility, scheduler, duration, and packet settings identical
  in paired five-profile runs.
- Separate independent map realizations from correlated spatial samples.
- Report arithmetic definitions for every aggregate and denominator.
- Preserve raw logs outside Git; commit compact CSV/JSON summaries and the
  exact input configurations.
- Record source commit, dirty state, compiler, Python and NumPy versions,
  hostname, CPU model, kernel, command line, wall time, and return code.

## Canonical profile protocol

The canonical profiles are:

1. `offline_umi_n40_npn.ini`;
2. `offline_uma_n78_pedestrian.ini`;
3. `offline_rural_n78_vehicular.ini`;
4. `offline_indoor_hotspot_n78_pedestrian.ini`;
5. `offline_umi_n258_fwa.ini`.

Each production-map run uses seed `20260927` and 180 simulated seconds. Summary
statistics exclude the first 20 seconds when the timestamped source metric
supports warm-up filtering; metrics that cannot be filtered are explicitly
labelled as whole-run values. The report includes, by direction:

- offered, delivered, and error throughput;
- SINR and MCS quantiles;
- PHY-outage UE count;
- demand satisfaction;
- grant-to-payload efficiency;
- zero-service fractions in 10 ms, 100 ms, and 1 s windows;
- service-gap P50/P95/P99 and maximum for active, backlogged, non-outage UEs;
- wall-clock runtime.

An outage UE is one whose logged MCS is below zero in at least 99% of eligible
samples. Starvation is never inferred from a single TTI: it is reported through
time-window and service-gap statistics, separately from outage.

## Map ensemble protocol

The production master seed is `20260927`. Statistical validation uses 30
independent master seeds per scenario-frequency catalog entry; one map is
generated and discarded at a time.

For each realization, record:

- empirical LOS probability in fixed radial bins;
- LOS and NLOS shadow mean and standard deviation;
- axial shadow autocorrelation at fixed physical lags, including the declared
  decorrelation distance;
- final link-gain P05, P50, and P95;
- finite-value, dimension, origin, and metadata checks;
- a profile-specific coverage proxy where a canonical link budget exists.

Report the ensemble mean and a two-sided 95% confidence interval over
independent realizations. Spatial cells are not counted as independent
replicates. The confidence interval characterizes generator variability, not
field-measurement uncertainty.

## Runtime protocol

Functional results and timing results are separate. Timing uses the optimized
build, disabled verbose logging, a single emulator thread, repeated samples,
and the named host recorded in the evidence manifest. Report P50, P95, and P99
wall-clock microseconds per TTI. Real-time support may be claimed only for
tested cases whose stated percentile is below 1,000 microseconds per simulated
TTI.

## Paper structure

The final paper uses the following structure:

1. contributions and falsifiable claims;
2. formal system model and approximation boundary;
3. identified defects and accepted corrections;
4. experimental methodology and artifact manifest;
5. controlled results;
6. open Phase 3 option matrix;
7. limitations and conclusion.

Accepted decisions are presented with rationale and measured consequences.
Unresolved Phase 3 topics are presented as alternatives with state, accuracy,
runtime, calibration requirements, and external references; they are not
presented as implemented features.

## Review gates

The manuscript must pass independent propagation, PHY/link-abstraction,
MAC/statistics/runtime, and hostile editorial reviews. A finding is blocking if
it identifies a dimensionally invalid equation, unsupported causal claim,
irreproducible result, incorrect citation, hidden confounder, or materially
misleading validation claim. At least two complete review rounds are required,
with a maximum of four.
