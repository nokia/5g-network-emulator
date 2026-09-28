# PHY Model V2 Validation Matrix

## Executed gates

| Area | Evidence | Result |
|---|---|---|
| Production map determinism | Two independently generated 21-map catalogs; production regeneration after diagnostics refactor | All map files and `CATALOG.json` byte-identical |
| Candidate promotion | Numeric-array comparison between reviewed candidates and production files | 21/21 arrays identical |
| Manifest integrity | `build_manifest.py --check`; `validate_maps.py` | 21 v2 entries, metadata and hashes consistent |
| Origin/interpolation | `map_handler_v2_test` | Explicit centre cell and axial lookup passed |
| Exact n40 lookup | `profile_config_test` | 2.38 GHz UMi map selected exactly |
| Configuration migration | `ue_location_type_config_test` | Canonical values passed; removed `location` key rejected; pre-feature `o2i` warning path retained |
| Allocation bandwidth | `phy_power_model_test` | Integrated signal/noise SINR invariant for 1/2/4/8/16-PRB grouping |
| UL total power | `phy_power_model_test`; `ul_power_finalization_test` | Per-PRB reconstruction conserved total power for 1–275 PRBs; 23 dBm cap respected |
| PF metric and state | `pf_metric_test`; `pf_state_test`; `pf_scheduler_test` | Common alpha, 1 ms EWMA aging, zero-service update, fair homogeneous service passed |
| PF migration | `pf_config_migration_test` | Legacy per-UE PF beta rejected |
| Reranking/granularity | repeated runtime/fairness envelope | Grouped reranking materially reduced service gaps; detailed trade-off recorded |
| TDD resource accounting | `tdd_resource_metrics_test`; per-TTI grid summaries | Directionally unavailable units excluded before empty/fill metrics |
| Building/vehicle loss | `penetration_model_test`; controlled n258 high-loss profile; RMa vehicle profile | Formula, deterministic profile, and end-to-end behavior passed |
| Canonical end-to-end profiles | Five 180 s runs, seed `20260927`, 20 s warm-up analysis | All completed; summaries and exact inputs committed |
| Candidate/production parity | Identical analyzer applied to both batches | Maximum shared numeric delta was exactly zero |
| Map ensemble | 30 master seeds × 21 catalog entries | 630 realizations; LOS, shadow, autocorrelation, link-gain CIs reported |
| Runtime envelope | 10 repeats × 100 measured TTIs after 20 warm-up TTIs | P50/P95/P99 recorded for 1/16/64/256 UEs and low/high-resolution grids |
| Complete build | `make test`; dashboard test; `make smoke` | Passed |

## Commands

```bash
make -j4 test
make -j4 smoke
python3 tools/maps/build_manifest.py --check
python3 tools/maps/validate_maps.py
python3 tools/maps/validate_v2_statistics.py \
  --realizations 30 \
  --master-seed-base 20270000 \
  --output results/map-v2-multiseed
python3 tools/run_phy_v2_profiles.py \
  --seed 20260927 \
  --duration-s 180 \
  --output results/phy-v2-production-maps
python3 tools/analyze_phy_v2_profiles.py \
  results/phy-v2-production-maps/manifest.json \
  --warmup-s 20 \
  --output results/phy-v2-production-analysis
python3 tools/benchmark_pf_granularity.py \
  --ue-counts 1,16,64,256 \
  --envelope-modes \
  --warmup-steps 20 \
  --steps 100 \
  --repeats 10 \
  --output results/pf-runtime-envelope-bounded
```

## Interpretation boundaries

- Unit and deterministic tests establish internal consistency, not external
  channel-model validity.
- The map ensemble confidence unit is the independent realization, never the
  spatial cell.
- The five profiles are common-seed characterizations, not confidence
  intervals over traffic, mobility, or deployment uncertainty.
- The runtime benchmark uses approximately 4 Gbit/s aggregate offered traffic
  per direction. The earlier 100 Gbit/s-per-UE stress input caused artificial
  unbounded packet-queue growth and was discarded as an invalid runtime
  experiment.
- Real-time claims are limited to the measured host and exact grid/population
  combinations. High-resolution 100/400 MHz per-PRB modes exceed a 1 ms budget
  in several cases.
- Raw logs remain local because of their size; committed inputs, hashes,
  manifests, summaries, and analysis definitions are sufficient to regenerate
  them.
