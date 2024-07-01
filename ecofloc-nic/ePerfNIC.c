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
#include <signal.h>
//For threads

#include <getopt.h>
#include <semaphore.h>
#include <pthread.h>  

#include "ePerfNIC.h"
#include "../find_by_name.h"

// For sharing-memory objects

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define SHARED_OBJ_NAME "/NIC_shared_memory"
#define SHARED_OBJ_SIZE 4096 // 4KB


int file_descriptor;  // The id of the shared object for IPC. See create_shared_object()
void* ptr = NULL; // Pointer to the mapped shared object memory region

sem_t mutex; 
FILE* fileOutput;



// Auxiliar function to convert a String in an array of String that is passed by reference
/*
void str2array (char * line, int length, char str2array [16][256])
{
	char delim[] = " ";
	char *ptr = strtok(line, delim);
	int i = 0;

    // We tokenize the string line and we store every token in a position of the array
	while(ptr != NULL && i != length)
	{
		strcpy(str2array[i], ptr);
		ptr = strtok(NULL, delim);
		i++;
	}		
}*/


////////////////////////////////////////////////////////////////////////////////////
///////////////////SHARED MEMORY FOR IPC//////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//This creates an in-memory shared object for IPC storing consumption results
void create_shared_object() {
    file_descriptor = shm_open(SHARED_OBJ_NAME, O_CREAT | O_RDWR, 0666);
    if (file_descriptor == -1) {
        perror("Shared object creation failed");
        return;
    }
    if (ftruncate(file_descriptor, SHARED_OBJ_SIZE) == -1) {
        perror("Setting size failed");
        close(file_descriptor);
        return;
    }


    ptr = mmap(0, SHARED_OBJ_SIZE, PROT_WRITE, MAP_SHARED, file_descriptor, 0);
    if (ptr == MAP_FAILED) {
        perror("Mapping failed");
        close(file_descriptor);
        ptr = NULL;
        return;
    }
    sprintf(ptr, "current_power=%f\ntotal_power=%f\ncounter=%lld", 0.0, 0.0, 0);



}

void write_to_shared_object(double current_power, double total_power, long long counter) 
{
    //printf("current_power=%f\ntotal_power=%f\ncounter=%lld", current_power, total_power, counter);
    ptr = mmap(0, SHARED_OBJ_SIZE, PROT_WRITE, MAP_SHARED, file_descriptor, 0);
    if (ptr == MAP_FAILED) {
        perror("Mapping failed");
        close(file_descriptor);
        ptr = NULL;
        return;
    }
    sprintf(ptr, "current_power=%f\ntotal_power=%f\ncounter=%lld", current_power, total_power, counter);
}


double get_shared_total_power() 
{
    double total_power=0;
    double current_power=0;
    long long counter = 0;  
    // Map the shared memory object
    void *ptr = mmap(0, SHARED_OBJ_SIZE, PROT_READ, MAP_SHARED, file_descriptor, 0);
    if (ptr == MAP_FAILED) {
        perror("Mapping failed");
        close(file_descriptor);
        return 0.0;
    }

    // //TODO -> SKIP the counter and current_power values.
    sscanf(ptr, "current_power=%lf\ntotal_power=%lf\ncounter=%lld", &current_power, &total_power, &counter);
    return total_power;

}


void print_shared_object(int pid, int total_time_seconds) //TODO -> Fix this ugly style of passing these by parameter
{

    // Map the shared memory object
    void *ptr = mmap(0, SHARED_OBJ_SIZE, PROT_READ, MAP_SHARED, file_descriptor, 0);
    if (ptr == MAP_FAILED) {
        perror("Mapping failed");
        close(file_descriptor);
        return;
    }

    double current_power = 0;
    double total_power = 0;
    long long counter = 0;

    // Parse the values from the shared memory
    sscanf(ptr, "current_power=%lf\ntotal_power=%lf\ncounter=%lld", &current_power, &total_power, &counter);

    // Calculate average power and total energy
    double avg_nic_power = total_power / counter;
    double nic_energy = total_power;

    // Print the values in the desired format

    printf("PID: %d\nNIC_MEASURE_DURATION (S): %d\nAVG_NIC_POWER (W): %f\nENERGY_NIC (J): %f\n",
           pid, total_time_seconds, avg_nic_power, nic_energy);

}



void close_shared_object() {
    if (ptr != NULL && munmap(ptr, SHARED_OBJ_SIZE) == -1) {
        perror("Unmapping failed");
    }
    if (close(file_descriptor) == -1) {
        perror("Close failed");
    }
    ptr = NULL; // Reset the pointer after unmapping
}




