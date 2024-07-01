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

//from the datasheet
float max_download_power=0.55; //W
float max_upload_power=1.029; //W

//from the datasheet
float max_download_rate=150500; //KBps
float max_upload_rate=152500; //KBps

long get_net_dev_bytes(int pid, const char* key, int net_key);
void calculate_nic_power(int pid, int interval_milliseconds, double total_time_seconds, int mode);
