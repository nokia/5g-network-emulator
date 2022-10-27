/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

//--------------------------------------------------------------------------------------------------
// log_config(): logger configuration struct.
// Input: 
//              *log_freq: data is logged according to this period in ms.
//              *log_ue: enable/disable UE logging
//              *log_id: UE's unique ID
//              *verbosity: enable/disable verbosity
//--------------------------------------------------------------------------------------------------
struct log_config
{
    log_config(bool _log_mac, int _log_freq, std::string _log_id, int _verbosity)
    {
        log_mac = _log_mac; 
        log_freq = _log_freq; 
        log_id = _log_id; 
        verbosity = _verbosity; 
    }
    bool log_mac = false; 
    int log_freq = 100; 
    std::string log_id = ""; 
    int verbosity = 0; 
};

//--------------------------------------------------------------------------------------------------
// log_handler(): log handling class which interfaces with spdlog library. 
// Input: 
//              *dir: directory where the file will be created. 
//              *filename: filename name, has to be UNIQUE.
//--------------------------------------------------------------------------------------------------
class log_handler
{
public:
    void init( std::string dir, std::string filename)
    {
        if(dir[dir.length() - 1] != '/') dir += "/";
        logger = spdlog::basic_logger_mt<spdlog::async_factory_nonblock>(filename, dir + filename + ".txt");
        logger->set_pattern("%v");
        is_init = true; 
        is_ready = true; 
    }

    void init( std::string dir, std::string filename, int log_freq)
    {
        if(dir[dir.length() - 1] != '/') dir += "/";
        logger = spdlog::basic_logger_mt<spdlog::async_factory_nonblock>(filename, dir + filename + ".txt");
        logger->set_pattern("%v");
        use_counter = log_freq != -1;
        counter_goal = 1000/log_freq - 1; 
        is_init = true; 
        is_ready = true; 
    }

    bool ready()
    {
        return is_init && is_ready;
    }

    void log(std::string info)
    {
        if(is_ready) logger->info(info);
        if(use_counter) handle_counter(); 
    }

    void log_force(std::string info)
    {
        logger->info(info);
    }

    void handle_counter()
    {
        counter++; 
        if(counter > counter_goal)
        {
            counter = 0; 
            is_ready = true; 
        }
        else is_ready = false; 
    }

    template<typename FormatString, typename... Args>
    void log(const FormatString &fmt, Args&&... args)
    {
        if(is_ready) logger->info(fmt, std::forward<Args>(args)...);
        if(use_counter) handle_counter(); 
    }

    template<typename FormatString, typename... Args>
    void log_force(const FormatString &fmt, Args&&... args)
    {
        logger->info(fmt, std::forward<Args>(args)...);
    }

   void log_partial(std::string info)
    {
         if(is_ready) info_out += info; ;
    }

    template<typename FormatString, typename... Args>
    void log_partial(const FormatString &fmt, Args&&... args)
    {
        if(is_ready) info_out += fmt::format(fmt, std::forward<Args>(args)...);
    }

    void flush(std::string info)
    {
        if(is_ready && info_out.size() > 0) 
        {
            info_out += info; 
            if(info_out.size() > 0) logger->info(info_out);
            reset_info(); 
        }
        if(use_counter) handle_counter(); 
    }

    template<typename FormatString, typename... Args>
    void flush(const FormatString &fmt, Args&&... args)
    {
        if(is_ready)
        {
            info_out += fmt::format(fmt, std::forward<Args>(args)...);
            if(info_out.size() > 0) logger->info(info_out);
            reset_info(); 
        }
        if(use_counter) handle_counter(); 
    }

    void flush()
    {
        if(is_ready)
        {
            if(info_out.size() > 0) logger->info(info_out);
            reset_info(); 
        }
        if(use_counter) handle_counter(); 
    }
    
private: 
    void reset_info()
    {
        info_out.erase();
    }
private: 
    bool is_init = false; 
    bool is_ready = false; 
    bool use_counter = false; 
    int log_freq = -1;

    int counter = 0; 
    int counter_goal = 0; 

private: 
    std::string info_out = ""; 

private: 
    std::shared_ptr<spdlog::logger> logger; 
};