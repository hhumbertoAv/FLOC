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
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#include "ePerfRAM.h"
#include <signal.h> 

//For threads
#include <getopt.h>
#include <semaphore.h>
#include <pthread.h>  

//File for finding processes' names
#include "../find_by_name.h"


// For sharing-memory objects
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define SHARED_OBJ_NAME "/RAM_shared_memory"
#define SHARED_OBJ_SIZE 4096 // 4KB
int file_descriptor;  // The id of the shared object for IPC. See create_shared_object()
void* ptr = NULL; // Pointer to the mapped shared object memory region

sem_t mutex;
FILE* fileOutput;



// Auxiliar function to convert a String in an array of String that is passed by reference
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
}


////////////////////////////////////////////////////////////////////////////////////
///////////////////SHARED MEMORY FOR IPC//////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//This creates an in-memory shared object for IPC storing consumption results
void create_shared_object() 
{
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
//the total_power is used as energy
{
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
    double avg_ram_power = total_power / counter;
    double ram_energy = total_power;

    // Print the values in the desired format
    printf("PID: %d\nRAM_MEASURE_DURATION (S): %d\nAVG_RAM_POWER (W): %f\nENERGY_RAM (J): %f\n",
           pid, total_time_seconds, avg_ram_power, ram_energy);

}



void close_shared_object() 
{
    if (ptr != NULL && munmap(ptr, SHARED_OBJ_SIZE) == -1) {
        perror("Unmapping failed");
    }
    if (close(file_descriptor) == -1) {
        perror("Close failed");
    }
    // Remove the shared memory object
    if (shm_unlink(SHARED_OBJ_NAME) == -1) {
        perror("Shared object unlink failed");
        return;
    }
    ptr = NULL; // Reset the pointer after unmapping
}





////////////////////////////////////////////////////////////////////////////////////
/////////////////////////POWER CALCULATION////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////



//The output of PERF give an space for thousands separator. This function deletes this space
void strip_non_digit(char *src, char *dst) {
    while (*src) {
        if (isdigit(*src) || *src == '.') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

/****************************************************************************
*   Function   : calculate_ram_power
*   Description: This function calculate the power consumed by the given 
                 PID. The values calculated are stored in a shared memory. 
*   Parameters : pid - PID to the proccess to analyze.
                 interval_milliseconds - interval between calculations.
                 total_time_seconds - time specified to do the calculations.
                 mode - variable to know if the function is called with
                 the -n or -p argument. 
*   Returned   : None.
****************************************************************************/
void calculate_ram_power(int pid, int interval_milliseconds, double total_time_seconds, int mode) {
    time_t start, end;
    start = time(NULL);
    long long counter = 0;
    double total_ram_power = 0;
    char command[512];
    char output[1024];
    char clean_output[1024];

    while (total_time_seconds < 0 || difftime(end, start) < total_time_seconds) {

        // Check if the process still exists
        if (kill(pid, 0) == -1) {
            if (ram_additional_arguments.dynamicMode == 1) {
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

        // Construct the command to run perf
        sprintf(command, "perf stat -e mem-stores,mem-loads -p %d --timeout=%d 2>&1", pid, interval_milliseconds);

        // Run perf
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            perror("Failed to run command");
            exit(1);
        }

        // Variables to hold parsed values
        double mem_stores = 0, mem_loads = 0;
        double cpu_core_mem_stores = 0.0;
        double cpu_core_mem_loads = 0.0;
        int case_type = 0;

        while (fgets(output, sizeof(output) - 1, fp) != NULL) {
            char *percent_ptr = strchr(output, '(');
            if (percent_ptr != NULL) {
                *percent_ptr = '\0';
            }

            // Remove trailing whitespace
            for (int j = strlen(output) - 1; j >= 0; j--) {
                if (isspace(output[j])) {
                    output[j] = '\0';
                } else {
                    break;
                }
            }

            if (strstr(output, "cpu_core/mem-stores/") != NULL || strstr(output, "cpu_core/mem-loads/") != NULL) {
                case_type = 2;
                if (strstr(output, "cpu_core/mem-stores/") != NULL) {
                    strip_non_digit(output, clean_output);
                    sscanf(clean_output, "%lf", &cpu_core_mem_stores);
                } else if (strstr(output, "cpu_core/mem-loads/") != NULL) {
                    strip_non_digit(output, clean_output);
                    sscanf(clean_output, "%lf", &cpu_core_mem_loads);
                }
            } else if (strstr(output, "mem-stores") != NULL || strstr(output, "mem-loads") != NULL) {
                if (case_type != 2) {
                    case_type = 1;
                    if (strstr(output, "mem-stores") != NULL) {
                        strip_non_digit(output, clean_output);
                        sscanf(clean_output, "%lf", &mem_stores);
                        //printf("stores %f\n", mem_stores);
                    } else if (strstr(output, "mem-loads") != NULL) {
                        strip_non_digit(output, clean_output);
                        sscanf(clean_output, "%lf", &mem_loads);
                        //printf("loads %f\n", mem_loads);
                    }
                }
            }
        }

        // Close the pipe
        pclose(fp);

        // Calculate RAM active energy consumption
        double ram_act = 0.0;
        if (case_type == 1) {
            ram_act = (mem_loads * 6.6) + (mem_stores * 8.7);
        } else if (case_type == 2) {
            ram_act = (cpu_core_mem_stores * 8.7) + (cpu_core_mem_loads * 6.6);
        }
        ram_act /= 1000000000; // Convert from nanoJoules to Joules

        // Calculate total RAM power
        double ram_power = ram_act;

        if (mode == 1) {
            sem_wait(&mutex);
            total_ram_power = get_shared_total_power();
            total_ram_power += ram_power;
            write_to_shared_object(ram_power, total_ram_power, counter);
            if (ram_additional_arguments.fileToPrint != NULL) {
                fprintf(fileOutput, "%f,%f\n", ram_power, total_ram_power);
            }
            sem_post(&mutex);
        } else {
            total_ram_power += ram_power;
            write_to_shared_object(ram_power, total_ram_power, counter);
            if (ram_additional_arguments.fileToPrint != NULL) {
                fprintf(fileOutput, "%f,%f\n", ram_power, total_ram_power);
            }
        }

        counter++;
        struct timespec interval = {0, interval_milliseconds * 1000000};
        nanosleep(&interval, NULL);
        end = time(NULL);
    }
}


////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////THREADS MANAGEMENT///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************
*   Function   : thread_ram_name
*   Description: This is a thread function that calls to calculate_ram_power. 
*   Parameters : vargp - a pointer to the PID of the proccess to calculate. 
*   Returned   : None.
****************************************************************************/
void *thread_ram_name(void *vargp) 
{
   int pid = (int) vargp;
   //MODE = 1 -> used in the critical area in the calculate power function.
   calculate_ram_power(pid, Ptimes.interval_ms, Ptimes.total_time_s, 1);
}

/****************************************************************************
*   Function   : calculate_ram_power_name
*   Description: Function that, given a name, gets all PIDs associatted to it 
                 and launches threads, each one with a different PID, to 
                 calculate its power consumption. 
*   Parameters : name - name of the proccess
                 interval - interval between calculation
                 timeout - total time of calculation. 
*   Returned   : None.
****************************************************************************/
void calculate_ram_power_name(char* name, int interval, double timeout) 
{

    time_t start, end;
    start= time(NULL);

    


    if (ram_additional_arguments.dynamicMode == 1) 
    {   

       
        do {
             
            int *getMatchPID = getAllNamePID(name);
            int size = getMatchPID[0];
            pthread_t tid[size-1];
            if (size > 1) {
                for (int i = 1; i < size; i++) {
                    pthread_create(&tid[i-1], NULL, thread_ram_name, (void *)getMatchPID[i]);
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
        }while(Ptimes.total_time_s > 0);
    }else {


        


        int *getMatchPID = getAllNamePID(name);

            

        int size = getMatchPID[0];
        pthread_t tid[size-1];

    



        for (int i = 1; i < size; i++) {
                
                
            pthread_create(&tid[i-1], NULL, thread_ram_name, (void *)getMatchPID[i]);
                         

        }

        for (int i = 0; i < size-1; i++) 
        {
            pthread_join(tid[i],NULL);
        }
    }
    // if (ram_additional_arguments.fileToPrint != NULL) {
    //     fprintf(fileOutput, "NAME: %s\nCPU_MEASURE_DURATION (S): %f\nAVG_CPU_POWER (W): %f\nENERGY_CPU (J): %f\n"
    //       ,name,timeout,ram_shared.avg_ram_power,ram_shared.energy_ram);
    // }else {
    //     printf("\nNAME: %s\nCPU_MEASURE_DURATION (S): %f\nAVG_CPU_POWER (W): %f\nENERGY_CPU (J): %f\n"
    //       ,name,timeout,ram_shared.avg_ram_power,ram_shared.energy_ram);
    // }
}





////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////





int main(int argc, char *argv[]) 
{
     
    
    //Options
    int opt;
    int pid = 0;
    int interval_milliseconds = -1;
    int total_time_seconds = -1;
    char *proccessName = NULL;
    ram_additional_arguments.dynamicMode = 0;
    ram_additional_arguments.fileToPrint = NULL;
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
                    ram_additional_arguments.dynamicMode = 1;
                }
                break;
            case 'f':
                ram_additional_arguments.fileToPrint = optarg;
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
    if (ram_additional_arguments.fileToPrint != NULL) {
        //We open the file in append mode (we write at the end of the file)
        //If it doesnt exist, it is created
        fileOutput = fopen(ram_additional_arguments.fileToPrint, "at+");
        if (fileOutput == NULL) {
            //If, for whatever reason, the file cannot be opnened, we discard the path and print through terminal
            perror("File to print out couldnt be opened, changing to terminal output\n");
            ram_additional_arguments.fileToPrint == NULL;
        }
    }  

    create_shared_object();

    if (proccessName != NULL) //if n is specified, it starts threads
    {
        sem_init(&mutex, 0, 1);
        Ptimes.interval_ms = interval_milliseconds;
        Ptimes.total_time_s = total_time_seconds;
        //Threads go here in the following function.
        calculate_ram_power_name(proccessName,interval_milliseconds,total_time_seconds);
    }
    else 
    {
        //Only calculates by PID
        calculate_ram_power(pid,interval_milliseconds,total_time_seconds, 0);
    }

    if (ram_additional_arguments.fileToPrint != NULL) {
        fclose(fileOutput);
    }
    print_shared_object(pid,total_time_seconds);
    //close_shared_object(&file_descriptor);

    //Set the power values to 0 after the total time is finished...I did This for plotting coherence
    write_to_shared_object(0.0,0.0,0.0);

    return EXIT_SUCCESS;
}

