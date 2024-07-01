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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include "ePerfSD.h"


////////////////////////////////////////////////////////////////////////////////////
//       Main :)
///////////////////////////////////////////////////////////////////////////////////


int main(int argc, char *argv[]) {
    
    int opt;
    int pid = 0;
    int interval_milliseconds = -1;
    int total_time_seconds = -1;
    char *processName = NULL;
    sd_additional_arguments.dynamicMode = 0;
    sd_additional_arguments.fileToPrint = NULL;

    static struct option long_options[] = {
        {"no-verbose", no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };

    int long_index = 0;
    while ((opt = getopt_long(argc, argv, "p:i:t:n:d:f:", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'p':
            pid = atoi(optarg);
            break;
        case 'i':
            interval_milliseconds = atoi(optarg);
            break;
        case 't':
            total_time_seconds = atoi(optarg);
            break;
        case 'n':
            processName = optarg;
            break;
        case 'd':
            if (strcmp(optarg, "true") == 0) {
                sd_additional_arguments.dynamicMode = 1;
                }
            break;
        case 'f':
            sd_additional_arguments.fileToPrint = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [-p PID | -n processName] -i interval_milliseconds -t total_time_seconds [-d true|false] [-f filename] [--no-verbose]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (pid != 0 && processName != NULL) {
        fprintf(stderr, "Usage: %s [-p PID | -n processName] -i interval_milliseconds -t total_time_seconds [-d true|false] [-f filename]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (sd_additional_arguments.fileToPrint != NULL) {
        //We open the file in append mode (we write at the end of the file)
        //If it doesnt exist, it is created
        fileOutput = fopen(sd_additional_arguments.fileToPrint, "at+");
        if (fileOutput == NULL) {
            //If, for whatever reason, the file cannot be opnened, we discard the path and print through terminal
            perror("File to print out couldnt be opened, changing to terminal output\n");
            sd_additional_arguments.fileToPrint == NULL;
        }
    }  


    create_shared_object();


    if (processName != NULL) //if n is specified, it starts threads
    {
        sem_init(&mutex, 0, 1);
        Ptimes.interval_ms = interval_milliseconds;
        Ptimes.total_time_s = total_time_seconds;
        //Threads go here in the following function.
        calculate_sd_power_name(processName,interval_milliseconds,total_time_seconds);
    }
    else 
    {
        //Only calculates by PID
        calculate_sd_power(pid,interval_milliseconds,total_time_seconds, 0);
    }

    if (sd_additional_arguments.fileToPrint != NULL) {
        fclose(fileOutput);
    }


    print_shared_object(pid,total_time_seconds);
    //close_shared_object(&file_descriptor);

    //Set the power values to 0 after the total time is finished...I did This for plotting coherence
    write_to_shared_object(0.0,0.0,0.0);

    return EXIT_SUCCESS;
}
