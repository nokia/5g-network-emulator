/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <mac_layer/metric_handler.h>

// MAX METRIC HANDLER METHODS
//-------------------------
void max_metric_handler::reset()
{
    assigned = false; 
    tp = 0.0; 
    value = -1; 
    index = -1; 
    id = -1; 
}

void max_metric_handler::assign(float _tp, float _value, int _index, int _id)
{
    assigned = true; 
    tp = _tp; 
    value = _value; 
    index = _index; 
    id = _id; 
}

bool max_metric_handler::evaluate(float metric)
{
    return metric >= value;
}

bool max_metric_handler::is_assigned() const
{
    return assigned; 
}

bool max_metric_handler::has_data() const
{
    return tp > 0; 
}

float max_metric_handler::get_tp() const
{
    return tp; 
}

int max_metric_handler::get_index() const
{
    return index; 
}

int max_metric_handler::get_id() const
{
    return id; 
}

float max_metric_handler::get_value() const
{
    return value; 
}

// METRIC HANDLER METHODS
//-------------------------

metric_handler::metric_handler(
    int _metric_type,
    float _bet_beta,
    float _delay_t,
    float _delta,
    float _pf_alpha)
{
    metric_t = check_metric(_metric_type);
    assign_metric();
    bet_beta = _bet_beta;
    pf_alpha = _pf_alpha;
    delay_t = _delay_t; 
    delta = _delta; 
}

float metric_handler::get_metric(metric_info metric_i, float current_t, int f)
{
    return (this->*metric_f_ptr)(metric_i, current_t, f);
}

void metric_handler::assign_metric()
{
    if(metric_t == METRIC_FIFO) metric_f_ptr = &metric_handler::fifo; 
    if(metric_t == METRIC_BET) metric_f_ptr = &metric_handler::bet; 
    if(metric_t == METRIC_DIST_DELAY) metric_f_ptr = &metric_handler::dist_delay; 
    if(metric_t == METRIC_W_DELAY) metric_f_ptr = &metric_handler::w_delay; 
    if(metric_t == METRIC_MAX_TP) metric_f_ptr = &metric_handler::max_tp; 
    if(metric_t == METRIC_RR) metric_f_ptr = &metric_handler::rr; 
    if(metric_t == METRIC_PF) metric_f_ptr = &metric_handler::pf; 
}

float metric_handler::fifo(metric_info metric_i, float current_t, int f)
{
    return current_t - metric_i.req_time;
}

float metric_handler::bet(metric_info metric_i, float current_t, int f)
{
    return 1/(bet_beta*metric_i.avrg_tp + (1 - bet_beta)*metric_i.current_tp);
}

float metric_handler::dist_delay(metric_info metric_i, float current_t, int f)
{
    return 1/(metric_i.delay_t - metric_i.current_delay);
}

float metric_handler::w_delay(metric_info metric_i, float current_t, int f)
{
    return -(log(metric_i.delta)/metric_i.delay_t) * metric_i.current_delay;
}

float metric_handler::max_tp(metric_info metric_i, float current_t, int f)
{
    return metric_i.current_tp;
}

float metric_handler::rr(metric_info metric_i, float current_t, int f)
{
    return 0.0;
}

float metric_handler::pf(metric_info metric_i, float current_t, int f)
{
    if(metric_i.avrg_tp!=0)
        return pow(metric_i.current_tp, pf_alpha) / metric_i.avrg_tp;
    else return metric_i.current_tp;
}

bool metric_handler::is_rr()
{
    return metric_t == METRIC_RR; 
}

bool metric_handler::is_pf() const
{
    return metric_t == METRIC_PF;
}

float metric_handler::get_rr_metric(int f, int rank, int n_enabled)
{
    if(prev_f != f)
    {
        rr_index++;
        if(rr_index >= n_enabled) rr_index = 0; 
        prev_f = f; 
    }
    if(rr_index == rank) return 1.0;
    else return 0.0;
}

int metric_handler::check_metric(int metric)
{
    if (!(metric == METRIC_FIFO ||
            metric == METRIC_BET ||
            metric == METRIC_DIST_DELAY ||
            metric == METRIC_W_DELAY ||
            metric == METRIC_MAX_TP ||
            metric == METRIC_RR ||
            metric == METRIC_PF))
            {
                return SCH_METRIC_DEFAULT; 
            }
    else return metric;
}
