#include <algorithm>
#include <cstdint>

#include <netfilter/pkt_capture.h>
#include <pdcp_layer/release_handler_ip.h>

release_handler_ip::release_handler_ip(bool _do_check)
    : release_handler()
{
    do_check = _do_check;
}

void release_handler_ip::set_pkt_cptr(const std::shared_ptr<pkt_capture> &_pkt_cptr)
{
    pkt_cptr = _pkt_cptr;
    debug_queue_num = pkt_cptr->queue_num;
}

void release_handler_ip::sort_by_id()
{
    std::sort(out_pkts.begin(), out_pkts.end(), sorter);
}

void release_handler_ip::push(harq_pkt pkt)
{
    pkt.t_out += pkt.backhaul_d + gauss_dist(gauss_dist_gen) * pkt.backhaul_d_var;
    for(std::deque<ip_pkt>::iterator jt = pkt.pkts.begin(); jt != pkt.pkts.end(); jt++)
    {
        jt->t_out = pkt.t_out;
        jt->ip_t = pkt.ip_t;
        if(!add_data(&(*jt), pkt.id))
        {
            jt->frags_recovered++;
            out_pkts.push_back(std::move(*jt));
        }
    }
    sort_by_id();
}

bool release_handler_ip::add_data(ip_pkt *recv_pkt, int harq_id)
{
    (void)harq_id;
    for(std::deque<ip_pkt>::iterator jt = out_pkts.begin(); jt != out_pkts.end(); jt++)
    {
        if(jt->uid == recv_pkt->uid)
        {
            jt->size += recv_pkt->size;
            jt->frags_recovered++;
            jt->frags_created = recv_pkt->frags_created;
            return true;
        }
    }
    return false;
}

bool release_handler_ip::remove_data(int id, float bits)
{
    for(std::deque<ip_pkt>::iterator jt = out_pkts.begin(); jt != out_pkts.end(); jt++)
    {
        if(jt->uid == (uint32_t)id)
        {
            jt->erase = true;
            jt->size += bits;
            return true;
        }
    }
    return false;
}

void release_handler_ip::drop(harq_pkt pkt)
{
    for(std::deque<ip_pkt>::iterator jt = pkt.pkts.begin(); jt != pkt.pkts.end(); jt++)
    {
        jt->t_out = pkt.t_out;
        jt->ip_t = pkt.ip_t;
        if(!remove_data(jt->uid, jt->size))
        {
            jt->erase = true;
            out_pkts.push_back(std::move(*jt));
        }
    }
}

void release_handler_ip::fill_queue_status(pdcp_queue_status& status, float current_t) const
{
    status.release_size = (int)out_pkts.size();
    if(!out_pkts.empty())
    {
        const ip_pkt &pkt = out_pkts.front();
        status.release_oldest_uid = pkt.uid;
        status.release_oldest_age = current_t - pkt.ip_t;
    }
}

float release_handler_ip::release()
{
    int count = 0;
    float bits = 0;
    float latency = 0;
    float ip_latency = 0;
    for(std::deque<ip_pkt>::iterator it = out_pkts.begin(); it != out_pkts.end();)
    {
        if(it->is_ready())
        {
            if(it->erase)
            {
                update_order(it->uid);
                pkt_cptr->drop(it->uid);
                it = out_pkts.erase(it);
            }
            else
            {
                if(check_order(it->prev_uid))
                {
                    if(it->t_out <= current_t)
                    {
                        bits += it->original_size;
                        latency += current_t - it->current_t;
                        ip_latency += current_t - it->ip_t;
                        update_order(it->uid);
                        pkt_cptr->release(it->uid);
                        it = out_pkts.erase(it);
                        count++;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else break;
    }
    tp_mean.add(bits);
    if(count > 0)
    {
        l_mean.add(latency/count);
        ipl_mean.add(ip_latency/count);
        l_mean.step();
        ipl_mean.step();
    }
    return bits;
}

bool release_handler_ip::check_order(int id)
{
    if(prev_id > -1 && do_check) return prev_id == id;
    return true;
}

void release_handler_ip::update_order(int id)
{
    prev_id = id;
}
