/*
 * color_sensor.h
 *
 *  Created on: Mar 25, 2023
 *      Author: Ruben Agustin & Hao Feng
 */

#ifndef COLOR_SENSOR_H
#define COLOR_SENSOR_H
#include "controlador.h"

#define I2C_BUS             "/dev/i2c-1"
#define COLOR_SENSOR_ADDR   0x29

void colors();
void write_register(int file, unsigned char reg, unsigned char value);
void read_color(int file, unsigned char* buf);
void init_signals();
void restore_signals();
void toggle_flash(uint8_t *flash);
void color_sensor_teminator(int signum);

extern uint8_t color_sensor_alive;

extern char color_sensor_msg[1500];

extern uint8_t flash;

typedef struct{
	uint16_t clear;					// Clear
	uint16_t red, green, blue;		// RGB values
}t_raw_color;

typedef struct{
	float ir;
	float clear;
	float red, green, blue;		// RGB values
}t_proc_color;


#endif /* COLOR_SENSOR_H_ */
