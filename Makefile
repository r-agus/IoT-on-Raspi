# Makefile

# Compiler
CXX = /home/ruben/Documents/buildroot-2022.02.9/output/host/bin/arm-buildroot-linux-gnueabihf-g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++17

# Source files
SRCS = accelerometer.cpp client.cpp color_sensor.cpp controlador.cpp ticker.cpp

# Header files
HDRS = accelerometer.h color_sensor.h controlador.h ticker.h

# Object files
OBJS = $(SRCS:.cpp=.o)

# Targets
all: client

client: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) client
