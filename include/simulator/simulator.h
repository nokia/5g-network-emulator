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

    void handle_time_log(unsigned int _ts);

    void step(unsigned int _ts);

private: 
    configuration_loader config_loader; 
    ue_handler ue_h; 
    mac_layer mac_l;
private: 
    int freq = 1000; 
    int step_c = 0; 
    std::chrono::duration<double> time_mac = std::chrono::duration<double>().zero(); 
    std::chrono::duration<double> time_ue = std::chrono::duration<double>().zero(); 
    float total_time_mac; 
    float total_time_ue; 
private: 
    timer ticker; 
private: 
    float duration;
private: 
    int verbosity = 0; 

};
#endif
