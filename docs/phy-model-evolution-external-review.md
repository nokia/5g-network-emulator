# Physical and MAC-Layer Abstractions in FikoRE for 5G Application Emulation

## Abstract

FikoRE is a single-cell 5G radio-access-network emulator designed to expose real or generated application traffic to controlled variations in coverage, capacity, latency, loss, mobility, and scheduling. Its physical layer is therefore a system-level abstraction: it represents the main relationships between position, propagation, received power, interference, modulation and coding, radio-resource allocation, and effective packet delivery, but it does not generate waveforms or emulate a complete receiver. This paper gives a self-contained description of the physical and MAC-layer model currently implemented in FikoRE, explains the simplifications that define its validity boundary, presents deterministic and statistical validation results, and describes the planned evolution toward multicell interference, explicit beam state, carrier aggregation, calibrated MIMO, and link-to-system error prediction. The current model uses deterministic spatial maps based on Alpha-Beta-Gamma path loss and correlated shadowing, explicit building and vehicle penetration states, resource-consistent signal and noise accounting, allocation-aware uplink power finalization, threshold-based link adaptation, and proportional-fair scheduling with optional intra-TTI reranking. Validation covers 630 independently seeded maps, five 180-second packet-level scenarios, a controlled penetration-loss comparison, and repeated scheduler timing for 1 to 256 UEs. The results establish deterministic reproducibility and internal consistency, but not deployment-level predictive accuracy. Long delivery gaps remain possible in challenging UMa and RMa scenarios, the present interference and MIMO abstractions are deliberately limited, and BLER-driven HARQ is not enabled in the evaluated implementation.

## 1. Purpose and modelling philosophy

FikoRE occupies a middle ground between a static link-budget calculator and a complete link- or system-level NR simulator. A link-budget calculator can estimate received power and theoretical capacity at one position, but it cannot expose an application to mobility, packet queues, TDD availability, scheduling competition, delay expiry, or changes in effective delivered payload. A full NR simulator can model many of those effects with substantially greater radio fidelity, but it also requires more scenario state, calibration data, runtime, and specialist knowledge. FikoRE instead models the radio mechanisms that are most visible to an application while retaining a 1 ms execution loop and a configuration surface that can be understood and modified without implementing a complete baseband chain.

The current model is intended to answer questions such as how offered traffic is divided among users with different radio conditions, how coverage and MCS affect nominal radio capacity, how uplink power limits interact with the number of allocated PRBs, how TDD and scheduler granularity affect service continuity, and how packet demand differs from nominal grid capacity. It is not intended to predict the exact BLER of a commercial receiver, perform site planning, reproduce beam-management procedures, or replace deployment-specific propagation calibration.

The original FikoRE architecture was introduced as an application-level RAN emulator rather than a standards-calibration simulator [21]. The physical-model work described here refines that architecture by making the bandwidth reference, uplink power semantics, map generation, penetration state, scheduler history, and validation boundary explicit. Comparable open simulators such as 5G-LENA and Simu5G provide broader NR system models and useful calibration precedents [19], [20], while the Vienna methodology illustrates the link-level calibration process that remains necessary before FikoRE can claim predictive MCS or BLER accuracy [18].

## 2. End-to-end architecture

FikoRE advances the serving cell in transmission time intervals of \(\Delta t=1\ \mathrm{ms}\). During each interval, UE positions are updated; when a channel-state refresh is due, the macroscopic map is sampled at the current position and penetration and small-scale effects are applied; the physical layer then supplies per-resource SINR and nominal capacity to the MAC scheduler, which allocates available time-frequency units before the packet queues consume the resulting grants. The packet layer records effective delivered bits and queue or expiry losses; these effective bits update scheduler history at the end of the TTI.

The evaluated model contains one serving cell and one configured carrier. It supports TDD and symmetric FDD, although all reference evidence uses TDD. A carrier is represented by a rectangular time-frequency grid. Frequency resources may be scheduled as individual physical resource blocks or grouped into resource-block groups, while the time dimension may be localized over the complete 1 ms interval or distributed over the numerology-dependent slots inside that interval.

The physical layer supplies nominal bits per scheduling unit. The packet layer may deliver fewer useful bits because a queue does not contain enough data, a packet does not fit the available grant, or a packet expires under its delay budget. The evaluated implementation contains HARQ timing and buffering structures, but its BLER-driven retransmission decision is disabled; consequently, the reported error throughput is queue- and expiry-related rather than a calibrated radio-decoding failure rate.

### 2.1 Nomenclature

| Symbol or term | Meaning |
|---|---|
| UE | User equipment attached to the emulated serving cell |
| DL / UL | Downlink from gNB to UE / uplink from UE to gNB |
| TTI | One 1 ms MAC scheduling interval |
| PRB | Physical resource block containing 12 adjacent subcarriers |
| RBG | Configurable group of complete PRBs used as one scheduling unit |
| CQI | Channel-quality indicator derived from the selected spectral efficiency |
| MCS | Modulation-and-coding index selected from an SINR-threshold table |
| RI / rank | Number of simultaneously modelled spatial layers |
| O2I | Outdoor-to-indoor or environmental penetration state |
| PF | Proportional-fair scheduler |
| \(N_{\mathrm{RB,car}}\) | Number of modelled frequency-domain PRBs in the carrier |
| \(n_b\) | Number of PRBs in scheduling unit \(b\) |
| \(\Gamma_{i,b}\) | SINR of UE \(i\) on scheduling unit \(b\) |
| \(B_{i,b}\) | Nominal capacity bits granted to UE \(i\) on unit \(b\) |
| \(B^{\mathrm{eff}}_{i,b}\) | Effective packet payload delivered from that grant |
| \(\bar R_i\) | PF exponentially weighted service history for UE \(i\) |

## 3. Time-frequency resource model

For numerology \(\mu\), FikoRE uses the NR subcarrier-spacing relation

\[
\Delta f = 15\cdot 2^\mu\ \mathrm{kHz}.
\]

Given a configured usable radio bandwidth \(B_{\mathrm{RF}}\), the frequency-domain PRB count is

\[
N_{\mathrm{RB,car}}=\left\lfloor\frac{B_{\mathrm{RF}}}{12\Delta f}\right\rfloor.
\]

A scheduling unit that groups \(n_b\) PRBs contains \(N_{\mathrm{SC},b}=12n_b\) subcarriers. Grouped scheduling uses only complete RBGs, so a carrier may contain a small remainder of modelled PRBs that is not represented by a scheduling unit. This is a resource-granularity limitation rather than an SINR-bandwidth ambiguity: total downlink carrier power is divided by the physical carrier PRB count, not by the number of scheduled groups.

Localized scheduling uses one time group over the complete TTI. Distributed scheduling divides the TTI into \(2^\mu\) time groups. The number of independent scheduler decisions per direction and TTI is therefore

\[
N_{\mathrm{dec}}=N_{\mathrm{freq\ units}}N_{\mathrm{time\ units}}.
\]

