# Macroscopic Fading Map Generation

This folder contains the Matlab scripts used to generate the macroscopic fading maps consumed by the PHY layer.

The emulator stores the generated maps in `include/maps_scenarios/` as JSON files. Each file contains:

* `cell_number`: number of cells per side of the square map.
* `cell_size`: spatial resolution in meters.
* `map`: macroscopic fading values in dB.

The PHY layer reads these values through `MapHandler` and interpolates them using the UE position.

## Files

* `main_generate_maps.mlx`: main Live Script. It configures the scenario, carrier frequency and map size, generates LOS/NLOS pathloss and shadow fading maps, combines them into the final macroscopic fading map and writes the JSON output.
* `functions/adjust_parameters.m`: returns the LOS/NLOS shadow fading standard deviation and decorrelation distance for each scenario.
* `functions/generate_pathloss_map.m`: generates the ABG pathloss map for the selected scenario and frequency.
* `functions/generate_filtered_matrix.m`: generates a spatially correlated shadow fading map using a 2D filtering method.
* `functions/get_probability_LOS.m`: implements the scenario-dependent LOS probability models.
* `functions/map_LOS.m`: generates the stochastic LOS/NLOS map used to combine the LOS and NLOS maps.
* `functions/R.m`: Gudmundson autocorrelation function.

## Generation Flow

The main script performs the following steps:

1. Select `cellNumber`, `freqGHz` and `scenario`.
2. Load the shadow fading decorrelation distances and standard deviations for LOS and NLOS.
3. Generate LOS and NLOS ABG pathloss maps.
4. Generate LOS and NLOS spatially correlated shadow fading maps.
5. Generate a stochastic LOS/NLOS map from the 3GPP LOS probability model.
6. Build the macroscopic fading maps:

```text
macroscopic_fading_LOS  = shadow_fading_LOS  - pathloss_LOS
macroscopic_fading_NLOS = shadow_fading_NLOS - pathloss_NLOS
```

7. Combine the LOS and NLOS values according to the LOS/NLOS map.
8. Write `maps/macroscopic_fading_map_<SCENARIO>_<FREQ_GHZ>.json`.

## ABG Pathloss Model

The pathloss map is generated with the ABG model:

```text
PL(dB) = 10 * alpha * log10(d) + beta + 10 * gamma * log10(f)
```

where `d` is the distance in meters and `f` is the carrier frequency in GHz.

| Scenario | Condition | Frequency | alpha | beta | gamma |
| --- | --- | --- | ---: | ---: | ---: |
| `RURAL_MACROCELL` | LOS | all | 2.16 | 32.4 | 2 |
| `RURAL_MACROCELL` | NLOS | all | 2.75 | 32.4 | 2 |
| `URBAN_MACROCELL` | LOS | < 6 GHz | 2.2 | 28 | 2 |
| `URBAN_MACROCELL` | LOS | >= 6 GHz | 1.9 | 35.8 | 1.9 |
| `URBAN_MACROCELL` | NLOS | all | 3.5 | 13.6 | 2.4 |
| `URBAN_MICROCELL` | LOS | < 6 GHz | 2.27 | 27.02 | 2 |
| `URBAN_MICROCELL` | LOS | >= 6 GHz | 1.1 | 46.8 | 2.1 |
| `URBAN_MICROCELL` | NLOS | all | 2.8 | 31.4 | 2.7 |
| `INDOOR_OPEN_OFFICE` / `INDOOR_MIXED_OFFICE` | LOS | < 6 GHz | 1.87 | 32.82 | 2 |
| `INDOOR_OPEN_OFFICE` / `INDOOR_MIXED_OFFICE` | LOS | >= 6 GHz | 1.6 | 32.9 | 1.8 |
| `INDOOR_OPEN_OFFICE` / `INDOOR_MIXED_OFFICE` | NLOS | < 6 GHz | 4.33 | 11.5 | 2 |
| `INDOOR_OPEN_OFFICE` / `INDOOR_MIXED_OFFICE` | NLOS | >= 6 GHz | 3.9 | 19 | 2.1 |
| `INDOOR_SHOPPING_MALL` | LOS | all | 1.9 | 31.2 | 2.2 |
| `INDOOR_SHOPPING_MALL` | NLOS | all | 2.0 | 34.4 | 2.3 |

## Generating a Map

Open `main_generate_maps.mlx` in Matlab, set the required values and run the script:

```matlab
cellNumber = 290;
freqGHz = 3.5;
scenario = 'URBAN_MICROCELL';
```

The valid scenario names are:

* `RURAL_MACROCELL`
* `URBAN_MICROCELL`
* `URBAN_MACROCELL`
* `INDOOR_OPEN_OFFICE`
* `INDOOR_MIXED_OFFICE`
* `INDOOR_SHOPPING_MALL`

The output file is written under the local `maps/` folder. To use it in the emulator, copy the generated JSON file to `include/maps_scenarios/` using the naming pattern expected by the configuration loader:

```text
macroscopic_fading_map_<SCENARIO>_<FREQ_GHZ>.json
```

For example:

```text
macroscopic_fading_map_URBAN_MICROCELL_3.5.json
```

The emulator selects the closest available map for the configured `scenario_type` and `frequency`.
