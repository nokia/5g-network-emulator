/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <mac_layer/resource_block.h>
#include <utils/terminal_logging.h>

rb::rb(int _t, int _f, int _tx, int _mimo_layers,  int _n_sc, std::vector<ue> *_ue_list, int _verbosity, log_handler * _logger)
{
    t = _t; 
    f = _f; 
    tx = _tx; 
    mimo_layers = _mimo_layers; 
    n_sc = _n_sc; 
    ue_list = _ue_list; 
    log = _logger != nullptr; 
    if(log) logger = _logger; 
    verbosity = _verbosity; 
}

void rb::reset_metric()
{
    max_metric.reset();
}

bool rb::check_ue_assignment()
{
    if(max_metric.is_assigned())
    {
        if(max_metric.has_data())
        {
            return true; 
        }
        else
        {
            max_metric.reset(); 
            return false; 
        } 
    }
    return false; 
}
 
void rb::estimate_params(int syms, float _current_t)
{
    int index = 0; 
    current_t = _current_t;
    int n_ues = ue_list->size(); 
    if(syms > 0)
    {
        for(std::vector<ue>::iterator it = ue_list->begin(); it != ue_list->end(); ++it)
        {
            schedule_candidate candidate = it->get_schedule_candidate(tx, f, n_ues, index);
            if(candidate.has_data && max_metric.evaluate(candidate.metric)){
                max_metric.assign(candidate.bits_per_symbol, candidate.metric, candidate.ue_index, candidate.ue_id);
            }
            index++; 
        }
    }
}

// Handle Packets
float rb::handle_packet(int syms)
{
    last_scheduled = false;
    last_ue_index_v = -1;
    last_scheduled_bits_v = 0.0f;
    last_effective_bits_v = 0.0f;
    if(check_ue_assignment())
    {
        float tp = max_metric.get_tp(); 
        float eff_tp = (*ue_list)[max_metric.get_index()].handle_pkt(tp*syms, tx, f);
        last_scheduled = true;
        last_ue_index_v = max_metric.get_index();
        last_scheduled_bits_v = tp * syms;
        last_effective_bits_v = eff_tp;
        if(log) logger->log_partial("tx:{} f:{} t:{} id:{} m:{} tp:{} e_tp:{} ts:{} \n", tx, f, t, max_metric.get_id(), max_metric.get_value(),tp*syms, eff_tp, current_t);
        //LOG_INFO_I("GRID") << current_t << (tx ? " UL[" : " DL[") << f << "," << t << "]: " <<
        //    "ID [" << max_metric.get_id() << "][]" max_metric.get_value(),tp*syms, eff_tp, current_t << END();
        max_metric.reset(); 
        return eff_tp; 
    }
    max_metric.reset(); 
    return 0;
}
//MAC_LOGGER
