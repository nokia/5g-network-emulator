/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <math.h>
#include <iostream>

//--------------------------------------------------------------------------------------------------
// pos2d(): custom 2D position storaging struct with some specific implemented functionality.
//--------------------------------------------------------------------------------------------------
struct pos2d
{
    pos2d(float _x = 0.0, float _y = 0.0)
    {
        x = _x; 
        y = _y; 
    }
    float x; 
    float y; 

public: 
    float get_distance(pos2d pos = pos2d())
    {
        return sqrt((x-pos.x)*(x-pos.x) + (y-pos.y)*(y-pos.y));
    }

    float get_distance(float _x, float _y)
    {
        return sqrt((x-_x)*(x-_x) + (y-_y)*(y-_y));
    }

    float _x()
    {
        return x; 
    }

    float _y()
    {
        return y; 
    }

    void _x(float x_)
    {
        x = x_; 
    }

    void _y(float y_)
    {
        y = y_; 
    }

    pos2d operator+(const pos2d& _pos)
    {
        return pos2d(x + _pos.x, y + _pos.y); 
    }

    pos2d& operator=(const pos2d& _pos)
    {
        x = _pos.x; 
        y = _pos.y; 
        return *this; 
    }

    pos2d operator*(const float& val)
    {
        return pos2d(x * val, y * val); 
    }

    pos2d& operator/=(const float& val)
    {
        x /= val; 
        y /= val; 
        return *this; 
    }
};

float get_distance(pos2d a, pos2d b);