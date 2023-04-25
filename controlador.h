/*
 * main.h
 *
 *  Created on: Mar 25, 2023
 *      Author: Ruben Agustin & Hao Feng
 */

#ifndef CONTROL_H
#define CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <termios.h>
#include <pthread.h>
//#ifndef __cplusplus
//# include <stdatomic.h>
//#else
//#endif
void* control(void* args);

#endif /* _CONTROL_H_ */
