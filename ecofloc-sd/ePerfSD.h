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
#include <signal.h> 
//For threading and mutual exclusion
#include <semaphore.h>
#include <pthread.h>
#include <signal.h> 
//For finding proccesses by name
#include "../find_by_name.h"  

#define PROC_IO_PATH_FORMAT "/proc/%d/io"
#define READ_BYTES_KEY "read_bytes:"
#define WRITE_BYTES_KEY "write_bytes:"

// For sharing-memory objects
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#define SHARED_OBJ_NAME "/SD_shared_memory"
#define SHARED_OBJ_SIZE 4096 // 4KB
int file_descriptor;  // The id of the shared object for IPC. See create_shared_object()
void* ptr = NULL; // Pointer to the mapped shared object memory region

sem_t mutex;
FILE* fileOutput;


//Important ref: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/filesystems/proc.rst?id=HEAD#l1305


////////////////////////////////////////////////////////////////////////////////////
// CONFIG SECTION -> Depends on sdFeatures.txt (I/O watts and max bytes per second) 
///////////////////////////////////////////////////////////////////////////////////


// Struct to hold the SD features. Variables' mame correspond to file fileds 
typedef struct {
    float write_power; //in Watts
    float read_power; //in Watts
    long write_max_rate; //in bytes per second
    long read_max_rate; //in bytes per second
} StorageValues;

/*
 Structs to hold the arguments values
*/
struct sd_power_values{
    double avg_sd_power;
    double energy_cpy
}sd_shared;

struct program_times{
    int interval_ms;
    double total_time_s;
}Ptimes;

struct additional_arguments{
    int dynamicMode;
    char *fileToPrint;
}sd_additional_arguments;

void extract_SD_features(const char* filename, StorageValues* values) {
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        if (sd_additional_arguments.dynamicMode == 1) {
            return;
        }else {
            fprintf(stderr, "Could not open file %s\n", filename);
            exit(EXIT_FAILURE);
        }
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        
        if (line[0] == '#') {
                continue;
        }
            // Parse each line
        if (sscanf(line, "write_power=%f", &(values->write_power)) == 1) {
                // Successfully parsed write_power
        } else if (sscanf(line, "read_power=%f", &(values->read_power)) == 1) {
                // Successfully parsed read_power
        } else if (sscanf(line, "write_max_rate=%ld", &(values->write_max_rate)) == 1) {
                // Successfully parsed write_max_rate
        } else if (sscanf(line, "read_max_rate=%ld", &(values->read_max_rate)) == 1) {
                // Successfully parsed read_max_rate
        }
    }

    fclose(file);
}

////////////////////////////////////////////////////////////////////////////////////
//       POWER FORMULA SECTION 
///////////////////////////////////////////////////////////////////////////////////

// Struct to hold the power values
typedef struct {
 
    double read_power;   // avg power for reading in the interval
    double write_power; // avg power for writing in the interval   
    double total_power; // read + write
    double accumulated_power; //To store the accumulated power in each while iteration

} PowerValues;

PowerValues power_formula(StorageValues *values, long read_rate, long write_rate) {
        
    PowerValues power_values;
    power_values.read_power = values->read_power * (double)read_rate / values->read_max_rate;
    power_values.write_power = values->write_power * (double)write_rate / values->write_max_rate;
    power_values.total_power = power_values.read_power + power_values.write_power;

    return power_values;
}


////////////////////////////////////////////////////////////////////////////////////
//       I/O RATES
///////////////////////////////////////////////////////////////////////////////////


long get_io_bytes(int pid, const char* key) {

//This function opens the specified /proc/<PID>/io file, reads the file line by line until it finds the line that starts with the specified key ("read_bytes:" or "write_bytes:"), and returns the number of bytes.
    char path[256];
    snprintf(path, sizeof(path), PROC_IO_PATH_FORMAT, pid);

    FILE* file = fopen(path, "r");
    if (file == NULL) 
    {
        if (sd_additional_arguments.dynamicMode == 1) 
        {
            return 0;
        }
        else 
        {
            perror("Could not open /proc/<PID>/io file");
            exit(1);
        }

    }

    char line_key[256];
    long bytes;
    // Read the file until the desired key is found
    while (fscanf(file, "%s %ld", line_key, &bytes) == 2) {
            if (strcmp(line_key, key) == 0) {
                break;
        }
    }

    fclose(file);
    // Return the number of bytes associated with the key
    return bytes;
}

