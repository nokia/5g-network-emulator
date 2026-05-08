#ifndef PHY_SHARED_H
#define PHY_SHARED_H

#include <algorithm>
#include <random>
#include <string>
#include <mobility_models/pos2d.h>
#include <ue/ue_config.h>

class phy_shared
{
public:
    phy_shared(int _ue_id, ue_config ue_c, scenario_config _scenario_c, phy_enb_config _phy_enb_config);

private:
    std::mt19937 gen;
    std::uniform_real_distribution<float> uniform_stochastics{0.0, 1.0};

public:
    int compute_outdoor_to_indoor();
    int verify_outdoor_to_indoor();
    float get_correlation_distance();
    int get_o2i();

    
public:

protected:
    int scenario;
    float freq_ghz;
    int n_antennas;
    float correlation_distance;
    int o2i;
    pos2d prev_pos;
    pos2d pos;
    float c_dist;
    bool update_cs = true;
    bool stochastics = true;
    float eNB_h;
    float corr_distance;
};

#endif