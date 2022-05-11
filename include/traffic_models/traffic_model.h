/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef TRAFFIC_MODEL_H
#define TRAFFIC_MODEL_H

#include <traffic_models/traffic_model_base.h>
//--------------------------------------------------------------------------------------------------
// traffic_model(): class which selects the traffic generator model which will be used to generate
// virtual bits for simulated users. 
// Input: 
//      id: unique id of the UE used to initialize the random generator's seed. 
//      type: id of the traffic generator model which was selected for the current UE.
//      _ul_target/_dl_target: target throughput for UL and DL respectively. 
//      _var_perc: size of the variance of the generated noise. 
//      _pkt_size: virtual IP packet size in bits.
//--------------------------------------------------------------------------------------------------
class traffic_model
{
public:
    traffic_model(  int id, 
                    int type, 
                    float _ul_target,
                    float _dl_target,
                    float _var_perc, 
                    int _pkt_size)
    {
        _ul_target = _ul_target*MBIT2BIT;
        _dl_target = _dl_target*MBIT2BIT;
        
        if(type == CONSTANT_TRAFFIC_MODEL)
        {
            traff_model = std::unique_ptr<traffic_model_base>(new traffic_model_base(id, _ul_target, _dl_target,_var_perc, _pkt_size));
        }
        else
        {
            traff_model = std::unique_ptr<traffic_model_base>(new traffic_model_base(id, _ul_target, _dl_target,_var_perc, _pkt_size));
        }
    }
     traffic_model( int id, traffic_config traffic_c)
    {
        traffic_c.ul_target = traffic_c.ul_target*MBIT2BIT;
        traffic_c.dl_target = traffic_c.dl_target*MBIT2BIT;
        if(traffic_c.type == CONSTANT_TRAFFIC_MODEL)
        {
            traff_model = std::unique_ptr<traffic_model_base>(new traffic_model_base(id, traffic_c.ul_target, traffic_c.dl_target, traffic_c.var_perc, traffic_c.pkt_size));
        }
        else
        {
            traff_model = std::unique_ptr<traffic_model_base>(new traffic_model_base(id, traffic_c.ul_target, traffic_c.dl_target, traffic_c.var_perc, traffic_c.pkt_size));
        }
    }

public:
    float generate(int tx, float t)
    {
        return traff_model->generate(tx, t);
    }

    int get_pkt_size(int tx)
    {
        return traff_model->get_pkt_size(tx);
    }

private: 
    std::unique_ptr<traffic_model_base> traff_model; 
};

#endif
