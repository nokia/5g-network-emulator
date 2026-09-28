# Original Baseline versus Production PHY Model V2

Both inputs use whole-run metrics. This is a characterization of the combined approved configuration, PHY, PF, O2I, and map changes; it is not a one-factor causal attribution.

| Profile | Dir. | Baseline throughput | V2 throughput | Delta | Outage UEs | Maximum non-outage gap | Payload/grant |
|---|---|---:|---:|---:|---:|---:|---:|
| offline_indoor_hotspot_n78_pedestrian | DL | 61.16 | 43.55 | -28.8% | 0 -> 0 | 0 -> 2430 ms | 24.9% -> 21.6% |
| offline_indoor_hotspot_n78_pedestrian | UL | 90.00 | 58.77 | -34.7% | 0 -> 0 | 0 -> 1070 ms | 57.6% -> 47.0% |
| offline_rural_n78_vehicular | DL | 82.64 | 53.23 | -35.6% | 0 -> 0 | 32120 -> 104340 ms | 97.7% -> 98.6% |
| offline_rural_n78_vehicular | UL | 97.11 | 27.60 | -71.6% | 0 -> 0 | 13380 -> 98700 ms | 98.7% -> 82.6% |
| offline_uma_n78_pedestrian | DL | 60.29 | 106.92 | +77.3% | 3 -> 6 | 180000 -> 126870 ms | 43.8% -> 72.5% |
| offline_uma_n78_pedestrian | UL | 32.32 | 14.02 | -56.6% | 2 -> 1 | 180000 -> 179990 ms | 30.1% -> 98.0% |
| offline_umi_n258_fwa | DL | 51.45 | 499.98 | +871.8% | 5 -> 0 | 10 -> 0 ms | 88.4% -> 38.1% |
| offline_umi_n258_fwa | UL | 40.01 | 156.53 | +291.3% | 6 -> 0 | 1670 -> 600 ms | 92.5% -> 21.1% |
| offline_umi_n40_npn | DL | 14.93 | 47.06 | +215.2% | 0 -> 0 | 124270 -> 100 ms | 20.5% -> 74.1% |
| offline_umi_n40_npn | UL | 18.19 | 24.46 | +34.5% | 0 -> 0 | 130310 -> 390 ms | 48.2% -> 75.9% |

This combined table is not the map-catalog ablation. The paired legacy-v1 versus padded-v2.1 comparison is reported separately in `map-v2.1-paired-profile-comparison.md`.
