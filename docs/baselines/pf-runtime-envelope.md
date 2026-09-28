# PF Granularity Benchmark

Warm-up steps: 20; measured steps per sample: 100; repeats: 10.

P50/P95/P99 are computed from individually timed warmed-up TTIs across all repeats. A deadline miss is a TTI above 1,000 us.

Host: `pitahaya`; platform: `Linux-5.15.0-58-generic-x86_64-with-glibc2.35`.

Time modes: `localized` uses one time allocation group per 1 ms; `distributed` uses one group per numerology slot.

Frequency modes: `grouped` uses configured RBGs; `per_rb` uses one PRB per allocation unit.

Offered traffic is approximately 4 Gbit/s in each direction per case, divided equally across UEs. This maintains backlog without the unbounded memory growth caused by a 100 Gbit/s target per UE.

| grid | ues | time_mode | frequency_mode | reranking | decisions_per_tti | us_per_tti_p50 | us_per_tti_p95 | us_per_tti_p99 | deadline_miss_pct | dl_total_mbps | dl_jain | dl_max_service_gap_ttis |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 20mhz_mu1 | 1 | localized | grouped | none | 6 | 29.941 | 37.372 | 42.251 | 0.0 | 61.632 | 1 | 1 |
| 20mhz_mu1 | 1 | localized | grouped | allocation_unit | 6 | 34.45 | 44.045 | 47.561 | 0.0 | 61.632 | 1 | 1 |
| 20mhz_mu1 | 1 | distributed | per_rb | none | 100 | 89.734 | 117.383 | 127.16 | 0.0 | 64.192 | 1 | 1 |
| 20mhz_mu1 | 1 | distributed | per_rb | allocation_unit | 100 | 88.401 | 111.361 | 119.309 | 0.0 | 64.192 | 1 | 1 |
| 20mhz_mu1 | 16 | localized | grouped | none | 6 | 48.842 | 65.904 | 74.254 | 0.0 | 61.632 | 0.9957 | 27 |
| 20mhz_mu1 | 16 | localized | grouped | allocation_unit | 6 | 50.861 | 68.162 | 74.532 | 0.0 | 61.632 | 0.99988 | 6 |
| 20mhz_mu1 | 16 | distributed | per_rb | none | 100 | 203.683 | 280.854 | 318.305 | 0.0 | 64.192 | 0.99442 | 26 |
| 20mhz_mu1 | 16 | distributed | per_rb | allocation_unit | 100 | 256.462 | 336.482 | 377.34 | 0.0 | 64.192 | 0.99999 | 2 |
| 20mhz_mu1 | 64 | localized | grouped | none | 6 | 112.517 | 172.212 | 198.669 | 0.0 | 61.632 | 0.93476 | 102 |
| 20mhz_mu1 | 64 | localized | grouped | allocation_unit | 6 | 115.417 | 171.867 | 190.703 | 0.0 | 61.632 | 0.99842 | 19 |
| 20mhz_mu1 | 64 | distributed | per_rb | none | 100 | 594.292 | 921.8 | 1050.435 | 2.4 | 64.192 | 0.93996 | 106 |
| 20mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 100 | 689.521 | 989.433 | 1122.02 | 4.7 | 64.192 | 0.99955 | 15 |
| 20mhz_mu1 | 256 | localized | grouped | none | 6 | 369.193 | 629.519 | 760.253 | 0.1 | 61.309 | 0.3094 | 117 |
| 20mhz_mu1 | 256 | localized | grouped | allocation_unit | 6 | 378.757 | 629.234 | 757.971 | 0.1 | 61.632 | 0.96276 | 70 |
| 20mhz_mu1 | 256 | distributed | per_rb | none | 100 | 2549.14 | 3812.911 | 4292.032 | 100.0 | 64.163 | 0.31668 | 117 |
| 20mhz_mu1 | 256 | distributed | per_rb | allocation_unit | 100 | 2638.85 | 3908.618 | 4352.557 | 100.0 | 64.192 | 0.99555 | 55 |
| 100mhz_mu1 | 1 | localized | grouped | none | 17 | 39.965 | 51.799 | 58.432 | 0.0 | 349.256 | 1 | 1 |
| 100mhz_mu1 | 1 | localized | grouped | allocation_unit | 17 | 39.499 | 50.401 | 55.434 | 0.0 | 349.256 | 1 | 1 |
| 100mhz_mu1 | 1 | distributed | per_rb | none | 546 | 664.808 | 822.352 | 920.584 | 0.3 | 350.491 | 1 | 1 |
| 100mhz_mu1 | 1 | distributed | per_rb | allocation_unit | 546 | 672.809 | 834.562 | 900.031 | 0.1 | 350.491 | 1 | 1 |
| 100mhz_mu1 | 16 | localized | grouped | none | 17 | 79.043 | 115.249 | 123.322 | 0.0 | 349.22 | 0.99569 | 27 |
| 100mhz_mu1 | 16 | localized | grouped | allocation_unit | 17 | 88.096 | 129.933 | 140.003 | 0.0 | 349.256 | 0.99999 | 3 |
| 100mhz_mu1 | 16 | distributed | per_rb | none | 546 | 1235.44 | 1663.175 | 1857.385 | 97.1 | 350.488 | 0.99523 | 35 |
| 100mhz_mu1 | 16 | distributed | per_rb | allocation_unit | 546 | 1285.62 | 1779.32 | 2041.42 | 91.6 | 350.488 | 0.99958 | 20 |
| 100mhz_mu1 | 64 | localized | grouped | none | 17 | 194.345 | 343.876 | 364.87 | 0.0 | 347.878 | 0.94502 | 103 |
| 100mhz_mu1 | 64 | localized | grouped | allocation_unit | 17 | 217.684 | 361.941 | 394.911 | 0.0 | 349.256 | 0.99985 | 13 |
| 100mhz_mu1 | 64 | distributed | per_rb | none | 546 | 3494.805 | 5054.333 | 5640.034 | 100.0 | 350.467 | 0.92496 | 105 |
| 100mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 546 | 3716.015 | 5558.198 | 6007.9 | 100.0 | 350.467 | 0.98266 | 48 |
| 100mhz_mu1 | 256 | localized | grouped | none | 17 | 725.582 | 1350.322 | 1541.855 | 21.1 | 342.512 | 0.35414 | 117 |
| 100mhz_mu1 | 256 | localized | grouped | allocation_unit | 17 | 798.514 | 1456.682 | 1666.945 | 24.4 | 348.137 | 0.9965 | 44 |
| 100mhz_mu1 | 256 | distributed | per_rb | none | 546 | 12784.3 | 19120.875 | 21508.299 | 100.0 | 350.258 | 0.36771 | 117 |
| 100mhz_mu1 | 256 | distributed | per_rb | allocation_unit | 546 | 12504.45 | 18799.685 | 21182.324 | 100.0 | 350.274 | 0.86913 | 97 |
| 400mhz_mu3 | 1 | localized | grouped | none | 15 | 48.826 | 60.714 | 68.451 | 0.0 | 1159.39 | 1 | 0 |
| 400mhz_mu3 | 1 | localized | grouped | allocation_unit | 15 | 51.276 | 61.078 | 66.585 | 0.0 | 1159.39 | 1 | 0 |
| 400mhz_mu3 | 1 | distributed | per_rb | none | 2000 | 5055.375 | 6619.197 | 7173.823 | 100.0 | 1146.85 | 1 | 0 |
| 400mhz_mu3 | 1 | distributed | per_rb | allocation_unit | 2000 | 5024.035 | 6700.032 | 7351.942 | 100.0 | 1146.85 | 1 | 0 |
| 400mhz_mu3 | 16 | localized | grouped | none | 15 | 91.752 | 121.976 | 134.941 | 0.0 | 1152.36 | 0.99781 | 24 |
| 400mhz_mu3 | 16 | localized | grouped | allocation_unit | 15 | 103.735 | 130.647 | 141.878 | 0.0 | 1153.88 | 0.99996 | 7 |
| 400mhz_mu3 | 16 | distributed | per_rb | none | 2000 | 4860.395 | 6295.718 | 7043.81 | 100.0 | 1146.91 | 0.99205 | 16 |
| 400mhz_mu3 | 16 | distributed | per_rb | allocation_unit | 2000 | 4756.535 | 5758.071 | 6236.328 | 100.0 | 1146.88 | 0.99496 | 15 |
| 400mhz_mu3 | 64 | localized | grouped | none | 15 | 228.965 | 334.822 | 353.834 | 0.0 | 1135.92 | 0.96213 | 83 |
| 400mhz_mu3 | 64 | localized | grouped | allocation_unit | 15 | 257.734 | 366.568 | 387.342 | 0.0 | 1141.01 | 0.99938 | 22 |
| 400mhz_mu3 | 64 | distributed | per_rb | none | 2000 | 10638.35 | 12670.545 | 13190.019 | 100.0 | 1146.85 | 0.9568 | 56 |
| 400mhz_mu3 | 64 | distributed | per_rb | allocation_unit | 2000 | 10857.7 | 12857.91 | 13612.739 | 100.0 | 1146.97 | 0.95821 | 56 |
| 400mhz_mu3 | 256 | localized | grouped | none | 15 | 816.947 | 1406.813 | 1628.472 | 23.5 | 992.703 | 0.55079 | 117 |
| 400mhz_mu3 | 256 | localized | grouped | allocation_unit | 15 | 832.567 | 1411.094 | 1675.03 | 25.8 | 1026.03 | 0.99359 | 66 |
| 400mhz_mu3 | 256 | distributed | per_rb | none | 2000 | 32686.45 | 39471.185 | 40576.093 | 100.0 | 1145.08 | 0.5268 | 117 |
| 400mhz_mu3 | 256 | distributed | per_rb | allocation_unit | 2000 | 33611.7 | 40409.36 | 42355.892 | 100.0 | 1144.9 | 0.56569 | 117 |

Failed cases: 0.
