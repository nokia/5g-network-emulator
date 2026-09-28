# PF Granularity Benchmark

Warm-up steps: 500; measured steps per sample: 2000; repeats: 1.

P50/P95/P99 are computed from individually timed warmed-up TTIs across all repeats. A deadline miss is a TTI above 1,000 us.

Host: `pitahaya`; platform: `Linux-5.15.0-58-generic-x86_64-with-glibc2.35`.

Time modes: `localized` uses one time allocation group per 1 ms; `distributed` uses one group per numerology slot.

Frequency modes: `grouped` uses configured RBGs; `per_rb` uses one PRB per allocation unit.

Offered traffic is approximately 4 Gbit/s in each direction per case, divided equally across UEs. This maintains backlog without the unbounded memory growth caused by a 100 Gbit/s target per UE.

| grid | ues | time_mode | frequency_mode | reranking | decisions_per_tti | us_per_tti_p50 | us_per_tti_p95 | us_per_tti_p99 | deadline_miss_pct | dl_total_mbps | dl_jain | dl_max_service_gap_ttis |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 20mhz_mu1 | 64 | localized | grouped | none | 6 | 113.238 | 165.963 | 198.364 | 0.0 | 66.467 | 0.99912 | 102 |
| 20mhz_mu1 | 64 | localized | grouped | allocation_unit | 6 | 116.834 | 171.886 | 195.177 | 0.0 | 66.474 | 0.9999 | 19 |
| 20mhz_mu1 | 64 | distributed | per_rb | none | 100 | 568.521 | 869.22 | 993.49 | 0.85 | 69.239 | 0.99911 | 106 |
| 20mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 100 | 582.428 | 902.829 | 1028.218 | 1.8 | 69.235 | 1.0 | 15 |
| 100mhz_mu1 | 64 | localized | grouped | none | 17 | 192.506 | 328.039 | 357.129 | 0.0 | 376.638 | 0.99913 | 103 |
| 100mhz_mu1 | 64 | localized | grouped | allocation_unit | 17 | 229.996 | 368.677 | 410.362 | 0.0 | 376.745 | 1.0 | 13 |
| 100mhz_mu1 | 64 | distributed | per_rb | none | 546 | 3482.295 | 4934.064 | 5467.69 | 100.0 | 378.033 | 0.99928 | 112 |
| 100mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 546 | 4643.085 | 6525.325 | 7467.394 | 100.0 | 378.043 | 0.99998 | 48 |
| 400mhz_mu3 | 64 | localized | grouped | none | 15 | 228.725 | 336.393 | 362.557 | 0.0 | 1240.33 | 0.99952 | 83 |
| 400mhz_mu3 | 64 | localized | grouped | allocation_unit | 15 | 261.806 | 370.893 | 430.431 | 0.0 | 1240.38 | 1.0 | 22 |
| 400mhz_mu3 | 64 | distributed | per_rb | none | 2000 | 11493.1 | 13427.045 | 14023.011 | 100.0 | 1228.25 | 0.99984 | 83 |
| 400mhz_mu3 | 64 | distributed | per_rb | allocation_unit | 2000 | 13765.85 | 16481.81 | 17892.318 | 100.0 | 1228.27 | 0.99999 | 56 |

Failed cases: 0.
