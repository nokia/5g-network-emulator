# PHY Model V2 Five-Profile Validation

- Batch: `phy-v2-final-172f1a8-seed20260927`
- Source: `172f1a8936e903ba93308c809dcc8ab309e59bfd`
- Seed: `20260927`
- Warm-up excluded: 20 seconds

| Profile | Dir. | Offered | Delivered | Errors | Outage UEs | Zero-delivery windows (10/100/1000 ms) | UE max delivery-gap P50/P95/P99/max (ms) | Grid assigned/effective | Payload/grant | Wall time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| offline_indoor_hotspot_n78_pedestrian | DL | 65.00 | 44.61 | 20.50 | 0 | 0.2% / 0.1% / 0.1% | 0 / 1300 / 2204 / 2430 | 88.8% / 88.8% | 21.9% | 10.29 s |
| offline_indoor_hotspot_n78_pedestrian | UL | 90.00 | 57.52 | 32.48 | 0 | 14.5% / 0.1% / 0.0% | 20 / 890 / 1034 / 1070 | 100.0% / 60.7% | 44.1% | 10.29 s |
| offline_rural_n78_vehicular | DL | 120.01 | 52.82 | 66.99 | 0 | 31.4% / 29.1% / 24.0% | 10400 / 72820 / 98036 / 104340 | 98.0% / 98.0% | 98.6% | 105.26 s |
| offline_rural_n78_vehicular | UL | 120.01 | 27.21 | 90.57 | 0 | 48.3% / 40.2% / 28.1% | 10580 / 59715 / 90903 / 98700 | 100.0% / 54.8% | 82.6% | 105.26 s |
| offline_uma_n78_pedestrian | DL | 180.01 | 106.75 | 73.25 | 6 | 34.3% / 28.6% / 25.9% | 40 / 103030 / 106110 / 106880 | 99.6% / 99.6% | 72.6% | 13.52 s |
| offline_uma_n78_pedestrian | UL | 70.00 | 13.91 | 56.09 | 0 | 69.2% / 50.1% / 42.7% | 29960 / 160000 / 160000 / 160000 | 100.0% / 16.3% | 98.0% | 13.52 s |
| offline_umi_n258_fwa | DL | 500.02 | 500.02 | 0.00 | 0 | 0.0% / 0.0% / 0.0% | 0 / 0 / 0 / 0 | 98.7% / 98.7% | 38.1% | 10.26 s |
| offline_umi_n258_fwa | UL | 200.01 | 156.73 | 43.27 | 0 | 14.3% / 0.0% / 0.0% | 10 / 10 / 10 / 10 | 100.0% / 27.3% | 21.0% | 10.26 s |
| offline_umi_n40_npn | DL | 50.00 | 47.19 | 2.81 | 0 | 7.1% / 0.0% / 0.0% | 10 / 45 / 49 / 50 | 100.0% / 100.0% | 73.7% | 7.26 s |
| offline_umi_n40_npn | UL | 35.00 | 24.16 | 10.84 | 0 | 34.8% / 0.0% / 0.0% | 20 / 25 / 29 / 30 | 100.0% / 80.9% | 73.4% | 7.26 s |

Rates are Mbit/s. Delivery-window and delivery-gap statistics exclude UEs classified as permanent PHY outage. A PHY-outage UE has MCS below zero in at least 99% of post-warm-up radio samples.

A zero-delivery window has positive offered traffic and no delivered payload in that non-overlapping window. This avoids interpreting every unassigned TTI as user starvation.

The delivery-gap distribution in the table is the distribution of each non-outage UE's maximum observed gap inside contiguous positive-offer segments. It is not a queue-backlog or scheduler-starvation metric. Pooled gap quantiles remain available in the CSV.

Grid assigned/effective values are fractions of physically available frequency-time units. Per-TTI summaries separate TDD-unavailable, available-but-empty, and assigned-with-zero-effective-payload units.

Payload/grant efficiency is sampled from logged grid grants and is the sum of effective payload bits divided by nominal grant bits.
