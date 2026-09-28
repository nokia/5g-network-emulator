# V2 Map Multi-Seed Validation

## Method

- Independent master seeds: 30
- Master-seed sequence: `20270000` through `20270029`
- Catalog entries per seed: 21
- Runtime: 32.04 seconds
- Confidence intervals: two-sided 95% intervals over independent realizations; spatial cells are not treated as replicates.
- Coverage proxy: fraction of map cells with map-only, pre-interference full-channel DL SNR >= 0 dB.

## Per-map summary

| Scenario | GHz | Link gain P50 (dB) | LOS radial RMSE | LOS shadow std (dB) | NLOS shadow std (dB) | LOS/NLOS correlation observed/target | Coverage proxy |
|---|---:|---:|---:|---:|---:|---:|---:|
| INDOOR_MIXED_OFFICE | 3.5 | -131.12 [-131.15, -131.09] | 0.021 | 4.30 | 7.90 | 0.406/0.407; 0.367/0.368 | n/a |
| INDOOR_MIXED_OFFICE | 26.0 | -146.54 [-146.56, -146.52] | 0.022 | 4.30 | 7.90 | 0.407/0.407; 0.368/0.368 | n/a |
| INDOOR_MIXED_OFFICE | 28.0 | -147.20 [-147.22, -147.18] | 0.018 | 4.30 | 7.90 | 0.403/0.407; 0.368/0.368 | n/a |
| INDOOR_OPEN_OFFICE | 3.5 | -129.26 [-129.30, -129.23] | 0.034 | 4.30 | 7.90 | 0.405/0.407; 0.367/0.368 | 26.4% |
| INDOOR_OPEN_OFFICE | 26.0 | -144.74 [-144.77, -144.71] | 0.040 | 4.30 | 7.90 | 0.408/0.407; 0.367/0.368 | n/a |
| INDOOR_OPEN_OFFICE | 28.0 | -145.43 [-145.46, -145.40] | 0.040 | 4.30 | 7.90 | 0.407/0.407; 0.368/0.368 | n/a |
| INDOOR_SHOPPING_MALL | 3.5 | -101.11 [-101.12, -101.10] | 0.015 | 3.30 | 4.60 | 0.366/0.368; 0.368/0.368 | n/a |
| INDOOR_SHOPPING_MALL | 26.0 | -121.13 [-121.14, -121.12] | 0.011 | 3.30 | 4.60 | 0.370/0.368; 0.368/0.368 | n/a |
| INDOOR_SHOPPING_MALL | 60.0 | -129.48 [-129.50, -129.47] | 0.015 | 3.30 | 4.60 | 0.368/0.368; 0.369/0.368 | n/a |
| RURAL_MACROCELL | 0.7 | -118.52 [-118.57, -118.47] | 0.020 | 1.70 | 6.70 | 0.369/0.368; 0.388/0.397 | n/a |
| RURAL_MACROCELL | 0.8 | -119.69 [-119.75, -119.63] | 0.022 | 1.70 | 6.70 | 0.367/0.368; 0.392/0.397 | n/a |
| RURAL_MACROCELL | 3.5 | -132.57 [-132.61, -132.52] | 0.024 | 1.70 | 6.70 | 0.369/0.368; 0.398/0.397 | 78.5% |
| URBAN_MACROCELL | 3.5 | -142.29 [-142.31, -142.26] | 0.015 | 2.40 | 5.30 | 0.369/0.368; 0.330/0.330 | 38.2% |
| URBAN_MACROCELL | 4.9 | -145.78 [-145.80, -145.76] | 0.012 | 2.40 | 5.30 | 0.367/0.368; 0.329/0.330 | n/a |
| URBAN_MACROCELL | 26.0 | -163.17 [-163.21, -163.14] | 0.014 | 2.40 | 5.30 | 0.367/0.368; 0.328/0.330 | n/a |
| URBAN_MACROCELL | 28.0 | -163.95 [-163.98, -163.92] | 0.014 | 2.40 | 5.30 | 0.368/0.368; 0.327/0.330 | n/a |
| URBAN_MICROCELL | 2.38 | -117.57 [-117.59, -117.55] | 0.020 | 4.30 | 6.80 | 0.367/0.368; 0.317/0.315 | 100.0% |
| URBAN_MICROCELL | 3.5 | -122.07 [-122.09, -122.05] | 0.021 | 4.30 | 6.80 | 0.368/0.368; 0.315/0.315 | n/a |
| URBAN_MICROCELL | 4.9 | -126.02 [-126.04, -126.00] | 0.020 | 4.30 | 6.80 | 0.368/0.368; 0.319/0.315 | n/a |
| URBAN_MICROCELL | 26.0 | -145.58 [-145.60, -145.55] | 0.020 | 4.30 | 6.80 | 0.367/0.368; 0.316/0.315 | 98.5% |
| URBAN_MICROCELL | 28.0 | -146.45 [-146.48, -146.43] | 0.020 | 4.30 | 6.80 | 0.369/0.368; 0.316/0.315 | n/a |

## Checks and interpretation

- Maximum absolute radial LOS-probability bias: 0.025.
- Maximum absolute ensemble shadow-standard-deviation error: 0.000 dB.
- Maximum absolute axial-autocorrelation error at the nearest grid-representable lag: 0.008.
- Autocorrelation targets use `exp(-lag/d_cor)` at the reported integer-cell lag; they are not incorrectly compared with `exp(-1)` when the declared distance falls between cells.
- Link-gain intervals quantify realization variability for the generator. They are not confidence intervals for field prediction error.
- The proxy excludes penetration, small-scale fading, external interference, mobility, MIMO, scheduling, HARQ, and traffic.
