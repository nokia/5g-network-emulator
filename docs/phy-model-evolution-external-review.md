# FikoRE Physical-Layer Model Evolution

## Validated Corrections, Design Constraints, and Open Research Decisions

**Status:** Technical decision paper for expert review  
**Branch baseline:** `dev` at `e995563fd3e8fa300f4b0accca3b494f7177101c`  
**Date:** 2026-09-24

## Abstract

FikoRE is a real-time, single-cell 5G RAN emulator intended for
application-level experimentation. Its physical layer deliberately favors a
computationally light system-level abstraction over a complete link-level or
3GPP calibration simulator. A 2025 redesign replaced the former simplified
3GPP TR 38.901 path-loss implementation with measurement-derived
Alpha-Beta-Gamma (ABG) models, introduced spatially correlated macroscopic
maps, refined O2I, noise, interference, mobility, and MIMO handling, and
validated selected outputs against a private 5G deployment, the Vienna LTE
simulator, and urban/rural field measurements.

Subsequent deterministic analysis and five 180-second offline FikoRE runs
confirm that the high-level abstraction is useful, but reveal correctness and
configuration issues that should be addressed before adding advanced channel
features. These include invalid reference-carrier configurations, inconsistent
signal/noise bandwidth under RBG scheduling, proportional-fair metrics with
per-UE exponents and CQI-coupled service history, non-reproducible map
realizations, and several map/O2I implementation questions.

This paper separates approved corrections from pending internal decisions and
open topics requiring external consultation. The proposed initial effort
corrects reference profiles, makes resource-level SINR dimensionally
consistent, replaces the legacy PF implementation with a TTI-updated model,
benchmarks optional intra-TTI reranking, and introduces deterministic map
provenance without changing the approved ABG path-loss family. Multicell
interference, beamforming, carrier aggregation, MIMO redesign, and advanced
link abstraction remain open design topics.

## 1. Scope and abstraction principles

FikoRE processes actual or simulated traffic in a 1 ms emulator loop. It
models a single serving cell, maps channel state to MCS and resource-level
capacity, schedules UEs on a time-frequency grid, and applies queueing, HARQ,
delay, and packet-loss behavior.

The intended abstraction is:

- accurate enough to expose applications to realistic coverage, capacity,
  latency, and loss trends;
- deterministic and reproducible when requested;
- scalable to many UEs and accelerated offline execution;
- easy to configure without requiring a complete site-planning model;
- modular enough to replace propagation and scheduling policies.

It is not intended to be:

- a ray-tracing or deterministic planning tool;
- a complete 3GPP multicell calibration simulator;
- a waveform-level PHY implementation;
- a substitute for deployment-specific antenna and interference planning.

These boundaries are important when deciding which corrections are necessary
for internal consistency and which advanced additions would change the nature
of the emulator.

## 2. Current physical-layer pipeline

The current logical pipeline is:

1. Resolve a scenario-frequency macroscopic map.
2. Interpolate a spatial value containing path loss, LOS/NLOS state, and
   correlated shadow fading.
3. Add runtime penetration, indoor-depth, vehicle, oxygen, antenna, and
   optional small-scale fading terms.
4. Estimate desired signal, thermal noise, and aggregate interference.
5. Calculate SINR, MCS, CQI, rank, and spectral efficiency.
6. Convert spectral efficiency and the scheduling unit into available bits.
7. Calculate a scheduler metric and allocate each time-frequency unit.
8. Pass grants to the packet/HARQ model.

### 2.1 Deliberate ABG path-loss design

The physical redesign documented by Reyero Lobo selected measurement-based ABG
models instead of directly applying the complete TR 38.901 path-loss
equations. The generic model is

\[
PL_{\mathrm{ABG}}(f,d)
=10\alpha\log_{10}\left(\frac{d}{1\,\mathrm{m}}\right)
+\beta
+10\gamma\log_{10}\left(\frac{f}{1\,\mathrm{GHz}}\right)
+X_\sigma .
\]

