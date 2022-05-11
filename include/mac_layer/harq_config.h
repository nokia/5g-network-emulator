/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

//--------------------------------------------------------------------------------------------------
// pdcp_config(): HARQ configuration struct
// Input: 
//              *mod: 0 - 64 QAM | 1 - 256 QAM
//              *layers: Number of MIMO layers. 
//              *log_units: number of RBs per RBG group. 
//--------------------------------------------------------------------------------------------------
struct harq_config
{
    harq_config(int _mod, int _layers, int _log_units)
    {
        mod = _mod; 
        layers = _layers; 
        log_units = _log_units; 
    }
    int mod; 
    int layers; 
    int log_units; 
};