////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

// TODO -> USING NETHOGS HERE.....Need to redo this after (the BKP folder, but using the correct file)



/****************************************************************************
*   Function   : calculate_nic_power
*   Description: This function calculate the power consumed by the given 
                 PID. The values calculated are stored in a shared memory. 
*   Parameters : pid - PID to the proccess to analyze.
                 interval_milliseconds - interval between calculations.
                 total_time_seconds - time specified to do the calculations.
                 mode - variable to know if the function is called with
                 the -n or -p argument. 
*   Returned   : None.
****************************************************************************/
void calculate_nic_power(int pid, int interval_milliseconds, double total_time_seconds, int mode) {
    time_t start, end;
    start = time(NULL);

    long long counter = 0;
    double total_nic_power = 0;
    char command[512];
    char output[1024];

    while (total_time_seconds < 0 || difftime(end, start) < total_time_seconds) {
        // check if the process still exists
        if (kill(pid, 0) == -1) {
            if (nic_additional_arguments.dynamicMode == 1) {
                if (mode == 1) {
                    break;
                } else {
                    end = time(NULL);
                    continue;
                }
            } else {
                printf("Process %d was killed\n", pid);
                break;
            }
        }

        // Construct the command to run nethogs
        sprintf(command, "timeout %d stdbuf -oL -eL nethogs wlo1 -t -P %d -v 0 -d 1 | awk '/%d/{print $(NF-1),$NF}'", 
                interval_milliseconds / 1000, pid, pid);

        //printf("%s\n", command);

        // Run nethogs
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            if (!nic_additional_arguments.dynamicMode == 1) {
                perror("Failed to run command");
                exit(1);
            }
        }

        while (fgets(output, sizeof(output) - 1, fp) != NULL) {
            double upload_rate, download_rate;
            if (sscanf(output, "%lf %lf", &download_rate, &upload_rate) != 2) {
                fprintf(stderr, "Failed to parse output: %s", output);
                continue;
            }

            double upload_power = max_upload_power * (upload_rate / max_upload_rate);
            double download_power = max_download_power * (download_rate / max_download_rate);
            double nic_power = upload_power + download_power;
            counter++;

            if (mode == 1) {
                sem_wait(&mutex);
                total_nic_power = get_shared_total_power();
                total_nic_power += nic_power;
                write_to_shared_object(nic_power, total_nic_power, counter);

                if (nic_additional_arguments.fileToPrint != NULL) {
                    fprintf(fileOutput, "%f,%f\n", nic_power, total_nic_power);
                }
                sem_post(&mutex);
            } else {
                total_nic_power += nic_power;
                write_to_shared_object(nic_power, total_nic_power, counter);
                if (nic_additional_arguments.fileToPrint != NULL) {
                    fprintf(fileOutput, "%f,%f\n", nic_power, total_nic_power);
                }
            }
        }

        // Close the pipe
        pclose(fp);

        // Sleep for the specified interval
        struct timespec interval = {0, interval_milliseconds * 1000000};
        nanosleep(&interval, NULL);
        end = time(NULL);
    }

    
    // if (counter > 0) 
    // {
    //     double avg_nic_power = total_nic_power / counter;
    //     double nic_energy = avg_nic_power * (difftime(end, start));
    //     if (mode == 0) {
    //         printf("PID: %d\nNIC_MEASURE_DURATION (S): %f\nAVG_NIC_POWER (W): %f\nENERGY_NIC (J): %f\n",
    //            pid, difftime(end, start), avg_nic_power, nic_energy);
    //     }else {
    //         sem_wait(&mutex);
    //         nic_shared.avg_cpu_power += avg_nic_power;
    //         nic_shared.energy_cpy += nic_energy;
    //         sem_post(&mutex);
    //     }

    // }
}







////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////THREADS MANAGEMENT///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////





/****************************************************************************
*   Function   : thread_nic_name
*   Description: This is a thread function that calls to calculate_ram_power. 
*   Parameters : vargp - a pointer to the PID of the proccess to calculate. 
*   Returned   : None.
****************************************************************************/
void *thread_nic_name(void *vargp) 
{
    int pid = (int) vargp;

   calculate_nic_power(pid, Ptimes.interval_ms, Ptimes.total_time_s, 1);
}