The decision was motivated by:

- concerns about extrapolating some standardized sub-6/RMa fits to mmWave;
- opaque environment parameters and corrective `min`/`max` operations;
- lower computational and configuration complexity;
- one uniform representation for multiple scenarios and bands;
- published measurement campaigns for the selected coefficients.

**Decision status:** **Approved for implementation continuity.** The initial
work shall not replace ABG with TR 38.901 path loss. Map-generation details and
coefficient provenance remain subject to internal review.

### 2.2 Spatial consistency

The map design targets Gudmundson correlation

\[
R(\Delta x)=\exp\left(-\frac{|\Delta x|}{d_{\mathrm{cor}}}\right).
\]

Independent LOS and NLOS path-loss and shadow fields are generated, filtered,
and combined through a spatial LOS state. A DFT/IDFT filter was selected after
evaluating local Cholesky, Gaussian, and Bessel alternatives.

This replaced independent per-UE shadow/LOS draws and ensures that nearby UEs,
as well as UL and DL for the same UE, observe coherent macroscopic conditions.

### 2.3 Small-scale fading

The emulator estimates coherence time as

\[
T_c=\frac{0.423}{f_D}
\]

and approximates coherence bandwidth from scenario delay spread. One
small-scale coefficient is reused over each frequency-coherence block.

The design document distinguishes Rayleigh NLOS and Rician LOS behavior, while
the current implementation predominantly applies Rayleigh fading. Whether to
add Rician LOS behavior is not approved in the initial scope.

### 2.4 Link budget and noise

The generic thermal-noise relation is

\[
N_{\mathrm{dBm}}
=-174+NF+10\log_{10}(B).
\]

Typical configured receiver noise figures are approximately 9 dB at the UE
and 2 dB at the gNB. The 2025 redesign intentionally changed noise from a
fixed full-channel value to a per-RB value to support per-RB SINR.

The current implementation can, however, combine one-RB noise with signal,
interference, and capacity defined over a multi-RB RBG. This is an abstraction
boundary error rather than a rejection of the per-RB design.

### 2.5 MCS, CQI, and throughput

When tables are enabled, FikoRE uses standard MCS/CQI spectral-efficiency
tables and externally generated SINR-BLER thresholds. Otherwise it uses an SNR
gap approximation.

The resource-level ideal data rate is represented as

\[
R
=v\,Q_m\,f\,R_c\,N_{\mathrm{SC}}\,B\,(1-OH),
\]

where \(v\) is the number of layers, \(Q_m\) is modulation order, \(R_c\) is
coding rate, \(N_{\mathrm{SC}}\) is the number of subcarriers in the
allocation unit, and \(B\) is the number of usable OFDM symbols.

The scheduler capacity is an ideal grant size. Packet errors, delay expiry,
and HARQ later reduce effective payload.

## 3. Evidence base

### 3.1 2025 thesis validation

The strongest direct validation evidence is:

- MT9 n40 modem mean RSRP: approximately -98.34 dBm;
- comparable FikoRE mean RSRP: approximately -100.89 dBm;
- SINR distributions within several dB in the matched setup;
- simulated MAC throughput following a modified Shannon reference;
- close SINR CDF alignment with a matched Vienna LTE scenario;
- qualitatively similar Round-Robin and Best-CQI throughput distributions.

The Vienna PF comparison showed a material scale difference, attributed to
CQI, metric, and scheduling granularity differences. It should be treated as a
qualitative warning rather than a strict validation.

Field campaigns were explicitly intended as empirical guidance rather than
direct FikoRE validation. Two parameter-level findings are relevant:

- urban NLOS fitted \(\alpha\approx3.47\), close to the configured 3.5;
- rural NLOS compensated shadow standard deviation was approximately 6.24 dB,
  close to the configured 6.7 dB.

Measurements also showed RSRP/SINR decoupling, strong height sensitivity in
rural NLOS, context-dependent vehicle penetration, and dynamic MIMO rank.

