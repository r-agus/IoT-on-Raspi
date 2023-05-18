/*
 * accelerometer.h
 *
 *  Created on: Mar 25, 2023
 *      Author: Ruben Agustin & Hao Feng
 */

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H
#include "controlador.h"

#define I2C_BUS          "/dev/i2c-1"
#define MPU6050_ADDR     0x68

#define MPU6050_ACCEL_XOUT_H   0x3B
#define MPU6050_ACCEL_XOUT_L   0x3C
#define MPU6050_ACCEL_YOUT_H   0x3D
#define MPU6050_ACCEL_YOUT_L   0x3E
#define MPU6050_ACCEL_ZOUT_H   0x3F
#define MPU6050_ACCEL_ZOUT_L   0x40

extern float ax, ay, az;       // Acceleration values
extern uint8_t accelerometer_alive;


void acceleration();
void write_acc_register(int file, unsigned char reg, unsigned char value);
void read_acceleration(int file, unsigned char* buf);
void stop_acc_measurements(int signum);

#endif /* ACCELEROMETER_H_ */
