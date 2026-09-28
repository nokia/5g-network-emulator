# PF Granularity Benchmark

Warm-up steps: 20; measured steps per sample: 100; repeats: 10.

P50/P95/P99/max are computed from individually timed warmed-up TTIs across all repeats. A compute-budget exceedance is a TTI above 1,000 us.

Host: `pitahaya`; platform: `Linux-5.15.0-58-generic-x86_64-with-glibc2.35`.

Time modes: `localized` uses one time allocation group per 1 ms; `distributed` uses one group per numerology slot.

Frequency modes: `grouped` uses configured RBGs; `per_rb` uses one PRB per allocation unit.

Offered traffic is approximately 4 Gbit/s in each direction per case, divided equally across UEs. This maintains backlog without the unbounded memory growth caused by a 100 Gbit/s target per UE.

| grid | ues | time_mode | frequency_mode | reranking | decisions_per_tti | us_per_tti_p50 | us_per_tti_p95 | us_per_tti_p99 | us_per_tti_max | compute_budget_exceedance_pct | dl_total_mbps | dl_jain | dl_max_service_gap_ttis |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 20mhz_mu1 | 1 | localized | grouped | none | 6 | 29.465 | 39.245 | 49.824 | 60.222 | 0.0 | 66.291 | 1 | 1 |
| 20mhz_mu1 | 1 | localized | grouped | allocation_unit | 6 | 31.199 | 39.971 | 43.933 | 55.434 | 0.0 | 66.291 | 1 | 1 |
| 20mhz_mu1 | 1 | distributed | per_rb | none | 100 | 88.912 | 110.874 | 119.849 | 135.063 | 0.0 | 69.12 | 1 | 1 |
| 20mhz_mu1 | 1 | distributed | per_rb | allocation_unit | 100 | 88.04 | 112.801 | 125.916 | 145.273 | 0.0 | 69.12 | 1 | 1 |
| 20mhz_mu1 | 16 | localized | grouped | none | 6 | 50.66 | 70.652 | 78.093 | 105.057 | 0.0 | 66.291 | 0.98699 | 27 |
| 20mhz_mu1 | 16 | localized | grouped | allocation_unit | 6 | 52.794 | 72.58 | 82.53 | 112.26 | 0.0 | 66.291 | 0.99974 | 5 |
| 20mhz_mu1 | 16 | distributed | per_rb | none | 100 | 222.472 | 305.714 | 350.139 | 383.549 | 0.0 | 69.056 | 0.98664 | 26 |
| 20mhz_mu1 | 16 | distributed | per_rb | allocation_unit | 100 | 275.276 | 380.206 | 432.065 | 483.787 | 0.0 | 69.013 | 0.99988 | 2 |
| 20mhz_mu1 | 64 | localized | grouped | none | 6 | 117.019 | 180.894 | 194.794 | 227.035 | 0.0 | 66.547 | 0.93982 | 99 |
| 20mhz_mu1 | 64 | localized | grouped | allocation_unit | 6 | 114.559 | 179.117 | 199.209 | 287.189 | 0.0 | 66.291 | 0.9958 | 18 |
| 20mhz_mu1 | 64 | distributed | per_rb | none | 100 | 613.5 | 973.488 | 1072.915 | 1214 | 3.6 | 69.024 | 0.91477 | 100 |
| 20mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 100 | 689.693 | 1006.851 | 1119.399 | 1203.7 | 5.5 | 69.056 | 0.99963 | 8 |
| 20mhz_mu1 | 256 | localized | grouped | none | 6 | 393.413 | 664.389 | 778.802 | 985.357 | 0.0 | 66.205 | 0.27381 | 100 |
| 20mhz_mu1 | 256 | localized | grouped | allocation_unit | 6 | 409.974 | 742.207 | 915.337 | 1160.38 | 0.4 | 66.376 | 0.91861 | 69 |
| 20mhz_mu1 | 256 | distributed | per_rb | none | 100 | 2535.64 | 3883.76 | 4519.381 | 5093.83 | 100.0 | 69.046 | 0.27835 | 100 |
| 20mhz_mu1 | 256 | distributed | per_rb | allocation_unit | 100 | 2737.27 | 4106.633 | 4637.372 | 6163.37 | 100.0 | 69.088 | 0.99528 | 44 |
| 100mhz_mu1 | 1 | localized | grouped | none | 17 | 43.171 | 55.414 | 61.588 | 68.489 | 0.0 | 376.086 | 1 | 1 |
| 100mhz_mu1 | 1 | localized | grouped | allocation_unit | 17 | 44.353 | 57.947 | 65.433 | 91.291 | 0.0 | 376.086 | 1 | 1 |
| 100mhz_mu1 | 1 | distributed | per_rb | none | 546 | 655.033 | 806.354 | 871.558 | 1051.84 | 0.1 | 377.004 | 1 | 1 |
| 100mhz_mu1 | 1 | distributed | per_rb | allocation_unit | 546 | 665.187 | 819.855 | 898.182 | 1019.21 | 0.1 | 377.004 | 1 | 1 |
| 100mhz_mu1 | 16 | localized | grouped | none | 17 | 77.871 | 118.18 | 128.306 | 140.814 | 0.0 | 375.572 | 0.98803 | 27 |
| 100mhz_mu1 | 16 | localized | grouped | allocation_unit | 17 | 88.04 | 133.893 | 144.862 | 210.024 | 0.0 | 375.572 | 0.99989 | 3 |
| 100mhz_mu1 | 16 | distributed | per_rb | none | 546 | 1247.58 | 1682.858 | 1879.121 | 2163.57 | 97.1 | 377.101 | 0.97899 | 35 |
| 100mhz_mu1 | 16 | distributed | per_rb | allocation_unit | 546 | 1354.885 | 1836.388 | 2065.003 | 2241.91 | 97.3 | 376.887 | 0.99111 | 12 |
| 100mhz_mu1 | 64 | localized | grouped | none | 17 | 194.78 | 354.58 | 376.113 | 414.036 | 0.0 | 376.086 | 0.93717 | 100 |
| 100mhz_mu1 | 64 | localized | grouped | allocation_unit | 17 | 223.364 | 385.403 | 417.865 | 432.161 | 0.0 | 376.086 | 0.99962 | 11 |
| 100mhz_mu1 | 64 | distributed | per_rb | none | 546 | 3624.085 | 5408.613 | 6218.08 | 8250.93 | 100.0 | 376.897 | 0.89979 | 100 |
| 100mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 546 | 3773.78 | 5643.78 | 6228.943 | 6524.17 | 100.0 | 377.058 | 0.96132 | 48 |
| 100mhz_mu1 | 256 | localized | grouped | none | 17 | 747.712 | 1417.903 | 1649.995 | 2029.05 | 23.2 | 373.038 | 0.29829 | 100 |
| 100mhz_mu1 | 256 | localized | grouped | allocation_unit | 17 | 780.433 | 1492.234 | 1715.658 | 2144.05 | 22.8 | 375.401 | 0.96741 | 38 |
| 100mhz_mu1 | 256 | distributed | per_rb | none | 546 | 12027.3 | 18431.855 | 21060.913 | 21955.1 | 100.0 | 377.07 | 0.3244 | 100 |
| 100mhz_mu1 | 256 | distributed | per_rb | allocation_unit | 546 | 12563.75 | 19554.5 | 21950.625 | 24286.7 | 100.0 | 377.133 | 0.91595 | 91 |
| 400mhz_mu3 | 1 | localized | grouped | none | 15 | 48.566 | 58.33 | 67.571 | 77.755 | 0.0 | 1249.01 | 1 | 0 |
| 400mhz_mu3 | 1 | localized | grouped | allocation_unit | 15 | 48.251 | 58.912 | 65.465 | 79.449 | 0.0 | 1249.01 | 1 | 0 |
| 400mhz_mu3 | 1 | distributed | per_rb | none | 2000 | 5410.61 | 7085.852 | 7853.593 | 10802.1 | 100.0 | 1233.48 | 1 | 0 |
| 400mhz_mu3 | 1 | distributed | per_rb | allocation_unit | 2000 | 5333.265 | 7018.127 | 7650.78 | 8228.76 | 100.0 | 1233.48 | 1 | 0 |
| 400mhz_mu3 | 16 | localized | grouped | none | 15 | 94.046 | 125.97 | 138.101 | 157.175 | 0.0 | 1246.57 | 0.99468 | 24 |
| 400mhz_mu3 | 16 | localized | grouped | allocation_unit | 15 | 110.151 | 141.325 | 156.703 | 430.778 | 0.0 | 1245.66 | 0.99983 | 4 |
| 400mhz_mu3 | 16 | distributed | per_rb | none | 2000 | 4992.4 | 6431.479 | 7210.819 | 8093.91 | 100.0 | 1232.82 | 0.98135 | 16 |
| 400mhz_mu3 | 16 | distributed | per_rb | allocation_unit | 2000 | 4858.425 | 5872.963 | 6325.085 | 7338.22 | 100.0 | 1232.89 | 0.99432 | 15 |
| 400mhz_mu3 | 64 | localized | grouped | none | 15 | 228.423 | 345.176 | 377.307 | 394.219 | 0.0 | 1242.7 | 0.92353 | 83 |
| 400mhz_mu3 | 64 | localized | grouped | allocation_unit | 15 | 255.584 | 372.741 | 399.965 | 442.38 | 0.0 | 1248.56 | 0.99375 | 18 |
| 400mhz_mu3 | 64 | distributed | per_rb | none | 2000 | 10466.9 | 12587.585 | 13493.553 | 13866.7 | 100.0 | 1233.21 | 0.97905 | 56 |
| 400mhz_mu3 | 64 | distributed | per_rb | allocation_unit | 2000 | 10728.55 | 12930.82 | 13547.185 | 15128.2 | 100.0 | 1233.15 | 0.97937 | 56 |
| 400mhz_mu3 | 256 | localized | grouped | none | 15 | 803.807 | 1466.483 | 1741.979 | 1914.67 | 22.4 | 1163.34 | 0.5199 | 100 |
| 400mhz_mu3 | 256 | localized | grouped | allocation_unit | 15 | 860.584 | 1513.339 | 1772.455 | 2152.28 | 29.6 | 1204.7 | 0.9859 | 66 |
| 400mhz_mu3 | 256 | distributed | per_rb | none | 2000 | 32909.65 | 40158.04 | 41623.354 | 44516.4 | 100.0 | 1232.77 | 0.49466 | 100 |
| 400mhz_mu3 | 256 | distributed | per_rb | allocation_unit | 2000 | 34267.95 | 43087.11 | 44661.062 | 50279.2 | 100.0 | 1232.75 | 0.54132 | 100 |

Failed cases: 0.
