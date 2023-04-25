/*
 * color_sensor.c
 *
 *  Created on: Mar 25, 2023
 *      Author: Ruben Agustin & Hao Feng
 */
#include "color_sensor.h"
#include <atomic>
extern std::atomic<int> color_sensor_data_ready;

int i2c_fd;
uint8_t fd_open = 0;
uint8_t light = 0;

extern t_raw_color	raw_colors;
extern t_proc_color	proc_colors;

void color_sensor_teminator(int signum) {
    printf("\033[J\033[38;2;255;255;255mColor sensor off\n");
    printf("\033[?25h");
    close(i2c_fd);
}

void colors(){
    unsigned char color[8];					// clear + RGB
    uint8_t error_measure = 0;

    time_t start, end;
    double elapsed_time;

    i2c_fd = open(I2C_BUS, O_RDWR);
    if (i2c_fd < 0) {
        perror("Failed to open I2C bus");
        exit(1);
    } else fd_open = 1;

    // Set the I2C slave address
    if (ioctl(i2c_fd, I2C_SLAVE, COLOR_SENSOR_ADDR) < 0) {
        perror("Failed to set the I2C slave address");
        exit(1);
    }

    write_register(i2c_fd, 0x80, 0x03);
    usleep(5000);
    write_register(i2c_fd, 0x81, 0x00);
    usleep(500);
    write_register(i2c_fd, 0x83, 0x07);
    usleep(500);
    write_register(i2c_fd, 0x8F, 0x00);
    usleep(500);
    write_register(i2c_fd, 0x8D, 0x11);
    usleep(500);
    write_register(i2c_fd, 0x80, 0x43);               // Turns off flash

    start = time(NULL);
    while(color_sensor_alive){
    	usleep(10000);
    	if(!fd_open){
    		i2c_fd = open(I2C_BUS, O_RDWR);
    		    if (i2c_fd < 0) {
    		        perror("Failed to open I2C bus");
    		        exit(1);
    		    }
    	}
        if(flash == 1) {				// 'f' pressed
        	toggle_flash(&light);
            flash = 0;
        } else{
            end = time(NULL);
            elapsed_time = difftime(end, start);

            if (elapsed_time >= 1.0) {
//            	fflush(stdout);
            	error_measure = 0;
            	// Old data
            	atomic_exchange(&color_sensor_data_ready,0);
                read_color(i2c_fd, color);
                raw_colors.clear	=	color[1] << 8 | color[0];
                raw_colors.red		= 	color[3] << 8 | color[2];
                raw_colors.green	= 	color[5] << 8 | color[4];
                raw_colors.blue		= 	color[7] << 8 | color[6];

                proc_colors.ir 		= (float)(raw_colors.red + raw_colors.green + raw_colors.blue - raw_colors.clear)/2.0;
                proc_colors.clear 	= (float)(raw_colors.clear	- proc_colors.ir);
                proc_colors.red 	= (float)(raw_colors.red 	- proc_colors.ir);				//255/65535.0;
                proc_colors.green 	= (float)(raw_colors.green	- proc_colors.ir);
                proc_colors.blue 	= (float)(raw_colors.blue	- proc_colors.ir);

                if(proc_colors.clear != 0){
					proc_colors.red 	= proc_colors.red  /proc_colors.clear * 255.0;
					proc_colors.blue 	= proc_colors.blue /proc_colors.clear * 255.0;
					proc_colors.green	= proc_colors.green/proc_colors.clear * 255.0;
                } else error_measure |= 0x8;

            	if(proc_colors.ir > proc_colors.red)   error_measure |= 0x4;
				if(proc_colors.ir > proc_colors.green) error_measure |= 0x2;
				if(proc_colors.ir > proc_colors.blue)  error_measure |= 0x1;

                sprintf(color_sensor_msg, "\033[?25l");
                if (!(error_measure & 0x8)){	// Clear == 0
                	char tmp[100] = "";
                	sprintf(tmp, "\033[38;2;255;0;0mR: %.0f        \r", proc_colors.red);
					(error_measure & 0x4) ? strcat(color_sensor_msg, "\033[38;2;255;0;0mIR > RED      \r")
							: strcat(color_sensor_msg, tmp);  // Set foreground color to red and write red value
					*tmp = 0;
					sprintf(tmp, "\n\033[38;2;0;255;0mG: %.0f      \r", proc_colors.green);
					(error_measure & 0x2) ? strcat(color_sensor_msg, "\n\033[38;2;0;255;0mIR > GREEN  \r")
							: strcat(color_sensor_msg, tmp);  // Set foreground color to green and write green value
					*tmp = 0;
					sprintf(tmp, "\n\033[38;2;0;0;255mB: %.0f      \r\033[2A\033[38;2;255;255;255m", proc_colors.blue);
					(error_measure & 0x1) ? strcat(color_sensor_msg, "\n\033[38;2;0;0;255mIR > BLUE   \r\033[2A\033[38;2;255;255;255m")
							: strcat(color_sensor_msg, tmp);  // Set foreground color to blue and write blue value

                } else{
                	strcat(color_sensor_msg, "\033[38;2;255;0;0mR : 255 *                \r");
					strcat(color_sensor_msg, "\n\033[38;2;0;255;0mG : 255 *              \r");
					strcat(color_sensor_msg, "\n\033[38;2;0;0;255mB : 255 *            \r\033[2A\033[38;2;255;255;255m");

                }
                atomic_exchange(&color_sensor_data_ready,1);
                start = end; // reset the start time
            }
        }

    }

    //exit_handler(1);
}

void write_register(int i2c_fd, unsigned char reg, unsigned char value){
    struct i2c_rdwr_ioctl_data i2c_data;
    struct i2c_msg msgs[1];

    unsigned char wr_buf[2];

    wr_buf[0] = reg;
    wr_buf[1] = value;

    msgs[0].addr = COLOR_SENSOR_ADDR;
    msgs[0].flags = 0;                  // Write operation
    msgs[0].len = 2;                    // 1st byte dir, 2nd value
    msgs[0].buf = wr_buf;

    // Set up I2C transaction
    i2c_data.msgs = msgs;
    i2c_data.nmsgs = 1;

    // Send the I2C transaction
    ioctl(i2c_fd, I2C_RDWR, &i2c_data);
}

void read_color(int i2c_fd, unsigned char* buf){
    struct i2c_msg messages[2];
    struct i2c_rdwr_ioctl_data ioctl_data;

    unsigned char start_reg = 0x94;			// Clear low byte
    messages[0].addr  = COLOR_SENSOR_ADDR;  // device address
    messages[0].flags = 0;                  // Write operation
    messages[0].len   = 1;                  // One byte to write
    messages[0].buf   = &start_reg;

    messages[1].addr  = COLOR_SENSOR_ADDR;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = 8;                  // Number of bytes to read (clear, red, green, blue)
    messages[1].buf   = buf;

    ioctl_data.msgs  = messages;
    ioctl_data.nmsgs = 2;

    ioctl(i2c_fd, I2C_RDWR, &ioctl_data);
}

void toggle_flash(uint8_t *flash){
    *flash = *flash == 0 ? 1 : 0;
    if(*flash) write_register(i2c_fd, 0x80, 0x03);                 // Enciende el flash
    else  write_register(i2c_fd, 0x80, 0x43);                      // Apaga el flash
}



