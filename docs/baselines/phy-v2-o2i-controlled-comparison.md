# Controlled n258 O2I Comparison

The paired profiles use identical traffic, map, seed, power, gains, scheduler, and duration. The stress profile changes only the UE environment from outdoor/no penetration to indoor/high-loss building penetration.

| Direction | Outdoor throughput | High-loss throughput | Delta | Median-UE SINR delta | Outage UEs |
|---|---:|---:|---:|---:|---:|
| DL | 500.02 | 495.50 | -0.9% | -42.06 dB | 0 -> 0 |
| UL | 156.73 | 146.67 | -6.4% | -37.78 dB | 0 -> 1 |

Equivalent scalar FWA gains keep DL near demand even under the high-loss profile; the controlled result therefore demonstrates configured penetration behavior, not a general FR2 indoor coverage claim.
