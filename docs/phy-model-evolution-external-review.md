# A Reproducible Resource-Consistent PHY/MAC Abstraction for 5G Application Emulation

**Status:** External technical-review manuscript
**Model under test:** `feature/phy-model-v2`
**Validated simulator source:** `fe9307795130804fe41b9edf6730b35eb75cf162`
**Runtime-benchmark source:** `9862842045d0dc136e5c458546d09455efc7070b`
**Evidence protocol:** `docs/phy-model-v2-evidence-protocol.md`
**Date:** 2026-09-28

## Abstract

FikoRE is a packet-carrying, single-cell 5G emulator whose 1 ms loop must
remain fast enough to expose real applications to coverage, capacity, delay,
loss, and scheduling effects. It therefore uses a system-level PHY/MAC
abstraction rather than waveform processing. We identify three classes of
internal inconsistency in the previous abstraction: mixed bandwidth references
in the link budget, uplink power reuse across independently scheduled grants,
and proportional-fair (PF) history coupled to CQI update cadence. We introduce
a per-PRB signal/noise reference, allocation-aware two-stage uplink power
finalization, and a TTI-updated PF state with optional intra-TTI reranking. We
also activate 21 deterministic spatial maps with explicit origin and
provenance, separate map loss from runtime penetration, and make physical UE
environment state explicit.

Validation combines unit invariants, byte reproduction, 630 independently
seeded map realizations, five 180 s packet-level profiles, a controlled O2I
stress pair, and repeated timing over 1–256 UEs. All promoted numeric map
arrays exactly reproduce the reviewed candidates. Map LOS bias is at most
0.025 in the tested radial bins and axial shadow-correlation error is at most
0.008 at the nearest grid-representable lag. Grouped PF reranking materially improves
64-UE Jain fairness and maximum service gaps with sub-millisecond P99 cost on
the measured host, whereas distributed per-PRB 100/400 MHz grids exceed the
real-time budget. Windowed delivery metrics show that per-TTI zero grants need
not imply application-visible starvation, but UMa, RMa, and indoor UL retain
multi-second delivery gaps whose queue/scheduler/radio causes are not
separable from current logs. These results establish internal
consistency and reproducibility, not predictive validity. Multicell
interference, beam state, carrier aggregation, calibrated MIMO, and
link-to-system error prediction remain open design decisions.

## 1. Contributions and claim boundary

This work makes four falsifiable contributions.

1. **Resource-consistent link accounting.** Desired signal, thermal noise,
   aggregate interference, uplink power, and grant capacity share a per-PRB
   reference. Equivalent grouping does not change estimated SINR.
2. **Scheduler state with explicit temporal semantics.** PF achievable rate is
   refreshed by channel reporting, while service history is committed exactly
   once per active TTI using effective payload. Optional reranking uses only a
   provisional current-TTI denominator.
3. **Reproducible propagation inputs.** A 21-map catalog records generator
   version, seed, realization identity, coefficients, grid origin, and byte
   hash. The UE environment and penetration state are not hidden in the map.
4. **Controlled evidence.** Code, exact inputs, map bytes, summary data,
   confidence units, runtime samples, and figure generation are linked by a
   hash-verified manifest.

The work does **not** propose a new propagation model, prove 3GPP calibration,
validate the emulator against a held-out deployment, or claim a complete NR
PHY. The deliberately selected ABG path-loss family is preserved. Its external
validity, and that of the current MCS/BLER and MIMO abstractions, remains an
open calibration problem.

The original FikoRE publication positions the system as a modifiable RAN
emulator for application experimentation rather than a standards-calibration
simulator [21]. This work refines that existing architecture; its contribution
is audited resource/temporal semantics and reproducible evidence, not a new
general-purpose simulator. 5G-LENA and Simu5G provide broader system-level NR
models and calibration precedents [19], [20], while the Vienna methodology
demonstrates the missing link-to-system validation step [18].

## 2. Emulator scope and processing boundary

FikoRE executes one application-facing MAC step every
\(\Delta t=1\ \mathrm{ms}\). It carries generated or captured packets through
traffic, queue, scheduling, grant consumption, and delay-expiry logic. HARQ
timing/buffer scaffolding exists, but the compiled V2 branch disables the
BLER-driven retransmission decision. The physical abstraction supplies
resource-level rates; it does not synthesize IQ samples, reference signals,
decoding, or channel matrices.

![PHY Model V2 processing pipeline](figures/phy-v2-pipeline.svg)

The intended operating boundary is:

- one serving cell and one configured carrier;
- scenario-level large-scale gain with optional stochastic small-scale fading;
- resource-grid scheduling with selectable frequency/time aggregation;
- scalar antenna gains and threshold-based rank;
- actual packet queues and effective delivered payload;
- reproducible accelerated offline execution and configuration-dependent
  per-TTI timing measurements.

This boundary is narrower than 3GPP calibration simulators and wider than a
static link-budget calculator. In particular, packet demand can leave resources
unused even when radio capacity exists, and an allocated grant can carry less
effective payload because of packet size, queue state, or expiry.

## 3. Formal model

### 3.1 Grid and scheduling units

For numerology \(\mu\), subcarrier spacing is

\[
\Delta f = 15\cdot 2^\mu\ \mathrm{kHz}.
\]

Given usable RF bandwidth \(B_{\mathrm{RF}}\), the modeled PRB count is

\[
N_{\mathrm{PRB}}
=\left\lfloor\frac{B_{\mathrm{RF}}}{12\Delta f}\right\rfloor.
\]

