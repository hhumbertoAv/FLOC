CC=gcc
CFLAGS=-Wall -lpthread -lrt -w
ADDITIONAL_LIBRARIES = ../find_by_name.c

TARGET_RAM=ecofloc-ram.out
SOURCE_RAM=ePerfRAM.c

all: $(TARGET_RAM)

ram: $(TARGET_RAM)

$(TARGET_RAM): $(SOURCE_RAM)
	$(CC) $(CFLAGS) $(SOURCE_RAM) $(ADDITIONAL_LIBRARIES) -o $(TARGET_RAM)

clean:
	rm -f $(TARGET_RAM) 
