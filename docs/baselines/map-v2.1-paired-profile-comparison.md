# Legacy V1 versus Padded V2.1 Map Profiles

Both batches use the same simulator code, profile settings, traffic, mobility, seed, duration, and analysis. Only the explicit map catalog differs. The legacy n40 profile uses its historical nearest 3.5 GHz UMi map; V2.1 uses the exact 2.38 GHz map.

| Profile | Dir. | Legacy throughput | V2.1 throughput | Delta | Median-UE SINR delta | Outage UEs |
|---|---|---:|---:|---:|---:|---:|
| offline_indoor_hotspot_n78_pedestrian | DL | 45.74 | 44.61 | -2.5% | +4.10 dB | 0 -> 0 |
| offline_indoor_hotspot_n78_pedestrian | UL | 56.56 | 57.52 | +1.7% | +0.20 dB | 0 -> 0 |
| offline_rural_n78_vehicular | DL | 46.16 | 52.82 | +14.4% | +0.93 dB | 1 -> 0 |
| offline_rural_n78_vehicular | UL | 25.65 | 27.21 | +6.1% | +0.15 dB | 0 -> 0 |
| offline_uma_n78_pedestrian | DL | 111.48 | 106.75 | -4.2% | -5.55 dB | 5 -> 6 |
| offline_uma_n78_pedestrian | UL | 13.79 | 13.91 | +0.9% | -3.11 dB | 0 -> 0 |
| offline_umi_n258_fwa | DL | 500.02 | 500.02 | +0.0% | -14.58 dB | 0 -> 0 |
| offline_umi_n258_fwa | UL | 156.76 | 156.73 | -0.0% | -13.26 dB | 0 -> 0 |
| offline_umi_n40_npn | DL | 48.24 | 47.19 | -2.2% | +6.96 dB | 0 -> 0 |
| offline_umi_n40_npn | UL | 25.03 | 24.16 | -3.5% | +2.95 dB | 0 -> 0 |

This is a one-factor catalog comparison for the fixed seed. It does not estimate the expected field-performance change over map realizations.
