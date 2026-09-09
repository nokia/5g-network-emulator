/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#pragma once

#include <vector>

#include <utils/control/command.h>

//--------------------------------------------------------------------------------------------------
// control_transport(): how commands reach the emulator. The core knows nothing about
// files or sockets; it only drains whatever the transport has ready at the start of a
// TTI and answers through reply().
//
// poll() is called from the simulation thread at the quiescent point, never concurrently
// with itself. A transport that owns a thread must do its own locking internally.
//--------------------------------------------------------------------------------------------------
class control_transport
{
public:
    virtual ~control_transport() {}

    // Moves everything ready into out. Returns false when the transport is done and will
    // never produce anything again, which lets the manager stop asking.
    virtual bool poll(std::vector<command> &out) = 0;

    virtual void reply(const ack &a) = 0;

    virtual void stop() {}
};
