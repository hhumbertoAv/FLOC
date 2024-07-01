CC=gcc
CFLAGS= -DDEFAULT_CONFIG_PATH=\"/opt/ecofloc/sdFeatures.conf\" -lpthread -lrt -w
ADDITIONAL_LIBRARIES = ../find_by_name.c

# define your targets
TARGET_SD=ecofloc-sd.out
SOURCE_SD=ePerfSD.c

# 'all' target will build both targets
all: $(TARGET_SD)

# 'sd' target will build only the SD target
sd: $(TARGET_SD)

$(TARGET_SD): $(SOURCE_SD)
	$(CC) $(CFLAGS) $(SOURCE_SD) $(ADDITIONAL_LIBRARIES) -o $(TARGET_SD)

clean:
	rm -f $(TARGET_SD)

