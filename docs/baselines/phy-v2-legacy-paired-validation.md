# PHY Model V2 Five-Profile Validation

- Batch: `phy-v2-legacy-paired-c0e5d11-seed20260927`
- Source: `c0e5d114cd4715b53d9557905017745fa2af6f05`
- Seed: `20260927`
- Warm-up excluded: 20 seconds

| Profile | Dir. | Offered | Delivered | Errors | Outage UEs | Zero-delivery windows (10/100/1000 ms) | UE max delivery-gap P50/P95/P99/max (ms) | Grid assigned/effective | Payload/grant | Wall time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| offline_indoor_hotspot_n78_pedestrian | DL | 65.00 | 45.74 | 19.38 | 0 | 0.4% / 0.2% / 0.2% | 0 / 1495 / 2659 / 2950 | 90.4% / 90.4% | 24.6% | 11.32 s |
| offline_indoor_hotspot_n78_pedestrian | UL | 90.00 | 56.56 | 33.44 | 0 | 13.6% / 0.3% / 0.2% | 20 / 1865 / 3333 / 3700 | 100.0% / 60.4% | 43.2% | 11.32 s |
| offline_rural_n78_vehicular | DL | 120.01 | 46.16 | 73.83 | 1 | 32.7% / 29.8% / 25.1% | 8290 / 56472 / 75638 / 80430 | 95.2% / 95.2% | 98.4% | 111.27 s |
| offline_rural_n78_vehicular | UL | 120.01 | 25.65 | 94.23 | 0 | 57.6% / 48.4% / 35.3% | 11530 / 66940 / 83276 / 87360 | 99.6% / 41.2% | 68.4% | 111.27 s |
| offline_uma_n78_pedestrian | DL | 180.01 | 111.48 | 68.51 | 5 | 20.0% / 15.7% / 13.1% | 20 / 65528 / 67201 / 67620 | 100.0% / 100.0% | 73.0% | 15.51 s |
| offline_uma_n78_pedestrian | UL | 70.00 | 13.79 | 56.20 | 0 | 57.5% / 41.6% / 38.9% | 10460 / 160000 / 160000 / 160000 | 100.0% / 20.7% | 100.0% | 15.51 s |
| offline_umi_n258_fwa | DL | 500.02 | 500.02 | 0.00 | 0 | 0.0% / 0.0% / 0.0% | 0 / 0 / 0 / 0 | 98.7% / 98.7% | 38.1% | 10.27 s |
| offline_umi_n258_fwa | UL | 200.01 | 156.76 | 43.25 | 0 | 14.0% / 0.0% / 0.0% | 10 / 10 / 10 / 10 | 100.0% / 27.4% | 21.1% | 10.27 s |
| offline_umi_n40_npn | DL | 50.00 | 48.24 | 1.75 | 0 | 7.4% / 0.0% / 0.0% | 20 / 45 / 49 / 50 | 100.0% / 100.0% | 75.1% | 8.27 s |
| offline_umi_n40_npn | UL | 35.00 | 25.03 | 9.97 | 0 | 33.6% / 0.0% / 0.0% | 20 / 30 / 30 / 30 | 100.0% / 83.8% | 77.4% | 8.27 s |

Rates are Mbit/s. Delivery-window and delivery-gap statistics exclude UEs classified as permanent PHY outage. A PHY-outage UE has MCS below zero in at least 99% of post-warm-up radio samples.

A zero-delivery window has positive offered traffic and no delivered payload in that non-overlapping window. This avoids interpreting every unassigned TTI as user starvation.

The delivery-gap distribution in the table is the distribution of each non-outage UE's maximum observed gap inside contiguous positive-offer segments. It is not a queue-backlog or scheduler-starvation metric. Pooled gap quantiles remain available in the CSV.

Grid assigned/effective values are fractions of physically available frequency-time units. Per-TTI summaries separate TDD-unavailable, available-but-empty, and assigned-with-zero-effective-payload units.

Payload/grant efficiency is sampled from logged grid grants and is the sum of effective payload bits divided by nominal grant bits.
