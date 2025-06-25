# Makefile for host-side FT8 unit test

CC      = gcc
CXX     = g++
CFLAGS  = -DHOST_HAL_MOCK -g -Wall -Werror -I../Inc -I../FT8_library
CXXFLAGS = -DHOST_HAL_MOCK -g -Wall -Werror -I../Inc -I../FT8_library -std=c++17

C_SRC   = ../Src/main.c ../Src/autoseq_engine.c ../Src/ADIF.c ../Src/qso_display.c host_mocks.c
CXX_SRC = json_parser.cpp
C_OBJS  = $(C_SRC:.c=.o)
CXX_OBJS = $(CXX_SRC:.cpp=.o)
TARGET  = host_test

.PHONY: all clean

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(C_OBJS) $(CXX_OBJS)
	$(CXX) $(C_OBJS) $(CXX_OBJS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(C_OBJS) $(CXX_OBJS)
