# PHY Model V2 Five-Profile Validation

- Batch: `phy-v2-production-tdd-metrics-62764dc-seed20260927`
- Source: `62764dc6957c9dadff3beecac4ac1c4e268a6ec3`
- Seed: `20260927`
- Warm-up excluded: 20 seconds

| Profile | Dir. | Offered | Delivered | Errors | Outage UEs | Zero-service windows (10/100/1000 ms) | UE max-gap P50/P95/P99/max (ms) | Grid fill | Payload/grant | Wall time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| offline_indoor_hotspot_n78_pedestrian | DL | 65.00 | 46.57 | 18.55 | 0 | 0.1% / 0.1% / 0.1% | 0 / 760 / 1360 / 1510 | 86.6% | 22.8% | 13.36 s |
| offline_indoor_hotspot_n78_pedestrian | UL | 90.00 | 45.20 | 44.80 | 0 | 24.1% / 3.8% / 3.4% | 30 / 24865 / 39973 / 43750 | 100.0% | 44.5% | 13.36 s |
| offline_rural_n78_vehicular | DL | 120.01 | 62.38 | 57.55 | 1 | 30.4% / 28.6% / 25.1% | 13195 / 67575 / 87195 / 92100 | 96.9% | 98.6% | 160.49 s |
| offline_rural_n78_vehicular | UL | 120.01 | 27.01 | 89.74 | 0 | 47.1% / 40.5% / 30.6% | 11150 / 33885 / 41857 / 43850 | 99.9% | 88.8% | 160.49 s |
| offline_uma_n78_pedestrian | DL | 180.01 | 108.75 | 71.40 | 5 | 26.8% / 20.8% / 16.8% | 30 / 99830 / 108422 / 110570 | 100.0% | 72.5% | 30.69 s |
| offline_uma_n78_pedestrian | UL | 70.00 | 14.38 | 55.61 | 1 | 55.6% / 40.4% / 37.6% | 9280 / 160000 / 160000 / 160000 | 100.0% | 98.3% | 30.69 s |
| offline_umi_n258_fwa | DL | 500.02 | 500.02 | 0.00 | 0 | 0.0% / 0.0% / 0.0% | 0 / 0 / 0 / 0 | 98.7% | 38.1% | 12.39 s |
| offline_umi_n258_fwa | UL | 200.01 | 156.73 | 43.27 | 0 | 14.4% / 0.0% / 0.1% | 10 / 10 / 10 / 10 | 100.0% | 20.9% | 12.39 s |
| offline_umi_n40_npn | DL | 50.00 | 48.27 | 1.73 | 0 | 4.7% / 0.0% / 0.0% | 20 / 40 / 40 / 40 | 100.0% | 75.0% | 19.43 s |
| offline_umi_n40_npn | UL | 35.00 | 24.33 | 10.67 | 0 | 32.7% / 0.0% / 0.3% | 20 / 30 / 30 / 30 | 100.0% | 77.9% | 19.43 s |

Rates are Mbit/s. Service-window and service-gap statistics exclude UEs classified as permanent PHY outage. A PHY-outage UE has MCS below zero in at least 99% of post-warm-up channel samples.

A zero-service window has positive offered traffic and no delivered payload in that non-overlapping window. This avoids interpreting every unassigned TTI as user starvation.

The service-gap distribution in the table is the distribution of each non-outage UE's maximum post-warm-up gap. Pooled inter-service gap quantiles remain available in the CSV.

Grid fill is the assigned fraction of physically available frequency-time allocation units for that direction. Per-TTI grid summaries separate TDD-unavailable units from available-but-empty units.

Payload/grant efficiency is sampled from logged grid grants and is the sum of effective payload bits divided by nominal grant bits.
