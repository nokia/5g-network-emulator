# Original Baseline versus Production PHY Model V2

Both inputs use whole-run metrics. This is a characterization of the combined approved configuration, PHY, PF, O2I, and map changes; it is not a one-factor causal attribution.

| Profile | Dir. | Baseline throughput | V2 throughput | Delta | Outage UEs | Maximum non-outage gap | Payload/grant |
|---|---|---:|---:|---:|---:|---:|---:|
| offline_indoor_hotspot_n78_pedestrian | DL | 61.16 | 47.84 | -21.8% | 0 -> 0 | 0 -> 1510 ms | 24.9% -> 23.2% |
| offline_indoor_hotspot_n78_pedestrian | UL | 90.00 | 47.62 | -47.1% | 0 -> 0 | 0 -> 43750 ms | 57.6% -> 48.7% |
| offline_rural_n78_vehicular | DL | 82.64 | 62.25 | -24.7% | 0 -> 0 | 32120 -> 110330 ms | 97.7% -> 98.6% |
| offline_rural_n78_vehicular | UL | 97.11 | 27.49 | -71.7% | 0 -> 0 | 13380 -> 43850 ms | 98.7% -> 88.4% |
| offline_uma_n78_pedestrian | DL | 60.29 | 108.73 | +80.4% | 3 -> 5 | 180000 -> 110570 ms | 43.8% -> 72.5% |
| offline_uma_n78_pedestrian | UL | 32.32 | 14.31 | -55.7% | 2 -> 1 | 180000 -> 180000 ms | 30.1% -> 98.5% |
| offline_umi_n258_fwa | DL | 51.45 | 499.98 | +871.8% | 5 -> 0 | 10 -> 10 ms | 88.4% -> 38.1% |
| offline_umi_n258_fwa | UL | 40.01 | 156.53 | +291.3% | 6 -> 0 | 1670 -> 610 ms | 92.5% -> 20.9% |
| offline_umi_n40_npn | DL | 14.93 | 48.28 | +223.4% | 0 -> 0 | 124270 -> 170 ms | 20.5% -> 74.8% |
| offline_umi_n40_npn | UL | 18.19 | 24.19 | +33.0% | 0 -> 0 | 130310 -> 500 ms | 48.2% -> 76.3% |

The production run is independently compared with the reviewed v2 candidate run. All shared numeric metrics are exactly equal, confirming that activation changed metadata and default lookup but not the approved numeric map arrays.
