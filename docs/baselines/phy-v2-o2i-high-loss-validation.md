# PHY Model V2 Five-Profile Validation

- Batch: `phy-v2-o2i-high-loss-975f983-seed20260927`
- Source: `975f983dda975187c5aa9f6efbd8c09d7e1be334`
- Seed: `20260927`
- Warm-up excluded: 20 seconds

| Profile | Dir. | Offered | Delivered | Errors | Outage UEs | Zero-service windows (10/100/1000 ms) | UE max-gap P50/P95/P99/max (ms) | Grid fill | Payload/grant | Wall time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| offline_umi_n258_fwa_high_loss | DL | 500.02 | 495.86 | 4.16 | 0 | 1.0% / 0.0% / 0.0% | 0 / 25 / 29 / 30 | 100.0% | 52.0% | 11.28 s |
| offline_umi_n258_fwa_high_loss | UL | 200.01 | 146.88 | 53.12 | 1 | 3.4% / 0.0% / 0.1% | 10 / 20 / 20 / 20 | 100.0% | 76.8% | 11.28 s |

Rates are Mbit/s. Service-window and service-gap statistics exclude UEs classified as permanent PHY outage. A PHY-outage UE has MCS below zero in at least 99% of post-warm-up channel samples.

A zero-service window has positive offered traffic and no delivered payload in that non-overlapping window. This avoids interpreting every unassigned TTI as user starvation.

The service-gap distribution in the table is the distribution of each non-outage UE's maximum post-warm-up gap. Pooled inter-service gap quantiles remain available in the CSV.

Grid fill is the assigned fraction of frequency-time allocation units in logged opportunities for that direction. TDD slots of the other direction are not included in this denominator.

Payload/grant efficiency is sampled from logged grid grants and is the sum of effective payload bits divided by nominal grant bits.
