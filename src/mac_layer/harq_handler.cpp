/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <mac_layer/harq_handler.h>

harq_handler::harq_handler(int _max_rtx, float _air_delay, 
                 float _rtx_period, float _rtx_period_var, 
                 float _rtx_p_delay, float _rtx_p_delay_var, int _verbosity)
            :generator(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
{
    max_rtx = _max_rtx; 
    air_delay = _air_delay;
    stchstc_air = !(air_delay == 0);
    rtx_p_delay = _rtx_p_delay;
    rtx_p_delay_var = _rtx_p_delay_var;
    rtx_period = _rtx_period;
    rtx_period_var = _rtx_period_var;
    stchstc_delay = !(rtx_p_delay_var == 0)||!(rtx_period_var == 0);
    verbosity = 0; 
}
void harq_handler::step(float t)
{
    current_t = t; 
}

int harq_handler::get_rtx_mbits()
{
    return rtx_mbit; 
}

void harq_handler::queue(harq_pkt pkt, float distance)
{
    rtx_mbit += pkt.bits*BIT2MBIT; 
    pkt.n_tx++; 
    pkt.set_delay(emulate_ack_delay(distance));
    harq_buffer.push_back(std::move(pkt));
    if(harq_buffer.size() >= 1)
    {
        oldest_t = pkt.current_t; 
        t_out = pkt.t_out; 
    } 
}

void harq_handler:: add_pkts(harq_pkt pkt)
{
    pkt.set_delay(emulate_ack_delay(pkt.distance));
    harq_buffer.push_back(std::move(pkt));
    if(harq_buffer.size() >= 1)
    {
        oldest_t = pkt.current_t; 
        t_out = pkt.t_out; 
    } 
}

bool harq_handler::is_pkt_ready()
{
    if(harq_buffer.size() > 0)  return (harq_buffer.size() > 0 && t_out <= current_t);
    else return false; 
}

harq_pkt harq_handler::get_pkt()
{
	
    assert(is_pkt_ready());
    harq_pkt pkt = harq_buffer.front();
    if(harq_buffer.size() > 1)
    {
        harq_buffer.pop_front();
        oldest_t = harq_buffer.front().current_t; 
        t_out = harq_buffer.front().t_out; 
    }
    else
    {
        t_out = INF; 
        harq_buffer.pop_front();
    }
    return std::move(pkt); 
}

float harq_handler::get_oldest_t()
{
    if(harq_buffer.size() > 0) return oldest_t; 
    else return current_t; 
}

float harq_handler::emulate_ack_delay(float distance)
{
    float tx_delay = 2 * distance / SPEED_OF_LIGHT; 
    if(stchstc_air) tx_delay += rtx_period + rtx_period_var*rtx_period_dist(generator);
    else tx_delay += rtx_period ; 
    if(stchstc_delay) tx_delay += rtx_p_delay + rtx_p_delay_var * p_delay_dist(generator); 
    else tx_delay += rtx_p_delay; 
    return tx_delay; 
}

void harq_handler::init(int _mod_i, int _layers, int _logic_units)
{
    rbg_i = GET_RBG_INDEX(_logic_units);
    mod_i = _mod_i; 
    l_i = _layers - 1; 
    logic_units = _logic_units;
}

bool harq_handler::get_rtx(int mcs, float sinr, int n_tx)
{
    int sinr_i = SINR_TO_INDEX(sinr);
    double bler = BLER_MCS_SINR[mod_i][rbg_i][l_i][mcs][sinr_i];     
    double success = std::pow(1-bler, n_tx + 1);
    // Not necessary, we already estimate the BLER for the entire block
    //success = pow(success, logic_units);
    double error = (1 - success)*std::pow((1-ERROR_RED_HARQ), n_tx-1);
    //std::cout << "ERROR: " << error << " sinr: " << sinr << " bler: " << bler << " mcs: " << mcs << std::endl; 
    return rtx_prob(generator) <= error; 
}

bool harq_handler::get_rtx()
{
    return rtx_prob(generator) <= TARGET_BLER; 
}
