/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <chrono>
#include <functional>
#include <thread>
#include <iostream>

#define S2US 1000000
#define US2S 0.000001

//--------------------------------------------------------------------------------------------------
// timer(): simple ticker module in charge of syncrhonizing the emulator, triggering a signal
// every [period] milliseconds. 
// Input: 
//      _period: of the signal triggering, in ms. If set to <0, simulation mode, the emulator runs
//               as fast as possible.
//      _goal: duration of the simulation
//--------------------------------------------------------------------------------------------------
class timer
{
public: 
    timer(float _period, float _goal = -1)
    {
        if(_period > 0)
        {
            period = (int)(_period*1000); 
            do_wait = true; 
        }
        else
        {
            do_wait = false; 
        }
        goal = (int)(S2US * _goal); 
    }

    ~timer()
    {
        if(run)
        {
            run = false; 
            tickthread.join(); 
        }
    }

public: 
    void start(std::function<void(unsigned int)> _func)
    {
        {
            init_t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
            func = _func; 
            run = true; 
            if(goal < 0)
            {
                tickthread = std::thread(&timer::ticker_inf, this);
            }
            else
            {
                tickthread = std::thread(&timer::ticker, this);
            }           
        }
    }

    void stop()
    {
        if(run == true)
        {
            run = false; 
            tickthread.join(); 
        }
    }

    std::chrono::microseconds * get_init_t()
    {
        return &init_t; 
    }

    void wait_finished()
    {
        while(!finished)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    unsigned int get_current_ts()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch() - init_t).count(); 
    }

private: 
    void ticker()
    {
        finished = false; 
        std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
        int count = 0; 
        int diff_prev = 0; 
        while(run && (previous_ts < goal))
        {
            if(do_wait)
            {
                current_ts = get_current_ts(); 
                func(current_ts); 
                int wait = std::max(0, period*(count+1) - (int)get_current_ts());
                std::this_thread::sleep_for(std::chrono::microseconds(wait));
                previous_ts = current_ts; 
                count++;
            }
            else
            {
                func(count*1000);
                previous_ts = count*1000; 
                count++;               
            }
        }
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t);
        if(do_wait) LOG_INFO_I("timer::ticker") << "Simulation time: " << goal*US2S << " - actual time: " << time_span.count() << END(); 
        else  LOG_INFO_I("timer::ticker") << "Simulation time: " << count*0.001 << " - actual time: " << time_span.count() << END(); 
        LOG_INFO_I("timer::ticker")  << "Total iterations: " << count << " mean step time: " << time_span.count()/count << END(); 
        finished = true; 
   }

    void ticker_inf()
    {
        finished = false; 
        std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
        int count = 0; 
        while(run)
        {
            if(do_wait)
            {
                current_ts = get_current_ts(); 
                func(current_ts); 
            
                int diff = (int)(period - (current_ts - previous_ts)); 
                int sign = (diff > 0) ? 1 : -1 ; 
                int wait = std::max(0, (int)(period - sign*diff -(get_current_ts() - current_ts)));
                std::this_thread::sleep_for(std::chrono::microseconds(wait));
                previous_ts = current_ts; 
                count++;
            }
            else
            {
                func(count*1000);
                previous_ts = count*1000; 
                count++;                  
            }
        }
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t);
        if(do_wait)  LOG_INFO_I("timer::ticker") << "Simulation time: " << goal*US2S << " - actual time: " << time_span.count() << END(); 
         LOG_INFO_I("timer::ticker") << "Simulation time: " << count*0.001 << " - actual time: " << time_span.count() << END(); 
         LOG_INFO_I("timer::ticker") << "Total iterations: " << count << " mean step time: " << time_span.count()/count << END(); 
        finished = true;
    }
private: 

    int period;
    std::thread tickthread;  
    std::function<void(unsigned int)> func; 
    std::chrono::microseconds init_t;

    unsigned int current_ts; 
    unsigned int previous_ts; 
    bool run = false; 
    int goal = 0; 
    bool finished = false; 
    bool do_wait = false; 
};