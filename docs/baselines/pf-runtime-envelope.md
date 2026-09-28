# PF Granularity Benchmark

Warm-up steps: 20; measured steps per sample: 100; repeats: 10.

Host: `pitahaya`; platform: `Linux-5.15.0-58-generic-x86_64-with-glibc2.35`.

Time modes: `localized` uses one time allocation group per 1 ms; `distributed` uses one group per numerology slot.

Frequency modes: `grouped` uses configured RBGs; `per_rb` uses one PRB per allocation unit.

Offered traffic is approximately 4 Gbit/s in each direction per case, divided equally across UEs. This maintains backlog without the unbounded memory growth caused by a 100 Gbit/s target per UE.

| grid | ues | time_mode | frequency_mode | reranking | decisions_per_tti | us_per_tti_p50 | us_per_tti_p95 | us_per_tti_p99 | dl_total_mbps | dl_jain | dl_max_service_gap_ttis |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 20mhz_mu1 | 1 | localized | grouped | none | 6 | 36.844 | 53.857 | 58.334 | 61.632 | 1 | 1 |
| 20mhz_mu1 | 1 | localized | grouped | allocation_unit | 6 | 35.033 | 51.051 | 54.675 | 61.632 | 1 | 1 |
| 20mhz_mu1 | 1 | distributed | per_rb | none | 100 | 103.26 | 116.169 | 119.971 | 64.192 | 1 | 1 |
| 20mhz_mu1 | 1 | distributed | per_rb | allocation_unit | 100 | 103.727 | 108.966 | 110.5 | 64.192 | 1 | 1 |
| 20mhz_mu1 | 16 | localized | grouped | none | 6 | 60.187 | 69.033 | 72.87 | 61.632 | 0.9957 | 27 |
| 20mhz_mu1 | 16 | localized | grouped | allocation_unit | 6 | 62.045 | 71.707 | 73.618 | 61.632 | 0.99988 | 6 |
| 20mhz_mu1 | 16 | distributed | per_rb | none | 100 | 253.839 | 267.256 | 270.546 | 64.192 | 0.99442 | 26 |
| 20mhz_mu1 | 16 | distributed | per_rb | allocation_unit | 100 | 279.391 | 306.813 | 310.918 | 64.192 | 0.99999 | 2 |
| 20mhz_mu1 | 64 | localized | grouped | none | 6 | 143.233 | 151.351 | 153.237 | 61.632 | 0.93476 | 102 |
| 20mhz_mu1 | 64 | localized | grouped | allocation_unit | 6 | 147.82 | 161.621 | 163.451 | 61.632 | 0.99842 | 19 |
| 20mhz_mu1 | 64 | distributed | per_rb | none | 100 | 758.988 | 788.416 | 799.95 | 64.192 | 0.93996 | 106 |
| 20mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 100 | 884.188 | 921.127 | 923.873 | 64.192 | 0.99955 | 15 |
| 20mhz_mu1 | 256 | localized | grouped | none | 6 | 542.798 | 588.985 | 597.707 | 61.309 | 0.3094 | 117 |
| 20mhz_mu1 | 256 | localized | grouped | allocation_unit | 6 | 555.923 | 639.647 | 667.497 | 61.632 | 0.96276 | 70 |
| 20mhz_mu1 | 256 | distributed | per_rb | none | 100 | 3464.135 | 3702.185 | 3786.205 | 64.163 | 0.31668 | 117 |
| 20mhz_mu1 | 256 | distributed | per_rb | allocation_unit | 100 | 3568.865 | 3945.255 | 3965.923 | 64.192 | 0.99555 | 55 |
| 100mhz_mu1 | 1 | localized | grouped | none | 17 | 45.359 | 53.538 | 57.036 | 349.256 | 1 | 1 |
| 100mhz_mu1 | 1 | localized | grouped | allocation_unit | 17 | 46.473 | 52.428 | 54.105 | 349.256 | 1 | 1 |
| 100mhz_mu1 | 1 | distributed | per_rb | none | 546 | 828.547 | 909.328 | 932.226 | 350.491 | 1 | 1 |
| 100mhz_mu1 | 1 | distributed | per_rb | allocation_unit | 546 | 882.394 | 1000.892 | 1072.754 | 350.491 | 1 | 1 |
| 100mhz_mu1 | 16 | localized | grouped | none | 17 | 98.03 | 121.518 | 133.912 | 349.22 | 0.99569 | 27 |
| 100mhz_mu1 | 16 | localized | grouped | allocation_unit | 17 | 104.674 | 118.038 | 119.329 | 349.256 | 0.99999 | 3 |
| 100mhz_mu1 | 16 | distributed | per_rb | none | 546 | 1630.8 | 1688.691 | 1700.618 | 350.488 | 0.99523 | 35 |
| 100mhz_mu1 | 16 | distributed | per_rb | allocation_unit | 546 | 1809.765 | 1922.29 | 1965.778 | 350.488 | 0.99958 | 20 |
| 100mhz_mu1 | 64 | localized | grouped | none | 17 | 268.377 | 296.69 | 301.977 | 347.878 | 0.94502 | 103 |
| 100mhz_mu1 | 64 | localized | grouped | allocation_unit | 17 | 292.711 | 307.852 | 310.528 | 349.256 | 0.99985 | 13 |
| 100mhz_mu1 | 64 | distributed | per_rb | none | 546 | 4703.44 | 5180.031 | 5329.19 | 350.467 | 0.92496 | 105 |
| 100mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 546 | 4708.4 | 4848.255 | 4864.955 | 350.467 | 0.98266 | 48 |
| 100mhz_mu1 | 256 | localized | grouped | none | 17 | 1017.245 | 1225.672 | 1254.486 | 342.512 | 0.35414 | 117 |
| 100mhz_mu1 | 256 | localized | grouped | allocation_unit | 17 | 1091.245 | 1250.277 | 1320.023 | 348.137 | 0.9965 | 44 |
| 100mhz_mu1 | 256 | distributed | per_rb | none | 546 | 15829.45 | 16817.44 | 16849.408 | 350.258 | 0.36771 | 117 |
| 100mhz_mu1 | 256 | distributed | per_rb | allocation_unit | 546 | 16355.3 | 17110.87 | 17251.054 | 350.274 | 0.86913 | 97 |
| 400mhz_mu3 | 1 | localized | grouped | none | 15 | 57.625 | 87.653 | 93.233 | 1101.73 | 1 | 0 |
| 400mhz_mu3 | 1 | localized | grouped | allocation_unit | 15 | 54.688 | 60.994 | 61.821 | 1101.73 | 1 | 0 |
| 400mhz_mu3 | 1 | distributed | per_rb | none | 2000 | 6735.725 | 7204.208 | 7244.586 | 1086.44 | 1 | 0 |
| 400mhz_mu3 | 1 | distributed | per_rb | allocation_unit | 2000 | 6869.425 | 7157.476 | 7303.647 | 1086.44 | 1 | 0 |
| 400mhz_mu3 | 16 | localized | grouped | none | 15 | 104.921 | 124.711 | 131.066 | 1096.54 | 0.99809 | 24 |
| 400mhz_mu3 | 16 | localized | grouped | allocation_unit | 15 | 127.691 | 141.263 | 141.513 | 1097.43 | 0.99996 | 7 |
| 400mhz_mu3 | 16 | distributed | per_rb | none | 2000 | 6372.425 | 6707.477 | 6744.432 | 1086.19 | 0.99499 | 16 |
| 400mhz_mu3 | 16 | distributed | per_rb | allocation_unit | 2000 | 6224.35 | 6489.307 | 6605.101 | 1086.49 | 0.99605 | 15 |
| 400mhz_mu3 | 64 | localized | grouped | none | 15 | 291.999 | 388.839 | 407.089 | 1085.93 | 0.96143 | 85 |
| 400mhz_mu3 | 64 | localized | grouped | allocation_unit | 15 | 332.322 | 369.296 | 376.345 | 1085.48 | 0.99969 | 22 |
| 400mhz_mu3 | 64 | distributed | per_rb | none | 2000 | 13095.95 | 13607.195 | 13639.919 | 1085.96 | 0.95281 | 57 |
| 400mhz_mu3 | 64 | distributed | per_rb | allocation_unit | 2000 | 13122.2 | 13497.24 | 13595.448 | 1086.05 | 0.95317 | 57 |
| 400mhz_mu3 | 256 | localized | grouped | none | 15 | 1198.24 | 1292.353 | 1293.815 | 950.893 | 0.53936 | 117 |
| 400mhz_mu3 | 256 | localized | grouped | allocation_unit | 15 | 1261.625 | 1312.841 | 1313.896 | 982.909 | 0.98975 | 73 |
| 400mhz_mu3 | 256 | distributed | per_rb | none | 2000 | 39237.8 | 41292.35 | 41304.23 | 1084.81 | 0.50889 | 117 |
| 400mhz_mu3 | 256 | distributed | per_rb | allocation_unit | 2000 | 40525.95 | 42064.94 | 42251.708 | 1084.6 | 0.55345 | 117 |

Failed cases: 0.
