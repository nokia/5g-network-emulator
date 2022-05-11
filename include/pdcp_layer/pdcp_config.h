/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once
//--------------------------------------------------------------------------------------------------
// pdcp_config(): PDCP configuration struct.
// Input: 
//              *max_rtx_ul/dl: max number of retransmissions for UL/DL HARQ packets.
//              *air_delay_var: added variance to the delay comming from the air propagation.
//              *rtx_period_ul/dl: ack/nack delay in seconds for UL/DL HARQ packets.
//              *rtx_period_ul/dl_var: white noise spread around the ack/nack delay in seconds for UL/DL HARQ packets.
//              *rtx_proc_delay_ul/dl: ack/nack processing delay in seconds for UL/DL HARQ packets.
//              *rtx_proc_delay_ul/dl_var: white noise spread around the ack/nack processing delay in seconds for UL/DL HARQ packets.
//              *backhaul_d: backhauling delay, use for Release buffer.
//              *backhaul_d_var: white noise spread around the backhauling delay.
//              *order_pkts: enable/disable RLC reordering.
//--------------------------------------------------------------------------------------------------
struct pdcp_config
{
    pdcp_config(){}
    pdcp_config(int _max_rtx, float _air_delay_var, 
                float _rtx_period, float _rtx_period_var, float _rtx_proc_delay, 
                float _rtx_proc_delay_var, float _bh_d, float _bh_d_var, bool _order_packets)
                {
                    max_rtx = _max_rtx; 
                    air_delay_var = _air_delay_var; 
                    rtx_period = _rtx_period; 
                    rtx_period_var = _rtx_period_var; 
                    rtx_proc_delay = _rtx_proc_delay;
                    rtx_proc_delay_var = _rtx_proc_delay_var;
                    bh_d = _bh_d; 
                    bh_d_var = _bh_d_var;
                    order_packets = _order_packets;
                }
    int max_rtx; 
    float air_delay_var; 
    float rtx_period; 
    float rtx_period_var; 
    float rtx_proc_delay; 
    float rtx_proc_delay_var; 
    float bh_d; 
    float bh_d_var; 
    bool order_packets; 
};
