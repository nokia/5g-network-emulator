# PF Granularity Benchmark

Warm-up steps: 500; measured steps per sample: 2000; repeats: 1.

P50/P95/P99 are computed from individually timed warmed-up TTIs across all repeats. A deadline miss is a TTI above 1,000 us.

Host: `pitahaya`; platform: `Linux-5.15.0-58-generic-x86_64-with-glibc2.35`.

Time modes: `localized` uses one time allocation group per 1 ms; `distributed` uses one group per numerology slot.

Frequency modes: `grouped` uses configured RBGs; `per_rb` uses one PRB per allocation unit.

Offered traffic is approximately 4 Gbit/s in each direction per case, divided equally across UEs. This maintains backlog without the unbounded memory growth caused by a 100 Gbit/s target per UE.

| grid | ues | time_mode | frequency_mode | reranking | decisions_per_tti | us_per_tti_p50 | us_per_tti_p95 | us_per_tti_p99 | deadline_miss_pct | dl_total_mbps | dl_jain | dl_max_service_gap_ttis |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 20mhz_mu1 | 64 | localized | grouped | none | 6 | 139.171 | 220.483 | 290.506 | 0.0 | 66.467 | 0.99912 | 102 |
| 20mhz_mu1 | 64 | localized | grouped | allocation_unit | 6 | 141.465 | 217.674 | 279.382 | 0.0 | 66.474 | 0.9999 | 19 |
| 20mhz_mu1 | 64 | distributed | per_rb | none | 100 | 676.693 | 1092.548 | 1347.415 | 9.5 | 69.239 | 0.99911 | 106 |
| 20mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 100 | 736.826 | 1165.431 | 1358.822 | 13.0 | 69.235 | 1.0 | 15 |
| 100mhz_mu1 | 64 | localized | grouped | none | 17 | 216.39 | 358.777 | 403.643 | 0.0 | 376.638 | 0.99913 | 103 |
| 100mhz_mu1 | 64 | localized | grouped | allocation_unit | 17 | 269.525 | 433.998 | 497.494 | 0.0 | 376.745 | 1.0 | 13 |
| 100mhz_mu1 | 64 | distributed | per_rb | none | 546 | 4166.285 | 5882.609 | 6477.855 | 100.0 | 378.033 | 0.99928 | 112 |
| 100mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 546 | 5038.095 | 7316.474 | 8329.584 | 100.0 | 378.043 | 0.99998 | 48 |
| 400mhz_mu3 | 64 | localized | grouped | none | 15 | 258.164 | 385.235 | 444.708 | 0.0 | 1240.33 | 0.99952 | 83 |
| 400mhz_mu3 | 64 | localized | grouped | allocation_unit | 15 | 320.52 | 486.307 | 583.33 | 0.0 | 1240.38 | 1.0 | 22 |
| 400mhz_mu3 | 64 | distributed | per_rb | none | 2000 | 13391.65 | 15748.515 | 16882.134 | 100.0 | 1228.25 | 0.99984 | 83 |
| 400mhz_mu3 | 64 | distributed | per_rb | allocation_unit | 2000 | 15259.0 | 18199.92 | 19276.74 | 100.0 | 1228.27 | 0.99999 | 56 |

Failed cases: 0.
