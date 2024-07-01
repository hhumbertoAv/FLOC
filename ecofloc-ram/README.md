# RAM Energy Performance Tool (`ePerfRAM`) Installation and Usage

## System Requirements

Before using `ePerfRAM`, make sure to have `iconv` and `perf` installed on your system:

- **For Arch-based distributions:**
  - Install as root `iconv` (usually included with `glibc`) and `perf` by running:
    - `# pacman -Syu glibc`
    - `# pacman -S linux-tools` (or `linux-zen-tools` for the Zen kernel, adjust accordingly for your kernel)

- **For Debian-based distributions:**
  - `iconv` should be pre-installed with the `libc-bin` package. Install `perf` by running:
    - `# apt-get update`
    - `# apt-get install linux-tools-common linux-tools-$(uname -r)`

## Installation

To install the `ePerfRAM` application on your computer, follow these steps:

1. **Compile the Source Code:**
   - Open your terminal.
   - Navigate to the directory containing the `ramEnergyPerf` source code.
   - Execute the command `make` to compile the application.

2. **Uninstallation:**
   - To uninstall, run `make clean` in the same directory. This will remove compiled binaries and clean up the directory.

## Usage

To run the `ramEnergyPerf` program, you need to execute it as root and specify certain parameters:

- **Execute as Root:**
  - Run the program using `sudo ./ePerfRAM.out` with the necessary options.

- **Options:**
  1. `-p`: Specify the Process ID (PID) of the process you want to analyze. Alternatively, you can use `-n` to specify the name of the application to measure.
  2. `-t`: Define the total duration of the analysis period.
  3. `-i`: Set the measurement interval (i.e., how frequently the application will measure power consumption).
  4. `-d`: Enable Dynamic Mode, allowing the tool to evaluate applications that can be closed and reopened during the analysis.
  5. `-f`: Export the measurement results to a specified CSV file.
  

  Example Command:

  `# sudo ./ePerfRAM.out -p [PID or App Name] -i [interval in seconds] -t [total time in seconds] --d --f [file.csv]`

## Important Considerations

- The energy measurement is executed continuously until the specified process (PID) terminates. There is no need to set a total analysis time.
- If the script returns unexpected results or an error, ensure that the PID exists and the measurement interval is correctly specified.
