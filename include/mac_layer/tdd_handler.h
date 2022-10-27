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
    tdd_handler(int n_dl_slots, int n_ul_slots, int transition_c, int _tx)
    {
        if(!(transition_c > 0 && transition_c < N_CONFIG)) transition_c = FALLBACK_CONFIG;         
        for(int i = 0; i < n_dl_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[0]);
        for(int i = 0; i < 3; i++) for(int j = 0; j < TDD_5G_CONFIGURATION[transition_c][i]; j++) tdd_scheme.push_back(TDD_ORDER[i]) ; 
        for(int i = 0; i < n_ul_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[2]);
        tx = _tx;
        dl_slots = n_dl_slots; 
        ul_slots = n_ul_slots; 
        t_config = transition_c; 
        LOG_INFO_I("tdd_handler::tdd_handler") << "TDD Configuration DL [" << n_dl_slots << "]  UL [" << n_ul_slots << "] Transition [" << transition_c  << "] [" ;  
        for(int i = 0; i < 3; i++) for(int j = 0; j < TDD_5G_CONFIGURATION[transition_c][i]; j++)  std::cout << TDD_ORDER[i];
        LOG_INFO() << "]" << END(); 
    }
    
    tdd_handler(tdd_config tdd_c, int _tx)
    {

        if(!(tdd_c.transition_c > 0 && tdd_c.transition_c < N_CONFIG)) tdd_c.transition_c = FALLBACK_CONFIG;         
        for(int i = 0; i < tdd_c.n_dl_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[0]);
        for(int i = 0; i < 3; i++) for(int j = 0; j < TDD_5G_CONFIGURATION[tdd_c.transition_c][i]; j++) tdd_scheme.push_back(TDD_ORDER[i]) ; 
        for(int i = 0; i < tdd_c.n_ul_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[2]);
        tx = _tx;   
        dl_slots = tdd_c.n_dl_slots; 
        ul_slots = tdd_c.n_ul_slots; 
        t_config = tdd_c.transition_c;  
        LOG_INFO_I("tdd_handler::tdd_handler") << "TDD Configuration DL [" << tdd_c.n_dl_slots << "]  UL [" << tdd_c.n_ul_slots << "] Transition [" << tdd_c.transition_c  << "] [" ; 
        for(int i = 0; i < 3; i++) for(int j = 0; j < TDD_5G_CONFIGURATION[tdd_c.transition_c][i]; j++)  std::cout << TDD_ORDER[i];
        LOG_INFO() << "]" << END();     
    }
public: 

    int get_dl_slots(){return dl_slots; }
    int get_ul_slots(){return ul_slots; }
    int get_transition(){return t_config; }
   
    void set_syms(int _syms)
    {
        syms = _syms; 
    }

    int get_syms()
    {
        int syms_out = 0; 
        int index = c_index; 
        for(int i = 0; i < syms; i++)
        {
            if(index >= tdd_scheme.size()) index = 0; 
            if(tdd_scheme[index] == UL_SYM && tx == TX_UL) syms_out++;
            if(tdd_scheme[index] == DL_SYM && tx == TX_DL) syms_out++;
            index += 1; 
        }
        c_index = index;
        return syms_out;
    }

    void step()
    {
        c_index += syms; 
    }

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