A frequency allocation unit contains \(n_b\) PRBs and therefore
\(N_{\mathrm{SC},b}=12n_b\) subcarriers. A localized time unit spans all
numerology slots in the 1 ms TTI; a distributed unit schedules each slot
separately. The decision count per direction is

\[
N_{\mathrm{dec}}
=N_{\mathrm{freq\ units}}N_{\mathrm{time\ units}}.
\]

The current grid uses configured usable-bandwidth approximations and does not
apply a standards-table PRB cap. Grouped scheduling uses only complete RBGs;
any remainder PRBs are not represented. Canonical profiles are tested
explicitly rather than inferred to cover every NR bandwidth/numerology pair.

For TDD, the configured pattern determines the usable symbols for each
direction. A DL-ineligible UL slot, an UL-ineligible DL slot, and configured
transition symbols are intentional duplexing structure, not unassigned
scheduler resources. Reported grid fill is therefore

\[
U_d=
\frac{N_{\mathrm{assigned},d}}
     {N_{\mathrm{available\ units},d}},
\]

where the denominator contains physically available units for direction \(d\);
per-TTI summaries record structurally unavailable, empty, assigned, and
effective-payload units separately.

### 3.2 Deterministic macroscopic gain

The map stores macroscopic **link gain**, not positive path loss. For
LOS state \(q\in\{\mathrm{L},\mathrm{N}\}\),

\[
PL_q(d,f)
=10\alpha_q\log_{10}\!\left(
\frac{\max(d,1\ \mathrm{m})}{1\ \mathrm{m}}\right)
+\beta_q
+10\gamma_q\log_{10}\!\left(\frac{f}{1\ \mathrm{GHz}}\right),
\]

and the corresponding map field is

\[
G_q(\mathbf{x})=S_q(\mathbf{x})-PL_q(d(\mathbf{x}),f)
\quad[\mathrm{dB}],
\]

where \(S_q\) is zero-mean correlated shadowing. The target shadow covariance
is the Gudmundson form

\[
\rho_q(\Delta r)=\exp\!\left(-\frac{|\Delta r|}{d_{\mathrm{cor},q}}\right).
\]

V2.1 generates a correlated Gaussian field \(Z(\mathbf{x})\) by embedding the
target covariance in a grid twice as wide and cropping its center, transforms it to
\(U(\mathbf{x})=\Phi(Z(\mathbf{x}))\), and selects the binary LOS state

\[
L(\mathbf{x})
=\mathbb{1}\!\left[
U(\mathbf{x})\le p_{\mathrm{LOS}}(d(\mathbf{x}),h_{\mathrm{UT}})
\right].
\]

The final stored gain is

\[
G_{\mathrm{map}}(\mathbf{x})
=L(\mathbf{x})G_{\mathrm{L}}(\mathbf{x})
+(1-L(\mathbf{x}))G_{\mathrm{N}}(\mathbf{x}).
\]

The LOS marginal follows the retained scenario equations from TR 38.901
[1], including the corrected UMa dependence on UE height, except for the
shopping-mall profile, which is explicitly labelled as a legacy FikoRE
heuristic with pending provenance. UMa height is a fixed map-generation
parameter (1.5 m in the production catalog), not runtime per-UE state. This
does not make the complete map a TR 38.901 channel realization: path loss
remains the measurement-derived ABG family selected in the 2025 redesign.

V2 uses an odd 291×291 grid. For cell spacing \(c\), runtime coordinates map to

\[
i_x=\frac{x}{c}+\frac{N-1}{2},\qquad
i_y=\frac{y}{c}+\frac{N-1}{2},
\]

followed by bilinear interpolation. Thus \((0,0)\) is one explicit center cell.
Binary LOS semantics apply at generated nodes; off-grid interpolation blends
already-selected dB gains and is therefore an effective-gain field, not a
categorical LOS decision. V1 files retain their historical origin rule when
explicitly selected.

### 3.3 Runtime penetration and additional loss

The canonical configuration surface is

```ini
ue_location_type: outdoor | indoor | vehicle | random
building_penetration: none | low_loss | high_loss
vehicle_penetration: standard | metallized
```

The feature-branch-only key `location` is rejected. The older integer `o2i`
key is retained solely as a pre-feature migration path.

Additional runtime loss is

\[
L_{\mathrm{add}}
=L_{\mathrm{wall}}+0.5d_{\mathrm{in}}
+a_{\mathrm{O_2}}(f)\frac{d}{1000}
\quad[\mathrm{dB}]
\]

for an indoor UE with an enabled facade profile. The low- and high-loss wall
terms use the material-mixture form

\[
L_{\mathrm{wall}}
=\max\!\left[
0,\,
5-10\log_{10}
\left(\sum_m p_m10^{-L_m(f)/10}\right)+X_\sigma
\right],
\]

with \(\sigma=4.4\ \mathrm{dB}\) and \(6.5\ \mathrm{dB}\), respectively,
and one deterministic UE-shared draw. Indoor depth, building loss, and vehicle
loss are shared by DL and UL. Separate keyed streams isolate environment,
fast-fading, interference, and distance-CQI draws. Indoor-scenario profiles in
which both endpoints are indoors set facade penetration to `none`; they do not
apply an outdoor wall a second time. Oxygen loss applies independently of UE
environment type.

Vehicle loss is separate:

\[
L_{\mathrm{vehicle}}
=\max(0,\mu_v+5Z)\ \mathrm{dB},
\]