/****************************************************************************
*   Function   : calculate_nic_power_name
*   Description: Function that, given a name, gets all PIDs associatted to it 
                 and launches threads, each one with a different PID, to 
                 calculate its power consumption. 
*   Parameters : name - name of the proccess
                 interval - interval between calculation
                 timeout - total time of calculation. 
*   Returned   : None.
****************************************************************************/
void calculate_nic_power_name(char* name, int interval, double timeout) {

    time_t start, end;
    
    start= time(NULL);

    

    if (nic_additional_arguments.dynamicMode == 1) 
    {   
        do {
            int *getMatchPID = getAllNamePID(name);
            int size = getMatchPID[0];
            pthread_t tid[size-1];
            if (size > 1) {
                for (int i = 1; i < size; i++) {
                    pthread_create(&tid[i-1], NULL, thread_nic_name, (void *)getMatchPID[i]);
                }

                for (int i = 0; i < size-1; i++) {
                    pthread_join(tid[i],NULL);
                }
                free(getMatchPID);
            }else 
            {
                //SLEEP ORDER
                struct timespec interval_timespec = {interval / 1000, (interval % 1000) * 1000000};
                nanosleep(&interval_timespec, NULL);
            }
            end = time(NULL);
            Ptimes.total_time_s = timeout - difftime(end,start);
            printf("Ptimes.total_time_s = %f\n", Ptimes.total_time_s);
        }while(Ptimes.total_time_s > 0);
    }else {
        int *getMatchPID = getAllNamePID(name);
        int size = getMatchPID[0];
        pthread_t tid[size-1];
        for (int i = 1; i < size; i++) {
            pthread_create(&tid[i-1], NULL, thread_nic_name, (void *)getMatchPID[i]);
        }

        for (int i = 0; i < size-1; i++) 
        {
            pthread_join(tid[i],NULL);
        }
    }



    // printf("\n");
    // printf("NAME: %s\nNIC_MEASURE_DURATION (S): %f\nAVG_NIC_POWER (W): %f\nENERGY_NIC (J): %f\n"
    //       ,name,Ptimes.total_time_s,nic_shared.avg_cpu_power,nic_shared.energy_cpy);
}

int main(int argc, char *argv[]) 
{
    
    //Options
    int opt;
    int pid = 0;
    int interval_milliseconds = -1;
    int total_time_seconds = -1;
    char *proccessName = NULL;
    nic_additional_arguments.dynamicMode = 0;
    nic_additional_arguments.fileToPrint = NULL;
    //int verbose = 1;
    
    static struct option long_options[] = {
            {"no-verbose", no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };

    int long_index = 0;
    while((opt = getopt_long(argc, argv, "p:i:t:n:d:f:", long_options, &long_index)) != -1) 
    {
        switch(opt) {
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
                proccessName = optarg;
                break;
            case 'd':
                if (strcmp(optarg, "true") == 0) {
                    nic_additional_arguments.dynamicMode = 1;
                }
                break;
            case 'f':
                nic_additional_arguments.fileToPrint = optarg;
                break;
            /*case 'v':
                verbose = 0;
                break;*/
            default:
                fprintf(stderr, "Usage: %s -p PID -i interval_milliseconds -t total_time_seconds [-d true|false] [-f filename] [--no-verbose]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (pid != 0 && proccessName != NULL) {
        fprintf(stderr, "Usage: %s [-p PID or -n processName] -i INTERVAL_MILLISECONDS -t TIMEOUT_SECONDS [-d true|false] [-f filename]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (nic_additional_arguments.fileToPrint != NULL) {
        //We open the file in append mode (we write at the end of the file)
        //If it doesnt exist, it is created
        fileOutput = fopen(nic_additional_arguments.fileToPrint, "at+");
        if (fileOutput == NULL) {
            //If, for whatever reason, the file cannot be opnened, we discard the path and print through terminal
            perror("File to print out couldnt be opened, changing to terminal output\n");
            nic_additional_arguments.fileToPrint == NULL;
        }
    }  

    create_shared_object();

    if (proccessName != NULL) //if n is specified, it starts threads
    {
        sem_init(&mutex, 0, 1);
        Ptimes.interval_ms = interval_milliseconds;
        Ptimes.total_time_s = total_time_seconds;
        //Threads go here in the following function.
        calculate_nic_power_name(proccessName,interval_milliseconds,total_time_seconds);
    }
    else 
    {
        //Only calculates by PID
        calculate_nic_power(pid,interval_milliseconds,total_time_seconds, 0);
    }

    if (nic_additional_arguments.fileToPrint != NULL) {
        fclose(fileOutput);
    }
    print_shared_object(pid,total_time_seconds);
    //close_shared_object(&file_descriptor);

    //Set the power values to 0 after the total time is finished...I did This for plotting coherence
    write_to_shared_object(0.0,0.0,0.0);


    return EXIT_SUCCESS;
}
