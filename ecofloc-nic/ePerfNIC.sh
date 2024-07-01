<<lic
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
*/
lic

#!/bin/bash

################################################################################
# Script Name: ePerfNIC.sh
# Description: This script calculates a PID's NIC power consumption.
# From variables: PID transfer rate and NIC MAX transfer rate
# Usage: ePerfNIC.sh -p PID -t windowTime milliseconds
# Note: A safe windowTime is 100 msecs, otherwise empty values may exist
    #WARN: Lower values make the script calculate low initial values
# Important REFs and values for the NIC: 
  # source to intel confidential (?) sheet specs: 
  #https://www.tonymacx86.com/attachments/cnvi-and-9560ngw-documentation-pdf.342854/
  #https://fccid.io/B94-9560D2WZ/User-Manual/Users-Manual-3800018.pdf

  #Power TpT – 11n HB-40 Rx 11n (at max TpT) 550 mW
  #TpT – 11ac HB-80 Tx 11ac (at max TpT) 1029 mW

  #11ac 160 MHz 2SS Rx Conductive, best attenuation, TCP/IP 1204 Mbps - 150500 KBps
  #11ac 160 MHz 2SS TX Conductive, best attenuation, TCP/IP 1220 Mbps - 152500 KBps
# NIC_INFO: the intel 6 AX201 has Capabilities: [c8] Power Management version 3 (lspci -v)
#...we can custom it depending on the kernel driver for further optimizations
################################################################################

#TODO -> insert the interval logic


pid=0
total_time_seconds=0
interval_milliseconds=0  # Initialize the interval_milliseconds variable

nic_energy=0
nic_power_AVG=0

getInput()
{  
  while getopts "t:p:i:" opt; do
    case ${opt} in
      t )
        total_time_seconds=$OPTARG 
        ;;
      p )
        pid=$OPTARG
        ;;
      i )  # Handling the new -i option for interval in milliseconds
        interval_milliseconds=$OPTARG
        ;;
      \? )
        echo "Invalid option: -$OPTARG" >&2
        exit 1
        ;;
      : )
        echo "Option -$OPTARG requires an argument." >&2
        exit 1
        ;;
    esac
  done
}


verifyInput()
{

  #if [ $total_time_seconds -lt 100 ]; then    -> this for the interval 
  #  echo "I need a bigger windowTime :(..."
  #   exit
  # fi
  if [ ! -e "/proc/$1/stat" ]; then #If the process does not exist -> exit
    echo "Non-existent PID"
    exit
  fi

}


verifySystemRequirements()
{

    #########################
    ######NETHOGS############
    ######################### 


    required_version="0.8.7"

    # Current version of netgohfs -> command: nethogs -V
    current_version=$(nethogs -V 2>&1 | grep -oP 'version \K(\d+\.\d+\.\d+)')
    if [ $? -ne 0 ]; then
        echo "nethogs is not installed or the version could not be determined."
        exit 1
    fi

    # Verify the expected 
    if [ "$(printf '%s\n' "$required_version" "$current_version" | sort -V | head -n1)" != "$required_version" ]; then
        echo "Your netgohfs version ($current_version) is lower than the required version ($required_version)."
        exit 1
    fi


}


getNICCons()
{

count=0
uplCount=0
dlCount=0 


#nethogs accepts seconds
interval_seconds=$(echo "$interval_milliseconds / 1000" | bc -l)

while IFS=' ' read -r upload download; do

    uplCount=$(echo "$uplCount + $upload" | bc) 
    dlCount=$(echo "$dlCount + $download" | bc)
    count=$(echo "$count + 1" | bc) 


# the last to words of nethogs output are upl and downl rates. STDBUFF is for disable buffering in std: input, output and error
done < <(timeout $total_time_seconds nethogs wlo1 -t -P $pid -v 0 -d $interval_seconds | \
      stdbuf -i0 -o0 -e0 grep $pid | stdbuf -oL awk '{print $(NF-1),$NF}')


#This ensure that the power and energy values are updated (0 above) if the function enters the while at least 1 time
if [ "$count" -ne 0 ]; then


    upload_rate_avg=$(echo "scale=10; $uplCount / $count" | bc)
    download_rate_avg=$(echo "scale=10; $dlCount / $count" | bc)

    #from the datasheet (look above)
    max_download_power=0.55 #W
    max_upload_power=1.029 #W

    #from the datasheet (look above)
    max_download_rate=150500 #KBps
    max_upload_rate=152500 #KBps

    upload_power=$(echo "scale=10; $max_upload_power*($upload_rate_avg / $max_upload_rate)" | bc)
    download_power=$(echo "scale=10; $max_download_power*($download_rate_avg / $max_download_rate)" | bc)

    nic_power_AVG=$(echo "scale=10; $upload_power + $download_power" | bc)
    nic_energy=$(echo "scale=10; $nic_power_AVG*$total_time_seconds*0.001" | bc)

fi

}

verifyPrintOutput()
{
    #verify if it's a non empty numerical value  
  if [[ ! -z $nic_energy ]] && \
     [[ $nic_energy =~ ^[0-9]*([.][0-9]+)?$ ]] && \
     [[ ! -z $nic_power_AVG ]] && \
     [[ $nic_power_AVG =~ ^[0-9]*([.][0-9]+)?$ ]]; then

        echo "PID : $pid" 
        echo "NIC_MEASURE_DURATION (S) : $total_time_seconds"
        echo "AVG_NIC_POWER (W) : $nic_power_AVG"
        echo "ENERGY_NIC (J) : $nic_energy" 
  else
        echo error somewhere
    fi
}

main()
{
  verifySystemRequirements
  getInput "$@"
  verifyInput
  getNICCons 2> /dev/null
  verifyPrintOutput
}

main "$@"