with \(\mu_v=9\ \mathrm{dB}\) for standard glazing and
\(20\ \mathrm{dB}\) for metallized glazing. Vehicle UEs do not receive the
building indoor-depth term.

### 3.4 Per-resource power, noise, and SINR

Downlink total carrier power \(P_{\mathrm{DL,tot}}\) is spread uniformly over
carrier PRBs:

\[
P_{\mathrm{DL,PRB}}
=P_{\mathrm{DL,tot}}-10\log_{10}N_{\mathrm{PRB}}
\quad[\mathrm{dBm}].
\]

Thermal noise is referenced to the same PRB:

\[
N_{\mathrm{PRB}}
=N_0+NF+10\log_{10}(12\Delta f)
\quad[\mathrm{dBm}],
\]

where canonical profiles use \(N_0=-174\ \mathrm{dBm/Hz}\).

For fractional uplink control, the nominal per-PRB power is

\[
P_{\mathrm{nom,PRB}}
=P_0+\alpha PL_{\mathrm{pc}}(\max[d-d_{\mathrm{in}},1\ \mathrm{m}]),
\]

and for \(M_i\) granted PRBs,

\[
\begin{aligned}
P_{i,\mathrm{tot}}
&=\operatorname{clip}\!\left(
P_{\mathrm{nom,PRB}}+10\log_{10}M_i,
P_{\min},P_{\max}\right),\\
P_{i,\mathrm{PRB}}
&=P_{i,\mathrm{tot}}-10\log_{10}M_i.
\end{aligned}
\]

The present implementation uses \(P_{\min}=10\ \mathrm{dBm}\) and
\(P_{\max}=23\ \mathrm{dBm}\); fixed-power mode treats the configured value as
total UE power. \(PL_{\mathrm{pc}}\) is the LOS ABG control-path estimate, not
the complete map/O2I realization. Both choices must remain visible because the
10 dBm floor and control-path simplification are not universal NR behavior.

The scheduler first evaluates UL candidates using nominal power corrected by
the most recently finalized allocation when power-limited. There is one such
state per UE: in localized mode it comes from the previous TTI, while later
distributed time groups inherit the preceding group's state. After all
assignments in the current time group are known, the emulator recomputes
\(P_{i,\mathrm{PRB}}\), SINR, MCS, and grant bits. It does not reschedule.
This prevents each independently considered grant from reusing the complete
23 dBm budget, but remains an order-dependent heuristic rather than a joint
power/scheduling optimum.

Let \(I_{\mathrm{PRB}}\) be configured aggregate co-channel interference in
linear power, \(F_b\) a small-scale term, \(v_i\) the selected rank, and
\(\Delta_i\) a configured SINR offset. Resource-level SINR is

\[
N\!I_{\mathrm{PRB,dBm}}
=10\log_{10}\!\left(
1000\,[N_{\mathrm{PRB,W}}+I_{\mathrm{PRB,W}}]
\right),
\]

\[
\Gamma_{i,b}
=P_{i,\mathrm{PRB}}+G_{\mathrm{tx}}+G_{\mathrm{rx}}
+G_{\mathrm{map}}(\mathbf{x}_i)-L_{\mathrm{add},i}
+F_{i,b}
-10\log_{10}v_i
-N\!I_{\mathrm{PRB,dBm}}
+\Delta_i.
\]

All remaining additive terms are in dB/dBm as appropriate.
Current interference is an aggregate per-PRB PSD approximation using a
configured interferer count, overlap ratio, random power, distance offset, and
LOS ABG loss. It has no neighbor geometry, load, beam, or scheduler state.

### 3.5 MCS, rank, and grant bits

With table mode enabled, MCS is

\[
m_{i,b}
=\max\{m:\Gamma_{i,b}\ge\theta_{m,n_b,\ell_{\mathrm{cfg}}}\},
\]

where thresholds depend on table, allocation-size class, and configured layer
count. The zero-based table axis is derived from the one-based layer count; an
off-by-one defect found during adversarial review was corrected and bounded for
one through four layers. If \(\Gamma_{i,b}<\theta_0\), FikoRE returns \(m=-1\),
sets spectral efficiency to zero, and the UE is not a positive-rate candidate
on that unit. This is an MCS-table boundary, not a separate configured
minimum-SINR admission rule.

For spectral efficiency \(\eta_m\), allocation subcarriers
\(N_{\mathrm{SC},b}\), usable symbols \(N_{\mathrm{sym},b}\), rank \(v_i\),
overhead \(o(f,d)\), and scaling \(s_i\), nominal grant bits are

\[
B_{i,b}
=v_iN_{\mathrm{SC},b}N_{\mathrm{sym},b}
\eta_m(1-o)s_i.
\]

The current rank abstraction is threshold-based in mean SINR, capped by UE
antenna count and configured layer count; UL rank is one. The selected MCS
threshold family remains tied to configured layers rather than dynamically
changing rank. The model neither represents a channel matrix nor predicts
post-processing layer SINR. Packet/queue handling maps nominal grant bits
\(B_{i,b}\) to effective payload \(B^{\mathrm{eff}}_{i,b}\le B_{i,b}\) for
new transmissions. Reported error throughput is queue/expiry accounting, not a
calibrated radio-BLER outcome. The disabled HARQ retry path is excluded from
the evidence and requires a grant-cap invariant before re-enablement.

### 3.6 Proportional-fair state and reranking

The implemented PF metric is

\[
M_{i,b}(t)
=\frac{w_i r_{i,b}(t)^\alpha}
       {\max(\bar R_i(t),\epsilon)},
\]

