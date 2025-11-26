/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

struct pdcp_queue_status
{
    int ip_buffer_size = 0;
    int ip_oldest_uid = -1;
    float ip_oldest_age = -1.0f;

    int harq_size = 0;
    int harq_oldest_id = -1;
    float harq_oldest_age = -1.0f;
    int harq_oldest_n_tx = -1;

    int capture_size = 0;
    int capture_oldest_uid = -1;
    float capture_oldest_age = -1.0f;

    int release_size = 0;
    int release_oldest_uid = -1;
    float release_oldest_age = -1.0f;
};
