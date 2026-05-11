/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <utility>

#include <pdcp_layer/pdcp_layer.h>
#include <utils/terminal_logging.h>

pdcp_layer::pdcp_layer(packet_handler_config handler_cfg, int _verbosity)
    : _harq_buffer(handler_cfg.pdcp_c.max_rtx, handler_cfg.pdcp_c.air_delay_var,
                   handler_cfg.pdcp_c.rtx_period, handler_cfg.pdcp_c.rtx_period_var,
                   handler_cfg.pdcp_c.rtx_proc_delay, handler_cfg.pdcp_c.rtx_proc_delay_var,
                   _verbosity),
      _ip_buffer(_verbosity),
      _packet_h(make_packet_handler(handler_cfg)),
      verbosity(_verbosity),
      tx_dir(handler_cfg.tx_dir)
{
    _ip_buffer.debug_queue_num = handler_cfg.queue_num;
    _ip_buffer.configure_l4s(handler_cfg.l4s_c);
    bh_d = handler_cfg.pdcp_c.bh_d;
    bh_d_var = handler_cfg.pdcp_c.bh_d_var;
}

void pdcp_layer::exit()
{
    _packet_h->quit();
}

void pdcp_layer::init(int _mod_i, int _layers, int _logic_units)
{
    _harq_buffer.init(_mod_i, _layers, _logic_units);
    _packet_h->init();
}

float pdcp_layer::release()
{
	return _packet_h->release();
}

void pdcp_layer::step(float t)
{
    current_t = t;
    _ip_buffer.step(current_t);
    _harq_buffer.step(current_t);
    _packet_h->step(current_t);
    _packet_h->ingest(tx_dir, current_t);
    drain_ingress_pkts();
    cleanup_expired_pkts();
}

void pdcp_layer::drain_ingress_pkts()
{
    while(_packet_h->has_ingress_pkts())
    {
        ip_pkt pkt = _packet_h->pop_ingress_pkt();
        if(!_ip_buffer.add_pkt(pkt))
        {
            _packet_h->drop_ingress_pkt(std::move(pkt));
        }
    }
    harq_pkt dropped;
    while(_ip_buffer.pop_aqm_dropped_pkt(dropped))
    {
        _packet_h->record_error(dropped.bits);
        _packet_h->drop(std::move(dropped));
    }
}

void pdcp_layer::release_pkts(harq_pkt pkt)
{
    _packet_h->push(std::move(pkt));
}

bool pdcp_layer::has_pkts()
{
    return _ip_buffer.has_pkts() || _harq_buffer.is_pkt_ready();
}

float pdcp_layer::get_oldest_timestamp()
{
    if(_harq_buffer.is_pkt_ready())
    {
        return _harq_buffer.get_oldest_t();
    }
    else if(_ip_buffer.has_pkts())
    {
        return _ip_buffer.get_oldest_timestamp();
    }
    return current_t;
}

float pdcp_layer::handle_pkt(float bits, int mcs, float sinr, float distance)
{
    if(!_harq_buffer.is_pkt_ready())
    {
        if(has_pkts())
        {
            while(bits > 0 && has_pkts())
            {
                harq_pkt pkt(current_id, _ip_buffer.get_oldest_timestamp(), current_t, mcs, distance, 0, bh_d, bh_d_var);
                current_id++;
                _ip_buffer.get_pkts(bits, pkt);
                harq_pkt dropped;
                while(_ip_buffer.pop_aqm_dropped_pkt(dropped))
                {
                    _packet_h->record_error(dropped.bits);
                    _packet_h->drop(std::move(dropped));
                }
                if(pkt.bits <= 0.0f) continue;
                if(is_expired(pkt))
                {
                    drop_harq_pkt(std::move(pkt));
                    continue;
                }
                if(_harq_buffer.get_rtx(mcs, sinr, 0))
                {
                    _harq_buffer.add_pkts(std::move(pkt));
                   return 0;
                }
                else
                {
                    bits = pkt.bits;
                    release_pkts(std::move(pkt));
                    return bits;
                }
            }
            return 0.0;
        }
		return 0.0;
    }
    else
    {
        harq_pkt pkt = _harq_buffer.get_pkt();
        if(_harq_buffer.get_rtx(pkt.mcs_i, sinr, pkt.n_tx))
        {
            if(pkt.n_tx < 4) _harq_buffer.queue(std::move(pkt), distance);
            else
            {
                drop_harq_pkt(std::move(pkt));
            }
            return 0;
        }
        else
        {
            bits = pkt.bits;
            release_pkts(std::move(pkt));
            return bits;
        }
    }
}

