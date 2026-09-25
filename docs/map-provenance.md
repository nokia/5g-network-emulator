# Macroscopic Map Provenance

## Current production set

FikoRE ships 20 legacy-v1 stochastic map realizations under
`include/maps_scenarios/`. Their current hashes and basic statistics are
recorded in `MANIFEST.json`.

The current files were inherited from the 2025 physical-layer redesign. Their
numeric arrays are intentionally unchanged by the PHY Model V2 infrastructure
work.

Known limitations:

- original RNG seeds are unavailable;
- the exact MATLAB session/version is not recorded;
- generation was performed manually per scenario/frequency;
- JSON files contain no model metadata;
- several historical versions exist for the same filename.

## Historical lineage

The design archive is stored outside this repository under
`/home/pablop/devel/chema`.

Relevant snapshots:

1. `NOKIA/MAP_GENERATION/`: original MATLAB scripts and output maps;
2. `5g-diego/maps_scenarios/`: emulator handoff snapshot;
3. `old/5g-network-emulator/include/maps_scenarios/`: earlier frequency set;
4. `shadow_fading_maps/`: experimental Python port;
5. current `tools/matlab/shadow_fading_map/`: documented MATLAB refactor.

The raw design archive and measurement datasets are not vendored here.

## Known divergent files

| Map | Original MATLAB hash prefix | Handoff hash prefix | Current hash prefix |
|---|---|---|---|
| `RURAL_MACROCELL_3.5` | `468207af4f3a` | `b18f7b896d7b` | `3d89a1fa6d94` |
| `URBAN_MICROCELL_3.5` | `f5c3369d8c7d` | `f5c3369d8c7d` | `705e41914989` |

An older UMi 2.38 GHz realization exists with SHA-256
`3ff2eb544e2efe4a0add85ada5b20316431faa9914e85f456ecd63f8c8891995`.
It must not be restored as a production map until its generator semantics and
validation are approved.

## Legacy generation model

The archived generator:

1. builds LOS and NLOS ABG path-loss maps;
2. generates independent Gaussian shadow fields;
3. filters shadow fields using a DFT-derived spatial filter;
4. draws a LOS state from the scenario probability;
5. smooths the LOS state spatially;
6. combines LOS/NLOS macroscopic fields;
7. exports a 290×290 JSON matrix.

The deterministic batch wrapper in this repository sets an explicit MATLAB
RNG seed and preserves these legacy operations. It is a reproduction tool, not
an approval of the current LOS-mask or origin semantics.

## Pending semantic review

The following items require an owner-approved design note before any production
map is regenerated:

- UMa LOS height correction;
- floating-point smoothed LOS-mask combination;
- even-grid origin convention;
- MATLAB/Python equivalence;
- ABG coefficient/source catalog;
- valid dimensions and distance ranges;
- exact-frequency selection, including n40 2.38 GHz;
- realization count per scenario;
- replacement tolerances;
- O2I and vehicle-loss interaction;
- Rayleigh versus Rician small-scale fading.

## Reproduction policy

Every future map must have:

- deterministic seed and named RNG;
- generator source commit/version;
- complete parameter metadata;
- immutable realization ID;
- input/output hashes;
- statistical validation report;
- explicit approval before replacing a shipped realization.
