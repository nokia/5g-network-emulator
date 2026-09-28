# PF Granularity Benchmark

Warm-up steps: 20; measured steps per sample: 100; repeats: 10.

Host: `pitahaya`; platform: `Linux-5.15.0-58-generic-x86_64-with-glibc2.35`.

Time modes: `localized` uses one time allocation group per 1 ms; `distributed` uses one group per numerology slot.

Frequency modes: `grouped` uses configured RBGs; `per_rb` uses one PRB per allocation unit.

Offered traffic is approximately 4 Gbit/s in each direction per case, divided equally across UEs. This maintains backlog without the unbounded memory growth caused by a 100 Gbit/s target per UE.

| grid | ues | time_mode | frequency_mode | reranking | decisions_per_tti | us_per_tti_p50 | us_per_tti_p95 | us_per_tti_p99 | dl_total_mbps | dl_jain | dl_max_service_gap_ttis |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 20mhz_mu1 | 1 | localized | grouped | none | 6 | 36.854 | 39.713 | 39.971 | 61.632 | 1 | 1 |
| 20mhz_mu1 | 1 | localized | grouped | allocation_unit | 6 | 36.459 | 39.525 | 40.188 | 61.632 | 1 | 1 |
| 20mhz_mu1 | 1 | distributed | per_rb | none | 100 | 103.478 | 111.188 | 111.504 | 64.192 | 1 | 1 |
| 20mhz_mu1 | 1 | distributed | per_rb | allocation_unit | 100 | 91.535 | 112.739 | 112.763 | 64.192 | 1 | 1 |
| 20mhz_mu1 | 16 | localized | grouped | none | 6 | 58.1 | 62.229 | 62.277 | 61.632 | 0.9957 | 27 |
| 20mhz_mu1 | 16 | localized | grouped | allocation_unit | 6 | 61.011 | 65.625 | 66.28 | 61.632 | 0.99988 | 6 |
| 20mhz_mu1 | 16 | distributed | per_rb | none | 100 | 258.688 | 268.367 | 270.469 | 64.192 | 0.99442 | 26 |
| 20mhz_mu1 | 16 | distributed | per_rb | allocation_unit | 100 | 296.425 | 309.218 | 310.079 | 64.192 | 0.99999 | 2 |
| 20mhz_mu1 | 64 | localized | grouped | none | 6 | 139.101 | 148.763 | 148.769 | 61.632 | 0.93476 | 102 |
| 20mhz_mu1 | 64 | localized | grouped | allocation_unit | 6 | 137.74 | 150.397 | 152.745 | 61.632 | 0.99842 | 19 |
| 20mhz_mu1 | 64 | distributed | per_rb | none | 100 | 718.504 | 797.911 | 805.294 | 64.192 | 0.93996 | 106 |
| 20mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 100 | 830.862 | 893.647 | 901.391 | 64.192 | 0.99955 | 15 |
| 20mhz_mu1 | 256 | localized | grouped | none | 6 | 426.18 | 486.342 | 498.418 | 61.309 | 0.3094 | 117 |
| 20mhz_mu1 | 256 | localized | grouped | allocation_unit | 6 | 459.385 | 597.743 | 644.269 | 61.632 | 0.96276 | 70 |
| 20mhz_mu1 | 256 | distributed | per_rb | none | 100 | 2957.48 | 3011.767 | 3014.481 | 64.163 | 0.31668 | 117 |
| 20mhz_mu1 | 256 | distributed | per_rb | allocation_unit | 100 | 2994.795 | 3089.48 | 3102.872 | 64.192 | 0.99555 | 55 |
| 100mhz_mu1 | 1 | localized | grouped | none | 17 | 42.194 | 50.695 | 50.916 | 349.256 | 1 | 1 |
| 100mhz_mu1 | 1 | localized | grouped | allocation_unit | 17 | 43.116 | 48.139 | 49.871 | 349.256 | 1 | 1 |
| 100mhz_mu1 | 1 | distributed | per_rb | none | 546 | 697.865 | 784.134 | 806.428 | 350.491 | 1 | 1 |
| 100mhz_mu1 | 1 | distributed | per_rb | allocation_unit | 546 | 717.958 | 788.602 | 797.664 | 350.491 | 1 | 1 |
| 100mhz_mu1 | 16 | localized | grouped | none | 17 | 88.609 | 92.089 | 92.421 | 349.22 | 0.99569 | 27 |
| 100mhz_mu1 | 16 | localized | grouped | allocation_unit | 17 | 96.642 | 100.882 | 100.96 | 349.256 | 0.99999 | 3 |
| 100mhz_mu1 | 16 | distributed | per_rb | none | 546 | 1377.175 | 1535.786 | 1548.613 | 350.488 | 0.99523 | 35 |
| 100mhz_mu1 | 16 | distributed | per_rb | allocation_unit | 546 | 1471.595 | 1568.129 | 1616.578 | 350.488 | 0.99958 | 20 |
| 100mhz_mu1 | 64 | localized | grouped | none | 17 | 252.298 | 265.138 | 265.226 | 347.878 | 0.94502 | 103 |
| 100mhz_mu1 | 64 | localized | grouped | allocation_unit | 17 | 268.796 | 289.745 | 292.439 | 349.256 | 0.99985 | 13 |
| 100mhz_mu1 | 64 | distributed | per_rb | none | 546 | 3996.875 | 4080.94 | 4104.38 | 350.467 | 0.92496 | 105 |
| 100mhz_mu1 | 64 | distributed | per_rb | allocation_unit | 546 | 4163.335 | 4258.426 | 4281.077 | 350.467 | 0.98266 | 48 |
| 100mhz_mu1 | 256 | localized | grouped | none | 17 | 923.236 | 962.154 | 963.0 | 342.512 | 0.35414 | 117 |
| 100mhz_mu1 | 256 | localized | grouped | allocation_unit | 17 | 947.877 | 1016.284 | 1042.041 | 348.137 | 0.9965 | 44 |
| 100mhz_mu1 | 256 | distributed | per_rb | none | 546 | 14639.35 | 15079.44 | 15135.168 | 350.258 | 0.36771 | 117 |
| 100mhz_mu1 | 256 | distributed | per_rb | allocation_unit | 546 | 14717.8 | 16080.315 | 16273.743 | 350.274 | 0.86913 | 97 |
| 400mhz_mu3 | 1 | localized | grouped | none | 15 | 53.605 | 65.363 | 71.414 | 1101.73 | 1 | 0 |
| 400mhz_mu3 | 1 | localized | grouped | allocation_unit | 15 | 51.71 | 53.819 | 54.207 | 1101.73 | 1 | 0 |
| 400mhz_mu3 | 1 | distributed | per_rb | none | 2000 | 5899.19 | 6206.095 | 6257.611 | 1086.44 | 1 | 0 |
| 400mhz_mu3 | 1 | distributed | per_rb | allocation_unit | 2000 | 5800.69 | 5945.357 | 5959.447 | 1086.44 | 1 | 0 |
| 400mhz_mu3 | 16 | localized | grouped | none | 15 | 101.627 | 113.444 | 114.002 | 1096.54 | 0.99809 | 24 |
| 400mhz_mu3 | 16 | localized | grouped | allocation_unit | 15 | 115.486 | 123.094 | 123.189 | 1097.43 | 0.99996 | 7 |
| 400mhz_mu3 | 16 | distributed | per_rb | none | 2000 | 5500.96 | 5700.401 | 5707.36 | 1086.19 | 0.99499 | 16 |
| 400mhz_mu3 | 16 | distributed | per_rb | allocation_unit | 2000 | 5215.09 | 5497.524 | 5596.089 | 1086.49 | 0.99605 | 15 |
| 400mhz_mu3 | 64 | localized | grouped | none | 15 | 267.469 | 289.106 | 294.753 | 1085.93 | 0.96143 | 85 |
| 400mhz_mu3 | 64 | localized | grouped | allocation_unit | 15 | 296.023 | 339.019 | 344.395 | 1085.48 | 0.99969 | 22 |
| 400mhz_mu3 | 64 | distributed | per_rb | none | 2000 | 11395.2 | 11808.425 | 11836.325 | 1085.96 | 0.95281 | 57 |
| 400mhz_mu3 | 64 | distributed | per_rb | allocation_unit | 2000 | 10533.95 | 10708.84 | 10735.048 | 1086.05 | 0.95317 | 57 |
| 400mhz_mu3 | 256 | localized | grouped | none | 15 | 893.716 | 945.314 | 956.437 | 950.893 | 0.53936 | 117 |
| 400mhz_mu3 | 256 | localized | grouped | allocation_unit | 15 | 1002.827 | 1084.802 | 1089.784 | 982.909 | 0.98975 | 73 |
| 400mhz_mu3 | 256 | distributed | per_rb | none | 2000 | 35075.7 | 36743.73 | 36782.826 | 1084.81 | 0.50889 | 117 |
| 400mhz_mu3 | 256 | distributed | per_rb | allocation_unit | 2000 | 33959.95 | 36174.93 | 36595.626 | 1084.6 | 0.55345 | 117 |

Failed cases: 0.
