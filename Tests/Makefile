# Makefile for host-side FT8 unit test

CC      = gcc
CFLAGS  = -DHOST_HAL_MOCK -g -Wall -I../Inc -I../FT8_library

SRC     = ../Src/main.c ../Src/autoseq_engine.c ../Src/ADIF.c host_mocks.c
TARGET  = host_test

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
