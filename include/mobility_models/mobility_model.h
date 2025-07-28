/**********************************************
 * Copyright 2022 Nokia
 * Licensed under the BSD 3-Clause Clear License
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 **********************************************/
#pragma once
#ifndef MOBILITY_MODEL_H
#define MOBILITY_MODEL_H

#include <mobility_models/manhattan_model.h>
#include <mobility_models/random_walk_model.h>
#include <mobility_models/random_waypoint_model.h>
#include <mobility_models/mobility_model_base.h>
#include <mobility_models/linear_drift_model.h>
#include <mobility_models/clockwise_model.h>
#include <phy_layer/phy_l_definitions.h>

#define MIN_DISTANCE 10

//--------------------------------------------------------------------------------------------------
// mobility_model(): class which selects the mobility model which will be used to estimate a new
// position for the associated UE in each time step.
// Input:
//      id: unique id of the UE used to initialize the random generator's seed.
//      type: id of the desired mobility model for the current UE.
//      _init_pos: initial position, only used if _random_init is set to false.
//      _random_init: wether to randomly initialized or not
//      _speed: target speed of the UEs in km/h.
//      _speed_var: size of the white noise to be applied in each timestep to the target speed.
//      _max_distance: max. distance of the UE to the gNB.
//      _time_target: target time used in some of the implemented models, such as random walk model.
//      _time_target_var: size of the white noise to be applied in each timestep to the target time.
//--------------------------------------------------------------------------------------------------
class mobility_model
{
public:
    mobility_model(int id, int type, pos2d _init_pos, bool _random_init, float _speed,
                   float _speed_var, float _max_distance,
                   float _time_target, float maxApothem, float _time_target_var, int scenario, bool _random_v)
    {
        limit_distance(_max_distance, scenario);
        limit_speed(_speed, scenario);
        if (type == RANDOM_WALK_MODEL)
        {
            mob_model = std::unique_ptr<mobility_model_base>(new random_walk_model(id, _init_pos, _random_init, _speed, _speed_var, _max_distance, _time_target, maxApothem,_time_target_var, _random_v));
        }
        else if (type == RANDOM_WAYPOINT_MODEL)
        {
            mob_model = std::unique_ptr<mobility_model_base>(new random_waypoint_model(id, _init_pos, _random_init, _speed, _speed_var, _max_distance, _time_target, maxApothem, _time_target_var, _random_v));
        }
        else if (type == RANDOM_MANHATTAN_MODEL)
        {
            mob_model = std::unique_ptr<mobility_model_base>(new manhattan_model(id, _init_pos, _random_init, _speed, _speed_var, _max_distance, _time_target, maxApothem, _time_target_var, _random_v));
        }
        else if (type == LINEAR_DRIFT_MODEL)
        {
            mob_model = std::unique_ptr<mobility_model_base>(new linear_drift_model(id, _init_pos, _random_init, _speed, _speed_var, _max_distance, _time_target, maxApothem,   _time_target_var, _random_v));
        }
        else if (type == CLOCKWISE_MODEL)
        {
            mob_model = std::unique_ptr<mobility_model_base>(new clockwise_model(id, _init_pos, _random_init, _speed, _speed_var, _max_distance, _time_target, maxApothem,   _time_target_var, _random_v));
        }
        else
        {
            mob_model = std::unique_ptr<mobility_model_base>(new mobility_model_base(id, _init_pos, _random_init, _speed, _speed_var, _max_distance, _time_target, maxApothem,   _time_target_var, _random_v));
        }
    }
    mobility_model(int id, mobility_config mobility_c, int scenario, float maxApothem)
    {
        limit_distance(mobility_c.max_distance, scenario);
        limit_speed(mobility_c.speed, scenario);

        if (mobility_c.type == RANDOM_WALK_MODEL)
        {
            mob_model = std::unique_ptr<mobility_model_base>(new random_walk_model(id, mobility_c.init_pos, mobility_c.random_init, mobility_c.speed, mobility_c.speed_var, mobility_c.max_distance, mobility_c.time_target, maxApothem,   mobility_c.time_target_var, mobility_c.random_v));
        }
        else if (mobility_c.type == RANDOM_WAYPOINT_MODEL)
        {
            mob_model = std::unique_ptr<mobility_model_base>(new random_waypoint_model(id, mobility_c.init_pos, mobility_c.random_init, mobility_c.speed, mobility_c.speed_var, mobility_c.max_distance, mobility_c.time_target, maxApothem,   mobility_c.time_target_var, mobility_c.random_v));
        }
        else if (mobility_c.type == RANDOM_MANHATTAN_MODEL)
        {
            mob_model = std::unique_ptr<mobility_model_base>(new manhattan_model(id, mobility_c.init_pos, mobility_c.random_init, mobility_c.speed, mobility_c.speed_var, mobility_c.max_distance, mobility_c.time_target, maxApothem,   mobility_c.time_target_var, mobility_c.random_v));
        }
        else if (mobility_c.type == LINEAR_DRIFT_MODEL)
        {
            mob_model = std::unique_ptr<mobility_model_base>(new linear_drift_model(id, mobility_c.init_pos, mobility_c.random_init, mobility_c.speed, mobility_c.speed_var, mobility_c.max_distance, mobility_c.time_target, maxApothem,   mobility_c.time_target_var, mobility_c.random_v));
        }
        else if (mobility_c.type == CLOCKWISE_MODEL)
        {
            mob_model = std::unique_ptr<mobility_model_base>(new clockwise_model(id, mobility_c.init_pos, mobility_c.random_init, mobility_c.speed, mobility_c.speed_var, mobility_c.max_distance, mobility_c.time_target, maxApothem,   mobility_c.time_target_var, mobility_c.random_v));
        }
        else
        {
            mob_model = std::unique_ptr<mobility_model_base>(new mobility_model_base(id, mobility_c.init_pos, mobility_c.random_init, mobility_c.speed, mobility_c.speed_var, mobility_c.max_distance, mobility_c.time_target, maxApothem,   mobility_c.time_target_var, mobility_c.random_v));
        }
    }

public:
    void update_pos(float t)
    {
        mob_model->update_pos(t);
    }
    void set_initial_position(float _max_apothem)
    {
        mob_model->set_random_initial_position(_max_apothem);
    }
    float get_distance()
    {
        return mob_model->get_current_d();
    }