where \(w_i\) is UE priority, \(r_{i,b}\) is current achievable nominal rate,
and \(\alpha\) is cell-wide. Per-UE PF exponents are rejected.

Service history is committed once per active 1 ms TTI:

\[
\begin{aligned}
a&=1-\exp(-1/\tau_{\mathrm{ms}}),\\
\bar R_i(t+1)
&=(1-a)\bar R_i(t)+a\,x_i(t),
\end{aligned}
\]

where \(x_i(t)\) is effective payload served in the TTI. An active,
positive-rate, backlogged UE that receives no payload contributes zero. An
idle UE freezes state. Detach resets state, and cold start uses the standalone
achievable rate rather than epsilon. CQI cadence changes \(r_{i,b}\); it does
not gate the EWMA update.

In `allocation_unit` mode, after each planned grant the scheduler projects

\[
\tilde R_i(t,b)
=(1-a)\bar R_i(t)+a\,x^{\mathrm{nom}}_i(t,b)
\]

using cumulative nominal bits already planned in the current TTI, then reranks
the next unit. The committed EWMA still updates once, using effective payload.
For UL, all units are planned and provisionally reranked before final
allocation-aware power is known; the final power correction does not trigger a
second scheduling pass. Exact metric ties use a persistent round-robin cursor.

## 4. Accepted corrections

| Defect | Accepted correction | Verification |
|---|---|---|
| Invalid or ambiguous reference carriers | n40 20 MHz/30 kHz; UMa and RMa n78 100 MHz/30 kHz; n258 400 MHz/120 kHz; explicit power/gain/NF profiles | Profile parser/grid tests |
| One-PRB noise combined with grouped signal/capacity | Per-PRB signal, noise, and interference reference; grouped bits | 1/2/4/8/16-PRB SINR invariant |
| UL total power reused by independently considered grants | Nominal planning plus allocation-aware total-power finalization | 1–275 PRB power conservation and 23 dBm cap |
| PF exponent and history embedded in per-UE/CQI behavior | Common \(\alpha\); 1 ms EWMA of effective service; fair ties | State, metric, migration, homogeneous-scheduler tests |
| Same-TTI repeated winners in grouped grids | Configurable provisional allocation-unit reranking | Fairness/gap/runtime benchmark |
| One-based layer count used as zero-based MCS table index | Bounded `layers - 1` index shared by MCS and HARQ; bounded RI threshold loop | Layer/rank index regression test |
| Unseeded, origin-ambiguous production maps | 21 deterministic v2 files, odd grid, binary LOS, manifest, exact n40 map | Two-catalog byte equality; origin test; 630-realization study |
| O2I overloaded into one integer/map assumption | Explicit UE environment, building, and vehicle fields; indoor-gNB profiles omit facade loss | Unit formulas and controlled high-loss n258 pair |

## 5. Experimental method and artifacts

The protocol was fixed before final profile analysis
(`docs/phy-model-v2-evidence-protocol.md`). The validation host was `pitahaya`,
an AMD Ryzen 7 5800H with 8 cores/16 threads, Linux 5.15.0-58, g++ 11.4.0,
Python 3.10.4, and NumPy 2.2.6.

### 5.1 Map ensemble

Thirty independent master seeds (`20270000`–`20270029`) generated every one of
the 21 scenario-frequency entries: 630 maps total. Confidence intervals use
the realization as the independent unit; spatial cells are never counted as
replicates. Reported quantities are radial LOS probability, link-gain
quantiles, shadow moments, and axial correlation at the nearest
grid-representable physical lag. Per-realization shadow standard deviation is
normalized by construction and is therefore a generator sanity check, not
independent validation.

### 5.2 Packet-level profiles

Five profiles ran for 180 simulated seconds with seed `20260927`; analysis
excluded the first 20 seconds:

- UMi n40 NPN, 11 UEs;
- UMa n78 pedestrian, 21 UEs;
- RMa n78 vehicular, 11 UEs;
- indoor open-office n78, 11 UEs;
- outdoor UMi n258 FWA, 11 UEs.

An additional n258 pair changed only
`outdoor + no penetration` to `indoor + high-loss penetration`.

An outage UE has MCS below zero in at least 99% of post-warm-up radio samples.
Starvation is not inferred from a zero-grant TTI. For non-outage UEs, the
analysis uses non-overlapping 10 ms, 100 ms, and 1 s zero-delivery windows and
each UE's maximum observed delivery gap inside contiguous positive-offer
segments. Queue backlog is not logged, so these are application-delivery—not
scheduler-starvation—metrics.

### 5.3 Runtime

The PF envelope uses one process/thread, disabled verbose logging, 20 warm-up
TTIs, 100 measured TTIs, and ten independent timing repetitions per case.
Populations are 1, 16, 64, and 256 UEs. The low-resolution case is localized
grouped RBG; the high-resolution case is distributed per-PRB. Aggregate
offered traffic is approximately 4 Gbit/s per direction, divided over UEs.
This bounded full-backlog input replaces an invalid 100 Gbit/s-per-UE stress
configuration that measured queue growth rather than scheduler cost.

The complete matrix, commands, and limitations are in
`docs/baselines/phy-v2-validation-matrix.md`. Hashes are verified by
`tools/verify_phy_v2_evidence.py`; figures are regenerated by
`tools/generate_phy_v2_figures.py`.

## 6. Results

### 6.1 Map activation and ensemble behavior

