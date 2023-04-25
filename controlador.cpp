/*
 * main.c
 *
 *  Created on: Mar 2023
 *      Author: Ruben Agustin & Hao Feng
 */
#include "controlador.h"
#include "color_sensor.h"
#include "accelerometer.h"
#include <atomic>

#define WRITE_BUF_SIZE	1500

void start_menu();
void* start_accelerometer(void* arg);
void* start_color_sensor(void* arg);


void toggle_value(uint8_t *value);

// Thread stuff
pthread_t thread_color_sensor, thread_acc;
int thread_error;
uint8_t color_sensor_alive, accelerometer_alive;

// Represent data
void init_signals();
void restore_signals();
struct termios original_termios = {0}, modified_termios = {0};

std::atomic<int> color_sensor_data_ready = 0, acc_data_ready = 0;

void sigint_handler(int signum) {
	int time = 1;			// Original time that requires to close
	printf("Exiting, wait till everything is closed                \n");
	if(accelerometer_alive) {accelerometer_alive = 0; stop_acc_measurements(1); time+=3;}
	if(color_sensor_alive)	{color_sensor_alive = 0; color_sensor_teminator(1); time+=3;}
	tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
	sleep(time);
    exit(1);
}

void* control(void* arg){
	// Input stuff
	char c = 0;	// keyboard input
	int result;	// error check

	init_signals();

	signal(SIGINT, sigint_handler);

	start_menu();

	while(1){
		result = read(STDIN_FILENO, &c, 1);
		usleep(10000);
		if(result > 0 && c == '1') { 						// Start/stop accelerometer
			c = 0;
			if(!accelerometer_alive){
				printf("Accelerometer starts\n");
				fflush(stdout);
				thread_error = pthread_create(&thread_color_sensor, NULL, start_accelerometer, NULL);
				accelerometer_alive = 1;
			} else{											// Turn off accelerometer
				accelerometer_alive = 0; 					// Fuction that closes acc
				stop_acc_measurements(1);
			}
		}else if(result > 0 && c == '2') { 					// Start/stop color_sensor
			c = 0;
			if(!color_sensor_alive){
				printf("Color sensor starts                                      \n");
				fflush(stdout);
				thread_error = pthread_create(&thread_acc, NULL, start_color_sensor, NULL);
				color_sensor_alive = 1;
			}else {
				color_sensor_alive = 0;
				color_sensor_teminator(1);							// Calls function that closes color sensor
			}
		} if(result > 0 && c == 'f') {
			flash = 1;
		}
	}
	return NULL;
}

void* start_color_sensor(void* arg){
	colors();
	return NULL;
}

void* start_accelerometer(void* arg){
	acceleration();
	return NULL;
}

void inline toggle_value(uint8_t *value){
	*value = !(*value);
}

void start_menu(){
	printf("\e[8;24;85t"); // Set terminal size
	fflush(stdin);
	printf("***************** Sensor measurements by Ruben Agustin & Hao Feng *****************\n\n");
	fflush(stdout);
	printf("-- Press 1 to start/disable accelerometer\n");
	fflush(stdout);
	printf("-- Press 2 to start/disable color sensor");
	fflush(stdout);
	printf("\nPress a key to start: ");
	fflush(stdout);
}

void init_signals(){
    // Save the original terminal settings
    tcgetattr(STDIN_FILENO, &original_termios);

    // Copy the original settings and modify them
    modified_termios = original_termios;
    modified_termios.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    modified_termios.c_cc[VMIN] = 0;    // Set VMIN to 0 to read input without blocking
    modified_termios.c_cc[VTIME] = 0;   // Disable timeout

    // Set the modified terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &modified_termios);

    // Set stdin to non-blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, flags);
}

void restore_signals(){
    // Set the modified terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}