This decision count is a primary runtime parameter. A grouped 100 MHz grid may require only 17 decisions per direction and TTI, whereas a distributed per-PRB 400 MHz grid requires approximately 2,000.

For TDD, a configured symbol pattern determines which symbols are physically available to each direction. DL-ineligible UL symbols, UL-ineligible DL symbols, and transition symbols are structural duplexing resources and must not be counted as scheduler failures. FikoRE reports structural unavailability separately from available-but-empty, assigned, and effective-payload units. Directional assignment fill is

\[
U_d=\frac{N_{\mathrm{assigned},d}}{N_{\mathrm{available},d}},
\]

and effective fill uses the same denominator but counts only assigned units that produce positive effective payload.

The current implementation supports symmetric FDD bandwidth division. Asymmetric FDD is rejected because DL and UL presently share one carrier-grid descriptor; supporting asymmetric division safely would require direction-specific PRB, RBG, coherence, and HARQ state.

## 4. Current physical-layer model

### 4.1 Scenario geometry and spatial maps

The serving gNB is placed at the origin of a two-dimensional horizontal coordinate system. Each UE has a configured or randomly generated position and a mobility model. The physical model samples one deterministic spatial map for the selected scenario and carrier frequency. The production catalog contains Rural Macrocell, Urban Macrocell, Urban Microcell, indoor open office, indoor mixed office, and indoor shopping-mall maps at the frequencies required by the reference scenarios and associated studies.

Each current map contains 291 by 291 samples with one explicit center cell. If \(c\) is the map spacing and \(N=291\), runtime coordinates are transformed to map indexes as

\[
i_x=\frac{x}{c}+\frac{N-1}{2},\qquad i_y=\frac{y}{c}+\frac{N-1}{2}.
\]

The runtime obtains off-grid gain by bilinear interpolation. The generated nodes have a binary LOS or NLOS state, but interpolation between unlike nodes blends their already-selected dB gains; the runtime map should therefore be interpreted as a continuous effective large-scale-gain field rather than a categorical LOS decision at every possible coordinate.

Map dimensions are chosen to provide a reusable finite field, not to assert that a propagation fit is valid over the entire square. Current corner distances are approximately 1.03 km for UMi and shopping-mall maps, 0.62 km for office maps, and 3.79 km for UMa and RMa maps. Mobility emits a warning when the requested radius exceeds the map corner.

### 4.2 ABG path loss

The map stores negative path loss plus shadowing, expressed as link gain in dB. For propagation state \(q\in\{\mathrm{LOS},\mathrm{NLOS}\}\), the retained Alpha-Beta-Gamma model is

\[
PL_q(d,f)=10\alpha_q\log_{10}\!\left(\frac{\max(d,1\ \mathrm{m})}{1\ \mathrm{m}}\right)+\beta_q+10\gamma_q\log_{10}\!\left(\frac{f}{1\ \mathrm{GHz}}\right).
\]

The corresponding state-specific gain field is

\[
G_q(\mathbf{x})=S_q(\mathbf{x})-PL_q(d(\mathbf{x}),f),
\]

where \(S_q\) is correlated lognormal shadowing represented as a zero-mean Gaussian field in dB. The ABG family was selected in the earlier FikoRE physical-layer redesign because it provides one computationally simple structure across outdoor, indoor, sub-6-GHz, and millimetre-wave profiles [6]. The retained coefficients originate from measurement-derived fits, but their exact campaign ranges and extrapolation limits still require a consolidated provenance table before deployment-level predictions can be claimed.

### 4.3 LOS state and correlated shadowing

The target shadow covariance follows the exponential Gudmundson form

\[
\rho_q(\Delta r)=\exp\!\left(-\frac{|\Delta r|}{d_{\mathrm{cor},q}}\right),
\]

with a scenario- and state-specific decorrelation distance. A zero-mean Gaussian field is generated from this covariance by padded FFT embedding followed by center cropping. Padding prevents the opposite production-map edges from becoming artificial periodic neighbours.

The LOS state is generated from a separate correlated Gaussian field \(Z(\mathbf{x})\). Transforming that field with the standard normal cumulative distribution gives \(U(\mathbf{x})=\Phi(Z(\mathbf{x}))\), and the binary state is

\[
L(\mathbf{x})=\mathbb{1}\!\left[U(\mathbf{x})\le p_{\mathrm{LOS}}(d(\mathbf{x}),h_{\mathrm{map}})\right].
\]

The final node gain is

\[
G_{\mathrm{map}}(\mathbf{x})=L(\mathbf{x})G_{\mathrm{LOS}}(\mathbf{x})+\left(1-L(\mathbf{x})\right)G_{\mathrm{NLOS}}(\mathbf{x}).
\]

The RMa, UMi, UMa, and indoor-office LOS probabilities follow the retained scenario formulas from TR 38.901 [1]. The UMa height term is evaluated when the map is generated, using \(h_{\mathrm{map}}=1.5\ \mathrm{m}\) for the production catalog; runtime UE height does not regenerate the LOS state. The shopping-mall LOS probability is a legacy FikoRE heuristic and is explicitly not attributed to a fixed TR 38.901 shopping-mall equation.

### 4.4 Environmental and penetration loss

UE environment is represented independently from the macroscopic map. A UE can be configured as outdoor, indoor, inside a vehicle, or randomly assigned according to a scenario-specific probability. Explicit states are preserved in every scenario; only the random state invokes scenario probabilities. The building-penetration profile can be disabled or set to low-loss or high-loss, and the vehicle profile can use standard or metallized glazing.

For an indoor UE with an enabled facade profile, additional loss is

\[
L_{\mathrm{add}}=L_{\mathrm{wall}}+0.5d_{\mathrm{in}}+a_{\mathrm{O_2}}(f)\frac{d}{1000}\quad[\mathrm{dB}],
\]

where \(d_{\mathrm{in}}\) is indoor depth in metres and \(a_{\mathrm{O_2}}(f)\) is the frequency-dependent oxygen attenuation in dB/km. Oxygen attenuation is applied independently of environment type. An indoor scenario in which gNB and UE are already in the same building uses no exterior-facade loss.

Wall loss uses a material-mixture model

\[
L_{\mathrm{wall}}=\max\!\left[0,\ 5-10\log_{10}\!\left(\sum_m p_m10^{-L_m(f)/10}\right)+X_\sigma\right].
\]

For low-loss facades, the material weights are 0.3 standard glass and 0.7 concrete with \(\sigma=4.4\ \mathrm{dB}\). For high-loss facades, the weights are 0.7 IRR glass and 0.3 concrete with \(\sigma=6.5\ \mathrm{dB}\). Material losses are

\[
L_{\mathrm{glass}}=2+0.2f_{\mathrm{GHz}},\qquad L_{\mathrm{IRR}}=23+0.3f_{\mathrm{GHz}},\qquad L_{\mathrm{concrete}}=5+4f_{\mathrm{GHz}}\quad[\mathrm{dB}].
\]