### 3.2 Five deterministic offline reference runs

Five canonical FikoRE profiles were run for 180 simulated seconds with seed
`20260924`. Comparing aggregate UL/DL throughput with the static theoretical
model yielded:

- throughput correlation: 0.91;
- mean absolute percentage error: 18.3%;
- mean absolute payload/grant efficiency difference: 3.6 percentage points.

Key outcomes:

- Indoor n78 confirmed no exterior-wall loss and near-equal aggregate
  throughput between FikoRE and the theoretical model.
- The 26 GHz FWA study UE remained in outage for the complete run.
- UMi n40 and UMa n78 confirmed severe PF service starvation of the nominally
  prioritized study UEs.
- RMa showed that mobility can rescue UEs classified as static outage, while
  still producing service gaps of tens of seconds.

These runs validate the theoretical model as an order-of-magnitude and MAC
diagnostic tool, not as a mobility/HARQ replacement.

## 4. Phase 0 — reference configuration corrections

### 4.1 Approved for implementation

- Keep UMi n40 at 20 MHz / 30 kHz.
- Keep UMa n78 at 100 MHz / 30 kHz.
- Represent the initial RMa profile as one valid 100 MHz n78 carrier.
- Rename the 26 GHz profile to n258.
- Represent the initial n258 profile as one valid 400 MHz / 120 kHz carrier.
- Keep CA out of the initial implementation.
- Use explicit equivalent scalar gains for early FWA profiles and state that
  they are not a beamforming model.

### 4.2 Pending internal review

The exact power/gain/NF table must declare whether each value means conducted
power, total carrier power, TRP, or EIRP. The following values remain to be
approved:

- RMa total carrier power;
- indoor local-area transmit power and antenna gain;
- n258 gNB and CPE equivalent gains;
- FWA outdoor, low-loss/window, and high-loss profile definitions.

No profile values should be changed until this table is reviewed.

## 5. Phase 1 — core corrections

### 5.1 Allocation-bandwidth consistency

**Status:** **Approved in principle; exact representation pending internal
review.**

For a fixed channel and power spectral density, estimated SINR must not change
only because the scheduler groups the same PRBs differently.

For an allocation unit containing \(N_{\mathrm{RB,unit}}\) PRBs,

\[
N_{\mathrm{unit,dBm}}
=-174+NF
+10\log_{10}
\left(N_{\mathrm{RB,unit}}\,12\,\Delta f\right).
\]

Signal, noise, interference overlap, MCS threshold context, and capacity must
refer to the same bandwidth. Two coherent implementations are possible:

1. estimate per-PRB SINR and aggregate bits; or
2. integrate signal/noise/interference over the allocation unit.

The implementation choice must preserve RBG-size invariance for equivalent
PSD and be verified against the SINR-BLER table assumptions.

### 5.2 New proportional-fair state

**Status:** **Approved for implementation.**

The target PF metric is

\[
M_{i,b}(t)=
\frac{w_i\,r_{i,b}(t)^\alpha}
{\max(\bar R_i(t),\epsilon)}.
\]

- \(r_{i,b}\) is refreshed by CQI.
- \(\bar R_i\) is service history updated every emulator TTI.
- \(\alpha\) is cell-level and common to all UEs.
- \(w_i\) is the UE priority.
- Exact ties use a persistent round-robin cursor.

The EWMA is

\[
\bar R_i(t+1)
=(1-a)\bar R_i(t)+a\,x_i(t),
\qquad
a=1-e^{-\Delta t/\tau}.
\]

Unserved active/backlogged UEs contribute \(x_i=0\). The exact \(\tau\),
cold-start value, detach/reattach policy, and migration treatment remain
pending internal review. A default \(\tau=100\) ms is proposed for evaluation.

The previous PF implementation does not require a runtime compatibility mode.
Historical behavior remains available through Git and deterministic baseline
artifacts.

### 5.3 Intra-TTI reranking