Two independent production generations were byte-identical. All 21 promoted
numeric arrays are identical to the reviewed candidate arrays; only production
metadata changed. Historical candidate profile logs predate the subsequent
MCS-layer indexing correction and are not used as a current profile oracle.

Across 630 maps:

- maximum absolute radial LOS-probability bias was 0.025;
- maximum axial-correlation error at the sampled physical lag was 0.008;
- normalized shadow standard deviations equaled their configured values;
- exact-frequency UMi 2.38 GHz lookup replaced nearest 3.5 GHz lookup.

![LOS-state ensemble validation](figures/phy-v2-map-los.svg)

The map-only 0 dB full-channel-SNR proxy was 100.0% for UMi 2.38 GHz, 98.5%
for gain-assisted UMi 26 GHz, 78.5% for RMa 3.5 GHz, 38.2% for UMa 3.5 GHz,
and 26.4% for indoor open-office 3.5 GHz. These fractions cover the complete
generated square, including distances that are not representative indoor
layouts and may exceed source-fit ranges. They expose a geometry/range
limitation; they are not coverage predictions.

### 6.2 Five packet-level profiles

| Profile | Dir. | Offered | Delivered | Outage UEs | Zero-delivery 1 s | Maximum UE delivery gap | Grid assigned/effective | Payload/grant |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| Indoor n78 | DL | 65.00 | 46.22 | 0 | 0.1% | 1.51 s | 86.2% | 22.6% |
| Indoor n78 | UL | 90.00 | 44.50 | 0 | 3.6% | 44.81 s | 100.0% | 44.1% |
| RMa n78 | DL | 120.01 | 61.93 | 1 | 24.7% | 92.09 s | 97.3% | 98.6% |
| RMa n78 | UL | 120.01 | 26.70 | 0 | 30.2% | 43.70 s | 99.9% | 89.7% |
| UMa n78 | DL | 180.01 | 108.21 | 5 | 18.1% | 110.57 s | 100.0% | 71.9% |
| UMa n78 | UL | 70.00 | 14.16 | 1 | 38.4% | 160.00 s | 100.0% | 99.4% |
| UMi n258 | DL | 500.02 | 500.02 | 0 | 0.0% | 0.00 s | 98.7% | 38.1% |
| UMi n258 | UL | 200.01 | 156.73 | 0 | 0.1% | 0.01 s | 100.0% | 20.9% |
| UMi n40 | DL | 50.00 | 48.24 | 0 | 0.0% | 0.04 s | 100.0% | 74.9% |
| UMi n40 | UL | 35.00 | 24.24 | 0 | 0.2% | 0.03 s | 100.0% | 77.3% |

Rates are Mbit/s.

Three conclusions follow.

First, a zero grant in an individual TTI is normal. UMi n40 has many 10 ms
zero-delivery windows, yet almost no 1 s zero-delivery windows and maximum
observed gaps below 40 ms in the current run. Labeling every zero TTI as
application-visible starvation substantially over-reports the problem.

Second, some long application-delivery gaps are observed. UMa has a high
assignment ratio while several non-outage UEs still show gaps of 110–160 s.
RMa has both outage/low-MCS periods and substantial zero-delivery 1 s windows;
indoor UL has no permanent outage but one long gap. These observations combine
source state, queueing, scheduling, expiry, and radio state. They require
backlog/eligibility/reason instrumentation and multiple seeds before causal
classification.

Third, fill below 100% does not imply structural TDD loss. Per-TTI summaries
separate directionally unavailable units before the utilization denominator.
RMa assigns 97.3% of available DL and 99.9% of available UL units despite its
long per-UE gaps; the effect is distribution among UEs, not loss of the cell
resource pool. Indoor DL leaves 13.4% unused under finite demand. Empty-unit
reason should next distinguish no backlog from no positive-rate candidate.

The combined original-baseline-to-V2 change is large and intentionally not
attributed to maps alone:

![Original baseline versus PHY Model V2](figures/phy-v2-throughput.svg)

For example, n40 DL rises from 14.93 to 48.26 Mbit/s and its maximum
non-outage gap falls from 124.27 s to 0.16 s over the whole run. Conversely,
RMa and indoor throughput fall after the combined power/noise/PF/profile/map
changes. The one-factor candidate/production comparison, not this combined
figure, establishes map-activation parity.

The controlled high-loss n258 pair reduced median-UE SINR by 42.06 dB DL and
37.78 dB UL, but throughput changed only \(-0.9\%\) DL and \(-6.4\%\) UL
because equivalent FWA gains and offered demand keep DL near saturation. This
is a useful warning: throughput alone can hide a very large physical-model
change.

### 6.3 PF reranking, fairness, and runtime

At 64 homogeneous UEs, allocation-unit reranking improves grouped-grid
fairness and continuity consistently:

| Grid | Unit | Jain none → rerank | Maximum DL gap none → rerank | P99 runtime none → rerank |
|---|---|---:|---:|---:|
| 20 MHz, \(\mu=1\) | grouped | 0.935 → 0.998 | 102 → 19 TTIs | 149 → 153 µs |
| 100 MHz, \(\mu=1\) | grouped | 0.945 → 1.000 | 103 → 13 TTIs | 265 → 292 µs |
| 400 MHz, \(\mu=3\) | grouped | 0.961 → 1.000 | 85 → 22 TTIs | 295 → 344 µs |
| 20 MHz, \(\mu=1\) | per-PRB | 0.940 → 1.000 | 106 → 15 TTIs | 805 → 901 µs |
| 100 MHz, \(\mu=1\) | per-PRB | 0.925 → 0.983 | 105 → 48 TTIs | 4104 → 4281 µs |
| 400 MHz, \(\mu=3\) | per-PRB | 0.953 → 0.953 | 57 → 57 TTIs | 11836 → 10735 µs |

