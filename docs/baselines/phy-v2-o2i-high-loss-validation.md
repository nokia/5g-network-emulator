# PHY Model V2 Five-Profile Validation

- Batch: `phy-v2-o2i-final-037280b-seed20260927`
- Source: `037280b46289ec46989e9f0627068afef852666c`
- Seed: `20260927`
- Warm-up excluded: 20 seconds

| Profile | Dir. | Offered | Delivered | Errors | Outage UEs | Zero-delivery windows (10/100/1000 ms) | UE max delivery-gap P50/P95/P99/max (ms) | Grid assigned/effective | Payload/grant | Wall time |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| offline_umi_n258_fwa_high_loss | DL | 500.02 | 264.26 | 235.75 | 0 | 2.5% / 0.0% / 0.0% | 0 / 40 / 40 / 40 | 100.0% / 100.0% | 35.0% | 11.26 s |
| offline_umi_n258_fwa_high_loss | UL | 200.01 | 61.16 | 138.85 | 0 | 29.1% / 18.2% / 18.1% | 30 / 123035 / 152607 / 160000 | 100.0% / 45.7% | 100.0% | 11.26 s |

Rates are Mbit/s. Delivery-window and delivery-gap statistics exclude UEs classified as permanent PHY outage. A PHY-outage UE has MCS below zero in at least 99% of post-warm-up radio samples.

A zero-delivery window has positive offered traffic and no delivered payload in that non-overlapping window. This avoids interpreting every unassigned TTI as user starvation.

The delivery-gap distribution in the table is the distribution of each non-outage UE's maximum observed gap inside contiguous positive-offer segments. It is not a queue-backlog or scheduler-starvation metric. Pooled gap quantiles remain available in the CSV.

Grid assigned/effective values are fractions of physically available frequency-time units. Per-TTI summaries separate TDD-unavailable, available-but-empty, and assigned-with-zero-effective-payload units.

Payload/grant efficiency is sampled from logged grid grants and is the sum of effective payload bits divided by nominal grant bits.
