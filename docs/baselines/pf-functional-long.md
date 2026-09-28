# PF Granularity Benchmark

Warm-up steps: 500; measured steps per sample: 2000; repeats: 1.

P50/P95/P99/max are computed from individually timed warmed-up TTIs across all repeats. A compute-budget exceedance is a TTI above 1,000 us.

Host: `pitahaya`; platform: `Linux-5.15.0-58-generic-x86_64-with-glibc2.35`.

Time modes: `localized` uses one time allocation group per 1 ms; `distributed` uses one group per numerology slot.

Frequency modes: `grouped` uses configured RBGs; `per_rb` uses one PRB per allocation unit.

Offered traffic is approximately 4 Gbit/s in each direction per case, divided equally across UEs. This maintains backlog without the unbounded memory growth caused by a 100 Gbit/s target per UE.

| grid | ues | time_mode | frequency_mode | reranking | decisions_per_tti | us_per_tti_p50 | us_per_tti_p95 | us_per_tti_p99 | us_per_tti_max | compute_budget_exceedance_pct | dl_total_mbps | dl_jain | dl_max_service_gap_ttis |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 20mhz_mu1 | 64 | localized | grouped | none | 6 | 112.822 | 174.457 | 192.16 | 374.212 | 0.0 | 66.758 | 0.99899 | 96 |
| 20mhz_mu1 | 64 | localized | grouped | allocation_unit | 6 | 123.667 | 187.486 | 219.128 | 465.102 | 0.0 | 66.731 | 0.99988 | 18 |
| 20mhz_mu1 | 64 | distributed | per_rb | none | 100 | 583.765 | 919.36 | 1030.19 | 1235.12 | 2.35 | 69.514 | 0.99899 | 96 |
| 20mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 100 | 638.367 | 996.172 | 1148.402 | 1344.03 | 4.9 | 69.514 | 1 | 2 |
| 100mhz_mu1 | 64 | localized | grouped | none | 17 | 192.796 | 351.268 | 370.736 | 519.063 | 0.0 | 378.18 | 0.999 | 96 |
| 100mhz_mu1 | 64 | localized | grouped | allocation_unit | 17 | 223.173 | 384.415 | 412.399 | 627.987 | 0.0 | 378.194 | 1.0 | 6 |
| 100mhz_mu1 | 64 | distributed | per_rb | none | 546 | 3526.93 | 5125.665 | 5752.862 | 6335.02 | 100.0 | 379.551 | 0.999 | 96 |
| 100mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 546 | 4511.885 | 6437.97 | 7460.712 | 8365.54 | 100.0 | 379.542 | 1 | 1 |
| 400mhz_mu3 | 64 | localized | grouped | none | 15 | 233.949 | 354.359 | 402.437 | 587.843 | 0.0 | 1245.61 | 0.99938 | 81 |
| 400mhz_mu3 | 64 | localized | grouped | allocation_unit | 15 | 258.024 | 375.318 | 403.511 | 596.979 | 0.0 | 1245.51 | 1.0 | 6 |
| 400mhz_mu3 | 64 | distributed | per_rb | none | 2000 | 11473.25 | 13471.29 | 14278.363 | 15810.2 | 100.0 | 1232.42 | 0.99965 | 83 |
| 400mhz_mu3 | 64 | distributed | per_rb | allocation_unit | 2000 | 13581.35 | 16396.885 | 17180.721 | 18227.2 | 100.0 | 1232.43 | 1 | 0 |

Failed cases: 0.