![PF reranking fairness and service gaps](figures/phy-v2-reranking.svg)

The production policy is therefore evidence-based: canonical grouped-RBG PF
profiles opt into reranking; the global default remains `none` for
high-resolution per-PRB experiments. The 400 MHz per-PRB case performs 2,000
decisions/TTI and gains no 64-UE continuity in this 120-TTI characterization.

Runtime is a property of the complete grid/population, not only reranking:

![Measured PF runtime envelope](figures/phy-v2-runtime.svg)

On the measured host, grouped P99 remains below 1 ms through 64 UEs for every
tested bandwidth. At 256 UEs, grouped reranking remains below 1 ms at 20 MHz
but reaches 1.04 ms at 100 MHz and 1.09 ms at 400 MHz. Distributed per-PRB
100 MHz crosses 1 ms at 16 UEs; both 400 MHz modes exceed 5.9 ms even at one
UE. At 256 UEs, per-PRB P99 reaches 16.27 ms for 100 MHz and 36.60 ms for
400 MHz. No general real-time claim is supported outside the tested grouped
envelope.

## 7. External validity and parameter realism

### 7.1 Carrier, power, gain, and noise assumptions

| Profile | Carrier | Total DL power | Scalar gains gNB/UE | NF gNB/UE |
|---|---|---:|---:|---:|
| UMi n40 | 2.38 GHz, 20 MHz | 43 dBm | 8.7/0 dBi | 2/9 dB |
| UMa n78 | 3.5 GHz, 100 MHz | 46 dBm | 8.7/0 dBi | 2/9 dB |
| RMa n78 | 3.5 GHz, 100 MHz | 46 dBm | 8.7/0 dBi | 2/9 dB |
| Indoor n78 | 3.5 GHz, 100 MHz | 24 dBm | 8.7/0 dBi | 2/9 dB |
| UMi n258 FWA | 26 GHz, 400 MHz | 35 dBm | 24/26 dBi | 7/10 dB |

The carrier widths and numerologies are valid single-carrier configurations
under TS 38.104 [2]. The powers are plausible classes of total-carrier values,
and the noise figures are plausible receiver abstractions, but they are not a
calibrated equipment table. In particular:

- the meaning of conducted power, TRP, and EIRP must remain explicit;
- 8.7 dBi is lower than many macro-sector antenna gains, while 43/46 dBm may
  already be interpreted differently by users;
- 24/26 dBi n258 gains are equivalent aligned FWA gains, not beamforming;
- the 10 dBm UL minimum and LOS-only power-control path loss are simplifications;
- receiver implementation loss is represented only through scalar NF and the
  MCS threshold data.

Thermal density \(-174\ \mathrm{dBm/Hz}\) is physically conventional. The
important correction is integrating it over the resource bandwidth rather
than replacing it with a fixed full-channel number.

### 7.2 Propagation and link adaptation

The ABG family was an explicit product decision based on the prior thesis and
published measurement fits. It is not automatically more valid than TR 38.901.
Sun *et al.* show that ABG parameters can be less stable than CI/CIF under
frequency/distance extrapolation [7], and the RMa literature specifically
warns about model range and height assumptions [8]. The current coefficient
values should therefore be accompanied by source campaign, fitted range,
height range, and extrapolation warnings before predictive coverage claims.

The 30-seed test validates generator behavior against its declared equations.
It does not validate those equations against a physical site. The wide indoor
map extent and low full-map coverage proxy reinforce the need for an approved
indoor geometry/range contract.

Current MCS thresholds have no committed link-level dataset, decoder
assumption, BLER target provenance, or held-out calibration. Current rank is a
scalar SINR threshold. Consequently, MCS, outage, and throughput are internally
reproducible but not yet externally calibrated NR predictions. Link-to-system
methodology requires per-MCS calibration and validation, as demonstrated by
EESM/MIESM work [15].

## 8. Open Phase 3 design space

No Phase 3 option is approved by this manuscript.

| Topic | Minimum useful option | Higher-fidelity option | State and cost | Required calibration |
|---|---|---|---|---|
| Multicell interference | Load-coupled per-resource interferer activity with explicit neighbor geometry | Wraparound multicell scheduler/channel realization | Minimum adds cell load state and a fixed point; full option multiplies scheduler work | 3GPP calibration layouts, load curves, RSRP/SINR joint distributions |
| Beamforming | Discrete serving/interfering beam states, aligned gain, sidelobe gain, misalignment/blockage loss, update cadence | 2D/3D array patterns and channel-aware beam selection | Discrete state is compatible with application emulation; full arrays require angle/channel state | Beam-sweep overhead, gain distributions, blockage and tracking traces |
| Carrier aggregation | Separate grid, numerology, CQI/MCS, queue/grant, and power budget per component carrier; aggregate above per-CC MAC | Cross-carrier scheduling and control overhead | Cannot be represented credibly by one wider carrier | UE band combinations, per-CC power, scheduler traces, guard/control overhead |
| MIMO | Direction-specific rank state and calibrated per-rank post-processing-SINR/BLER tables | Correlated channel matrices, precoding, codewords, receiver model | Table option is moderate; matrix option changes abstraction class | RI/rank distributions, antenna correlation, codebook/receiver and layer BLER |
| Link abstraction | Provenanced AWGN BLER curves plus explicit target and OLLA | EESM/MIESM across PRBs, HARQ mutual-information accumulation | Curves are the prerequisite; ESM adds per-codeword aggregation and calibration | Link-level traces per MCS/rank/receiver/channel; independent validation |

