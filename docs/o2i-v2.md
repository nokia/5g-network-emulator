# Explicit UE Location and Penetration Model

## Configuration

UE location and penetration are configured independently:

```ini
[UE]
location: outdoor
building_penetration: none
vehicle_penetration: standard
```

### `location`

- `outdoor`
- `indoor`
- `vehicle`
- `random`

### `building_penetration`

- `none`
- `low_loss`
- `high_loss`

### `vehicle_penetration`

- `standard`
- `metallized`

The legacy integer `o2i` key is deprecated. It remains readable during
migration and emits a warning.

## Ownership

- Macroscopic maps own basic scenario path loss, LOS/NLOS state, and shadow
  fading.
- Runtime UE state owns facade penetration, indoor depth, and vehicle
  penetration.
- O2I values are drawn once from the UE's deterministic random stream and
  remain stable for the session.

## Indoor scenarios

For InH/InF scenarios where both the gNB and UE are indoors:

```ini
location: indoor
building_penetration: none
```

No exterior facade loss is applied.

## Building penetration

Low-loss and high-loss profiles use the approved TR 38.901 material-mixture
formulas and add one stable UE-specific variation:

- low loss: standard deviation 4.4 dB;
- high loss: standard deviation 6.5 dB.

Indoor depth is generated only for an indoor UE with an enabled building
penetration profile.

## Vehicle penetration

- `standard`: \(N(9,5^2)\) dB;
- `metallized`: \(N(20,5^2)\) dB.

Loss is clamped at zero and does not include building indoor-depth loss.

## Reproducibility

UE penetration draws derive from the configured global seed and UE/direction
random stream. Runs with the same configuration and seed reproduce the same
penetration values.
