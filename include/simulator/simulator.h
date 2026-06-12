/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <thread>
#include <iostream>
#include <functional>

#include <simulator/configuration_loader.h>
#include <ue/ue_handler.h>
#include <mac_layer/mac_layer.h>
#include <timer/timer.h>
#include <utils/terminal_logging.h>

class simulator
{
public:
    simulator(std::string config_file);

public: 

    void override_duration(float _duration);

    void start();

    void join();
    
    void terminate();

    void print_traffic();
private: 

    void handle_time_log(double sim_time_s);
    void log_runtime_start();
    void log_runtime_stop(const std::string &reason);
    void maybe_log_progress(double sim_time_s);

    void step(unsigned int _ts);

private: 
    configuration_loader config_loader; 
    ue_handler ue_h; 
    mac_layer mac_l;
private: 
    int freq = 1000; 
    int step_c = 0; 
    std::uint64_t total_steps = 0;
    std::chrono::duration<double> time_mac = std::chrono::duration<double>().zero(); 
    std::chrono::duration<double> time_ue = std::chrono::duration<double>().zero(); 
    std::uint64_t progress_steps = 0;
    std::chrono::duration<double> progress_time_mac = std::chrono::duration<double>().zero();
    std::chrono::duration<double> progress_time_ue = std::chrono::duration<double>().zero();
    float total_time_mac; 
    float total_time_ue; 
private: 
    timer ticker; 
private: 
    float duration;
private: 
    int verbosity = 0; 
    bool runtime_started = false;
    bool runtime_stopped = false;
    std::chrono::steady_clock::time_point runtime_start_tp;
    float progress_log_period_s = 0.0f;
    float next_progress_log_sim_time_s = 0.0f;

};
#endif
