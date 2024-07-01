# Network Interface Controller Energy Performance Tool (`ePerfNIC`) Installation and Usage

## System Requirements

Before using `ePerfNIC`, ensure you have `nethogs` installed on your system, version greater than `0.8.7`:

- **For Arch-based distributions:**
  - Install `nethogs` by running as root:
    - `sudo pacman -S nethogs`

- **For Debian-based distributions:**
  - First, update your package list as root:
    - `# apt-get update`
  - Install as root `nethogs` (ensure you get a version > 0.8.7, you might need to check if the latest version is available in your distribution's repository or consider installing from source if the version is too old):
    - `# apt-get install nethogs`
    
## Installation

To install the `ePerfNIC` application on your computer, follow these steps:

1. **Compile the Source Code:**
   - Open your terminal.
   - Navigate to the directory containing the `ePerfNIC` source code.
   - Execute the command `make` to compile the application.

2. **Uninstallation:**
   - To uninstall, run `make clean` in the same directory. This will remove compiled binaries and clean up the directory.

## Usage

To run the `ePerfNIC` program, you need to execute it as root and specify certain parameters:

- **Execute as Root:**
  - Run the program using `sudo ./ePerfNIC.out` with the necessary options.

- **Options:**
  1. `-p`: Specify the Process ID (PID) of the process you want to analyze. Alternatively, you can use `-n` to specify the name of the application to measure.
  2. `-t`: Define the total duration of the analysis period.
  3. `-i`: Set the measurement interval (i.e., how frequently the application will measure network performance).
  4. `-d`: Enable Dynamic Mode, allowing the tool to evaluate network applications that can be closed and reopened during the analysis.
  5. `-f`: Export the measurement results to a specified CSV file.
  

  Example Command:

  `# sudo ./ePerfNIC.out -p [PID or App Name] -i [interval in seconds] -t [total time in seconds] --d --f [file.csv]`


## Important Considerations

- The energy measurement is executed continuously until the specified process (PID) terminates. There is no need to set a total analysis time.
- If the script returns unexpected results or an error, ensure that the PID exists and the measurement interval is correctly specified.
