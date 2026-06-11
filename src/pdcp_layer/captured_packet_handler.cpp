#include <algorithm>
#include <chrono>
#include <functional>
#include <utility>

#include <pdcp_layer/captured_packet_handler.h>

captured_packet_handler::captured_packet_handler(int queue_num, std::chrono::microseconds *_init_t, pdcp_config pdcp_c, int verbosity)
    : captured_packet_handler(std::unique_ptr<pkt_capture>(new pkt_capture(queue_num)), _init_t, pdcp_c, verbosity)
{
}

captured_packet_handler::captured_packet_handler(std::unique_ptr<pkt_capture> capture, std::chrono::microseconds *_init_t, pdcp_config pdcp_c, int verbosity)
    : packet_handler(pdcp_c, verbosity),
      init_t(_init_t),
      pkt_cptr(std::move(capture))
{
    do_check = pdcp_c.order_packets;
}

captured_packet_handler::~captured_packet_handler()
{
    quit();
}

void captured_packet_handler::init()
{
    if(!pkt_cptr) return;
    pkt_cptr->start();
}

void captured_packet_handler::quit()
{
    if(pkt_cptr) pkt_cptr->close();
}

float captured_packet_handler::ingest(int tx_dir, float current_t)
{
    (void)tx_dir;
    (void)current_t;
    if(!pkt_cptr) return 0.0f;

    float bits = 0.0f;
    captured_packet_info info;
    while(pkt_cptr->pop_captured_packet(info))
    {
        int size = (int)info.bytes * 8;
        ip_pkt pkt(get_current_ts(), size, size, prev_uid, info.pkt_id, bh_d, bh_d_var);
        pkt.ecn = info.ecn;
        pkt.original_ecn = info.ecn;
        bits += pkt.size;
        push_ingress_pkt(std::move(pkt));
        prev_uid = info.pkt_id;
    }
    return bits;
}

void captured_packet_handler::drop_ingress_pkt(ip_pkt pkt)
{
    pkt.erase = true;
    pkt.t_out = current_t;
    out_pkts.push_back(std::move(pkt));
    sort_by_id();
}

void captured_packet_handler::push(harq_pkt pkt)
{
    pkt.t_out += pkt.backhaul_d + gauss_dist(gauss_dist_gen) * pkt.backhaul_d_var;
    for(std::deque<ip_pkt>::iterator jt = pkt.pkts.begin(); jt != pkt.pkts.end(); jt++)
    {
        jt->t_out = pkt.t_out;
        jt->ip_t = pkt.ip_t;
        if(!add_data(&(*jt)))
        {
            jt->frags_recovered++;
            out_pkts.push_back(std::move(*jt));
        }
    }
    sort_by_id();
}

void captured_packet_handler::drop(harq_pkt pkt)
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

float captured_packet_handler::release()
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
                drop_packets++;
                pkt_cptr->verdict(it->uid, packet_capture_action::DROP);
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
                        if(it->ecn != it->original_ecn || it->ce_marked || it->force_ect1_applied)
                        {
                            rewrite_packets++;
                            if(it->ce_marked) ce_rewrite_packets++;
                            if(it->force_ect1_applied) force_ect1_packets++;
                            pkt_cptr->verdict(it->uid, packet_capture_action::ACCEPT_CE);
                        }
                        else
                        {
                            pkt_cptr->verdict(it->uid, packet_capture_action::ACCEPT);
                        }
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

void captured_packet_handler::fill_queue_status(pdcp_queue_status& status, float current_t) const
{
    if(pkt_cptr)
    {
        status.capture_size = pkt_cptr->captured_queue_size();
        captured_packet_info oldest_capture;
        if(pkt_cptr->peek_oldest_captured(oldest_capture))
        {
            status.capture_oldest_uid = (int)oldest_capture.pkt_id;
            status.capture_oldest_age = -1.0f;
        }

        packet_capture_stats stats = pkt_cptr->stats();
        status.nfqueue_queue_num = stats.queue_num;
        status.nfqueue_total_recv = (int)stats.total_recv;
        status.nfqueue_total_rlsd = (int)stats.total_rlsd;
        status.nfqueue_bytes_recv = (int)stats.bytes_recv;
        status.nfqueue_recv_fails = (int)stats.recv_fails;
        status.nfqueue_rlsd_fails = (int)stats.rlsd_fails;
    }

    status.release_size = (int)out_pkts.size();
    if(!out_pkts.empty())
    {
        const ip_pkt &pkt = out_pkts.front();
        status.release_oldest_uid = pkt.uid;
        status.release_oldest_age = current_t - pkt.ip_t;
    }
    status.nfqueue_rewrite_packets = rewrite_packets;
    status.nfqueue_ce_rewrite_packets = ce_rewrite_packets;
    status.nfqueue_force_ect1_packets = force_ect1_packets;
    status.nfqueue_drop_packets = drop_packets;
}

float captured_packet_handler::get_current_ts() const
{
    if(init_t == nullptr) return current_t;
    return (float)(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch() - *init_t).count()) * 0.000001f;
}

void captured_packet_handler::sort_by_id()
{
    std::sort(out_pkts.begin(), out_pkts.end(), sorter);
}

bool captured_packet_handler::add_data(ip_pkt *recv_pkt)
{
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

bool captured_packet_handler::remove_data(int id, float bits)
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

bool captured_packet_handler::check_order(int id)
{
    if(prev_released_id > -1 && do_check) return prev_released_id == id;
    return true;
}

void captured_packet_handler::update_order(int id)
{
    prev_released_id = id;
}
