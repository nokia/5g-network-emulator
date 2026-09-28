# Controlled n258 O2I Comparison

The paired profiles use identical map, traffic, mobility, power, gains, scheduler, duration, and keyed fast-fading/interference streams. The stress profile changes only the UE environment from outdoor/no penetration to indoor/high-loss building penetration.

| Direction | Outdoor throughput | High-loss throughput | Delta | Median-UE SINR delta | Outage UEs |
|---|---:|---:|---:|---:|---:|
| DL | 500.02 | 264.26 | -47.1% | -38.13 dB | 0 -> 0 |
| UL | 156.73 | 61.16 | -61.0% | -38.49 dB | 0 -> 0 |

Equivalent scalar FWA gains remain an alignment abstraction, not a beamforming model. The paired result isolates configured environment loss but is still one stochastic seed.