////////////////////////////////////////////////////////////////////////////////////
//              SHARED OBJECT FOR IPC
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

//Method to write in shared object the values of current power, total power and counter variables
void write_to_shared_object(double current_power, double total_power, long long counter) {
    ptr = mmap(0, SHARED_OBJ_SIZE, PROT_WRITE, MAP_SHARED, file_descriptor, 0);
    if (ptr == MAP_FAILED) {
        perror("Mapping failed");
        close(file_descriptor);
        ptr = NULL;
        return;
    }
    sprintf(ptr, "current_power=%f\ntotal_power=%f\ncounter=%lld", current_power, total_power, counter);
}

//Mehod that retrieves from the shared object the total power consumed
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

//Method to print the values of the varibales stored in the shared object
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
    double avg_sd_power = total_power / counter;
    double sd_energy = total_power;

    // Print the values in the desired format
    printf("PID: %d\nSD_MEASURE_DURATION (S): %d\nAVG_SD_POWER (W): %f\nENERGY_SD (J): %f\n",
           pid, total_time_seconds, avg_sd_power, sd_energy);

}

//Method to close the shared object
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

typedef struct {
    double power;
    double time;
    double energy;
} SDPowerAndTime;


////////////////////////////////////////////////////////////////////////////////////
/////////////////////////POWER CALCULATION////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************
*   Function   : calculate_power
*   Description: This function calculate the power consumed by the given 
                 PID. The values calculated are stored in a shared memory. 
*   Parameters : pid - PID to the proccess to analyze.
                 interval_milliseconds - interval between calculations.
                 total_time - time specified to do the calculations.
                 s_values - struct to send to the function that returns
                 the consume.
                 mode - variable to know if the function is called with
                 the -n or -p argument. 
*   Returned   : None.
****************************************************************************/

void calculate_power(int pid, int interval_milliseconds, double total_time,  StorageValues* s_values, int mode)
{

    time_t start, end;
    start = time(NULL);

    PowerValues total_power_values = {0};
    total_power_values.accumulated_power=0;

    long long counter = 0;
    long read_rate, write_rate;

    long initial_read_bytes = get_io_bytes(pid, READ_BYTES_KEY);
    long initial_write_bytes = get_io_bytes(pid, WRITE_BYTES_KEY);

    if (sd_additional_arguments.dynamicMode == 1) {
        total_time = Ptimes.total_time_s;
    }

     do {

         // check if the process still exists
        if (kill(pid, 0) == -1) {
            //We check for dynamic option
            if (sd_additional_arguments.dynamicMode == 1) {
                if (mode == 1) 
                {
                    break;
                }
                else 
                {
                    end=time(NULL);
                    continue;
                }
            }else {
                printf("Process %d was killed\n", pid);
                break;
            }
        }
   
        // nanosleep receives 
        struct timespec interval = {interval_milliseconds / 1000, (interval_milliseconds % 1000) * 1000000};
        nanosleep(&interval, NULL); 
        // After the sleep, read the final read and write bytes from /proc/<pid>/io
  
        long final_read_bytes = get_io_bytes(pid, READ_BYTES_KEY);
        long final_write_bytes = get_io_bytes(pid, WRITE_BYTES_KEY);

        // Calculate the interval in seconds as a float
        float interval_seconds = (float) interval_milliseconds / 1000;

        //Calculate the r/w rates by finding the difference between final and initial byte values and dividing by the interval
        read_rate = (long) ((final_read_bytes - initial_read_bytes) / interval_seconds);
        write_rate = (long) ((final_write_bytes - initial_write_bytes) / interval_seconds);

        PowerValues p_values = power_formula(s_values, read_rate, write_rate);

        //Write the results into the shared object for IPC
        if (mode == 1) 
        {
            sem_wait(&mutex);

            total_power_values.accumulated_power=get_shared_total_power();
            total_power_values.accumulated_power += p_values.total_power;
            write_to_shared_object(p_values.total_power,total_power_values.accumulated_power,counter);

            if (sd_additional_arguments.fileToPrint!= NULL) 
            {
                fprintf(fileOutput, "%f,%f\n", p_values.total_power, total_power_values.accumulated_power);
            }
            
            sem_post(&mutex);
        }
        else
        {
            total_power_values.accumulated_power += p_values.total_power;
            write_to_shared_object(p_values.total_power,total_power_values.accumulated_power,counter);

            if (sd_additional_arguments.fileToPrint!= NULL) 
            {
                fprintf(fileOutput, "%f,%f\n", p_values.total_power, total_power_values.accumulated_power);
            }

        }
        
        counter++;

        initial_read_bytes = final_read_bytes;
        initial_write_bytes = final_write_bytes;

        end = time(NULL);

    } while (total_time < 0 || difftime(end, start) < total_time);




    // if (counter > 0) {
    //     double avg_power = total_power_values.accumulated_power / counter;
    //     double total_energy = avg_power * difftime(end, start);
    //     if (mode == 0) {
    //         printf("PID: %d\nSD_MEASURE_DURATION (S): %f\nAVG_SD_POWER (W): %f\nENERGY_SD (J): %f\n",
    //            pid, difftime(end, start) ,avg_power, total_energy);
    //     }else {
    //         sem_wait(&mutex);
    //         sd_shared.avg_sd_power += avg_power;
    //         sd_shared.energy_cpy += total_energy;
    //         sem_post(&mutex);
    //     }
    // }
}



