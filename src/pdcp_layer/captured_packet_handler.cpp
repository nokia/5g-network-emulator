#include <algorithm>
#include <chrono>
#include <functional>
#include <utility>

#include <pdcp_layer/captured_packet_handler.h>

captured_packet_handler::captured_packet_handler(int queue_num, std::chrono::microseconds *_init_t, pdcp_config pdcp_c, int verbosity)
    : captured_packet_handler(std::unique_ptr<packet_capture_interface>(new real_packet_capture(queue_num)), _init_t, pdcp_c, verbosity)
{
}

captured_packet_handler::captured_packet_handler(std::unique_ptr<packet_capture_interface> capture, std::chrono::microseconds *_init_t, pdcp_config pdcp_c, int verbosity)
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
    pkt_cptr->start(std::bind(&captured_packet_handler::cb, this, std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4,
                              std::placeholders::_5, std::placeholders::_6));
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
    pkt_cptr->lock();
    for(std::deque<ip_pkt>::iterator it = captured_pkts.begin(); it != captured_pkts.end();)
    {
        bits += it->size;
        push_ingress_pkt(std::move(*it));
        it = captured_pkts.erase(it);
    }
    pkt_cptr->unlock();
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

void captured_packet_handler::fill_queue_status(pdcp_queue_status& status, float current_t) const
{
    if(pkt_cptr)
    {
        pkt_cptr->lock();
        status.capture_size = (int)captured_pkts.size();
        if(!captured_pkts.empty())
        {
            const ip_pkt &cap_pkt = captured_pkts.front();
            status.capture_oldest_uid = cap_pkt.uid;
            status.capture_oldest_age = current_t - cap_pkt.ip_t;
        }
        pkt_cptr->unlock();
    }

    status.release_size = (int)out_pkts.size();
    if(!out_pkts.empty())
    {
        const ip_pkt &pkt = out_pkts.front();
        status.release_oldest_uid = pkt.uid;
        status.release_oldest_age = current_t - pkt.ip_t;
    }
}

void captured_packet_handler::cb(void* handler, netfilter_interface_t *nfiface, uint64_t timestamp_sec,
                                 uint64_t timestamp_usec, uint64_t bytes, uint32_t pkt_id)
{
    (void)handler;
    (void)nfiface;
    (void)timestamp_sec;
    (void)timestamp_usec;

    int size = (int)bytes * 8;
    pkt_cptr->lock();
    captured_pkts.push_back(ip_pkt(get_current_ts(), size, size, prev_uid, pkt_id, bh_d, bh_d_var));
    prev_uid = pkt_id;
    pkt_cptr->unlock();
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
