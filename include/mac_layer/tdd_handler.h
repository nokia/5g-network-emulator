/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef TDD_HANDLER_H
#define TDD_HANDLER_H

#include <vector>
#include <mac_layer/mac_definitions.h>
#include <utils/terminal_logging.h>


struct tdd_config
{
    tdd_config(int _n_dl_slots, int _n_ul_slots, int _transition_c)
    {
        n_dl_slots = _n_dl_slots; 
        n_ul_slots = _n_ul_slots; 
        transition_c = _transition_c;       
    }
    int n_dl_slots; 
    int n_ul_slots; 
    int transition_c; 
};

//--------------------------------------------------------------------------------------------------
// tdd_handler(): class which determines the available number of DL/UL slots available for the current
// subframe (1ms) given the current timestep. The method get_symbols() returns the number of Resource
// Elements available for transmission in the selected TX direction (UL or DL) according to the current
// timestamp and the selected TDD configuration. 
// Input: 
//      * _n_dl_slots: number of DL slots within the TDD pattern.
//      * _n_ul_slots: number of UL slots within the TDD pattern.
//      * transition_c: Transition configuration which determines the number of DL/UL Resource Elements
//                      available within the transtion (DL <-> UL) slot.
//--------------------------------------------------------------------------------------------------
class tdd_handler
{
public: 
    tdd_handler(int n_dl_slots, int n_ul_slots, int transition_c, int _tx);
    
    tdd_handler(tdd_config tdd_c, int _tx);
public: 

    int get_dl_slots(){return dl_slots; }
    int get_ul_slots(){return ul_slots; }
    int get_transition(){return t_config; }
   
    void set_syms(int _syms);

    int get_syms();

    void step();

private: 
    std::vector<int> tdd_scheme; 
    int c_index = 0; 
    int syms; 
    int tx;

    int dl_slots; 
    int ul_slots; 
    int t_config; 

};

#endif
