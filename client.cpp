#include "controlador.h"
#include "accelerometer.h"
#include "color_sensor.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>

#include <atomic>

using namespace std;

#define MAX_STRING_LENGTH 100

typedef struct{
	float acc_x, acc_y, acc_z;
}t_acc_data;

typedef struct{
	uint8_t flags;
	t_proc_color color;
	t_acc_data acceleration;
}t_send_data;


extern atomic<int> color_sensor_data_ready, acc_data_ready;
char copy_color_sensor_msg[1500];
float copy_acc_values[3];
uint8_t represent_acc_data = 0, represent_color_sensor_data = 0;

uint8_t flash = 0;

float ax, ay, az;       // Acceleration values

char color_sensor_msg[1500] = "";

t_raw_color		raw_colors 	= {0};
t_proc_color	proc_colors	= {0};
t_send_data 	send_data = {0};

pthread_t controlador;

int main(int argc, char *argv[])
{
  if (argc< 3)
  {
     cout<<"Please pass the ip adress and port number on which the server is listening."<<endl;
     exit(-1);
  }

  sockaddr_in server;
  int sock_fd;

  if ((sock_fd= socket(AF_INET, SOCK_DGRAM, 0))==-1)
  {
     perror("socket: ");
     exit(-1);
  }

  server.sin_family= AF_INET;
  server.sin_port= htons(atoi(argv[2]));
  server.sin_addr.s_addr= inet_addr(argv[1]);;
  bzero(&server.sin_zero, 8);

  char messageBuffer[MAX_STRING_LENGTH+1]="";
  char serverReply[MAX_STRING_LENGTH+1]="";
  int len=0;

  pthread_create(&controlador, NULL, control, NULL);

  while (true)
  {
//     cout<<"Please enter the input to send to the server: "<<endl;
//     cin.getline(messageBuffer, MAX_STRING_LENGTH);
//
//     if (strcmp(messageBuffer, "exit")==0)
//      break;
//
//     if (sendto(sock_fd, (void*) messageBuffer, (size_t) strlen(messageBuffer)+1, 0, (sockaddr *) &server, sizeof(server))==-1)
//     {
//      perror("sendto: ");
//      continue;
//     }
//
//     if (recvfrom(sock_fd, (void*) serverReply, (size_t) (MAX_STRING_LENGTH+1), MSG_WAITALL, (sockaddr *) &server, (socklen_t*) &len)==-1)
//     {
//      perror("recvfrom:");
//      continue;
//     }
//
//     cout<<"The server sent: "<<serverReply<<endl<<endl;


     // Represent data
     if(accelerometer_alive && color_sensor_alive){
    	 if(atomic_load(&color_sensor_data_ready)) {
    		 atomic_exchange(&color_sensor_data_ready, 0);
    		 represent_color_sensor_data = 1;
    		 sprintf(copy_color_sensor_msg, color_sensor_msg);
    	 }
    	 if(atomic_load(&acc_data_ready)) {
    		 atomic_exchange(&acc_data_ready, 0);
    		 represent_acc_data = 1;
    		 copy_acc_values[0] = ax;
    		 copy_acc_values[1] = ay;
    		 copy_acc_values[2] = az;
    	 }
    	 if(represent_color_sensor_data && represent_acc_data){
    		 printf("Acceleration: x = %.2f, y = %.2f, z = %.2f\r", copy_acc_values[0], copy_acc_values[1], copy_acc_values[2]);
    		 printf("%s", copy_color_sensor_msg);
    		 fflush(stdout);
    		 represent_acc_data = represent_color_sensor_data = 0;
    		 send_data.flags = 3;								// accelerometer alive && color sensor alive
    		 send_data.color = proc_colors;
    		 send_data.acceleration.acc_x = ax;
    		 send_data.acceleration.acc_y = ay;
    		 send_data.acceleration.acc_z = az;

    		 char buffer[sizeof(t_send_data)];
    		 memcpy(buffer, reinterpret_cast<char*>(&send_data), sizeof(t_send_data));		// Serialize the structure
    		 if(sendto(sock_fd, buffer, sizeof(buffer), 0, (sockaddr*)&server, sizeof(server))==-1)// Send to the server
    		 {
    			 printf("**********ERROR**********");
    		 }
    	 }
     }
     if(accelerometer_alive && !color_sensor_alive){
    	 if(atomic_load(&acc_data_ready)){
    		 printf("Acceleration: x = %.2f, y = %.2f, z = %.2f\r", ax, ay, az);
    		 fflush(stdout);

    		 send_data.flags = 1;								// accelerometer alive && !color sensor alive
    		 send_data.color = {0};
    		 send_data.acceleration.acc_x = ax;
    		 send_data.acceleration.acc_y = ay;
    		 send_data.acceleration.acc_z = az;

    		 char buffer[sizeof(t_send_data)];
    		 memcpy(buffer, reinterpret_cast<char*>(&send_data), sizeof(t_send_data));		// Serialize the structure
    		 sendto(sock_fd, buffer, sizeof(buffer), 0, (sockaddr*)&server, sizeof(server));// Send to the server
    	 }
     }
     if(color_sensor_alive && !accelerometer_alive){
    	 if(atomic_load(&color_sensor_data_ready)){
    		 printf("%s", color_sensor_msg);

    		 send_data.flags = 2;								// !accelerometer alive && color sensor alive
    		 send_data.color = proc_colors;
    		 send_data.acceleration.acc_x = 0;
    		 send_data.acceleration.acc_y = 0;
    		 send_data.acceleration.acc_z = 0;

    		 char buffer[sizeof(t_send_data)];
    		 memcpy(buffer, reinterpret_cast<char*>(&send_data), sizeof(t_send_data));		// Serialize the structure
    		 sendto(sock_fd, buffer, sizeof(buffer), 0, (sockaddr*)&server, sizeof(server));// Send to the server

    	 }
    	 fflush(stdout);
     }

  }//end of while loop

  close(sock_fd);
  exit(0);
}