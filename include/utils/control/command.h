/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// A parameter value as it arrives over the wire. The registry decides what a given knob
// accepts and converts; nothing below this point guesses.
using param_value = std::variant<double, bool, std::string>;

enum class command_op
{
    set,
    get,
    describe,
    ping
};

//--------------------------------------------------------------------------------------------------
// command(): one unit of work for the control plane, already parsed and validated for
// shape but not yet for content.
//
// target: "ue/<id>", "ue/*" or "cell".
// at_tti: TTI at which it must be applied. Negative means "as soon as it is drained".
//         Canonical identifier for replay: in fast mode the TTI is exactly the ticker
//         counter, which at_t (a double) cannot guarantee.
// id:     correlation id chosen by the client, echoed back in the ack.
//--------------------------------------------------------------------------------------------------
struct command
{
    command_op op = command_op::set;
    std::string target;
    std::vector<std::pair<std::string, param_value>> sets;
    double at_t = -1.0;
    std::int64_t at_tti = -1;
    std::uint64_t id = 0;
};

struct ack_error
{
    std::string key;
    std::string reason;
};

struct ack
{
    std::uint64_t id = 0;
    bool ok = true;
    std::int64_t tti = 0;
    double t = 0.0;
    std::vector<ack_error> errors;
    // Free-form payload for get/describe, already serialized as JSON by the manager.
    std::string payload;
};
