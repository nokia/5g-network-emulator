#include <simulator/simulator.h>
#include <utils/monitoring/monitoring_manager.h>

namespace
{
metric_field make_runtime_field(double value, field_aggregation aggregation)
{
    metric_field field;
    field.value = value;
    field.aggregation = aggregation;
    return field;
}
}

simulator::simulator(std::string config_file)
    : config_loader(config_file),
      ue_h(config_loader.get_threads()),
      mac_l(config_loader.get_threading(), ue_h.get_ue_list(), config_loader.get_mac_config(), config_loader.get_tdd_config(), config_loader.get_log_config()),
      ticker(config_loader.get_period(), config_loader.get_duration())
{
    verbosity = config_loader.get_log_config().verbosity;
    duration = config_loader.get_duration();
    progress_log_period_s = config_loader.get_progress_log_period_s();
    if(progress_log_period_s > 0)
        next_progress_log_sim_time_s = progress_log_period_s;
    // initialize monitoring manager if configured
    monitoring_manager::instance().init(config_loader.get_monitoring_config());
    std::list<ue_full_config> ue_c_list = config_loader.get_ue_c_list();
    phy_enb_config phy_c = config_loader.get_phy_enb_config();
    phy_c.n_rbgs = mac_l.get_n_freq_rbg();
    phy_c.n_sc_rbg = mac_l.get_n_sc_rbg();
    pdcp_config pdcp_c_ul = config_loader.get_pdcp_config_ul();
    pdcp_config pdcp_c_dl = config_loader.get_pdcp_config_dl();
    scenario_config scenario_c = config_loader.get_scenario_config();
    for(std::list<ue_full_config>::iterator it = ue_c_list.begin(); it != ue_c_list.end(); it++)
    {
        harq_config harq_c(phy_c.modulation_m, it->ue_c.get_phy_config().n_antennas, mac_l.get_logical_units());
        ue_h.add_ues(ticker.get_init_t(), *it, phy_c, scenario_c, pdcp_c_ul, pdcp_c_dl, harq_c, config_loader.get_stochastics());
    }
    ue_h.init();
}

void simulator::override_duration(float _duration)
{
    duration = _duration;
}

void simulator::start()
{
    log_runtime_start();
    ticker.start(std::bind(&simulator::step, this, std::placeholders::_1));
}

void simulator::join()
{
    ticker.wait_finished();
    log_runtime_stop("completed");
}

void simulator::terminate()
{
    ticker.stop();
    log_runtime_stop("terminated");
}

void simulator::print_traffic()
{
    ue_h.print_traffic();
}

void simulator::log_runtime_start()
{
    if(runtime_started) return;
    runtime_started = true;
    runtime_start_tp = std::chrono::steady_clock::now();

    const bool realtime = config_loader.get_period() > 0;
    const monitoring_config &monitoring_cfg = monitoring_manager::instance().get_config();
    LOG_INFO_I("simulator::start")
        << "Starting main loop"
        << " mode=" << (realtime ? "realtime" : "fast")
        << " duration_s=" << duration
        << " period_ms=" << config_loader.get_period()
        << " progress_log_period_s=" << progress_log_period_s
        << " ues=" << ue_h.get_ue_list()->size()
        << " monitoring=" << (monitoring_cfg.enabled ? "on" : "off")
        << " outputs=" << monitoring_cfg.outputs.size()
        << END();
}

void simulator::log_runtime_stop(const std::string &reason)
{
    if(runtime_stopped || !runtime_started) return;
    runtime_stopped = true;

    const double wall_time_s = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now() - runtime_start_tp).count();
    LOG_INFO_I("simulator::stop")
        << "Stopping main loop"
        << " reason=" << reason
        << " sim_duration_s=" << duration
        << " iterations=" << total_steps
        << " wall_time_s=" << wall_time_s
        << END();
}

