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

#include <utils/conversions.h>

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
        LOG_INFO_I("timer::ticker") << "Period: " << period << " Goal: " << goal << END(); 
    }

    ~timer()
    {
        stop();
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

    // Safe to call more than once, and after request_stop: what decides whether there is
    // a thread to join is the thread, not the run flag. Joining on the flag left a
    // finished thread unjoined when the loop had ended on its own, and destroying a
    // joinable thread terminates the process.
    void stop()
    {
        run = false;
        if(tickthread.joinable()) tickthread.join();
    }

    // Asks the loop to finish without joining, so that it can be called from inside the
    // ticker thread itself, which is where simulator::step runs.
    void request_stop()
    {
        run = false;
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
        //LOG_INFO_I("timer::ticker") << "FIST Run: " << run << " previous_ts: " << previous_ts << " goal: " << goal << " do_wait: " << do_wait << END(); 
        while(run && (previous_ts < goal))
        {
            //LOG_INFO_I("timer::ticker") << "Run: " << run << " previous_ts: " << previous_ts << " goal: " << goal << " do_wait: " << do_wait << END(); 
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
                func(count*TTI_US);
                previous_ts = count*TTI_US; 
                count++;               
            }
        }
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t);
        if(do_wait) LOG_INFO_I("timer::ticker") << "Simulation time: " << goal*US2S << " - actual time: " << time_span.count() << END(); 
        else  LOG_INFO_I("timer::ticker") << "Simulation time: " << count*TTI_S << " - actual time: " << time_span.count() << END(); 
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
                func(count*TTI_US);
                previous_ts = count*TTI_US; 
                count++;                  
            }
        }
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t);
        if(do_wait)  LOG_INFO_I("timer::ticker") << "Simulation time: " << goal*US2S << " - actual time: " << time_span.count() << END(); 
         LOG_INFO_I("timer::ticker") << "Simulation time: " << count*TTI_S << " - actual time: " << time_span.count() << END(); 
         LOG_INFO_I("timer::ticker") << "Total iterations: " << count << " mean step time: " << time_span.count()/count << END(); 
        finished = true;
    }
private: 

    int period;
    std::thread tickthread;  
    std::function<void(unsigned int)> func; 
    std::chrono::microseconds init_t;

    unsigned int current_ts = 0; 
    unsigned int previous_ts = 0; 
    bool run = false; 
    int goal = 0; 
    bool finished = false; 
    bool do_wait = false; 
};