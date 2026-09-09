/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <cmath>
#include <fstream>

#include <nlohmann/json.hpp>
#include <utils/control/transport_file.h>
#include <utils/terminal_logging.h>

using json = nlohmann::json;

namespace
{
command_op parse_op(const std::string &s)
{
    if (s == "get") return command_op::get;
    if (s == "describe") return command_op::describe;
    if (s == "ping") return command_op::ping;
    return command_op::set;
}

// A line carries either one command or a list of them under "cmds", all sharing the same
// instant and correlation id.
void parse_line(const json &line, std::uint64_t line_no, std::vector<command> &out)
{
    std::int64_t at_tti = -1;
    if (line.contains("at_tti")) at_tti = line["at_tti"].get<std::int64_t>();

    double at_t = -1.0;
    if (line.contains("at_t"))
    {
        at_t = line["at_t"].get<double>();
        // at_tti wins when both are present: it is the canonical instant, the one that
        // replays a journal on the exact same step.
        if (at_tti < 0) at_tti = (std::int64_t)llround(at_t * 1000.0);
    }

    const std::uint64_t id = line.contains("id") ? line["id"].get<std::uint64_t>() : line_no;

    std::vector<json> items;
    if (line.contains("cmds"))
    {
        for (const auto &c : line["cmds"]) items.push_back(c);
    }
    else
    {
        items.push_back(line);
    }

    for (const json &item : items)
    {
        command c;
        c.id = id;
        c.at_t = at_t;
        c.at_tti = at_tti;
        c.op = parse_op(item.contains("op") ? item["op"].get<std::string>() : std::string("set"));
        if (item.contains("target")) c.target = item["target"].get<std::string>();

        if (item.contains("set"))
        {
            for (auto it = item["set"].begin(); it != item["set"].end(); ++it)
            {
                if (it.value().is_boolean()) c.sets.push_back(std::make_pair(it.key(), param_value(it.value().get<bool>())));
                else if (it.value().is_string()) c.sets.push_back(std::make_pair(it.key(), param_value(it.value().get<std::string>())));
                else c.sets.push_back(std::make_pair(it.key(), param_value(it.value().get<double>())));
            }
        }
        out.push_back(c);
    }
}
}

transport_file::transport_file(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_ERROR_I("transport_file") << " cannot open timeline file: " << path << END();
        return;
    }

    std::string text;
    std::uint64_t line_no = 0;
    while (std::getline(file, text))
    {
        line_no++;
        if (text.empty() || text[0] == '#') continue;

        try
        {
            parse_line(json::parse(text), line_no, pending_);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR_I("transport_file") << " line " << line_no << " ignored: " << e.what() << END();
        }
    }

    ok_ = true;
    LOG_INFO_I("transport_file") << " loaded " << pending_.size() << " commands from " << path << END();
}

bool transport_file::poll(std::vector<command> &out)
{
    if (delivered_) return false;
    delivered_ = true;
    out.insert(out.end(), pending_.begin(), pending_.end());
    pending_.clear();
    return false;   // nothing else will ever arrive from a file
}

void transport_file::reply(const ack &a)
{
    if (a.ok) return;
    for (size_t i = 0; i < a.errors.size(); i++)
    {
        LOG_ERROR_I("transport_file") << " command " << a.id << " rejected: "
                                      << a.errors[i].key << ": " << a.errors[i].reason << END();
    }
}
