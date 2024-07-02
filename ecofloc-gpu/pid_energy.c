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
 

#include"pid_energy.h"

double gpu_power() 
{
    char buffer[128];
    float power_draw = 0.0;
    FILE *fp;

    fp = popen("nvidia-smi --query-gpu=power.draw --format=csv,noheader,nounits", "r");
    if (fp == NULL) 
    {
        fprintf(stderr, "Failed to run command\n");
        exit(1);
    }

    if (fgets(buffer, sizeof(buffer) - 1, fp) != NULL) {
        sscanf(buffer, "%f", &power_draw);
    }

    pclose(fp);
    return power_draw;
}

int gpu_usage(int pid)
{
    char command[] = "nvidia-smi pmon -s um -c 1";
    char line[1024];
    int usage = -1;  
    FILE *fp;

    fp = popen(command, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to run nvidia-smi\n");
        exit(1);
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        int current_pid;
        char sm_usage_str[10];  
        char type[10];          

        if (sscanf(line, "%*s %*d %d %s %s", &current_pid, type, sm_usage_str) == 3)
        {
            if (current_pid == pid && strcmp(type, "G") == 0) // G -> GPU
            {
                if (strcmp(sm_usage_str, "-") != 0)  // IF NOT "-"
                    usage = atoi(sm_usage_str);  // Convert valid usage string to int
                else 
                    usage = 0;
                break;
            }
        }
    }
    pclose(fp);
    return usage;
}

double pid_energy(int pid, double interval_ms, double timeout_s)
{
    double total_energy = 0.0;
    double elapsed_time = 0.0;
    double interval_s = interval_ms / 1000.0; // Convert ms to seconds

    // Setup nanosleep interval
    struct timespec interval_time;
    interval_time.tv_sec = (time_t)interval_s;
    interval_time.tv_nsec = (long)((interval_s - interval_time.tv_sec) * 1e9); // Remaining part in nanoseconds

    while (elapsed_time < timeout_s)
    {
        float initial_power = gpu_power();
        int initial_usage = gpu_usage(pid);
        float initial_process_power = (initial_usage / 100.0) * initial_power;

        nanosleep(&interval_time, NULL); // Sleep for interval_time

        float final_power = gpu_power();
        int final_usage = gpu_usage(pid);
        float final_process_power = (final_usage / 100.0) * final_power;

        // Calculate average power over the interval
        float average_power = (initial_process_power + final_process_power) / 2.0;

        // Energy = Power * Time (in seconds)
        double energy = average_power * interval_s;
        total_energy += energy;

        elapsed_time += interval_s;
    }

    return total_energy; // Total energy in Joules
}