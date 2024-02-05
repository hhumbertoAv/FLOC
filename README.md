# <img src="https://github.com/hhumbertoAv/FLOC/assets/6061953/f79f8db7-b374-459b-90a8-fa1a5599e295" width="20"/> FLOC: Energy Measuring Tool for Linux

## Acknowledgments

<p align="center">  
<img src="https://github.com/hhumbertoAv/FLOC/assets/6061953/35039ac7-ea45-4c1b-9bc6-8a1f261a2bb0" width="350"/>
</p>

FLOC has been made possible thanks to Technopôle Domolandes[^1], an organization deeply committed to innovation and the development of the Landes region in France and beyond. Their continuous efforts in fostering technological progress and regional growth have been instrumental in bringing this project to life.

## Description
FLOC is a comprehensive and highly versatile tool, developed by the R&D laboratory of Têchnopole Domolandes[^1], designed to measure the energy consumption of applications in GNU/Linux environments. The core philosophy of FLOC is to enable users and developers to track energy usage across various software components using a single tool, leveraging existing libraries like RAPL[^2] or power equations. This first version comprises four applications, each dedicated to finding the equations-based power and energy consumption of four hardware components: CPU, RAM, hard drives, and network cards.


## Features
<p align="justify">
For each component, FLOC calculates the energy consumption based on the load generated by running processes identified by their PID. It uses load and energy consumption data provided by the GNU/Linux kernel interfaces or, in their absence, from a database of technical specifications (datasheets) which can be either predefined or customized. 
</p>

### Limitations

It is important to note that, as with any software tool that measures hardware energy consumption, there is a margin of variability. The values provided by FLOC are approximations and should not be considered absolutely precise.



### Mathematical Formulas and Customization
FLOC includes a set of pre-defined mathematical formulas tailored for the four devices. These formulas have been proven[^3] to reflect the proportion of energy consumption by a process in relation to the total computer usage. Developers and contributors have the flexibility to define and maintain their own formulas, making FLOC a highly adaptable tool for various usage scenarios.


## Technical Specifications

As we mentioned, this initial release of `floc` features four distinct applications, targeting the measurement of power and energy consumption across CPUs, RAM, storage devices, and network interface cards. Each application is in its own folder, where you'll find both the source code and configuration files. This setup not only facilitates easy exploration and customization but also facilitates understanding. While `floc` provides a unified interface to execute all tools collectively, users are still able to delve into each application's specific folder to explore its code, access comprehensive documentation, and make any desired modifications.


### General System Requirements
FLOC requires an Arch-based or Debian-based GNU/Linux distribution with the Linux kernel version 5.12 or higher. Please, refer to each application folder to explore applications' specific requirements. 

### General Inputs
<p align="justify">
FLOC operates with the following primary inputs:

1. **Process ID (PID):** The identifier for the processes under analysis.
2. **Total Analysis Time:** Sets the duration for energy and power value calculations.
3. **Measurement Interval:** Determines the interval at which FLOC takes load and power measurements, impacting the allocated CPU time for the tool.

More detailed specification of these parameters is specified in each application’s folder
</p>

### Outputs
<p align="justify">
The outputs of each application of FLOC after analyzing a process are as follows:

- **Execution Time:** Duration of the monitored process execution.
- **Average Power (in Watts):** The mean power consumption over the set analysis period.
- **Energy Consumption (in Joules):** Total energy used during the total analysis time.
</p>

### Compilation and Execution

To compile and set up FLOC, follow these steps please:

1. **Compilation:**
   - Open your terminal.
   - Navigate to the root directory of the FLOC project.
   - Execute the command `make` to compile all components and prepare the `floc` executable.

2. **Running FLOC:**
   - To use floc and specify which application to execute, run:
     ```
     ./floc --cpu -p [PID] -i [interval] -t [duration]
     ./floc --sd -p [PID] -i [interval] -t [duration]
     ./floc --ram -p [PID] -t [interval]
     ./floc --nic -p [PID] -t [interval]
     ```
   - Replace `[PID]`, `[interval]`, and `[duration]` with your desired values.



## References
[^1]: [Têchnopole Domolandes](https://www.domolandes.fr
[^2]: Running Average Power Limit (RAPL): [Intel® 64 and IA-32 Architectures Software Developer's Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html), specifically in the section detailing power and thermal management.
[^3]: Humberto, A.V.H.: An energy-saving perspective for distributed environments:
Deployment, scheduling and simulation with multidimensional entities for Software and Hardware. Ph.D. thesis, UPPA, https://www.theses.fr/s342134 (2022)
