#include <cassert>

#include <phy_shared/phy_shared.h>
#include <utils/rng_seed.h>

int main()
{
    set_rng_run_seed(20260927);

    ue_config terminal;
    terminal.mobility_c.random_v = true;
    terminal.ue_m.o2i = IN_BUILDING;
    scenario_config scenario(
        URBAN_MICROCELL,
        20.0f,
        10.0f,
        10.0f,
        "unused.json");
    phy_enb_config cell;
    cell.frequency = 3.5e9f;

    phy_shared first(7, terminal, scenario, cell);
    phy_shared repeated(7, terminal, scenario, cell);

    assert(
        first.get_building_penetration_sample()
        == repeated.get_building_penetration_sample());
    assert(
        first.get_vehicle_penetration_sample()
        == repeated.get_vehicle_penetration_sample());
    assert(first.get_indoor_depth(100.0f) == repeated.get_indoor_depth(100.0f));
    assert(first.get_indoor_depth(100.0f) == first.get_indoor_depth(100.0f));
    assert(first.get_indoor_depth(100.0f) >= 0.0f);
    assert(first.get_indoor_depth(100.0f) <= 25.0f);
    assert(first.get_indoor_depth(0.0f) == 0.0f);
    return 0;
}
