/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef METRIC_HANDLER_H
#define METRIC_HANDLER_H

#include <math.h>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <math.h>

#include <mac_layer/mac_definitions.h>

class max_metric_handler
{
private: 
    bool assigned = false; 
    float tp = 0.0; 
    float value = -1; 
    int index = -1; 
    int id = -1; 

public: 
    void reset();
    void assign(float _tp, float _value, int _index, int _id);
    bool evaluate(float metric);
    bool is_assigned();
    bool has_data();
    float get_tp();
    float get_value();
    int get_index();
    int get_id();
};

// Stores the metric info
struct metric_info{
    float req_time = 0; 
    float avrg_tp = 0; 
    float current_tp = 0; 
    float current_delay = 0; 
    float delta = 0.001; 
    float delay_t = 0.1; 
    int index = 0; 
};

class metric_handler
{
public: 
    metric_handler(int _metric_type, float _priority = 1, float _beta = 0.9, float _delay_t = 0.001, float _delta = 0.8 );
private: 
    int rr_index = 0; 
    int prev_f = -1; 
    int metric_t; 
    float beta; 
    float delay_t; 
    float delta;
    float priority;
    typedef float (metric_handler::*f_ptr)(metric_info metric_i , float current_t, int f);
    f_ptr metric_f_ptr;

public: 
    void add_ue(std::string id);  
    void remove_ue(std::string id);
    float get_metric(metric_info metric_i, float current_t, int f);
    float get_rr_metric(int f,int id, int n_ues);
    bool is_rr();
private: 
    void assign_metric();
    float fifo(metric_info metric_i, float current_t, int f);
    float bet(metric_info metric_i, float current_t, int f);
    float dist_delay(metric_info metric_i, float current_t, int f);
    float w_delay(metric_info metric_i, float current_t, int f);
    float max_tp(metric_info metric_i, float current_t, int f);
    float rr(metric_info metric_i, float current_t, int f);
    float pf(metric_info metric_i, float current_t, int f);
    int check_metric(int metric);
};

#endif