void pdcp_layer::drop_harq_pkt(harq_pkt pkt)
{
    _ip_buffer.drop_pkt(pkt.bits);
    _packet_h->record_error(pkt.bits);
    _packet_h->drop(std::move(pkt));
}

bool pdcp_layer::is_expired(float ip_t) const
{
    return ip_t < oldest_allowed_ip_t();
}

bool pdcp_layer::is_expired(const harq_pkt& pkt) const
{
    return is_expired(pkt.ip_t);
}

float pdcp_layer::oldest_allowed_ip_t() const
{
    return current_t - pkt_delay_budget_s;
}

float pdcp_layer::get_generated(bool partial)
{
    return _ip_buffer.get_generated(partial);
}

float pdcp_layer::get_error(bool partial)
{
    return _ip_buffer.get_error(partial);
}

float pdcp_layer::get_latency(bool partial)
{
    return _packet_h->get_latency(partial);
}

float pdcp_layer::get_ip_latency(bool partial)
{
    return _packet_h->get_ip_latency(partial);
}

float pdcp_layer::get_tp(bool partial)
{
    return _packet_h->get_tp(partial);
}

void pdcp_layer::cleanup_expired_pkts()
{
    cleanup_expired_ip_pkts();
    cleanup_expired_harq_pkts();
}

void pdcp_layer::cleanup_expired_ip_pkts()
{
    while(_ip_buffer.has_pkts())
    {
        float oldest_ip_t = _ip_buffer.get_oldest_timestamp();
        if(!is_expired(oldest_ip_t)) break;

        harq_pkt pkt(current_id, oldest_ip_t, current_t, 0, 0, 0, bh_d, bh_d_var);
        current_id++;

        if(!_ip_buffer.pop_oldest_pkt(pkt)) break;
        drop_harq_pkt(std::move(pkt));
    }
}

void pdcp_layer::cleanup_expired_harq_pkts()
{
    harq_pkt pkt;
    while(_harq_buffer.pop_pkt_older_than(oldest_allowed_ip_t(), pkt))
    {
        drop_harq_pkt(std::move(pkt));
    }
}

pdcp_queue_status pdcp_layer::get_queue_status() const
{
    pdcp_queue_status status;

    status.ip_buffer_size = _ip_buffer.size();
    const ip_pkt* oldest_ip = _ip_buffer.peek_oldest_pkt();
    if(oldest_ip != nullptr)
    {
        status.ip_oldest_uid = oldest_ip->uid;
        status.ip_oldest_age = current_t - oldest_ip->ip_t;
    }

    status.harq_size = _harq_buffer.size();
    const harq_pkt* oldest_harq = _harq_buffer.peek_oldest();
    if(oldest_harq != nullptr)
    {
        status.harq_oldest_id = oldest_harq->id;
        status.harq_oldest_age = current_t - oldest_harq->ip_t;
        status.harq_oldest_n_tx = oldest_harq->n_tx;
    }

    status.pkt_delay_budget_s = pkt_delay_budget_s;
    dualpi2_stats l4s_stats = _ip_buffer.get_l4s_stats();
    status.l4s_ce_packets = l4s_stats.ce_packets;
    status.l4s_ce_bits = l4s_stats.ce_bits;
    status.l4s_aqm_drops = l4s_stats.aqm_drops;
    status.l4s_aqm_drop_bits = l4s_stats.aqm_drop_bits;
    status.l4s_queue_size = l4s_stats.l4s_queue_size;
    status.classic_queue_size = l4s_stats.classic_queue_size;
    status.l4s_queue_bits = l4s_stats.l4s_queue_bits;
    status.classic_queue_bits = l4s_stats.classic_queue_bits;
    status.dualpi2_p_l = l4s_stats.p_l;
    status.dualpi2_p_c = l4s_stats.p_c;
    status.dualpi2_p_cl = l4s_stats.p_cl;
    _packet_h->fill_queue_status(status, current_t);
    return status;
}

dualpi2_stats pdcp_layer::get_l4s_interval_stats()
{
    return _ip_buffer.get_l4s_interval_stats();
}