A UE-shared environment stream supplies one building-loss draw, one vehicle-loss draw, and a deterministic sequence of indoor-depth candidates. For UMi and UMa, candidate depth is based on \(D_{\max}=25\ \mathrm{m}\); for RMa it uses \(D_{\max}=10\ \mathrm{m}\):

\[
d_j=\min(U_{j,1},U_{j,2})D_{\max}.
\]

At first channel evaluation, the first candidate satisfying \(d_j<d\) is selected, or the final candidate is clipped to the link distance. DL and UL use the same environmental realization, and separate keyed random streams prevent penetration configuration from shifting fast-fading or interference sequences.

Vehicle loss is

\[
L_{\mathrm{vehicle}}=\max(0,\mu_v+5Z)\quad[\mathrm{dB}],
\]

where \(\mu_v=9\ \mathrm{dB}\) for standard glazing and \(\mu_v=20\ \mathrm{dB}\) for metallized glazing. Vehicle UEs do not receive the building indoor-depth term.

### 4.5 Small-scale fading and spatial update

When stochastic fading is enabled, the resource-level power gain is

\[
F_{i,b}=10\log_{10}\!\left(\frac{X^2+Y^2}{2}\right),\qquad X,Y\sim\mathcal N(0,1),
\]

which has unit mean in linear power. Independent keyed streams are used for DL and UL. The present implementation applies Rayleigh fading to both LOS and NLOS links and holds or redraws the coefficient according to a coherence approximation expressed on scheduling units. Consequently, changing resource grouping can change the number and placement of stochastic fading samples even though deterministic carrier power and noise remain grouping-invariant. A future calibrated LOS model may replace universal Rayleigh fading with scenario-dependent Rician or beam-conditioned statistics.

The macroscopic map is spatially consistent by construction. Runtime samples the map at the current UE position whenever the SINR refresh cadence expires. The implementation also tracks movement relative to a scenario correlation distance and updates its reference position after that distance is exceeded, but this movement state does not currently gate map sampling; it is retained as preparation for a future spatial-consistency update policy.

### 4.6 Downlink power

Configured downlink power is interpreted as total power over the modelled carrier. It is distributed uniformly over the physical frequency-domain PRBs:

\[
P_{\mathrm{DL,PRB}}=P_{\mathrm{DL,tot}}-10\log_{10}N_{\mathrm{RB,car}}\quad[\mathrm{dBm}].
\]

This denominator is independent of RBG size. Grouping can still change the final represented capacity because only complete RBGs are scheduled and because the current stochastic fading and MCS-threshold resolution follow scheduling units.

### 4.7 Uplink power control

FikoRE supports fixed total UE power and a simplified fractional power-control mode. In fixed mode, the configured value is the UE total transmit-power budget. In fractional mode, the nominal per-PRB value is

\[
P_{\mathrm{nom,PRB}}=P_0+\alpha PL_{\mathrm{pc}}\!\left(\max[d-d_{\mathrm{in}},1\ \mathrm{m}]\right),
\]

where \(P_0\) is a configured nominal target, \(\alpha\) is the fractional path-loss compensation factor, and \(PL_{\mathrm{pc}}\) is a LOS ABG control-path estimate rather than the complete map and penetration realization. If UE \(i\) receives \(M_i\) PRBs, final power is

\[
\begin{aligned}
P_{i,\mathrm{tot}}&=\operatorname{clip}\!\left(P_{\mathrm{nom,PRB}}+10\log_{10}M_i,\ P_{\min},P_{\max}\right),\\
P_{i,\mathrm{PRB}}&=P_{i,\mathrm{tot}}-10\log_{10}M_i.
\end{aligned}
\]

The implementation currently uses \(P_{\min}=10\ \mathrm{dBm}\) and \(P_{\max}=23\ \mathrm{dBm}\). This is a reduced form of NR PUSCH power control [3]; it omits closed-loop commands, transport-format compensation, several numerology-dependent reference details, and simultaneous-carrier power sharing.

Uplink scheduling is two-stage. Candidate grants are first evaluated with the most recently finalized per-PRB power. After all assignments in one time group are known, the UE total power is redistributed over the current PRBs and SINR, MCS, and nominal bits are recomputed. The scheduler does not perform a second allocation pass after that correction, so the mechanism prevents repeated use of the complete UE power budget but remains an order-dependent heuristic rather than a joint power-and-scheduling optimum.

All reference UE classes use fixed total-power mode. Study UEs use 23 dBm. UMa, RMa, and n40 background UEs also use 23 dBm, whereas indoor and n258 background UEs use 10 dBm. Fractional power control has algebraic and UE-level finalization tests but has not yet been calibrated through a packet-level campaign.

### 4.8 Thermal noise and interference

Thermal noise is integrated over the same per-PRB bandwidth used by desired signal and interference:

\[
P_{N,\mathrm{PRB}}=N_0+NF+10\log_{10}(12\Delta f)\quad[\mathrm{dBm}],
\]

where \(N_0=-174\ \mathrm{dBm/Hz}\) in the reference configurations and \(NF\) is receiver noise figure. Noise and interference are combined in linear power:

\[
P_{N+I,\mathrm{PRB,dBm}}=10\log_{10}\!\left(1000\left[P_{N,\mathrm{PRB,W}}+P_{I,\mathrm{PRB,W}}\right]\right).
\]

The current interference model is an aggregate per-PRB surrogate. It uses a configured interferer count, overlap ratio, random power, distance offset, and LOS ABG attenuation. It has no neighbouring-cell geometry, load state, antenna pattern, beam state, or neighbour scheduler. The UL surrogate samples a per-PRB value over \([-23,+23]\ \mathrm{dBm}\) and omits interferer penetration and antenna gain; it must not be interpreted as a calibrated total-power UE model or as deployment-predictive received interference.

### 4.9 Resource-level SINR

Let \(G_{\mathrm{map}}(\mathbf{x}_i)\) be the interpolated map gain of UE \(i\), \(L_{\mathrm{add},i}\) its environmental loss, \(G_{\mathrm{tx}}\) and \(G_{\mathrm{rx}}\) scalar antenna gains, \(F_{i,b}\) small-scale power gain, \(v_i\) rank, and \(\Delta_i\) an optional configured offset. Resource-level SINR is

\[
\Gamma_{i,b}=P_{i,\mathrm{PRB}}+G_{\mathrm{tx}}+G_{\mathrm{rx}}+G_{\mathrm{map}}(\mathbf{x}_i)-L_{\mathrm{add},i}+F_{i,b}-10\log_{10}v_i-P_{N+I,\mathrm{PRB,dBm}}+\Delta_i.
\]

All additive terms are expressed in dB or dBm according to their role. The equal-power rank penalty is a scalar abstraction; it is not a post-processing SINR obtained from a channel matrix.

### 4.10 MCS, rank, and nominal capacity

With table-based adaptation enabled, MCS is selected as