### 8.1 Multicell interference

The current aggregate random interferer can raise or lower SINR but cannot
represent the feedback loop

\[
\rho_c
=\sum_{u\in c}
\frac{d_u}
{W\log_2(1+\mathrm{SINR}_u(\boldsymbol{\rho}))},
\]

in which neighbor load controls resource overlap and interference. A
load-coupled model [12] is the lightest candidate that separates RSRP from
traffic-dependent SINR. The external reviewer should decide whether explicit
three-sector/hexagonal geometry is required or whether scenario-calibrated
neighbor gains are sufficient.

### 8.2 Beam state

Scalar FWA gain closes the link budget but omits beam search, alignment,
sidelobes, blockage, and tracking. A discrete state model should precede full
arrays:

\[
G(t)\in
\{G_{\mathrm{aligned}},G_{\mathrm{misaligned}},
G_{\mathrm{blocked}},G_{\mathrm{sidelobe}}\}.
\]

Transitions can be tied to mobility and a configured beam-management cadence,
with explicit sweep/control overhead. This captures application-visible
outages and latency without synthesizing a spatial channel. Giordani *et al.*
[13] provide the relevant beam-management state and overhead taxonomy.

### 8.3 Carrier aggregation

CA must not be emulated by setting one invalid channel bandwidth. Each
component carrier needs its own frequency, numerology, grid, map/channel state,
MCS, scheduler opportunity, and power allocation. UE support is constrained by
band combinations and per-CC features [5]. Aggregation belongs above those
per-carrier grants, with optional cross-carrier scheduling [14].

### 8.4 MIMO and link abstraction

A practical next MIMO model could expose

\[
(v,\Gamma_1,\ldots,\Gamma_v,n_{\mathrm{cw}},m_1,m_2)
\]

from calibrated lookup distributions conditioned on scenario, SINR, and
antenna configuration. This would separate antenna count from rank and avoid
multiplying one scalar efficiency by rank without layer-quality evidence.

Before EESM/MIESM, FikoRE needs reproducible AWGN BLER curves. Given per-PRB
SINR \(\gamma_k\), EESM would use

\[
\gamma_{\mathrm{eff}}
=-\beta_m\ln\!\left(
\frac{1}{K}\sum_{k=1}^{K}e^{-\gamma_k/\beta_m}
\right),
\]

where \(\beta_m\) is calibrated per MCS. MIESM replaces the exponential
mapping with modulation-specific mutual information and is better suited to
rank/HARQ extensions. OLLA can then update a scheduling offset from ACK/NACK
feedback toward a declared BLER target [17]. Adding any of these without a
link-level calibration set would increase complexity without increasing known
validity.

## 9. Threats to validity

1. Five-profile outcomes use one traffic/mobility seed. They characterize
   deterministic cases but do not provide confidence intervals over user
   placement or traffic.
2. Thirty map realizations test the generator, not ABG field accuracy. Shadow
   variance is normalized by construction.
3. Service is sampled from logs; windows below the log cadence are not
   observable. The analysis uses the actual 10 ms traffic-log cadence.
4. Outage classification changes if warm-up is included: one mobile RMa UE has
   enough initial valid samples to avoid whole-run outage but is post-warm-up
   outage.
5. The PF runtime run is 120 TTIs including warm-up. It is adequate for
   execution percentiles and short-horizon continuity, not long-run utility
   convergence.
6. P95/P99 are empirical interpolated quantiles of ten process-level samples;
   P99 is close to the observed maximum, not a high-confidence tail estimate.
   The host was not CPU-isolated.
7. Timing includes the measured emulator loop but excludes process/UE
   construction. Results apply only to the named host and bounded benchmark
   traffic.
8. Equivalent scalar gains make the n258 profiles demand-saturated and hide
   large SINR changes in aggregate throughput.
9. Interference, MIMO, MCS/BLER, and HARQ lack a common held-out calibration
   oracle. Their interactions can dominate field throughput.

## 10. Recommendations

1. Release the accepted core and v2 map catalog with the exact evidence
   manifest and keep grouped allocation-unit reranking profile-specific.
2. Expose empty-grid reasons directly: no backlog, no positive-rate candidate,
   disabled UE, or packet-layer limitation. Do not infer them from TDD plots.
3. Add multi-seed packet-level campaigns for UMa, RMa, and indoor UL before
   treating their long service gaps as expected distributions.
4. Complete the ABG coefficient provenance/range table and validate selected
   scenarios against independent RSRP/SINR measurements.
5. Establish a link-level BLER dataset before changing MCS, MIMO, EESM/MIESM,
   HARQ combining, or OLLA.
6. Ask the external reviewer to select minimum Phase 3 state, calibration
   sources, and runtime budget; do not implement multicell, beamforming, CA, or
   MIMO redesign from this paper alone.

## 11. Conclusion

PHY Model V2 removes dimensionally inconsistent resource accounting,
allocation-order UL power reuse, and CQI-coupled PF history. It activates a
deterministic, origin-consistent map catalog and separates physical environment
state from map semantics. The resulting implementation is reproducible and
passes explicit invariants.