    void get_pos(float &x, float &y)
    {
        mob_model->get_pos(x, y);
    }

    pos2d *get_pos()
    {
        return mob_model->get_pos();
    }

    float get_max_speed()
    {
        return mob_model->get_max_speed();
    }

    float x()
    {
        return mob_model->x();
    }

    float y()
    {
        return mob_model->y();
    }

private:
    void limit_distance(float &distance, int scenario)
    {
        switch (scenario)
        {
        case RURAL_MACROCELL:
            distance = std::min(distance, 5000.0f);
            break;
        case URBAN_MACROCELL:
            distance = std::min(distance, 5000.0f);
            break;
        case URBAN_MICROCELL:
            distance = std::min(distance, 5000.0f);
            break;
        case INDOOR_OPEN_OFFICE:
            distance = std::min(distance, 150.0f);
            break;
        case INDOOR_MIXED_OFFICE:
            distance = std::min(distance, 150.0f);
            break;
        case INDOOR_SHOPPING_MALL:
            distance = std::min(distance, 600.0f);
            break;
        }
    }
void limit_speed(float &speed, int scenario)
{
    //std::cout << "Speed before limitation: " << speed << std::endl;
    
    float conversion_factor = 1000.0f / 3600.0f;  
    switch (scenario)
    {
    case RURAL_MACROCELL:
        speed = std::min(speed, 120.0f * conversion_factor); 
        break;
    default:
        speed = std::min(speed, 3.0f * conversion_factor);  
        break;
    }
    
   // std::cout << "Speed after limitation: " << speed << std::endl;
}

private:
    std::unique_ptr<mobility_model_base> mob_model;
};

#endif
