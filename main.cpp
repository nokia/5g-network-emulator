/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <simulator/simulator.h>

int main()
{
    simulator sim("./");
    sim.start(); 
    sim.join(); 
    sim.print_traffic();
}

