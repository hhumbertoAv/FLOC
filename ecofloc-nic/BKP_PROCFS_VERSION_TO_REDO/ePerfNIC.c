#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
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
////////////////////////////////////////////////////////////////////////////////////

long get_net_dev_bytes_from_PROC(int pid, const char* net_int, int net_field) // TO REDO 
//NET FS in PROC is not by PID
{
    char path[256];
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    snprintf(path, sizeof(path), PROC_NET_DEV_PATH_FORMAT, pid);

    FILE* file = fopen(path, "r");
    if (file == NULL) 
    {
        perror("Could not open /proc/<PID>/net/dev file");
        return -1;
    }
    char strarray [16][256];
    while ((read = getline(&line, &len, file)) != -1) 
    {
        // To search for a coincidence first we convert the line in an array of Strings
        // and then we look for a coincidence for our net interface
        str2array(line,len,strarray);
	    if (strcmp(strarray[0],net_int) == 0) 
	    {
		    break;
	    }
    }

    fclose(file);
    // Return the number of bytes associated with the key
    // Position 1 - returns the field of the bytes received by that interface
    //Position 9 - returns the field of the bytes transmitted by that interface
   
    return strtol(strarray[net_field],NULL,10);
}






long get_net_dev_bytes(int pid, const char* net_int, int net_field) {
    char path[256];
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    snprintf(path, sizeof(path), PROC_NET_DEV_PATH_FORMAT, pid);

    FILE* file = fopen(path, "r");
    if (file == NULL) 
    {
        perror("Could not open /proc/<PID>/net/dev file");
        return -1;
    }
    char strarray [16][256];
    while ((read = getline(&line, &len, file)) != -1) 
    {
        // To search for a coincidence first we convert the line in an array of Strings
        // and then we look for a coincidence for our net interface
        str2array(line,len,strarray);
	    if (strcmp(strarray[0],net_int) == 0) 
	    {
		    break;
	    }
    }

    fclose(file);
    // Return the number of bytes associated with the key
    // Position 1 - returns the field of the bytes received by that interface
    //Position 9 - returns the field of the bytes transmitted by that interface
   
    return strtol(strarray[net_field],NULL,10);
}

////////////////////////////////////////////////////////////////////////////////////
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
}

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

void close_shared_object() {
    if (ptr != NULL && munmap(ptr, SHARED_OBJ_SIZE) == -1) {
        perror("Unmapping failed");
    }
    if (close(file_descriptor) == -1) {
        perror("Close failed");
    }
    ptr = NULL; // Reset the pointer after unmapping
}




void calculate_nic_power(int pid, int interval_milliseconds, double total_time_seconds, int mode){ 
    

    time_t start, end;
    start = time(NULL);

    long long counter = 0;
    long total_download_bytes = 0;
    long total_upload_bytes = 0;
    double total_nic_power = 0;
    //Result are in Bps, we need to convert it to KBps
    long download_bytes = get_net_dev_bytes(pid, NET_INT, BYTES_RECEIVED)/1000;
    long upload_bytes = get_net_dev_bytes(pid, NET_INT, BYTES_TRANSMITTED)/1000;
    

     do {

         if (kill(pid, 0) == -1) {
            printf("Process %d was killed or does not exist\n", pid);
            break;
        }
  
        download_bytes += get_net_dev_bytes(pid, NET_INT, BYTES_RECEIVED)/1000;
        upload_bytes += get_net_dev_bytes(pid, NET_INT, BYTES_TRANSMITTED)/1000;

        //We calculate the download and upload power
        double upload_rate = upload_bytes / (interval_milliseconds / 1000.0); // converting to seconds
        double download_rate = download_bytes / (interval_milliseconds / 1000.0);

        //Power calculation considering the rates
        //TODO -> Put this in another file for allowing any formula
        double upload_power = max_upload_power * (upload_rate / max_upload_rate);
        double download_power = max_download_power * (download_rate / max_download_rate);

        double nic_power = upload_power + download_power;
        total_nic_power += nic_power;

        //Write the results into the shared object for IPC
        if (mode == 1) 
        {
            sem_wait(&mutex);
        }
        write_to_shared_object(nic_power,total_nic_power,counter);
        if (mode == 1) 
        {
            sem_post(&mutex);
        }

           
        // nanosleep receives 
        struct timespec interval = {interval_milliseconds / 1000, (interval_milliseconds % 1000) * 1000000};
        nanosleep(&interval, NULL); 
        // After the sleep, add the download and upload bytes from /proc/<pid>/net/dev

        counter++;
        end = time(NULL);


    } while (total_time_seconds < 0 || difftime(end, start) < total_time_seconds);

    if (counter > 0) {
        //We calculate the total power and energy consumed by the net interface
        double avg_nic_power = total_nic_power / counter;
        double nic_energy = avg_nic_power * difftime(end, start);


        

        if (mode == 0) {
            printf("PID: %d\nNIC_MEASURE_DURATION (S): %f\nAVG_NIC_POWER (W): %f\nENERGY_NIC (J): %f\n", 
            pid, total_time_seconds, avg_nic_power, nic_energy);
        }else {
            sem_wait(&mutex);
            nic_shared.avg_cpu_power += avg_nic_power;
            nic_shared.energy_cpy += nic_energy;
            sem_post(&mutex);
        }
    }
}

void *thread_sd_name(void *vargp) {
    int pid = (int *) vargp;

   calculate_nic_power(pid, Ptimes.interval_ms, Ptimes.total_time_s, 1);
}

void calculate_nic_power_name(char* name, double interval, double timeout) {

    int *getMatchPID = getAllNamePID(name);
    int size = getMatchPID[0];

    pthread_t tid[size-1];

    for (int i = 1; i < size; i++) 
    {
        pthread_create(&tid[i-1], NULL, thread_sd_name, (void *)getMatchPID[i]);
    }

    for (int i = 0; i < size-1; i++) 
    {
        pthread_join(tid[i],NULL);
    }



    printf("\n");
    printf("NAME: %s\nCPU_MEASURE_DURATION (S): %f\nAVG_CPU_POWER (W): %f\nENERGY_CPU (J): %f\n"
          ,name,Ptimes.total_time_s,nic_shared.avg_cpu_power,nic_shared.energy_cpy);
}

int main(int argc, char *argv[]) {
    
    //Options
    int opt;
    int pid = 0;
    int interval_milliseconds = -1;
    int total_time_seconds = -1;
    char *proccessName = NULL;
    //int verbose = 1;
    
    static struct option long_options[] = {
            {"no-verbose", no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };

    int long_index = 0;
    while((opt = getopt_long(argc, argv, "p:i:t:n:", long_options, &long_index)) != -1) 
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
            /*case 'v':
                verbose = 0;
                break;*/
            default:
                fprintf(stderr, "Usage: %s -p PID -i interval_milliseconds -t total_time_seconds [--no-verbose]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (pid != 0 && proccessName != NULL) {
        fprintf(stderr, "Usage: %s [-p PID or -n ProccessName ] -i INTERVAL_MILLISECONDS -t TIMEOUT_SECONDS \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    create_shared_object();
    if (proccessName != NULL) 
    {
        sem_init(&mutex, 0, 1);
        Ptimes.interval_ms = interval_milliseconds;
        Ptimes.total_time_s = total_time_seconds;
        calculate_nic_power_name(proccessName,interval_milliseconds,total_time_seconds);
    }else {
        calculate_nic_power(pid,interval_milliseconds,total_time_seconds, 0);
    }
    close_shared_object(&file_descriptor);

    return EXIT_SUCCESS;
}

