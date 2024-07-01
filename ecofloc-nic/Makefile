CC=gcc
CFLAGS=-Wall -lpthread -lrt -w
ADDITIONAL_LIBRARIES = ../find_by_name.c

TARGET_NIC=ecofloc-nic.out
SOURCE_NIC=ePerfNIC.c

all: $(TARGET_NIC)

nic: $(TARGET_NIC)

$(TARGET_NIC): $(SOURCE_NIC)
	$(CC) $(CFLAGS) $(SOURCE_NIC) $(ADDITIONAL_LIBRARIES) -o $(TARGET_NIC)

clean:
	rm -f $(TARGET_NIC)