void simulator::handle_time_log(double sim_time_s)
{
    step_c += 1;
    if(step_c == freq)
    {
        monitoring_manager &monitoring = monitoring_manager::instance();
        const monitoring_config &cfg = monitoring.get_config();

        if(monitoring.is_enabled() && cfg.emit_emulator_runtime)
        {
            metric_point point;
            point.measurement = "emulator_runtime";
            point.tags["mode"] = config_loader.get_period() > 0 ? "realtime" : "fast";
            point.fields["mac_step_time_ms_mean"] = make_runtime_field(1000.0 * time_mac.count() / step_c, field_aggregation::mean);
            point.fields["ue_step_time_ms_mean"] = make_runtime_field(1000.0 * time_ue.count() / step_c, field_aggregation::mean);
            point.fields["step_time_ms_mean"] = make_runtime_field(1000.0 * (time_ue.count() + time_mac.count()) / step_c, field_aggregation::mean);
            point.fields["sim_time_s_last"] = make_runtime_field(sim_time_s, field_aggregation::last);
            point.fields["loop_iterations_sum"] = make_runtime_field(step_c, field_aggregation::sum);
            point.fields["active_ues_last"] = make_runtime_field(ue_h.get_ue_list()->size(), field_aggregation::last);
            monitoring.publish(point);
        }

        if(cfg.emit_runtime_debug_logs)
        {
            LOG_INFO_I("simulator::handle_time_log") << " MAC Layer processing time: " << sim_time_s << ":" << (time_mac.count()/step_c) << END();
            LOG_INFO_I("simulator::handle_time_log") << " UEs processing time: " << sim_time_s << ":" << (time_ue.count()/step_c) << END();
            LOG_INFO_I("simulator::handle_time_log") << " Step Processing time: " << sim_time_s << ":" << ((time_ue.count()+time_mac.count())/step_c) << END();
            print_traffic();
        }
        step_c = 0;
        time_mac = std::chrono::duration<double>().zero();
        time_ue = std::chrono::duration<double>().zero();
    }
}

void simulator::maybe_log_progress(double sim_time_s)
{
    if(progress_log_period_s <= 0 || progress_steps == 0)
        return;

    if(sim_time_s + 1e-9 < next_progress_log_sim_time_s)
        return;

    const double mac_mean_ms = 1000.0 * progress_time_mac.count() / progress_steps;
    const double ue_mean_ms = 1000.0 * progress_time_ue.count() / progress_steps;

    LOG_INFO_I("simulator::progress")
        << "sim_time_s=" << sim_time_s
        << " mac_cycle_ms_mean=" << mac_mean_ms
        << " ue_cycle_ms_mean=" << ue_mean_ms
        << END();

    progress_steps = 0;
    progress_time_mac = std::chrono::duration<double>().zero();
    progress_time_ue = std::chrono::duration<double>().zero();

    while(next_progress_log_sim_time_s <= sim_time_s)
        next_progress_log_sim_time_s += progress_log_period_s;
}

void simulator::step(unsigned int _ts)
{
    double ts = (double)_ts * US2S;
    total_steps++;
    monitoring_manager &monitoring = monitoring_manager::instance();
    if(config_loader.get_period() > 0)
    {
        const std::int64_t init_ns = ticker.get_init_t()->count() * 1000LL;
        const std::int64_t slot_ts_ns = init_ns + static_cast<std::int64_t>(_ts) * 1000LL;
        monitoring.set_slot_timestamp_ns(slot_ts_ns);
    }
    else
    {
        monitoring.clear_slot_timestamp_ns();
    }

    std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
    mac_l.step(ts);
    const std::chrono::duration<double> mac_step_time =
        std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t);
    time_mac += mac_step_time;
    progress_time_mac += mac_step_time;

    t = std::chrono::steady_clock::now();
    ue_h.step(ts);
    const std::chrono::duration<double> ue_step_time =
        std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t);
    time_ue += ue_step_time;
    progress_time_ue += ue_step_time;
    progress_steps++;
    monitoring.clear_slot_timestamp_ns();
    handle_time_log(ts);
    maybe_log_progress(ts);
}