\[
m_{i,b}=\max\left\{m:\Gamma_{i,b}\ge\theta_{m,n_b,\ell_{\mathrm{cfg}}}\right\},
\]

where \(\theta\) depends on MCS, allocation-size class, and configured layer count. If SINR is below the first threshold, MCS is set to \(-1\), spectral efficiency is zero, and the UE is not a positive-rate candidate on that unit. This threshold is an internal table boundary, not a separately configured minimum-SINR admission rule.

For spectral efficiency \(\eta_m\), usable symbols \(N_{\mathrm{sym},b}\), subcarriers \(N_{\mathrm{SC},b}\), rank \(v_i\), direction- and frequency-dependent overhead \(o\), and scaling factor \(s_i\), nominal capacity is

\[
B_{i,b}=v_iN_{\mathrm{SC},b}N_{\mathrm{sym},b}\eta_m(1-o)s_i.
\]

This quantity is a continuous capacity proxy, not an NR transport-block calculation. It does not perform TBS rounding, CRC and code-block segmentation, rate matching, DMRS allocation, or codeword mapping. MCS thresholds likewise have no committed link-level receiver and BLER calibration dataset.

DL rank is selected from thresholds applied to mean SINR and is bounded by configured UE antennas and available layers; UL rank is one. The table axis is safely converted from one-based layer count to zero-based storage. Rank above one remains weakly modelled: selection observes SINR after the previous rank's equal-power penalty, no hysteresis is applied, and MCS thresholds remain tied to configured layers rather than selected dynamic rank. The evaluated reference profiles are rank one.

## 5. Interaction with packet handling and MAC scheduling

### 5.1 Nominal grants and effective payload

The scheduler assigns nominal capacity bits \(B_{i,b}\), but the packet layer reports effective bits \(B^{\mathrm{eff}}_{i,b}\). Effective payload can be lower because a queue does not contain enough bits, because grant and packet boundaries do not align, or because packets expire. Resource summaries therefore distinguish available units, assigned units, units with positive effective payload, and assigned units that produce no effective payload.

The current BLER-driven HARQ decision is disabled. HARQ state must not be re-enabled without calibrated link-error curves, actual grant rank and allocation context, and an invariant that prevents a retry from delivering more bits than its current grant.

### 5.2 Proportional-fair scheduler

The PF metric is

\[
M_{i,b}(t)=\frac{w_i r_{i,b}(t)^\alpha}{\max(\bar R_i(t),\epsilon)},
\]

where \(w_i\) is priority, \(r_{i,b}\) is the current nominal achievable rate, \(\alpha\) is a cell-wide throughput-versus-fairness exponent, and \(\bar R_i\) is service history. This rate-over-history structure follows established proportional-fair scheduling principles [10], [11].

PF history is updated once per active 1 ms TTI:

\[
\begin{aligned}
a&=1-\exp(-1/\tau_{\mathrm{ms}}),\\
\bar R_i(t+1)&=(1-a)\bar R_i(t)+a\,x_i(t),
\end{aligned}
\]

where \(x_i(t)\) is effective payload served during the TTI. An active positive-rate UE that receives no effective payload contributes zero. An idle UE freezes its history, detach resets it, and a newly active UE starts from its standalone achievable rate. CQI reporting changes the instantaneous numerator but does not control the service-history update cadence.

Exact metric comparisons use an absolute tolerance of \(10^{-6}\). Candidates within that tolerance share a persistent round-robin tie cursor so container order does not define long-term service.

### 5.3 Intra-TTI reranking

Without intra-TTI reranking, all allocation units in a TTI see the same committed PF denominator, so a user with the best initial metric can win several units before history changes. In `allocation_unit` mode, the scheduler maintains a provisional current-TTI service value

\[
\tilde R_i(t,b)=(1-a)\bar R_i(t)+a\,x_i^{\mathrm{nom}}(t,b),
\]

where \(x_i^{\mathrm{nom}}(t,b)\) is cumulative nominal service already planned in the current TTI. The next unit is reranked with this projected denominator. At TTI end, only one committed update is made, using effective payload rather than provisional nominal bits.

For UL, planning and provisional reranking occur before allocation-aware power is finalized. Final power and rate corrections do not trigger a second scheduling pass. This preserves bounded runtime but is an explicit approximation.

## 6. Reference physical configurations

The following profiles define representative, internally consistent single-carrier configurations used for validation. They are not universal deployment defaults.

| Profile | Scenario and carrier | DL carrier power | Scalar gNB/UE gain | NF gNB/UE | UL power |
|---|---|---:|---:|---:|---|
| UMi n40 NPN | UMi, 2.38 GHz, 20 MHz, \(\mu=1\) | 43 dBm | 8.7/0 dBi | 2/9 dB | 23 dBm |
| UMa n78 pedestrian | UMa, 3.5 GHz, 100 MHz, \(\mu=1\) | 46 dBm | 8.7/0 dBi | 2/9 dB | 23 dBm |
| RMa n78 vehicular | RMa, 3.5 GHz, 100 MHz, \(\mu=1\) | 46 dBm | 8.7/0 dBi | 2/9 dB | 23 dBm |
| Indoor n78 pedestrian | Indoor open office, 3.5 GHz, 100 MHz, \(\mu=1\) | 24 dBm | 8.7/0 dBi | 2/9 dB | 23 dBm study UE; 10 dBm background |
| UMi n258 FWA | UMi, 26 GHz, 400 MHz, \(\mu=3\) | 35 dBm | 24/26 dBi | 7/10 dB | 23 dBm study UE; 10 dBm background |

Configured DL power is treated as total carrier power. The distinction between conducted power, total radiated power, and EIRP must remain explicit when mapping these profiles to equipment. The n258 gains are equivalent aligned FWA gains and do not constitute a beamforming model.

## 7. Validation methodology

### 7.1 Deterministic and unit-level validation

Implementation-level tests verify per-PRB thermal noise, physical-carrier power distribution, deterministic grouped-versus-per-PRB SINR, total UL power conservation over 1 to 275 PRBs, FR1 and FR2 overhead selection, MIMO table indexes, CQI-independent PF history cadence, TDD structural-resource accounting, shared DL/UL environment state, map origin, exact-frequency lookup, and rejection of unsupported asymmetric FDD. Map catalogs and evidence artifacts are hash-verified recursively.

### 7.2 Map ensemble

Thirty independent master seeds are evaluated for each of the 21 scenario-frequency entries, giving 630 maps. The realization, not the spatial cell, is the independent statistical unit. Diagnostics include radial LOS probability over the inscribed disk, link-gain quantiles, shadow mean and standard deviation, axial correlation at the nearest representable target lag, and opposite-edge correlation to detect periodic seams. Confidence intervals are pointwise and describe generator variability; they are not simultaneous goodness-of-fit tests or field-prediction intervals.

### 7.3 Packet-level profiles

