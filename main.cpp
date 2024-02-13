/**********************************************
* Copyright 2022 Nokia
* Licensed under the BSD 3-Clause Clear License
* SPDX-License-Identifier: BSD-3-Clause-Clear
**********************************************/

#include <simulator/simulator.h>

int main(int argc, char** argv)
{
    std::string config_file;
    if (argc > 1) {
        config_file = argv[1];
        std::cout << "Config file: " << config_file << "\n";
    } else {
        config_file = "./config.ini";
        std::cout << "Config file by default: " << config_file << "\n";
    }
    //simulator sim("./");
    simulator sim(config_file);
    sim.start(); 
    sim.join(); 
    sim.print_traffic();
}