**Status:** **Pending benchmark and internal decision.**

The TTI EWMA update is fixed. The open choice is whether the scheduler updates
provisional current-TTI service after each allocation unit and reranks the
remaining candidates.

The experimental modes are:

```text
pf_intra_tti_update: none | allocation_unit
```

Both commit the EWMA once at TTI end.

The number of scheduler decisions per 1 ms is

\[
N_{\mathrm{decisions}}
=N_{\mathrm{frequency\ groups}}
\times N_{\mathrm{time\ groups}}.
\]

Representative values range from 17 decisions/ms for grouped, localized
100 MHz UMa to more than 2,000 decisions/ms for a distributed per-RB 400 MHz
FR2 grid. The benchmark must therefore cover every temporal/frequency
aggregation mode before a production default is selected.

## 6. Phase 2 — map and O2I review

**Overall status:** **Pending internal review. No semantic map change is
approved by this paper.**

### 6.1 Approved infrastructure work

- deterministic seed/RNG;
- batch scenario-frequency generation;
- map schema and manifests;
- source and coefficient provenance;
- requested versus selected frequency reporting;
- same-seed deterministic tests;
- statistical validation of shadow mean/std/autocorrelation;
- historical hash inventory;
- backward-compatible reading of current map files.

### 6.2 Decisions requiring review

1. UMa LOS height correction term.
2. Meaning of the smoothed LOS mask.
3. MATLAB floating-mask use of `~mapLOS`.
4. Even-grid origin and runtime indexing.
5. MATLAB/Python equivalence.
6. Approved ABG coefficient/source table.
7. Map dimensions and valid model ranges.
8. Exact-frequency policy, including n40 2.38 GHz.
9. Number of realizations distributed per profile.
10. Regression tolerances for replacing existing map bytes.
11. Vehicle-loss distribution.
12. Runtime versus precomputed O2I variation.
13. Rayleigh-only versus LOS Rician fading.

The selected ABG path-loss family remains unchanged unless a later supervised
decision explicitly replaces it.

## 7. Phase 3 — open design proposals

**Status:** **Open for external consultation. No production implementation is
approved.**

### 7.1 Multicell interference

Candidate abstractions:

- explicit scenario geometry with wraparound;
- stochastic-geometry interferers;
- scenario-calibrated interference distributions;
- load-coupled per-resource interference.

The expert should advise on the minimum model that separates RSRP from SINR
without turning FikoRE into a full network-planning simulator.

### 7.2 Beamforming

Potential progression:

1. equivalent scalar gain;
2. discrete beam state and alignment loss;
3. 2D/3D element and array patterns;
4. channel-aware beam selection.

The model must distinguish conducted power, TRP, EIRP, element gain, array
gain, blockage, and interferer sidelobes.

### 7.3 Carrier aggregation

A useful CA model requires:

- separate grid/numerology per component carrier;
- per-CC power allocation;
- CQI/MCS and scheduling per CC;
- UE CA capabilities;
- aggregation above the per-carrier MAC;
- duplicated control/guard overhead.

### 7.4 MIMO

The current equal-power, threshold-based rank abstraction should be reviewed
for:

- rank distribution;
- antenna versus layer count;
- per-layer/post-processing SINR;
- channel correlation;
- codeword count and MCS;
- UL versus DL rank;
- RI feedback cadence;
- interaction with beamforming.

### 7.5 Link abstraction

Candidate extensions:

- calibrated SINR-BLER curves with provenance;
- EESM/MIESM;
- outer-loop link adaptation;
- HARQ combining;
- finite TBS/blocklength effects.

## 8. Validation and performance methodology

Every accepted change should be validated at four levels:

1. Unit tests for equations, units, thresholds, and state updates.
2. Deterministic one-UE and homogeneous multi-UE characterizations.
3. The five canonical offline profiles.
4. Performance benchmarks with logging disabled.

Required metrics:

