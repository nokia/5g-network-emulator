/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <mobility_models/mobility_model_base.h>

mobility_model_base::mobility_model_base(int id, pos2d _init_pos, bool _random_init, float _speed, float _speed_var, 
                               float _max_distance, float _time_target, 
                               float _time_target_var, bool _random_v)
                    :uniform_gen(time(NULL)*_random_v + id)

{
    if(!_random_init) current_pos = pos2d(_init_pos.x, _init_pos.y); 
    else
    {
        float radius = fabs(uniform_dist(uniform_gen)*_max_distance);
        float theta = fabs(uniform_dist(uniform_gen)*2*M_PI);
        current_pos = pos2d(radius*cos(theta), radius*sin(theta));
    } 
    speed = _speed; 
    speed_var = _speed_var;
    current_distance = current_pos.get_distance(); 
    current_dir = pos2d(); 
    time_target = _time_target;
    time_target_var = _time_target_var; 
    t_target = 0; 
    target_pos = pos2d(); 
    max_distance = _max_distance;
    past_t = 0;
}

mobility_model_base::mobility_model_base(int id, mobility_config mobility_c)
                    :uniform_gen(time(NULL)*mobility_c.random_v + id)
{
    if(!mobility_c.random_init) current_pos = mobility_c.init_pos; 
    else
    {
        float radius = abs(uniform_dist(uniform_gen)*mobility_c.max_distance);
        float theta = abs(uniform_dist(uniform_gen)*2*M_PI);
        current_pos = pos2d(radius*cos(theta), radius*sin(theta));
    } 
    speed = mobility_c.speed; 
    speed_var = mobility_c.speed_var;
    current_distance = current_pos.get_distance(); 
    current_dir = pos2d(); 
    time_target = mobility_c.time_target;
    time_target_var = mobility_c.time_target_var; 
    t_target = 0; 
    target_pos = pos2d(); 
    max_distance = mobility_c.max_distance;
    past_t = 0;
}

void mobility_model_base::update_pos(float current_t){}
float mobility_model_base::get_current_d()
{
    return current_pos.get_distance(); 
}

void mobility_model_base::get_pos(float &x, float &y)
{
    x = current_pos._x(); 
    y = current_pos._y(); 
}

pos2d* mobility_model_base::get_pos()
{
    return &current_pos; 
}

float mobility_model_base::x()
{
    return current_pos._x(); 
}

float mobility_model_base::y()
{
    return current_pos._y(); 
}