Five profiles are run for 180 simulated seconds with one common seed and a 20-second analysis warm-up. The profiles contain 11 UEs for UMi n40, RMa, indoor n78, and n258, and 21 UEs for UMa n78. Reported quantities include offered and delivered throughput, queue or expiry errors, SINR and MCS quantiles, sampled outage classification, non-overlapping zero-delivery windows, observed delivery gaps inside contiguous positive-offer segments, resource assignment, effective-unit fill, and sampled payload-to-grant efficiency.

A UE is classified as sampled radio outage when its MCS is below zero in at least 99% of post-warm-up radio samples. Delivery gaps are not scheduler-starvation measurements because queue backlog is not present in the current logs and the observations combine traffic generation, queueing, scheduling, expiry, and radio state.

### 7.4 Controlled map and penetration comparisons

The legacy-map and current-map batches use identical code, traffic, mobility, seed, duration, and analysis; only the selected map catalog differs. The historical n40 batch uses the previous nearest 3.5 GHz UMi map, while the current catalog includes an exact 2.38 GHz map.

The penetration comparison uses the same map, traffic, mobility, powers, gains, scheduler, duration, and keyed fast-fading and interference streams. The control arm is outdoor with no building penetration; the stress arm is indoor with high-loss penetration. The experiment is a one-seed controlled ablation rather than a population estimate.

### 7.5 Runtime and PF functional evaluation

Runtime tests use one emulator thread, disabled verbose logging, 20 warm-up TTIs, 100 individually timed TTIs, and ten process repetitions per case, giving 1,000 TTI timings. Populations are 1, 16, 64, and 256 UEs, and the benchmark spans 20 MHz, 100 MHz, and 400 MHz grouped and distributed per-PRB grids. The 1 ms threshold is a compute-budget threshold for this host and benchmark configuration, not an end-to-end real-time guarantee.

PF functional behavior is measured separately for 64 UEs after 500 warm-up TTIs and over 2,000 measured TTIs. This duration is long relative to the configured 100 ms EWMA window but does not constitute a proof of asymptotic PF convergence.

## 8. Validation results

### 8.1 Spatial-map diagnostics

Two independent generations of the complete 21-map catalog are byte-identical. Across the 30-realization ensembles, the largest absolute bias of ensemble-mean radial LOS probability is 0.038, the largest absolute error of ensemble-mean axial shadow correlation is 0.007, and the largest absolute ensemble-mean opposite-edge correlation is 0.026. Per-realization shadow standard deviation matches the configured value because each field is normalized by construction.

The map-only full-channel 0 dB SNR proxy is 100.0% for UMi 2.38 GHz, 98.5% for gain-assisted UMi 26 GHz, 78.5% for RMa 3.5 GHz, 38.2% for UMa 3.5 GHz, and 26.4% for indoor open office at 3.5 GHz. These fractions cover the complete stored map, including distances outside typical indoor layouts and potentially outside the source measurement ranges; they are diagnostics, not coverage predictions.

In the same-code fixed-seed legacy-versus-current catalog comparison, current maps change DL throughput by \(-2.0\%\) for indoor n78, \(+14.1\%\) for RMa n78, \(-8.5\%\) for UMa n78, approximately \(0\%\) for demand-saturated n258, and \(-2.5\%\) for n40. These are one-realization deltas rather than expected field-performance changes.

### 8.2 Packet-level reference profiles

| Profile | Dir. | Offered (Mbit/s) | Delivered (Mbit/s) | Sampled outage UEs | Zero-delivery 1 s | Maximum observed UE delivery gap | Assigned/effective units | Payload/grant |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| Indoor n78 | DL | 65.00 | 42.54 | 0 | 0.0% | 0.81 s | 86.7% / 86.7% | 21.4% |
| Indoor n78 | UL | 90.00 | 60.35 | 0 | 0.0% | 0.09 s | 100.0% / 60.8% | 46.5% |
| RMa n78 | DL | 120.01 | 51.82 | 0 | 18.6% | 102.09 s | 96.0% / 96.0% | 98.7% |
| RMa n78 | UL | 120.01 | 26.28 | 0 | 3.2% | 5.10 s | 99.9% / 57.3% | 85.0% |
| UMa n78 | DL | 180.01 | 103.80 | 2 | 10.4% | 76.76 s | 99.0% / 99.0% | 73.5% |
| UMa n78 | UL | 70.00 | 13.37 | 0 | 28.2% | 160.00 s | 100.0% / 17.8% | 100.0% |
| UMi n258 | DL | 500.02 | 500.02 | 0 | 0.0% | 0.00 s | 99.5% / 99.5% | 39.5% |
| UMi n258 | UL | 200.01 | 157.05 | 0 | 0.0% | 0.02 s | 100.0% / 28.8% | 22.1% |
| UMi n40 | DL | 50.00 | 46.55 | 0 | 0.0% | 0.07 s | 100.0% / 100.0% | 71.9% |
| UMi n40 | UL | 35.00 | 24.29 | 0 | 0.0% | 0.03 s | 100.0% / 83.8% | 76.5% |

The n40 profile illustrates why a zero grant in one TTI must not automatically be labelled starvation. It contains many 10 ms windows with no delivered payload, but no 1 s zero-delivery windows and maximum observed gaps of 70 ms DL and 30 ms UL in this run.

Longer application-delivery gaps occur in UMa and RMa. UMa combines a high assignment ratio with individual non-outage delivery gaps up to the observation boundary, while RMa DL shows long periods without delivered payload despite assigning most available resources. These observations do not identify a unique cause: traffic state, queue occupancy, MCS validity, scheduling, packet fit, and expiry are not separately classified in the current logs.

Resource assignment and useful occupancy must also be distinguished. RMa UL assigns 99.9% of available units but only 57.3% produce positive effective payload; UMa UL assigns 100.0% while only 17.8% produce effective payload. TDD-unavailable symbols are excluded from both denominators.

### 8.3 Controlled penetration result

For the n258 FWA pair, high-loss indoor penetration reduces median-UE SINR by 38.15 dB DL and 38.71 dB UL. Delivered throughput falls from 500.02 to 235.16 Mbit/s in DL and from 157.05 to 55.22 Mbit/s in UL, corresponding to reductions of 53.0% and 64.8%. The standard deviation of each UE's paired DL SINR difference is below \(6.2\times10^{-6}\) dB, confirming that the keyed fast-fading streams remain aligned. The result isolates configured environment loss for one seed; it does not validate the scalar FWA gains as a beamforming model.

### 8.4 PF reranking and execution cost

The longer PF functional campaign shows that Jain fairness approaches one with and without reranking in the homogeneous 64-UE cases, while allocation-unit reranking substantially reduces maximum effective-service gaps.

