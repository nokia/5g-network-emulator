/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef MOBILITY_MODEL_BASE_H
#define MOBILITY_MODEL_BASE_H

#include <math.h>
#include <random>
#include <memory>
#include <chrono>
#include <iostream>
#include <functional>
#include <mobility_models/pos2d.h>
#include <mobility_models/mobility_config.h>
//--------------------------------------------------------------------------------------------------
// mobility_model_base(): is a base class that the custom implemented mobility models classes
// override. This base class simply does not update the position: is a static user model. The 
// initial position can be randomized or manually selected by the user using the configuration file.
// Input: 
//      id: unique id of the UE used to initialize the random generator's seed. 
//      _init_pos: initial position, only used if _random_init is set to false.
//      _random_init: wether to randomly initialized or not
//      _speed: target speed of the UEs.
//      _speed_var: size of the white noise to be applied in each timestep to the target speed.
//      _max_distance: max. distance of the UE to the gNB. 
//      _time_target: target time used in some of the implemented models, such as random walk model.
//      _time_target_var: size of the white noise to be applied in each timestep to the target time.
//--------------------------------------------------------------------------------------------------

class mobility_model_base
{
public: 
    mobility_model_base(int id, pos2d _init_pos, bool _random_init, float _speed, float _speed_var,
                    float _max_distance, float _time_target, 
                    float _time_target_var, bool _random_v);
    mobility_model_base(int it, mobility_config mobility_c);
    mobility_model_base(){}
    
protected: 
    std::mt19937 uniform_gen;
    std::uniform_real_distribution<float> uniform_dist{-1,1};

public: 
    virtual void update_pos(float current_t);
    float get_current_d();
    void get_pos(float &x, float &y);
    pos2d * get_pos();
    float get_max_speed(){ return speed + speed_var; }
    float x();
    float y();

protected: 
    float new_speed()
    {
        return uniform_dist(uniform_gen)*speed_var;
    }
protected: 
    pos2d current_pos; 
    pos2d target_pos; 
    float speed;
    float speed_var; 
    pos2d current_dir; 
    float current_distance; 
    float max_distance; 
    float time_target; 
    float time_target_var; 
    float t_target; 
    float past_t; 
};

#endif