/****************************************************************************
*   Function   : calculate_sd_power
*   Description: This function prepared the environment to call to 
                calculate_power function.
                 interval_milliseconds - interval between calculations.
                 total_time - time specified to do the calculations.
                 s_values - struct to send to the function that returns
                 the consume.
                 mode - variable to know if the function is called with
                 the -n or -p argument. 
*   Returned   : None.
****************************************************************************/
void calculate_sd_power(int pid, int interval_milliseconds, double total_time_seconds, int mode){ 
    
    StorageValues s_values;
    extract_SD_features(DEFAULT_CONFIG_PATH, &s_values); 
    //printf("write_power = %f, read_power = %f, write_max_rate = %ld, read_max_rate = %ld\n", 
      //     s_values.write_power, s_values.read_power, s_values.write_max_rate, s_values.read_max_rate);

    // pass the addresses of read_rate and write_rate to calculate_rate
    calculate_power(pid,interval_milliseconds, total_time_seconds, &s_values, mode);
}

////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////THREADS MANAGEMENT///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************
*   Function   : thread_sd_name
*   Description: This is a thread function that calls to calculate_sd_power. 
*   Parameters : vargp - a pointer to the PID of the proccess to calculate. 
*   Returned   : None.
****************************************************************************/
void *thread_sd_name(void *vargp) 
{
    int pid = (int) vargp;

   calculate_sd_power(pid, Ptimes.interval_ms, Ptimes.total_time_s, 1);
}

/****************************************************************************
*   Function   : calculate_sd_power_name
*   Description: Function that, given a name, gets all PIDs associatted to it 
                 and launches threads, each one with a different PID, to 
                 calculate its power consumption. 
*   Parameters : name - name of the proccess
                 interval - interval between calculation
                 timeout - total time of calculation. 
*   Returned   : None.
****************************************************************************/
void calculate_sd_power_name(char* name, int interval, double timeout) 
{

    time_t start, end;
    
    start= time(NULL);

    

    if (sd_additional_arguments.dynamicMode == 1) 
    {   
        do {
            int *getMatchPID = getAllNamePID(name);
            int size = getMatchPID[0];
            pthread_t tid[size-1];
            if (size > 1) {
                for (int i = 1; i < size; i++) {
                    pthread_create(&tid[i-1], NULL, thread_sd_name, (void *)getMatchPID[i]);
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
            pthread_create(&tid[i-1], NULL, thread_sd_name, (void *)getMatchPID[i]);
        }

        for (int i = 0; i < size-1; i++) 
        {
            pthread_join(tid[i],NULL);
        }
    }

    // if (sd_additional_arguments.fileToPrint != NULL) {
    //     fprintf(fileOutput, "NAME: %s\nSD_MEASURE_DURATION (S): %f\nAVG_CPU_POWER (W): %f\nENERGY_CPU (J): %f\n"
    //       ,name,timeout,sd_shared.avg_sd_power,sd_shared.energy_cpy);
    // }else {
    //     printf("\nNAME: %s\nCPU_MEASURE_DURATION (S): %f\nAVG_CPU_POWER (W): %f\nENERGY_CPU (J): %f\n"
    //       ,name,timeout,sd_shared.avg_sd_power,sd_shared.energy_cpy);
    // }
}