| Grid | Scheduling unit | DL Jain none → rerank | Maximum DL effective-service gap none → rerank |
|---|---|---:|---:|
| 20 MHz, \(\mu=1\) | grouped | 0.9990 → 0.9999 | 96 → 18 TTIs |
| 100 MHz, \(\mu=1\) | grouped | 0.9990 → 1.0000 | 96 → 6 TTIs |
| 400 MHz, \(\mu=3\) | grouped | 0.9994 → 1.0000 | 81 → 6 TTIs |
| 20 MHz, \(\mu=1\) | per-PRB | 0.9990 → 1.0000 | 96 → 2 TTIs |
| 100 MHz, \(\mu=1\) | per-PRB | 0.9990 → 1.0000 | 96 → 1 TTI |
| 400 MHz, \(\mu=3\) | per-PRB | 0.9997 → 1.0000 | 83 → 0 TTIs |

Timing results show that grouped modes remain below the 1 ms compute budget through 64 UEs for every tested bandwidth on the measured host. At 256 UEs, grouped exceedance fractions are 0.0% at 20 MHz, 17.2–21.3% at 100 MHz, and 20.4–22.5% at 400 MHz. Distributed per-PRB 100 MHz already exceeds the budget in most TTIs at 16 UEs, and all tested 400 MHz per-PRB TTIs exceed it.

| 64-UE grid | P99 none | P99 rerank | Compute-budget exceedance none / rerank |
|---|---:|---:|---:|
| 20 MHz grouped | 188 µs | 192 µs | 0.0% / 0.0% |
| 100 MHz grouped | 364 µs | 377 µs | 0.0% / 0.0% |
| 400 MHz grouped | 333 µs | 379 µs | 0.0% / 0.0% |
| 20 MHz distributed per-PRB | 989 µs | 1,050 µs | 0.7% / 3.1% |
| 100 MHz distributed per-PRB | 5,454 µs | 5,778 µs | 100.0% / 100.0% |
| 400 MHz distributed per-PRB | 12,362 µs | 13,021 µs | 100.0% / 100.0% |

These measurements justify enabling allocation-unit reranking in the grouped reference profiles while retaining a configurable conservative default for high-resolution per-PRB studies. They do not establish end-to-end real-time application performance because process construction, packet capture, external application I/O, pacing, operating-system scheduling, and telemetry overhead are outside the timed interval.

## 9. Current validity boundary

The current model is internally reproducible and dimensionally consistent for its declared abstractions, but several limitations prevent predictive deployment claims.

First, the ABG coefficients, shadow deviations, and map dimensions are not accompanied by one consolidated source-campaign table containing frequency, distance, height, and environment ranges. Generated map extent must therefore not be interpreted as model-validity extent.

Second, the interference surrogate does not represent neighbour geometry, antenna patterns, beams, load, scheduling, or penetration. It can perturb SINR but cannot reproduce the coupling between traffic load and inter-cell interference.

Third, rank and MIMO are scalar abstractions. The model has no channel matrix, precoder, receiver, layer-specific SINR, codeword mapping, or calibrated rank-transition distribution. The reference evidence validates rank one only.

Fourth, MCS and nominal capacity are not calibrated against a link-level NR implementation. Thresholds have no committed decoder, BLER, TBS, rank, or receiver provenance, and the packet evidence contains no BLER-driven HARQ.

Fifth, the five packet-level profiles use one traffic, map, mobility, and fading seed. They are deterministic characterizations rather than distributions over deployments or offered-load realizations.

Sixth, delivery metrics are sampled every 10 ms, and initial or final zero-delivery runs are lower bounds under censoring. Queue backlog and per-UE empty-resource reasons are not directly observed.

Finally, timing results apply to one non-isolated host and an accelerated benchmark with bounded generated load. Their empirical tails are useful for implementation choices but are not operating-system or application-level timing guarantees.

## 10. Planned model evolution

### 10.1 Multicell interference

The next interference model should separate reference-signal strength from traffic-dependent SINR without turning FikoRE into a site-planning simulator. The minimum useful extension is a small explicit neighbour geometry with per-resource activity determined by cell load. A load-coupled model can represent the fixed point

\[
\rho_c=\sum_{u\in c}\frac{d_u}{W\log_2\!\left(1+\mathrm{SINR}_u(\boldsymbol{\rho})\right)},
\]

where cell load \(\rho_c\) changes the interference imposed on neighbouring cells [12]. Open decisions include explicit wraparound geometry versus scenario-calibrated neighbour gains, whether interference is updated per PRB or per RBG, and how scheduler runtime constrains the number of neighbours.

### 10.2 Beam state and blockage

The existing n258 profiles use equivalent aligned scalar gains. A first explicit beam model should introduce serving and interfering Tx/Rx beam identifiers, alignment state, blockage state, measurement cadence, reporting delay, beam-sweep resource cost, and failure-recovery behavior. A possible link term is

\[
G_{\mathrm{beam}}(t)=G_{\mathrm{tx}}(b_{\mathrm{tx}})+G_{\mathrm{rx}}(b_{\mathrm{rx}})-L_{\mathrm{align}}(a_t)-L_{\mathrm{block}}(z_t).
\]

Alignment and blockage should be independent dimensions rather than mutually exclusive labels. The model must distinguish absolute antenna gain from a gain or loss delta relative to the current scalar profile [13].

### 10.3 Carrier aggregation

Carrier aggregation must not be represented as one invalidly wide carrier. One UE MAC entity should retain shared logical-channel and RLC queues, while each serving cell has its own frequency, bandwidth part, numerology, resource grid, channel state, CQI, MCS, HARQ state, and grants. PCell and SCell activation and optional cross-carrier scheduling add control state, and simultaneous UL carriers must share a joint \(P_{\mathrm{CMAX}}\) constraint rather than multiplying UE power [5], [14], [22]–[24].

### 10.4 MIMO

A practical next-stage MIMO abstraction should expose selected rank, post-processing SINR per layer, PRB, and codeword, power allocation, number of codewords, and MCS per codeword:

\[
\left(v,\Gamma_{\ell,k,\mathrm{cw}},P_{\ell,k},n_{\mathrm{cw}},m_{\mathrm{cw}}\right).
\]

The minimum implementation could use calibrated lookup distributions conditioned on scenario, antenna configuration, and reference SINR. A higher-fidelity option would add correlated channel matrices, codebook precoding, and an explicit receiver. Required calibration includes rank distributions, antenna correlation, layer-quality distributions, receiver type, and RI feedback cadence.

### 10.5 Link-to-system abstraction, HARQ, and OLLA

Before enabling BLER-driven HARQ, FikoRE needs reproducible AWGN BLER curves indexed by MCS, bounded TBS class, rank, receiver, and redundancy version. Frequency-selective SINR can then be compressed per codeword with a calibrated effective-SINR mapping. For EESM,

\[
\gamma_{\mathrm{eff}}=-\beta_m\ln\!\left(\frac{1}{K}\sum_{k=1}^{K}e^{-\gamma_k/\beta_m}\right),
\]

where \(\gamma_k\) and \(\beta_m\) are linear dimensionless SINR quantities and data-resource elements should be weighted appropriately. MIESM provides a modulation-aware alternative, while recursive effective-SINR methods can support HARQ accumulation [15], [16].

