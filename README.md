# FikoRE: 5g-network-emulator

Nokia Extended Reality Lab in Spain is focused on Immersive Media Technologies, with a special focus on what we denominated Distributed Reality (DR). The goal of (DR) is to be able to merge different local realities into a single real-time remote immersive experience. One key feature of DR is to allow the users to see his/her own hands and body within a fully immersive VR experience. However, this goal requires high-end hardware to run in real-time. Consequently, we are currently working on offloading architectures and implementations which allow us to run our demanding algorithms (AI/ML) on light VR/AR devices in real-time. 

5G technologies should be theoretically able to fulfil the network requirements of real-time task offloading. However, 5G technologies still have a long path ahead to reach their full potential. Consequently, we decided to implement our own 5G networks and beyond emulator to test and optimize our own solutions, determining which configuration (not only 5G, but also beyond) satisfy the requirements.  7

##  Goals

There are several emulators publicly available online, such as NS-3, SimuLTE or Simul5G. However, their complexity is extremely high, especially for non-telecommunications-specialists. We present FikoRE, our real-time 5G Radio Access Network (RAN) emulator carefully designed for application-level experimentation and prototyping. Its modularity and straightforward implementation allow multidisciplinary user to rapidly use or even modify it to test their own applications. Contrarily to the mentioned simulators/emulators, the goal of FikoRE is not mainly to understand and test the network, but study how the network and its different possible configurations behave for particular applications, use-cases and verticals.  

As we want to test our solutions in with actual networked applications we designed FikoRE to:  

* Work in Real-Time.
* Handle actual IP traffic efficiently. 
* Handle multiple emulated users with real or simulated traffic. 
* Model the real behavior of the network with sufficient accuracy. 

Besides, we need the emulator to be simple to use and, more importantly, easy to modify for possible particular needs. Consequently, FikoRE:  

* Has a high level of modularity to allow easy modifications.  
* Follows a straightforward implementation.  
* Is simple to use both as an emulator (real traffic) and simulator (simulated traffic).  

Furthermore, as the goal is to test actual applications on different possible scenarios and understand the most optimal network configurations, we understand is crucial to allow to easily modify the resource allocation algorithms and procedures. We believe it’s a crucial step in which an optimal algorithm design can produce an optimal behavior of the network for very specific applications. For this reason, we have designed the algorithm to allow the users to easily implement and test their own resource allocation algorithms.  

## Citing

If you want to use FikoRE in your research, don't forget to cite us!

```
@misc{GonzalezD2022,
  doi = {10.48550/ARXIV.2204.04290},
  url = {https://arxiv.org/abs/2204.04290},
  author = {Gonzalez Morin, Diego and Lopez Morales, Manuel-Jose and Pérez, Pablo and Villegas, Alvaro and García-Armada, Ana},
  title = {FikoRE: 5G and Beyond RAN Emulator for Application Level Experimentation and Prototyping},
  publisher = {arXiv},
  year = {2022},
}
```

## Description and Usage

For more info and usage check the [wiki](https://github.com/nokia/5g-network-emulator/wiki)
