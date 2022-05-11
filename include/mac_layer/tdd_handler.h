/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#ifndef TDD_HANDLER_H
#define TDD_HANDLER_H

#include <vector>
#include <mac_layer/mac_definitions.h>


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
    tdd_handler(int n_dl_slots, int n_ul_slots, int transition_c)
    {
        if(!(transition_c > 0 && transition_c < N_CONFIG)) transition_c = FALLBACK_CONFIG;         
        for(int i = 0; i < n_dl_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[0]);
        for(int i = 0; i < 3; i++) for(int j = 0; j < TDD_5G_CONFIGURATION[transition_c][i]; j++) tdd_scheme.push_back(TDD_ORDER[i]) ; 
        for(int i = 0; i < n_ul_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[2]);
    }
    
    tdd_handler(tdd_config tdd_c)
    {
        if(!(tdd_c.transition_c > 0 && tdd_c.transition_c < N_CONFIG)) tdd_c.transition_c = FALLBACK_CONFIG;         
        for(int i = 0; i < tdd_c.n_dl_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[0]);
        for(int i = 0; i < 3; i++) for(int j = 0; j < TDD_5G_CONFIGURATION[tdd_c.transition_c][i]; j++) tdd_scheme.push_back(TDD_ORDER[i]) ; 
        for(int i = 0; i < tdd_c.n_ul_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[2]);
    }
public: 
    // SHOULD ONLY BE CALLED ONCE EVERY TTI
    void get_symbols(int syms, int &dl_syms, int &ul_syms)
    {
        dl_syms = 0; 
        ul_syms = 0; 
        int index = c_index; 
        for(int i = 0; i < syms; i++)
        {
            index += i; 
            if(index > tdd_scheme.size()) index = 0; 
            if(tdd_scheme[index] == UL_SYM) ul_syms++;
            if(tdd_scheme[index] == DL_SYM) dl_syms++;
        }
        c_index = index; 
    }
    
    void set_syms(int _syms)
    {
        syms = _syms; 
    }

    int get_syms(int tx)
    {
        int syms_out = 0; 
        int index = c_index; 
        for(int i = 0; i < syms; i++)
        {
            index += 1; 
            if(index > tdd_scheme.size()) index = 0; 
            if(tdd_scheme[index] == UL_SYM && tx == TX_UL) syms_out++;
            if(tdd_scheme[index] == DL_SYM && tx == TX_DL) syms_out++;
        }
        c_index = index; 
        return syms_out;
    }

private: 
    std::vector<int> tdd_scheme; 
    int c_index = 0; 
    int syms; 

};

#endif