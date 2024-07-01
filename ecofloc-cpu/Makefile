CC=gcc
CFLAGS=-Wall -w  # Compiler flags
LDFLAGS=-lpthread -lrt  # Linker flags

# Define your targets
TARGET_CPU=ecofloc-cpu.out
SOURCE_CPU=main.c
ADDITIONAL_LIBRARIES=../find_by_name.c cpu_map.c pid_energy.c comm_energy.c usage.c results_map.c 

# 'all' target will build the target
all: $(TARGET_CPU)

# 'cpu' target will build only the CPU target
cpu: $(TARGET_CPU)

$(TARGET_CPU): $(SOURCE_CPU) $(ADDITIONAL_LIBRARIES)
	$(CC) $(CFLAGS) $(SOURCE_CPU) $(ADDITIONAL_LIBRARIES) $(LDFLAGS) -o $(TARGET_CPU)

clean:
	rm -f $(TARGET_CPU)
