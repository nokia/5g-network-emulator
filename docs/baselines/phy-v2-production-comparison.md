# Original Baseline versus Production PHY Model V2

Both inputs use whole-run metrics. This is a characterization of the combined approved configuration, PHY, PF, O2I, and map changes; it is not a one-factor causal attribution.

| Profile | Dir. | Baseline throughput | V2 throughput | Delta | Outage UEs | Maximum non-outage gap | Payload/grant |
|---|---|---:|---:|---:|---:|---:|---:|
| offline_indoor_hotspot_n78_pedestrian | DL | 61.16 | 47.59 | -22.2% | 0 -> 0 | 0 -> 1510 ms | 24.9% -> 23.0% |
| offline_indoor_hotspot_n78_pedestrian | UL | 90.00 | 46.90 | -47.9% | 0 -> 0 | 0 -> 44810 ms | 57.6% -> 48.2% |
| offline_rural_n78_vehicular | DL | 82.64 | 61.81 | -25.2% | 0 -> 0 | 32120 -> 110230 ms | 97.7% -> 98.6% |
| offline_rural_n78_vehicular | UL | 97.11 | 27.18 | -72.0% | 0 -> 0 | 13380 -> 43700 ms | 98.7% -> 89.4% |
| offline_uma_n78_pedestrian | DL | 60.29 | 108.22 | +79.5% | 3 -> 6 | 180000 -> 110570 ms | 43.8% -> 71.9% |
| offline_uma_n78_pedestrian | UL | 32.32 | 14.11 | -56.3% | 2 -> 1 | 180000 -> 180000 ms | 30.1% -> 99.4% |
| offline_umi_n258_fwa | DL | 51.45 | 499.98 | +871.8% | 5 -> 0 | 10 -> 10 ms | 88.4% -> 38.1% |
| offline_umi_n258_fwa | UL | 40.01 | 156.53 | +291.3% | 6 -> 0 | 1670 -> 610 ms | 92.5% -> 21.0% |
| offline_umi_n40_npn | DL | 14.93 | 48.26 | +223.2% | 0 -> 0 | 124270 -> 160 ms | 20.5% -> 74.8% |
| offline_umi_n40_npn | UL | 18.19 | 24.11 | +32.6% | 0 -> 0 | 130310 -> 510 ms | 48.2% -> 75.7% |

Map activation parity is established by direct equality of all 21 reviewed and promoted numeric arrays. Historical candidate profile logs predate the later MCS-layer indexing correction and are not used as a one-factor profile oracle for this table.
