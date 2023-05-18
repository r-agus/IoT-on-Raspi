/*
 * accelerometer.c
 *
 *  Created on: Mar 25, 2023
 *      Author: Ruben Agustin & Hao Feng
 */
#include "accelerometer.h"
#include <atomic>

extern std::atomic<int> acc_data_ready;

int fd;

void stop_acc_measurements(int signum) {
    printf("\033[KAccelerometer off\n");
    close(fd);
}
void acceleration(){
//    unsigned char start_reg = MPU6050_ACCEL_XOUT_H;
    unsigned char rd_buf[2];
    char write_buf[1500];

//    signal(SIGINT, stop_acc_measurements);
    // Open the I2C bus
    fd = open(I2C_BUS, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C bus");
        exit(1);
    }

    // Set the I2C slave address
    if (ioctl(fd, I2C_SLAVE, MPU6050_ADDR) < 0) {
        perror("Failed to set the I2C slave address");
        exit(1);
    }


    write_acc_register(fd, 0x6b, 0x80);     // Initial reset
    usleep(5000); // Wait to stabilize;
    write_acc_register(fd, 0x6c, 0xC7);
    usleep(500); // Wait to stabilize;
    write_acc_register(fd, 0x6b, 0x28);
    usleep(500); // Wait to stabilize;
    write_acc_register(fd, 0x1a, 0x03);
    usleep(500); // Wait to stabilize;
    write_acc_register(fd, 0x19, 0x08);

    while (accelerometer_alive){
        sleep(1);
        read_acceleration(fd, rd_buf);

        atomic_exchange(&acc_data_ready, 0);

		ax = rd_buf[0] << 8 | rd_buf[1];
        ay = rd_buf[2] << 8 | rd_buf[3];
        az = rd_buf[4] << 8 | rd_buf[5];

        // Convert to g:
        ax = (signed short)ax / 16384.0;
        ay = (signed short)ay / 16384.0;
        az = (signed short)az / 16384.0;

        // Send data to main
        sprintf(write_buf, "%f %f %f", ax, ay, az);

        atomic_exchange(&acc_data_ready, 1);
    }

//    stop_acc_measurements(1);
}

void write_acc_register(int file, unsigned char reg, unsigned char value){
    struct i2c_rdwr_ioctl_data i2c_data;
    struct i2c_msg msgs[1];

    unsigned char wr_buf[2];

    wr_buf[0] = reg;
    wr_buf[1] = value;

    msgs[0].addr = MPU6050_ADDR;
    msgs[0].flags = 0;  // Write operation
    msgs[0].len = 2;    // 1st byte dir, 2nd value
    msgs[0].buf = wr_buf;

    // Set up I2C transaction
    i2c_data.msgs = msgs;
    i2c_data.nmsgs = 1;

    // Send the I2C transaction
    ioctl(file, I2C_RDWR, &i2c_data);
}

void read_acceleration(int file, unsigned char* buf){
    struct i2c_msg messages[2];
    struct i2c_rdwr_ioctl_data ioctl_data;

    unsigned char start_reg = 0x3b;
    messages[0].addr  = 0x68; // MPU-6050 device address
    messages[0].flags = 0;    // Write operation
    messages[0].len   = 1;    // One byte to write
    messages[0].buf   = &start_reg;

    messages[1].addr  = 0x68; // MPU-6050 device address
    messages[1].flags = I2C_M_RD; // Read operation
    messages[1].len   = 6;    // Number of bytes to read
    messages[1].buf   = buf;

    ioctl_data.msgs  = messages;
    ioctl_data.nmsgs = 2;

    ioctl(file, I2C_RDWR, &ioctl_data);
}