- SINR/MCS quantiles;
- outage;
- generated/delivered/error throughput;
- payload/grant efficiency;
- service-gap P50/P95/P99/max under backlog;
- zero-service windows;
- Jain fairness and demand satisfaction;
- wall-clock µs/TTI and real-time headroom.

The reranking benchmark should span:

- 1, 16, 64, 256+ UEs;
- valid 20, 100, and 400 MHz profiles;
- μ0, μ1, and μ3 where valid;
- localized/distributed time grouping;
- grouped/per-RB frequency allocation;
- homogeneous and near/far channels;
- full-buffer and finite demand.

## 9. Decision matrix

| Topic | Status | Required next decision |
|---|---|---|
| Preserve ABG family | Approved | None |
| n40/UMa valid carrier profiles | Approved | Exact power table |
| RMa 100 MHz single carrier | Approved | RMa power |
| n258 400 MHz / 120 kHz | Approved | gNB/CPE gains and power reference |
| Indoor local-area profile | Approved direction | Exact power/gain |
| Resource-consistent noise | Approved direction | Per-PRB vs allocation-unit representation |
| Common PF exponent | Approved | Exact config name/default |
| PF history updated per TTI | Approved | EWMA \(\tau\), cold start, detach policy |
| Intra-TTI reranking | Pending benchmark | Production default |
| Deterministic map tooling | Approved | Maintained generator language |
| Map semantic fixes | Pending internal review | Signed design note |
| Multicell/beamforming/CA/MIMO | Open | External expert guidance |

## 10. Questions for external review

1. What is the lightest multicell interference abstraction that preserves
   meaningful SINR/load behavior?
2. Is a discrete beam-state model sufficient for FWA and application-level
   experiments?
3. What minimum CA abstraction is needed to represent 2×100 and 2×400 MHz
   profiles without full PHY control modeling?
4. Which MIMO state variables are essential for credible rank/layer capacity?
5. Should link adaptation use EESM/MIESM and OLLA, or are calibrated
   resource-level SINR-BLER tables sufficient?
6. Which reference scenarios and measurements should be used to calibrate
   indoor, rural, and FR2 behavior?
7. What runtime budget should constrain these additions for real-time use?

## 11. References

1. J. M. Reyero Lobo, *Advancing 5G Network Emulation: Comprehensive
   Enhancements to FikoRE's Physical and MAC Layers*, UPM, 2025.
2. 3GPP TR 38.901, *Study on channel model for frequencies from 0.5 to
   100 GHz*.
3. 3GPP TS 38.104, *NR; Base Station radio transmission and reception*.
4. 3GPP TS 38.214, *NR; Physical layer procedures for data*.
5. T. S. Rappaport et al., *Study on 3GPP Rural Macrocell Path Loss Models
   for Millimeter Wave Wireless Communications*, IEEE ICC, 2017.
6. S. Sun et al., *Investigation of Prediction Accuracy, Sensitivity, and
   Parameter Stability of Large-Scale Propagation Path Loss Models for 5G
   Wireless Communications*, IEEE TVT, 2016.
7. C. Zhang et al., *Two-Dimensional Shadow Fading Modeling on System
   Level*, IEEE PIMRC, 2012.
8. G. Nardini et al., *Simu5G — An OMNeT++ Library for End-to-End
   Performance Evaluation of 5G Networks*, IEEE Access, 2020.
9. 5G-LENA, *MAC Features* and `NrMacSchedulerUeInfoPF` implementation.
10. C. Mehlführer et al., *The Vienna LTE Simulators — Enabling
    Reproducibility in Wireless Communications Research*, EURASIP, 2011.

## 12. Requested review outcome

The requested external output is not a single proposed architecture. It is:

1. confirmation or correction of the Phase 3 option space;
2. recommended minimum abstractions for FikoRE's application-level purpose;
3. priority ordering based on benefit, complexity, and validation cost;
4. suitable calibration datasets and acceptance criteria;
5. identification of assumptions that should remain configurable rather than
   fixed.
