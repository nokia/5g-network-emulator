/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <chrono>

//--------------------------------------------------------------------------------------------------
// rng_seed(): common seeding policy for the emulator's pseudo-random generators.
// With random_v: true the seed comes from the wall clock, so every run explores a
// different realization of the stochastic models. With random_v: false the seed is a
// function of the stream identifier only, which is what makes a run reproducible:
// same configuration, same output. Mobility already followed this rule
// (mobility_model_base seeds with time(NULL) * random_v + id); this header lets the
// PHY layers follow it too.
// Input:
//      stochastics: value of random_v for the UE.
//      stream_id: identifier of the generator, unique within the run.
//--------------------------------------------------------------------------------------------------
inline unsigned int rng_seed(bool stochastics, int stream_id)
{
    if (!stochastics)
        return static_cast<unsigned int>(stream_id);

    return static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

// Generators living below PDCP: one stream per UE, direction and role, so that two
// generators of the same UE never share a sequence.
#define PDCP_STREAM_PACKET_HANDLER 0
#define PDCP_STREAM_HARQ 1
#define PDCP_STREAM_L4S 2

inline unsigned int pdcp_rng_seed(bool stochastics, int ue_id, int tx_dir, int stream)
{
    return rng_seed(stochastics, 8 * ue_id + 2 * stream + tx_dir + 1);
}
