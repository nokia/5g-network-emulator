# PHY Model V2 Five-Profile Validation

- Batch: `phy-v2-production-mimo-index-fe93077-seed20260927`
- Source: `fe9307795130804fe41b9edf6730b35eb75cf162`
- Seed: `20260927`
- Warm-up excluded: 20 seconds

| Profile | Dir. | Offered | Delivered | Errors | Outage UEs | Zero-service windows (10/100/1000 ms) | UE max-gap P50/P95/P99/max (ms) | Grid fill | Payload/grant | Wall time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| offline_indoor_hotspot_n78_pedestrian | DL | 65.00 | 46.22 | 18.89 | 0 | 0.1% / 0.1% / 0.1% | 0 / 760 / 1360 / 1510 | 86.2% | 22.6% | 9.27 s |
| offline_indoor_hotspot_n78_pedestrian | UL | 90.00 | 44.50 | 45.52 | 0 | 24.8% / 4.0% / 3.6% | 30 / 25600 / 40968 / 44810 | 100.0% | 44.1% | 9.27 s |
| offline_rural_n78_vehicular | DL | 120.01 | 61.93 | 58.00 | 1 | 30.2% / 28.3% / 24.7% | 13035 / 67556 / 87183 / 92090 | 97.3% | 98.6% | 100.27 s |
| offline_rural_n78_vehicular | UL | 120.01 | 26.70 | 89.91 | 0 | 46.7% / 40.3% / 30.2% | 11900 / 33810 / 41722 / 43700 | 99.9% | 89.7% | 100.27 s |
| offline_uma_n78_pedestrian | DL | 180.01 | 108.21 | 71.95 | 5 | 28.4% / 22.5% / 18.1% | 30 / 99830 / 108422 / 110570 | 100.0% | 71.9% | 12.54 s |
| offline_uma_n78_pedestrian | UL | 70.00 | 14.16 | 55.83 | 1 | 56.4% / 41.0% / 38.4% | 13445 / 160000 / 160000 / 160000 | 100.0% | 99.4% | 12.54 s |
| offline_umi_n258_fwa | DL | 500.02 | 500.02 | 0.00 | 0 | 0.0% / 0.0% / 0.0% | 0 / 0 / 0 / 0 | 98.7% | 38.1% | 9.28 s |
| offline_umi_n258_fwa | UL | 200.01 | 156.73 | 43.27 | 0 | 14.4% / 0.0% / 0.1% | 10 / 10 / 10 / 10 | 100.0% | 20.9% | 9.28 s |
| offline_umi_n40_npn | DL | 50.00 | 48.24 | 1.76 | 0 | 4.5% / 0.0% / 0.0% | 20 / 40 / 40 / 40 | 100.0% | 74.9% | 7.25 s |
| offline_umi_n40_npn | UL | 35.00 | 24.24 | 10.76 | 0 | 32.8% / 0.0% / 0.2% | 20 / 30 / 30 / 30 | 100.0% | 77.3% | 7.25 s |

Rates are Mbit/s. Service-window and service-gap statistics exclude UEs classified as permanent PHY outage. A PHY-outage UE has MCS below zero in at least 99% of post-warm-up channel samples.

A zero-service window has positive offered traffic and no delivered payload in that non-overlapping window. This avoids interpreting every unassigned TTI as user starvation.

The service-gap distribution in the table is the distribution of each non-outage UE's maximum post-warm-up gap. Pooled inter-service gap quantiles remain available in the CSV.

Grid fill is the assigned fraction of physically available frequency-time allocation units for that direction. Per-TTI grid summaries separate TDD-unavailable units from available-but-empty units.

Payload/grant efficiency is sampled from logged grid grants and is the sum of effective payload bits divided by nominal grant bits.
