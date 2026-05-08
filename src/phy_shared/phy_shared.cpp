#include <phy_shared/phy_shared.h>
#include <phy_layer/phy_l_definitions.h>
#include <utils/conversions.h>
#include <utils/terminal_logging.h>

phy_shared::phy_shared(int _ue_id, ue_config ue_c, scenario_config _scenario_c, phy_enb_config _phy_enb_config)
    : gen(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
{
    scenario = _scenario_c.type;
    eNB_h = _scenario_c.eNB_h;
    freq_ghz = _phy_enb_config.frequency / GHZ2HZ;
    correlation_distance = get_correlation_distance();
    o2i = (ue_c.ue_m.o2i == 13) ? compute_outdoor_to_indoor() : ue_c.ue_m.o2i; 
    o2i = verify_outdoor_to_indoor();
}

int phy_shared::get_o2i(){
    return o2i;
}

int phy_shared::compute_outdoor_to_indoor()
{
    float random = uniform_stochastics(gen);

    switch (scenario)
    {
    case RURAL_MACROCELL:
        return (random <= 0.5) ? IN_BUILDING : IN_CAR;

    case URBAN_MICROCELL:
    case URBAN_MACROCELL:
        return (random <= 0.8) ? IN_BUILDING : OUTDOOR;

    default:
        return OUTDOOR;
    }
}

int phy_shared::verify_outdoor_to_indoor()
{
    switch (scenario)
    {
    case 0: // URBAN_MICROCELL (UMI)
    case 1: // URBAN_MACROCELL (UMA)
        // Valid values: OUTDOOR (10), IN_BUILDING (11)
        if (o2i == IN_BUILDING || o2i == OUTDOOR)
            return o2i;
        break;

    case 2: // RURAL_MACROCELL (RMA)
        // Valid values: OUTDOOR (10), IN_BUILDING (11), IN_CAR (12)
        if (o2i == OUTDOOR || o2i == IN_BUILDING || o2i == IN_CAR)
            return o2i;
        break;

    case 3: // INDOOR_HOTSPOT
    case 4: // INDOOR_FACTORY
        // Only valid value: OUTDOOR (10)
        if (o2i == OUTDOOR)
            return o2i;
        break;

    default:
        break;
    }

    return compute_outdoor_to_indoor();
}

float phy_shared::get_correlation_distance()
{
    switch (scenario)
    {
    case RURAL_MACROCELL:
        return (o2i != OUTDOOR) ? 120 : 37;

    case URBAN_MACROCELL:
        return (o2i != OUTDOOR) ? 7 : 37;

    case URBAN_MICROCELL:
        return (o2i != OUTDOOR) ? 7 : (freq_ghz < 6 ? corr_distance : 10);

    case INDOOR_OPEN_OFFICE:
    case INDOOR_MIXED_OFFICE:
        return 6;

    case INDOOR_SHOPPING_MALL:
        return 10;

    default:
        return 0; 
    }
}

// getMacroFading removed from phy_shared: callers should use MapHandler directly from ue
