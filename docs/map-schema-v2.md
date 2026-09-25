# FikoRE Macroscopic Map Schema

## Legacy v1

Current production map files contain:

```json
{
  "cell_number": 290,
  "cell_size": 5.0,
  "map": [[-80.0]]
}
```

- `cell_number`: number of cells per side.
- `cell_size`: cell spacing in metres.
- `map`: square matrix of macroscopic link gain in dB, combining the selected
  path-loss and shadow-fading realization.

The v1 format does not identify scenario, carrier frequency, model
coefficients, generator version, RNG, seed, or coordinate-origin convention.
Those properties are inferred from filenames and historical context.

`include/maps_scenarios/MANIFEST.json` records immutable characterization and
checksums for the shipped v1 files without changing their bytes.

## Proposed v2 metadata

The v2 schema is a design contract for deterministic generation. It is not yet
used by production maps.

```json
{
  "schema_version": 2,
  "metadata": {
    "scenario": "URBAN_MICROCELL",
    "frequency_ghz": 3.5,
    "realization_id": "umi-3p5-seed-42",
    "rng": "approved-generator-and-version",
    "seed": 42,
    "generator": {
      "name": "fikore-map-generator",
      "version": "2",
      "source_commit": "git-sha"
    },
    "grid": {
      "cell_number": 290,
      "cell_size_m": 5.0,
      "origin": "cell-centred"
    },
    "pathloss": {
      "family": "ABG",
      "los": {"alpha": 2.27, "beta": 27.02, "gamma": 2.0},
      "nlos": {"alpha": 2.8, "beta": 31.4, "gamma": 2.7},
      "source": "approved-reference"
    },
    "shadow": {
      "los_sigma_db": 4.3,
      "nlos_sigma_db": 6.8,
      "los_decorrelation_m": 10.0,
      "nlos_decorrelation_m": 13.0,
      "filter": "approved-filter-profile"
    },
    "los_state": {
      "model": "pending-internal-review",
      "smoothing": "pending-internal-review"
    },
    "input_hashes": {}
  },
  "cell_number": 290,
  "cell_size": 5.0,
  "map": [[-80.0]]
}
```

## Compatibility policy

- The runtime must continue to read v1 files during migration.
- Optional v2 metadata must not change the interpretation of the numeric map
  array without an approved semantic version.
- Exact-frequency lookup is preferred for v2.
- If nearest-frequency fallback is used, the runtime must report both requested
  and selected frequencies.
- A map realization is immutable: changed bytes require a new realization ID
  and checksum.

## Validation requirements

For every generated realization:

1. Matrix dimensions and finite values match metadata.
2. Same generator, parameters, and seed produce identical bytes.
3. Different seeds preserve approved distribution tolerances.
4. LOS/NLOS path-loss slices match approved ABG equations.
5. Shadow mean, standard deviation, and radial autocorrelation match targets.
6. LOS probability versus distance matches the approved LOS-state algorithm.
7. Runtime interpolation and generator origin conventions agree.
8. Input and output hashes are recorded.

Map semantics—LOS smoothing, UMa LOS correction, origin, coefficient tables,
and O2I interaction—remain pending internal review.