The evidence also prevents an overly favorable conclusion. Per-TTI zero grants
are usually not starvation, TDD structure is not unreported resource loss, and
map activation reproduces reviewed candidates exactly. Nevertheless, long
windowed service gaps remain in several realistic profiles, and the current
interference, MIMO, and link-adaptation models are not externally calibrated.
The correct next step is targeted calibration and expert selection among the
Phase 3 abstractions, not additional unvalidated detail.

## References

1. 3GPP TR 38.901 v18.1.0, *Study on channel model for frequencies
   from 0.5 to 100 GHz*, Release 18, 2024.
2. 3GPP TS 38.104 v18.8.0, *NR; Base Station radio transmission and
   reception*, Release 18, 2025.
3. 3GPP TS 38.213 v18.7.0, *NR; Physical layer procedures for control*,
   Release 18, 2025.
4. 3GPP TS 38.214 v18.9.0, *NR; Physical layer procedures for data*,
   Release 18, 2026.
5. 3GPP TS 38.306 v18.8.0, *NR; User Equipment radio access
   capabilities*, Release 18, 2025.
6. J. M. Reyero Lobo, *Advancing 5G Network Emulation: Comprehensive
   Enhancements to FikoRE's Physical and MAC Layers*, Universidad Politécnica
   de Madrid, 2025.
7. S. Sun *et al.*, “Investigation of Prediction Accuracy, Sensitivity, and
   Parameter Stability of Large-Scale Propagation Path Loss Models for 5G
   Wireless Communications,” *IEEE Transactions on Vehicular Technology*,
   vol. 65, no. 5, 2016,
   <https://doi.org/10.1109/TVT.2016.2543139>.
8. G. R. MacCartney, Jr. and T. S. Rappaport, “Study on 3GPP Rural
   Macrocell Path Loss Models for Millimeter Wave Wireless Communications,”
   *IEEE ICC*, 2017, <https://doi.org/10.1109/ICC.2017.7996793>.
9. C. Zhang, X. Chen, H. Yin, and G. Wei, “Two-Dimensional Shadow Fading
   Modeling on System Level,” *IEEE PIMRC*, 2012,
   <https://doi.org/10.1109/PIMRC.2012.6362617>.
10. F. P. Kelly, “Charging and Rate Control for Elastic Traffic,”
    *European Transactions on Telecommunications*, vol. 8, no. 1, 1997,
    <https://doi.org/10.1002/ett.4460080106>.
11. H. J. Kushner and P. A. Whiting, “Convergence of Proportional-Fair
    Sharing Algorithms Under General Conditions,” *IEEE Transactions on
    Wireless Communications*, vol. 3, no. 4, 2004,
    <https://doi.org/10.1109/TWC.2004.830826>.
12. I. Siomina and D. Yuan, “Analysis of Cell Load Coupling for LTE Network
    Planning and Optimization,” *IEEE Transactions on Wireless
    Communications*, vol. 11, no. 6, 2012,
    <https://doi.org/10.1109/TWC.2012.051512.111532>.
13. M. Giordani, M. Polese, A. Roy, D. Castor, and M. Zorzi, “A Tutorial on
    Beam Management for 3GPP NR at mmWave Frequencies,”
    *IEEE Communications Surveys & Tutorials*, vol. 21, no. 1, 2019,
    <https://doi.org/10.1109/COMST.2018.2869411>.
14. K. I. Pedersen *et al.*, “Carrier Aggregation for LTE-Advanced:
    Functionality and Performance Aspects,” *IEEE Communications Magazine*,
    vol. 49, no. 6, 2011,
    <https://doi.org/10.1109/MCOM.2011.5783991>.
15. I. Latif, F. Kaltenberger, N. Nikaein, and R. Knopp, “Large Scale
    System Evaluations using PHY Abstraction for LTE with OpenAirInterface,”
    *SIMUTools*, 2013,
    <https://doi.org/10.4108/icst.simutools.2013.251738>.
16. B. Classon *et al.*, “Efficient OFDM-HARQ System Evaluation Using a
    Recursive EESM Link Error Prediction,” *IEEE WCNC*, 2006,
    <https://doi.org/10.1109/WCNC.2006.1696579>.
17. A. Sampath, P. S. Kumar, and J. M. Holtzman, “On Setting Reverse Link
    Target SIR in a CDMA System,” *IEEE VTC*, 1997,
    <https://doi.org/10.1109/VETEC.1997.600465>.
18. C. Mehlführer *et al.*, “The Vienna LTE Simulators—Enabling
    Reproducibility in Wireless Communications Research,” *EURASIP Journal on
    Advances in Signal Processing*, 2011,
    <https://doi.org/10.1186/1687-6180-2011-29>.
19. N. Patriciello, S. Lagen, B. Bojovic, and L. Giupponi, “An E2E Simulator
    for 5G NR Networks,” *Simulation Modelling Practice and Theory*, vol. 96,
    2019, <https://doi.org/10.1016/j.simpat.2019.101933>.
20. G. Nardini *et al.*, “Simu5G—An OMNeT++ Library for End-to-End
    Performance Evaluation of 5G Networks,” *IEEE Access*, vol. 8, 2020,
    <https://doi.org/10.1109/ACCESS.2020.3028550>.
21. D. González Morín, M. J. López-Morales, P. Pérez, A. García Armada, and
    Á. Villegas, “FikoRE: 5G and Beyond RAN Emulator for Application Level
    Experimentation and Prototyping,” *IEEE Network*, vol. 37, no. 4,
    pp. 48–55, 2023,
    <https://doi.org/10.1109/MNET.002.2200595>.
