# PHY Model V2 Five-Profile Validation

- Batch: `phy-v2-o2i-high-loss-fe93077-seed20260927`
- Source: `fe9307795130804fe41b9edf6730b35eb75cf162`
- Seed: `20260927`
- Warm-up excluded: 20 seconds

| Profile | Dir. | Offered | Delivered | Errors | Outage UEs | Zero-service windows (10/100/1000 ms) | UE max-gap P50/P95/P99/max (ms) | Grid fill | Payload/grant | Wall time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| offline_umi_n258_fwa_high_loss | DL | 500.02 | 495.50 | 4.52 | 0 | 0.9% / 0.0% / 0.0% | 0 / 25 / 29 / 30 | 100.0% | 51.5% | 9.25 s |
| offline_umi_n258_fwa_high_loss | UL | 200.01 | 146.67 | 53.33 | 1 | 3.4% / 0.0% / 0.0% | 10 / 20 / 20 / 20 | 100.0% | 73.3% | 9.25 s |

Rates are Mbit/s. Service-window and service-gap statistics exclude UEs classified as permanent PHY outage. A PHY-outage UE has MCS below zero in at least 99% of post-warm-up channel samples.

A zero-service window has positive offered traffic and no delivered payload in that non-overlapping window. This avoids interpreting every unassigned TTI as user starvation.

The service-gap distribution in the table is the distribution of each non-outage UE's maximum post-warm-up gap. Pooled inter-service gap quantiles remain available in the CSV.

Grid fill is the assigned fraction of physically available frequency-time allocation units for that direction. Per-TTI grid summaries separate TDD-unavailable units from available-but-empty units.

Payload/grant efficiency is sampled from logged grid grants and is the sum of effective payload bits divided by nominal grant bits.