Outer-loop link adaptation should maintain a separate bounded belief offset that is updated from ACK/NACK outcomes toward a declared BLER target [17]. The current generic SINR offset is unsuitable because it shifts both scheduling belief and reported physical SINR.

### 10.6 Calibration and observability

The highest-priority supporting work is not another channel feature but a calibration and observability layer. It should consolidate ABG coefficient provenance and validity ranges, generate link-level BLER reference data, compare selected scenarios against independent RSRP and SINR measurements, and record queue backlog, positive-rate eligibility, finalized MCS, nominal and effective grant bits, and empty-resource reason per UE. Those observations are needed to distinguish source idleness, sampled outage, scheduler starvation, packet-fit waste, expiry, and radio decoding loss.

## 11. Conclusions

The current FikoRE physical layer provides a deterministic, computationally bounded abstraction of a single serving carrier. It combines spatially correlated large-scale gain, explicit environment and penetration state, resource-consistent signal and noise accounting, bounded uplink total power, threshold-based MCS and rank, TDD-aware resource accounting, and a TTI-updated proportional-fair scheduler. This model is sufficient for controlled application-facing experiments in which relative effects and reproducibility are more important than exact receiver prediction.

The validation demonstrates deterministic map generation, correct local covariance behavior, reproducible packet-level scenarios, clear separation between TDD structure and resource assignment, strong service-continuity benefits from grouped-grid PF reranking, and a measured compute envelope that depends sharply on scheduling resolution. It also shows why assignment, effective payload, and application delivery must be reported separately.

The model remains intentionally incomplete. Predictive use requires propagation-range provenance, measured calibration, explicit multicell interference, better rank and MIMO state, link-level BLER data, and a calibrated HARQ and OLLA chain. The planned evolution therefore prioritizes calibration and observability before adding additional unvalidated detail.

## Appendix A — Implementation and evidence traceability

This appendix records the exact software and evidence identity used for the quantitative results. It is not required to understand the physical model described in the main text.

### A.1 Document identity

| Item | Value |
|---|---|
| Document revision | 1.0 |
| Revision date | 2026-09-29 |
| Review status | External technical-review manuscript |
| Repository branch | `feature/phy-model-v2` |
| Release tag | Not assigned |

The document revision is assigned independently of Git because a document cannot stably contain the SHA of the same commit that introduces that SHA. A release tag may be assigned after review without changing the document contents.

### A.2 Software revisions

**Model implementation source:** `1b55ca191e27a368936476fff33fc622833d8d13`

**Packet-profile source:** `1b55ca191e27a368936476fff33fc622833d8d13`

**Runtime-benchmark source:** `1b55ca191e27a368936476fff33fc622833d8d13`

| Component or campaign | Commit | Purpose |
|---|---|---|
| Integrated implementation and all rebased validation runs | `1b55ca191e27a368936476fff33fc622833d8d13` | Model, packet profiles, map statistics, analysis, and runtime benchmark |
| Evidence refresh after rebase | `b3b33885e09c38193fc6c0ecd8df3559f40222e9` | Portable manifests, refreshed figures, and committed summaries |
| Deterministic map-catalog implementation | `8e1b3680d76b21daf76fd43596a9918972368fb7` | Production map-generation semantics |
| Legacy-map source used for the paired catalog comparison | `648aa7bedab82e55faed769ba16234c94f4d749d` | Historical v1 map bytes |
| Development baseline before the original PHY feature series | `e995563fd3e8fa300f4b0accca3b494f7177101c` | Historical comparison baseline |

### A.3 Model and schema versions

| Item | Version or policy |
|---|---|
| Production map generator | `fikore-map-generator 2.1.0` |
| Production map semantics | `2.1.0` |
| Production map count | 21 |
| Production grid | Odd 291×291 with `explicit-center-cell` origin |
| Master map seed | `20260927` |
| Canonical UE environment key | `ue_location_type` |
| Nearest-frequency fallback | Disabled unless explicitly enabled |
| Default MIMO configuration | One antenna and one layer |
| Evaluated HARQ/BLER decision | Disabled |

### A.4 Experiment provenance

| Campaign | Source commit | Seed(s) | Duration or sample count | Date |
|---|---|---|---|---|
| Map ensemble | `1b55ca191e27a368936476fff33fc622833d8d13` | Master seeds `20270000`–`20270029` | 30 realizations × 21 catalog entries | 2026-09-29 |
| Five packet-level profiles | `1b55ca191e27a368936476fff33fc622833d8d13` | `20260927` | 180 s/profile, 20 s warm-up | 2026-09-29 |
| Controlled n258 O2I pair | `1b55ca191e27a368936476fff33fc622833d8d13` | `20260927` | 180 s/arm, 20 s warm-up | 2026-09-29 |
| Legacy-v1 versus current-map pair | `1b55ca191e27a368936476fff33fc622833d8d13` | `20260927` | 180 s/profile, 20 s warm-up | 2026-09-29 |
| Per-TTI runtime envelope | `1b55ca191e27a368936476fff33fc622833d8d13` | Deterministic benchmark | 1,000 TTI samples/case | 2026-09-29 |
| Longer PF functional campaign | `1b55ca191e27a368936476fff33fc622833d8d13` | Deterministic benchmark | 500 warm-up + 2,000 measured TTIs/case | 2026-09-29 |

The validation host used an AMD Ryzen 7 5800H with 8 physical cores and 16 logical CPUs, Linux 5.15.0-58, g++ 11.4.0, Python 3.10.4, NumPy 2.2.6, and Matplotlib 3.10.7. Runtime processes used one emulator thread and the host was not CPU-isolated.

### A.5 Validation and artifact record

| Artifact or check | Repository location |
|---|---|
| Unified evidence manifest | `docs/baselines/phy-model-v2-evidence-manifest.json` |
| Complete validation matrix and reproduction commands | `docs/baselines/phy-v2-validation-matrix.md` |
| Map catalog and map hashes | `include/maps_scenarios/CATALOG.json`, `include/maps_scenarios/MANIFEST.json` |
| Multi-seed map diagnostics | `docs/baselines/map-v2-multiseed-*.csv`, `docs/baselines/map-v2-multiseed-validation.md` |
| Packet-profile summaries | `docs/baselines/phy-v2-production-*.csv` |
| Controlled O2I comparison | `docs/baselines/phy-v2-o2i-controlled-comparison.*` |
| Legacy/current catalog comparison | `docs/baselines/map-v2.1-paired-profile-comparison.*` |
| PF runtime and functional results | `docs/baselines/pf-runtime-*`, `docs/baselines/pf-functional-long*` |
| Figure-generation script | `tools/generate_phy_v2_figures.py` |
| Evidence verifier | `tools/verify_phy_v2_evidence.py` |
| Python evidence dependencies | `tools/requirements-phy-v2.txt` |

