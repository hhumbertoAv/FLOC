/*
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


#define PROC_NET_DEV_PATH_FORMAT "/proc/%d/net/dev"
#define NET_INT "  wlo1:"
//BYTES_RECEIVED and BYTES_TRANSMITTED corresponds to the fields where the data is located inside /proc/<pid>/net/dev
#define BYTES_RECEIVED 1
#define BYTES_TRANSMITTED 9

struct nic_power_values{
    double avg_cpu_power;
    double energy_cpy
}nic_shared;

struct program_times{
    double interval_ms;
    double total_time_s;
}Ptimes;

struct additional_arguments{
    int dynamicMode;
    char *fileToPrint;
}nic_additional_arguments;


//from the datasheet
 float max_download_power=0.55; //W
 float max_upload_power=1.029; //W

// float max_download_power=10.55; //W -> TO TEST
// float max_upload_power=10.029; //W


//from the datasheet
float max_download_rate=150500; //KBps
float max_upload_rate=152500; //KBps

long get_net_dev_bytes(int pid, const char* key, int net_key);
void calculate_nic_power(int pid, int interval_milliseconds, double total_time_seconds, int mode);
