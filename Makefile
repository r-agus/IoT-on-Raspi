# Makefile

# Compiler
CXX = /home/ruben/Documents/buildroot-2022.02.9/output/host/bin/arm-buildroot-linux-gnueabihf-g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++17 -lmosquittopp

# Source files
SRCS = accelerometer.cpp client.cpp color_sensor.cpp controlador.cpp ticker.cpp

# Header files
HDRS = accelerometer.h color_sensor.h controlador.h ticker.h

# Options
CPPFLAGS = -DOPTION_4

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable
EXECUTABLE = client

# Targets
all: $(EXECUTABLE)

menu: 
	@./config_menu.sh

$(EXECUTABLE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $(EXECUTABLE) $(OBJS)

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXECUTABLE)