The final rebased validation passed the complete C++ and map test suite, smoke scenarios, API tests, transport-model unit and integration tests, deterministic figure regeneration, validation of all 21 production maps, and recursive verification of 89 committed evidence artifacts. Raw packet logs remain local because of their size; exact portable inputs, source identifiers, summaries, commands, and hashes are committed, so experiments can be regenerated but the original raw samples cannot be independently audited from the repository alone.

### A.6 Revision history

| Revision | Main change |
|---|---|
| Legacy physical model | Unseeded legacy maps, mixed resource-bandwidth references, CQI-coupled PF history, and overloaded O2I state |
| Core PHY/MAC correction | Resource-consistent power and noise, allocation-aware UL power, common PF exponent, and TTI-updated PF state |
| Initial deterministic maps | Seeded odd-grid maps, explicit origin, binary LOS generation, and exact 2.38 GHz UMi support |
| Current deterministic maps | Padded FFT embedding removes periodic edge seams; explicit provenance and exact-frequency lookup are enforced |
| Adversarial review corrections | Shared DL/UL environment state, normalized Rayleigh power, safe MIMO indexes, FR1/FR2 overhead correction, TDD-effective resource metrics, per-TTI timing, and portable evidence verification |
| Development integration | Feature history rebased onto the updated `dev` branch, full profile and runtime evidence regenerated, and all validation suites rerun |

### A.7 Reproduction caveats

The evidence package separates implementation source, experiment source, and document revision. The document may evolve editorially without invalidating numerical artifacts, provided the appendix retains the source commit used for each campaign. The committed summaries are hash-verified, but raw logs are not archived. Runtime results are host-specific and profile campaigns use one traffic and mobility seed. Map confidence intervals are pointwise across independent realizations and do not establish external propagation accuracy.

## References

1. 3GPP TR 38.901 v18.1.0, *Study on channel model for frequencies from 0.5 to 100 GHz*, Release 18, 2024.
2. 3GPP TS 38.104 v18.8.0, *NR; Base Station radio transmission and reception*, Release 18, 2025.
3. 3GPP TS 38.213 v18.7.0, *NR; Physical layer procedures for control*, Release 18, 2025.
4. 3GPP TS 38.214 v18.9.0, *NR; Physical layer procedures for data*, Release 18, 2026.
5. 3GPP TS 38.306 v18.8.0, *NR; User Equipment radio access capabilities*, Release 18, 2025.
6. J. M. Reyero Lobo, *Advancing 5G Network Emulation: Comprehensive Enhancements to FikoRE's Physical and MAC Layers*, Universidad Politécnica de Madrid, 2025.
7. S. Sun et al., “Investigation of Prediction Accuracy, Sensitivity, and Parameter Stability of Large-Scale Propagation Path Loss Models for 5G Wireless Communications,” *IEEE Transactions on Vehicular Technology*, vol. 65, no. 5, 2016, <https://doi.org/10.1109/TVT.2016.2543139>.
8. G. R. MacCartney, Jr. and T. S. Rappaport, “Study on 3GPP Rural Macrocell Path Loss Models for Millimeter Wave Wireless Communications,” *IEEE ICC*, 2017, <https://doi.org/10.1109/ICC.2017.7996793>.
9. C. Zhang, X. Chen, H. Yin, and G. Wei, “Two-Dimensional Shadow Fading Modeling on System Level,” *IEEE PIMRC*, 2012, <https://doi.org/10.1109/PIMRC.2012.6362617>.
10. F. P. Kelly, “Charging and Rate Control for Elastic Traffic,” *European Transactions on Telecommunications*, vol. 8, no. 1, 1997, <https://doi.org/10.1002/ett.4460080106>.
11. H. J. Kushner and P. A. Whiting, “Convergence of Proportional-Fair Sharing Algorithms Under General Conditions,” *IEEE Transactions on Wireless Communications*, vol. 3, no. 4, 2004, <https://doi.org/10.1109/TWC.2004.830826>.
12. I. Siomina and D. Yuan, “Analysis of Cell Load Coupling for LTE Network Planning and Optimization,” *IEEE Transactions on Wireless Communications*, vol. 11, no. 6, 2012, <https://doi.org/10.1109/TWC.2012.051512.111532>.
13. M. Giordani, M. Polese, A. Roy, D. Castor, and M. Zorzi, “A Tutorial on Beam Management for 3GPP NR at mmWave Frequencies,” *IEEE Communications Surveys & Tutorials*, vol. 21, no. 1, 2019, <https://doi.org/10.1109/COMST.2018.2869411>.
14. K. I. Pedersen et al., “Carrier Aggregation for LTE-Advanced: Functionality and Performance Aspects,” *IEEE Communications Magazine*, vol. 49, no. 6, 2011, <https://doi.org/10.1109/MCOM.2011.5783991>.
15. I. Latif, F. Kaltenberger, N. Nikaein, and R. Knopp, “Large Scale System Evaluations using PHY Abstraction for LTE with OpenAirInterface,” *SIMUTools*, 2013, <https://doi.org/10.4108/icst.simutools.2013.251738>.
16. B. Classon et al., “Efficient OFDM-HARQ System Evaluation Using a Recursive EESM Link Error Prediction,” *IEEE WCNC*, 2006, <https://doi.org/10.1109/WCNC.2006.1696579>.
17. A. Sampath, P. S. Kumar, and J. M. Holtzman, “On Setting Reverse Link Target SIR in a CDMA System,” *IEEE VTC*, 1997, <https://doi.org/10.1109/VETEC.1997.600465>.
18. C. Mehlführer et al., “The Vienna LTE Simulators—Enabling Reproducibility in Wireless Communications Research,” *EURASIP Journal on Advances in Signal Processing*, 2011, <https://doi.org/10.1186/1687-6180-2011-29>.
19. N. Patriciello, S. Lagen, B. Bojovic, and L. Giupponi, “An E2E Simulator for 5G NR Networks,” *Simulation Modelling Practice and Theory*, vol. 96, 2019, <https://doi.org/10.1016/j.simpat.2019.101933>.
20. G. Nardini et al., “Simu5G—An OMNeT++ Library for End-to-End Performance Evaluation of 5G Networks,” *IEEE Access*, vol. 8, 2020, <https://doi.org/10.1109/ACCESS.2020.3028550>.
21. D. González Morín, M. J. López-Morales, P. Pérez, A. García Armada, and Á. Villegas, “FikoRE: 5G and Beyond RAN Emulator for Application Level Experimentation and Prototyping,” *IEEE Network*, vol. 37, no. 4, pp. 48–55, 2023, <https://doi.org/10.1109/MNET.002.2200595>.
22. 3GPP TS 38.300, *NR; NR and NG-RAN Overall Description; Stage-2*, Release 18.
23. 3GPP TS 38.321, *NR; Medium Access Control (MAC) Protocol Specification*, Release 18.
24. 3GPP TS 38.331, *NR; Radio Resource Control (RRC) Protocol Specification*, Release 18.
