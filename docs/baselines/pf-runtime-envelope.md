# PF Granularity Benchmark

Warm-up steps: 20; measured steps per sample: 100; repeats: 10.

P50/P95/P99 are computed from individually timed warmed-up TTIs across all repeats. A deadline miss is a TTI above 1,000 us.

Host: `pitahaya`; platform: `Linux-5.15.0-58-generic-x86_64-with-glibc2.35`.

Time modes: `localized` uses one time allocation group per 1 ms; `distributed` uses one group per numerology slot.

Frequency modes: `grouped` uses configured RBGs; `per_rb` uses one PRB per allocation unit.

Offered traffic is approximately 4 Gbit/s in each direction per case, divided equally across UEs. This maintains backlog without the unbounded memory growth caused by a 100 Gbit/s target per UE.

| grid | ues | time_mode | frequency_mode | reranking | decisions_per_tti | us_per_tti_p50 | us_per_tti_p95 | us_per_tti_p99 | deadline_miss_pct | dl_total_mbps | dl_jain | dl_max_service_gap_ttis |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 20mhz_mu1 | 1 | localized | grouped | none | 6 | 31.399 | 40.218 | 47.465 | 0.0 | 61.632 | 1 | 1 |
| 20mhz_mu1 | 1 | localized | grouped | allocation_unit | 6 | 32.701 | 60.311 | 73.023 | 0.0 | 61.632 | 1 | 1 |
| 20mhz_mu1 | 1 | distributed | per_rb | none | 100 | 90.921 | 114.306 | 129.965 | 0.0 | 64.192 | 1 | 1 |
| 20mhz_mu1 | 1 | distributed | per_rb | allocation_unit | 100 | 95.579 | 125.747 | 147.32 | 0.0 | 64.192 | 1 | 1 |
| 20mhz_mu1 | 16 | localized | grouped | none | 6 | 54.933 | 85.028 | 111.413 | 0.0 | 61.632 | 0.9957 | 27 |
| 20mhz_mu1 | 16 | localized | grouped | allocation_unit | 6 | 54.657 | 82.667 | 104.8 | 0.0 | 61.632 | 0.99988 | 6 |
| 20mhz_mu1 | 16 | distributed | per_rb | none | 100 | 215.899 | 303.53 | 360.882 | 0.0 | 64.192 | 0.99442 | 26 |
| 20mhz_mu1 | 16 | distributed | per_rb | allocation_unit | 100 | 293.83 | 417.979 | 489.662 | 0.0 | 64.192 | 0.99999 | 2 |
| 20mhz_mu1 | 64 | localized | grouped | none | 6 | 122.299 | 188.113 | 228.011 | 0.0 | 61.632 | 0.93476 | 102 |
| 20mhz_mu1 | 64 | localized | grouped | allocation_unit | 6 | 123.376 | 185.777 | 208.311 | 0.0 | 61.632 | 0.99842 | 19 |
| 20mhz_mu1 | 64 | distributed | per_rb | none | 100 | 710.627 | 1114.492 | 1279.953 | 10.5 | 64.192 | 0.93996 | 106 |
| 20mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 100 | 796.317 | 1185.683 | 1366.315 | 19.3 | 64.192 | 0.99955 | 15 |
| 20mhz_mu1 | 256 | localized | grouped | none | 6 | 448.265 | 872.882 | 1132.764 | 2.5 | 61.309 | 0.3094 | 117 |
| 20mhz_mu1 | 256 | localized | grouped | allocation_unit | 6 | 463.85 | 907.3 | 1226.94 | 2.8 | 61.632 | 0.96276 | 70 |
| 20mhz_mu1 | 256 | distributed | per_rb | none | 100 | 3123.04 | 4784.111 | 5607.046 | 100.0 | 64.163 | 0.31668 | 117 |
| 20mhz_mu1 | 256 | distributed | per_rb | allocation_unit | 100 | 3366.26 | 5095.339 | 5822.052 | 100.0 | 64.192 | 0.99555 | 55 |
| 100mhz_mu1 | 1 | localized | grouped | none | 17 | 45.109 | 62.475 | 78.075 | 0.0 | 349.256 | 1 | 1 |
| 100mhz_mu1 | 1 | localized | grouped | allocation_unit | 17 | 42.831 | 57.88 | 70.843 | 0.0 | 349.256 | 1 | 1 |
| 100mhz_mu1 | 1 | distributed | per_rb | none | 546 | 782.808 | 1053.139 | 1284.0 | 7.8 | 350.491 | 1 | 1 |
| 100mhz_mu1 | 1 | distributed | per_rb | allocation_unit | 546 | 777.397 | 1095.022 | 1598.95 | 9.3 | 350.491 | 1 | 1 |
| 100mhz_mu1 | 16 | localized | grouped | none | 17 | 89.262 | 130.677 | 153.825 | 0.0 | 349.22 | 0.99569 | 27 |
| 100mhz_mu1 | 16 | localized | grouped | allocation_unit | 17 | 105.307 | 154.855 | 182.406 | 0.0 | 349.256 | 0.99999 | 3 |
| 100mhz_mu1 | 16 | distributed | per_rb | none | 546 | 1574.455 | 2187.557 | 2553.404 | 100.0 | 350.488 | 0.99523 | 35 |
| 100mhz_mu1 | 16 | distributed | per_rb | allocation_unit | 546 | 1560.145 | 2215.135 | 2463.913 | 99.9 | 350.488 | 0.99958 | 20 |
| 100mhz_mu1 | 64 | localized | grouped | none | 17 | 221.956 | 370.286 | 419.029 | 0.0 | 347.878 | 0.94502 | 103 |
| 100mhz_mu1 | 64 | localized | grouped | allocation_unit | 17 | 252.879 | 413.477 | 488.86 | 0.0 | 349.256 | 0.99985 | 13 |
| 100mhz_mu1 | 64 | distributed | per_rb | none | 546 | 4131.57 | 5946.549 | 6858.68 | 100.0 | 350.467 | 0.92496 | 105 |
| 100mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 546 | 4310.97 | 6354.272 | 6991.77 | 100.0 | 350.467 | 0.98266 | 48 |
| 100mhz_mu1 | 256 | localized | grouped | none | 17 | 919.063 | 1789.109 | 2396.066 | 39.5 | 342.512 | 0.35414 | 117 |
| 100mhz_mu1 | 256 | localized | grouped | allocation_unit | 17 | 958.277 | 1854.722 | 2270.228 | 43.4 | 348.137 | 0.9965 | 44 |
| 100mhz_mu1 | 256 | distributed | per_rb | none | 546 | 13870.15 | 20846.015 | 24111.795 | 100.0 | 350.258 | 0.36771 | 117 |
| 100mhz_mu1 | 256 | distributed | per_rb | allocation_unit | 546 | 14822.15 | 22683.905 | 26410.029 | 100.0 | 350.274 | 0.86913 | 97 |
| 400mhz_mu3 | 1 | localized | grouped | none | 15 | 53.52 | 95.566 | 110.42 | 0.0 | 1159.39 | 1 | 0 |
| 400mhz_mu3 | 1 | localized | grouped | allocation_unit | 15 | 51.416 | 69.354 | 99.609 | 0.0 | 1159.39 | 1 | 0 |
| 400mhz_mu3 | 1 | distributed | per_rb | none | 2000 | 6538.78 | 8813.699 | 9873.837 | 100.0 | 1146.85 | 1 | 0 |
| 400mhz_mu3 | 1 | distributed | per_rb | allocation_unit | 2000 | 6626.66 | 9152.541 | 10430.886 | 100.0 | 1146.85 | 1 | 0 |
| 400mhz_mu3 | 16 | localized | grouped | none | 15 | 108.002 | 150.213 | 186.58 | 0.0 | 1152.36 | 0.99781 | 24 |
| 400mhz_mu3 | 16 | localized | grouped | allocation_unit | 15 | 120.586 | 170.73 | 216.355 | 0.0 | 1153.88 | 0.99996 | 7 |
| 400mhz_mu3 | 16 | distributed | per_rb | none | 2000 | 6192.95 | 8219.496 | 9356.001 | 100.0 | 1146.91 | 0.99205 | 16 |
| 400mhz_mu3 | 16 | distributed | per_rb | allocation_unit | 2000 | 6008.65 | 7754.086 | 8910.967 | 100.0 | 1146.88 | 0.99496 | 15 |
| 400mhz_mu3 | 64 | localized | grouped | none | 15 | 261.706 | 407.862 | 489.413 | 0.0 | 1135.92 | 0.96213 | 83 |
| 400mhz_mu3 | 64 | localized | grouped | allocation_unit | 15 | 280.171 | 433.651 | 602.531 | 0.0 | 1141.01 | 0.99938 | 22 |
| 400mhz_mu3 | 64 | distributed | per_rb | none | 2000 | 12221.6 | 14751.775 | 15938.677 | 100.0 | 1146.85 | 0.9568 | 56 |
| 400mhz_mu3 | 64 | distributed | per_rb | allocation_unit | 2000 | 12699.2 | 15476.325 | 16904.85 | 100.0 | 1146.97 | 0.95821 | 56 |
| 400mhz_mu3 | 256 | localized | grouped | none | 15 | 1023.005 | 1967.224 | 2465.846 | 52.9 | 992.703 | 0.55079 | 117 |
| 400mhz_mu3 | 256 | localized | grouped | allocation_unit | 15 | 1101.025 | 1974.947 | 2430.129 | 65.6 | 1026.03 | 0.99359 | 66 |
| 400mhz_mu3 | 256 | distributed | per_rb | none | 2000 | 37506.05 | 45532.83 | 47657.829 | 100.0 | 1145.08 | 0.5268 | 117 |
| 400mhz_mu3 | 256 | distributed | per_rb | allocation_unit | 2000 | 37822.55 | 45827.695 | 48214.04 | 100.0 | 1144.9 | 0.56569 | 117 |

Failed cases: 0.
