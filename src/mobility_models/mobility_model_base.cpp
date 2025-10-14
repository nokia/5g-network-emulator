/**********************************************
 * Copyright 2022 Nokia
 * Licensed under the BSD 3-Clause Clear License
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 **********************************************/

#include <mobility_models/mobility_model_base.h>

mobility_model_base::mobility_model_base(int id, pos2d _init_pos, bool _random_init, float _speed, float _speed_var,
                                         float _max_distance, float _time_target, float maxApothem,
                                         float _time_target_var, bool _random_v)
    : uniform_gen(time(NULL) * _random_v + id)
{
    max_distance = _max_distance;
    max_apothem = maxApothem;

    if (!_random_init)
    {
        float init_x = std::min(fabs(_init_pos.x), max_apothem) * (_init_pos.x < 0 ? -1 : 1);
        float init_y = std::min(fabs(_init_pos.y), max_apothem) * (_init_pos.y < 0 ? -1 : 1);
        current_pos = pos2d(init_x, init_y);
    }
    else
    {
        set_random_initial_position(max_apothem); 
    }

    speed = _speed;
    speed_var = _speed_var;
    current_distance = current_pos.get_distance();
    current_dir = pos2d();
    time_target = _time_target;
    time_target_var = _time_target_var;
    t_target = 0;
    target_pos = pos2d();
    past_t = 0;
}

mobility_model_base::mobility_model_base(int id, mobility_config mobility_c, float maxApothem)
    : uniform_gen(time(NULL) * mobility_c.random_v + id)
{
    if (!mobility_c.random_init)
    {
        float init_x = std::min(fabs(mobility_c.init_pos.x), max_apothem) * (mobility_c.init_pos.x < 0 ? -1 : 1);
        float init_y = std::min(fabs(mobility_c.init_pos.y), max_apothem) * (mobility_c.init_pos.y < 0 ? -1 : 1);
        current_pos = pos2d(init_x, init_y);
    }
    else
    {
        float radius = fabs(uniform_dist(uniform_gen) * mobility_c.max_distance);
        float theta = fabs(uniform_dist(uniform_gen) * 2 * M_PI);
        current_pos = pos2d(radius * cos(theta), radius * sin(theta));
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
    max_apothem = maxApothem;
}

void mobility_model_base::set_random_initial_position(float max_apothem)
{
    bool valid_position = false;
    float new_x, new_y;

    for (int attempts = 0; attempts < 100; ++attempts)
    {
        float radius = fabs(uniform_dist(uniform_gen) * max_distance);
        float theta = fabs(uniform_dist(uniform_gen) * 2 * M_PI);

        new_x = radius * cos(theta);
        new_y = radius * sin(theta);

        if (fabs(new_x) <= max_apothem && fabs(new_y) <= max_apothem)
        {
            double distance_from_origin = sqrt(new_x * new_x + new_y * new_y);
            if (distance_from_origin > 10)
            {
                valid_position = true;
                break;
            }
        }
    }

    if (!valid_position)
    {
        std::cerr << "Error: Not a valid position." << std::endl;
        // Fallback to a default position
        new_x = 0.0f;
        new_y = 0.0f;
    }

    current_pos = pos2d(new_x, new_y);
}

void mobility_model_base::update_pos(float current_t) {}

float mobility_model_base::get_current_d()
{
    return current_pos.get_distance();
}

void mobility_model_base::get_pos(float &x, float &y)
{
    x = current_pos._x();
    y = current_pos._y();
}

pos2d *mobility_model_base::get_pos()
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
