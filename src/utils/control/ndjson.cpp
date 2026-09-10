/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <cmath>

#include <nlohmann/json.hpp>
#include <utils/control/ndjson.h>

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
}

namespace ndjson
{
bool parse_line(const std::string &line, std::uint64_t fallback_id,
                std::vector<command> &out, std::string &error)
{
    json msg;
    try
    {
        msg = json::parse(line);
    }
    catch (const std::exception &e)
    {
        error = e.what();
        return false;
    }

    if (!msg.is_object())
    {
        error = "expected a JSON object";
        return false;
    }

    std::int64_t at_tti = -1;
    double at_t = -1.0;
    try
    {
        if (msg.contains("at_tti")) at_tti = msg["at_tti"].get<std::int64_t>();
        if (msg.contains("at_t"))
        {
            at_t = msg["at_t"].get<double>();
            // at_tti wins when both are present: it is the instant that replays a journal
            // on the exact same step, which a double cannot guarantee.
            if (at_tti < 0) at_tti = (std::int64_t)llround(at_t * 1000.0);
        }

        const std::uint64_t id = msg.contains("id") ? msg["id"].get<std::uint64_t>() : fallback_id;

        std::vector<json> items;
        if (msg.contains("cmds"))
        {
            if (!msg["cmds"].is_array())
            {
                error = "cmds must be an array";
                return false;
            }
            for (const auto &c : msg["cmds"]) items.push_back(c);
        }
        else
        {
            items.push_back(msg);
        }

        for (size_t k = 0; k < items.size(); k++)
        {
            const json &item = items[k];
            command c;
            c.id = id;
            c.at_t = at_t;
            c.at_tti = at_tti;
            c.op = parse_op(item.contains("op") ? item["op"].get<std::string>() : std::string("set"));
            if (item.contains("target")) c.target = item["target"].get<std::string>();

            if (item.contains("set"))
            {
                if (!item["set"].is_object())
                {
                    error = "set must be an object";
                    return false;
                }
                for (auto it = item["set"].begin(); it != item["set"].end(); ++it)
                {
                    if (it.value().is_boolean())
                        c.sets.push_back(std::make_pair(it.key(), param_value(it.value().get<bool>())));
                    else if (it.value().is_string())
                        c.sets.push_back(std::make_pair(it.key(), param_value(it.value().get<std::string>())));
                    else if (it.value().is_number())
                        c.sets.push_back(std::make_pair(it.key(), param_value(it.value().get<double>())));
                    else
                    {
                        error = "unsupported value type for key " + it.key();
                        return false;
                    }
                }
            }

            if (c.op == command_op::set && c.sets.empty())
            {
                error = "set command without values";
                return false;
            }
            if (c.op != command_op::describe && c.target.empty())
            {
                error = "missing target";
                return false;
            }

            out.push_back(c);
        }
    }
    catch (const std::exception &e)
    {
        error = e.what();
        return false;
    }

    return true;
}

std::string serialize_ack(const ack &a)
{
    json j;
    j["id"] = a.id;
    j["status"] = a.ok ? "ok" : "error";
    j["tti"] = a.tti;
    j["t"] = a.t;

    if (!a.errors.empty())
    {
        json errs = json::array();
        for (size_t i = 0; i < a.errors.size(); i++)
        {
            json e;
            e["key"] = a.errors[i].key;
            e["reason"] = a.errors[i].reason;
            errs.push_back(e);
        }
        j["errors"] = errs;
    }

    if (!a.payload.empty())
    {
        try
        {
            j["result"] = json::parse(a.payload);
        }
        catch (const std::exception &)
        {
            j["result"] = a.payload;
        }
    }

    return j.dump() + "\n";
}
}
